#include "http/http.hpp"
#include "http/http_constants.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include "http_internal.hpp"
#include "http_connection.hpp"
#include "http_exceptions.hpp"
#include "tcp.hpp"
#include "event_manager.hpp"
#include "logger.hpp"

#include <map>
#include <cstring>

bool Logger::logger_running = false;

void initialize_logger(const bool external_logging)
{
    try
    {
        Logger::get_instance();
        if (external_logging)
        {
            try
            {
                Logger::set_external_logging("server.log");
            }
            catch (...)
            {
                Logger::get_instance().log("Failed to set external logging. Reverting to console logging.", Logger::LogLevel::ERR);
            }
        }
    }
    catch (...)
    {
        throw;
    }
}

http::HttpServer::HttpServer(HttpServerConfig _config, const std::function<void(const http::HttpRequest &, http::HttpResponse &)> handler)
{
    try
    {
        if (_config.enable_logging)
        {
            initialize_logger(_config.external_logging);
            Logger::logger_running = true;
        }
    }
    catch (...)
    {
    }

    try
    {
        pimpl = new Impl(std::move(tcp::ListeningSocket(_config.port, _config.max_pending_connections)), std::move(tcp::EventManager(_config.max_concurrent_connections + 1, -1)), std::move(tcp::EventManager(_config.max_concurrent_connections + 1, -1)), _config, handler);
        pimpl->log_info("Server created on port:" + std::to_string(_config.port));
    }
    catch (const tcp::exceptions::CanNotCreateSocket &e)
    {
        if (pimpl)
        {
            pimpl->log_error(std::string("Error opening server socket: ") + e.what());
        }
        throw http::exceptions::CanNotCreateServer(std::string(e.what()));
    }
    catch (const std::exception &e)
    {
        if (pimpl)
        {
            pimpl->log_error(std::string("Error creating server: ") + e.what());
        }
        throw http::exceptions::CanNotCreateServer(std::string(e.what()));
    }
    catch (...)
    {
        if (pimpl)
        {
            pimpl->log_error("Unknown error opening server socket.");
        }
        throw http::exceptions::CanNotCreateServer();
    }
}

http::HttpServer::~HttpServer()
{
    if (pimpl)
    {
        pimpl->log_info("Server closed.");
    }
    delete pimpl;
}

void http::HttpServer::start()
{
    pimpl->start_event_loop();
}

void http::HttpServer::Impl::start_event_loop()
{
    try
    {
        log_info("Server listening on port: " + std::to_string(config.port));
        int server_id = request_event_manager.register_for_read(server_socket.fd());
        while (true)
        {
            try
            {
                std::vector<int> active_connections = request_event_manager.wait_for_events();

                if (request_event_manager.is_readable(server_id))
                {
                    accept_new_connections();
                    request_event_manager.clear_status(server_id);
                }

                for (auto conn_id : active_connections)
                {
                    if (conn_id == server_id)
                        continue;
                    HttpConnection &connection = connections.at(conn_id);
                    if (request_event_manager.is_readable(conn_id))
                    {
                        connection.set_peer_writing();
                    }
                }

                for (auto conn_id : active_connections)
                {
                    if (conn_id == server_id)
                        continue;

                    request_event_manager.clear_status(conn_id);

                    HttpConnection &connection = connections.at(conn_id);

                    if (connection.peer_is_readable() && connection.get_current_request().get_status() < RequestStatus::HEADERS_DONE)
                    {
                        connection.read_request();
                    }

                    if (connection.get_current_request().get_status() == HEADERS_DONE)
                    {
                        request_event_manager.remove_socket(conn_id);
                    }

                    connection.set_peer_idle();
                }
                mark_inactive_connections();
            }
            catch (const std::exception &e)
            {
                log_error(std::string(e.what()));
            }
            catch (...)
            {
                log_error("Unknown unexpected error.");
            }
        }
    }
    catch (const std::exception &e)
    {
        log_error(std::string("Fatal error starting server: ") + e.what());
        throw;
    }
    catch (...)
    {
        log_error("Unknown fatal error starting server.");
        throw;
    }
}

void http::HttpServer::Impl::mark_inactive_connections()
{
    static time_t last_timeout_check = 0;
    if (time(nullptr) - last_timeout_check >= 1)
    {
        last_timeout_check = time(nullptr);
        for (auto &it : connections)
        {
            auto &conn = it.second;
            if (conn.idle_time() > config.inactive_connection_timeout_in_seconds)
            {
                log_info("Connection timed out: " + conn.get_ip() + ":" + std::to_string(conn.get_port()));
                conn.inactive = true;
            }
        }
    }
}

void http::HttpServer::Impl::accept_new_connections()
{
    std::vector<tcp::ConnectionSocket> new_connections = server_socket.accept_connections();
    for (auto &conn : new_connections)
    {
        int conn_id = request_event_manager.register_for_read(conn.fd());
        connections.insert({conn_id, http::HttpConnection(std::move(conn))});
        log_info("Connection accepted: " + connections.at(conn_id).get_ip() + ":" + std::to_string(connections.at(conn_id).get_port()));
    }
}