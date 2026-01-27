#include "includes/http.hpp"
#include "includes/http_connection.hpp"
#include "includes/http_exceptions.hpp"
#include "includes/http_parser.hpp"
#include "includes/http_constants.hpp"
#include "includes/http_request.hpp"
#include "includes/http_response.hpp"
#include "includes/tcp.hpp"
#include "includes/event_manager.hpp"
#include "includes/logger.hpp"

namespace http
{
    namespace sizes
    {
        const size_t MAX_REQUEST_LINE_SIZE = 8192; // 8 KB
        const size_t MAX_HEADER_SIZE = 8192;       // 8 KB
    }
}

struct http::HttpServer::Impl
{
    tcp::ListeningSocket server_socket;
    tcp::EventManager event_manager;
    Impl(tcp::ListeningSocket &&sock, tcp::EventManager &&em) : server_socket(std::move(sock)), event_manager(std::move(em)) {}
};

bool logger__running = false;

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
                Logger::get_instance().log("Failed to set external logging. Reverting to console logging.", Logger::LogLevel::ERROR);
            }
        }
    }
    catch (...)
    {
        throw;
    }
}

http::HttpServer::HttpServer(HttpServerConfig _config) : config(_config)
{
    try
    {
        initialize_logger(config.external_logging);
        logger__running = true;
    }
    catch (...)
    {
    }

    try
    {
        pimpl = new Impl(std::move(tcp::ListeningSocket(config.port, config.max_pending_connections)), std::move(tcp::EventManager(config.max_concurrent_connections + 1, -1)));
        log_info("Server created on port:" + std::to_string(config.port));
    }
    catch (const tcp::exceptions::CanNotCreateSocket &e)
    {
        log_error(std::string("Error opening server socket: ") + e.what());
        throw http::exceptions::CanNotCreateServer();
    }
    catch (const std::exception &e)
    {
        log_error(std::string("Error creating server: ") + e.what());
        throw http::exceptions::CanNotCreateServer();
    }
    catch (...)
    {
        log_error("Unknown error opening server socket.");
        throw http::exceptions::CanNotCreateServer();
    }
}

http::HttpServer::~HttpServer()
{
    log_info("Server closed.");
    delete pimpl;
}

void http::HttpServer::start()
{
    try
    {
        log_info("Server listening on port: " + std::to_string(get_port()));
        int server_id = pimpl->event_manager.register_socket(pimpl->server_socket.fd());
        while (true)
        {
            try
            {
                std::vector<int> active_connections = pimpl->event_manager.wait_for_events();

                if (pimpl->event_manager.is_readable(server_id))
                {
                    accept_new_connections();
                    pimpl->event_manager.clear_status(server_id);
                }

                for (auto conn_id : active_connections)
                {
                    if (conn_id == server_id)
                        continue;
                    HttpConnection &connection = connections.at(conn_id);
                    if (pimpl->event_manager.is_readable(conn_id))
                    {
                        connection.set_peer_writing();
                    }
                    if (pimpl->event_manager.is_writable(conn_id))
                    {
                        connection.set_peer_reading();
                    }
                }

                for (auto conn_id : active_connections)
                {
                    if (conn_id == server_id)
                        continue;

                    HttpConnection &connection = connections.at(conn_id);
                    connection.handle_request(route_handlers);

                    pimpl->event_manager.clear_status(conn_id);
                    connection.set_peer_idle();

                    if (connection.status() == request_status::SENDING_RESPONSE)
                    {
                        pimpl->event_manager.add_to_write_monitoring(conn_id);
                    }

                    if (connection.status() == request_status::COMPLETED || connection.status() == request_status::CLIENT_ERROR)
                    {
                        pimpl->event_manager.remove_socket(conn_id);
                        connections.erase(conn_id);
                    }
                }
                check_and_remove_inactive_connections();
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

void http::HttpServer::check_and_remove_inactive_connections()
{
    static time_t last_timeout_check = 0;
    if (time(nullptr) - last_timeout_check >= 5)
    {
        last_timeout_check = time(nullptr);
        for (auto &it : connections)
        {
            auto &conn = it.second;
            if (conn.idle_time() > config.inactive_connection_timeout)
            {
                log_info("Connection timed out: " + conn.get_ip() + ":" + std::to_string(conn.get_port()));
                pimpl->event_manager.remove_socket(it.first);
                connections.erase(it.first);
            }
        }
    }
}

void http::HttpServer::accept_new_connections()
{
    std::vector<tcp::ConnectionSocket> new_connections = pimpl->server_socket.accept_connections();
    for (auto &conn : new_connections)
    {
        int conn_id = pimpl->event_manager.register_socket(conn.fd());
        connections.insert({conn_id, http::HttpConnection(std::move(conn))});
        log_info("Connection accepted: " + connections.at(conn_id).get_ip() + ":" + std::to_string(connections.at(conn_id).get_port()));
    }
}

void http::HttpConnection::handle_request(std::map<std::pair<std::string, std::string>, std::function<void(const http::HttpRequest &, http::HttpResponse &)>> &route_handlers) noexcept
{
    try
    {
        if (current_request_status == request_status::CONNECTION_ESTABLISHED || current_request_status == request_status::READING_REQUEST_LINE || current_request_status == request_status::REQUEST_LINE_DONE || current_request_status == request_status::READING_HEADERS || current_request_status == request_status::HEADERS_DONE || current_request_status == request_status::READING_BODY)
        {
            if (peer_is_readable())
                read_request();
        }
        if (current_request_status == request_status::REQUEST_READING_DONE)
        {
            std::string path = http::HttpRequestParser::path_from_uri(current_request.uri);
            log_info("Request: " + current_request.method + " " + path);
            if (route_handlers.find({current_request.method, path}) == route_handlers.end())
            {
                log_warning(std::string("No handler found for ") + current_request.method + " " + path);
                current_response = http::HttpResponse(http::status_codes::NOT_FOUND, "Not Found");
            }
            else
            {
                route_handlers[{current_request.method, path}](current_request, current_response);
                current_response.add_header(http::headers::CONNECTION, "close");
                current_response.add_header(http::headers::CONTENT_LENGTH, std::to_string(current_response.body().size()));
            }
            if (peer_is_writable())
                send_response();
        }
        if (current_request_status == request_status::SENDING_RESPONSE || current_request_status == request_status::SERVER_ERROR)
        {
            if (peer_is_writable())
                send_response();
        }
    }
    catch (const std::exception &e)
    {
        log_error(std::string(e.what()));
        try
        {
            current_response = http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error");
            current_request_status = request_status::SERVER_ERROR;
            if (peer_is_writable())
                send_response();
        }
        catch (...)
        {
            // Suppress all exceptions in the outermost catch to avoid termination
        }
    }
    catch (...)
    {
        try
        {
            current_response = http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error");
            current_request_status = request_status::SERVER_ERROR;
            if (peer_is_writable())
                send_response();
        }
        catch (...)
        {
            // Suppress all exceptions in the outermost catch to avoid termination
        }
    }
}

void http::HttpConnection::send_response()
{
    try
    {
        if (current_request_status == request_status::REQUEST_READING_DONE || current_request_status == request_status::SERVER_ERROR)
        {
            buffer.clear();
            buffer_cursor = 0;
            buffer = http::HttpRequestParser::create_response_buffer(current_response);
            current_request_status = request_status::SENDING_RESPONSE;
        }
        int sent_bytes = client_socket.send_data(buffer, buffer_cursor);
        buffer_cursor += sent_bytes;
        if (buffer_cursor == buffer.size())
        {
            log_info("Response sent with status code: " + std::to_string(current_response.status_code()));
            current_request_status = request_status::COMPLETED;
        }
    }
    catch (const tcp::exceptions::CanNotSendData &e)
    {
        current_request_status = request_status::CLIENT_ERROR;
        throw http::exceptions::CanNotSendResponse(std::string(e.what()));
    }
    catch (...)
    {
        current_request_status = request_status::CLIENT_ERROR;
        throw http::exceptions::CanNotSendResponse();
    }
}

void http::HttpServer::add_route_handler(const std::string method, const std::string path, std::function<void(const http::HttpRequest &, http::HttpResponse &)> handler)
{
    try
    {
        if (route_handlers.find({method, path}) != route_handlers.end())
        {
            throw http::exceptions::UnableToAddRouteHandler("Handler already exists for " + method + " " + path);
        }
        route_handlers.insert({{method, path}, handler});
    }
    catch (const http::exceptions::UnableToAddRouteHandler &)
    {
        throw;
    }
    catch (const std::exception &e)
    {
        log_error(std::string("Error adding route handler for ") + method + " " + path + ": " + e.what());
        throw http::exceptions::UnableToAddRouteHandler();
    }
    catch (...)
    {
        log_error(std::string("Unknown error adding route handler for ") + method + " " + path);
        throw http::exceptions::UnableToAddRouteHandler();
    }
}

void http::HttpConnection::read_request()
{
    try
    {
        static long body_size = -1;
        static bool has_chunked_body = false;
        read_from_client();
        if (current_request_status == request_status::CONNECTION_ESTABLISHED)
        {
            current_request_status = request_status::READING_REQUEST_LINE;
        }
        if (current_request_status == request_status::READING_REQUEST_LINE)
        {
            read_request_line();
        }
        if (current_request_status == request_status::REQUEST_LINE_DONE)
        {
            if (!http::HttpRequestParser::validate_request_line(buffer))
            {
                throw http::exceptions::InvalidRequestLine();
            }
            else
            {
                http::HttpRequestLine req_line = http::HttpRequestParser::parse_request_line(buffer, parser_cursor);
                current_request.method = req_line.method;
                current_request.uri = req_line.uri;
                current_request.version = req_line.version;
                if (current_request.version != http::versions::HTTP_1_1)
                {
                    throw http::exceptions::VersionNotSupported{};
                }
                current_request_status = request_status::READING_HEADERS;
            }
        }
        if (current_request_status == request_status::READING_HEADERS)
        {
            read_headers();
        }
        if (current_request_status == request_status::HEADERS_DONE)
        {
            current_request.headers = http::HttpRequestParser::parse_headers(buffer, parser_cursor);
            for (auto &header : current_request.headers)
            {
                long content_length = http::HttpRequestParser::is_content_length_header(std::vector<char>(header.first.begin(), header.first.end()));
                if (content_length != -1)
                {
                    if (body_size != -1)
                        throw http::exceptions::MultipleContentLengthHeaders{};
                    body_size = content_length;
                    current_request_status = request_status::READING_BODY;
                    break;
                }
                if (http::HttpRequestParser::is_transfer_encoding_chunked_header(std::vector<char>(header.first.begin(), header.first.end())))
                {
                    has_chunked_body = true;
                    current_request_status = request_status::READING_BODY;
                    break;
                }
            }
            if (body_size != -1 && has_chunked_body)
            {
                throw http::exceptions::BothContentLengthAndChunked{};
            }
            if (body_size == -1 && !has_chunked_body)
            {
                current_request_status = request_status::REQUEST_READING_DONE;
            }
        }
        if (current_request_status == request_status::READING_BODY)
        {
            if (has_chunked_body)
            {
                read_body();
            }
            else
            {
                read_body(body_size);
            }
        }
        if (current_request_status == request_status::REQUEST_READING_DONE)
        {
            current_request.body = http::HttpRequestParser::parse_body(buffer, parser_cursor, current_request.headers);
        }
    }
    catch (const http::exceptions::UnexpectedEndOfStream &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::InvalidRequestLine &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::InvalidChunkedEncoding &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::InvalidContentLength &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::MultipleContentLengthHeaders &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::BothContentLengthAndChunked &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::TransferEncodingWithoutChunked &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::RequestLineTooLong &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::URI_TOO_LONG, "Invalid Request Line");
        return;
    }
    catch (const http::exceptions::HeadersTooLarge &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::HEADERS_TOO_LARGE, "Header Fields Too Large");
        return;
    }
    catch (const http::exceptions::VersionNotSupported &e)
    {
        log_error(std::string(e.what()));
        current_response = http::HttpResponse(http::status_codes::HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported");
        return;
    }
    catch (std::exception &e)
    {
        log_error(std::string("Unknown error reading request: ") + e.what());
        current_response = http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error");
        return;
    }
    catch (...)
    {
        log_error("Unknown error reading request.");
        current_response = http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error");
        return;
    }
}

void http::HttpConnection::read_request_line()
{
    try
    {
        long pos = buffer_cursor;
        bool found_end_of_request_line{false};
        for (; pos < (long)buffer.size() - 1; pos++)
        {
            if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
            {
                found_end_of_request_line = true;
                buffer_cursor = pos + 2;
                break;
            }

            if (pos >= http::sizes::MAX_REQUEST_LINE_SIZE)
            {
                throw http::exceptions::RequestLineTooLong();
            }
        }
        if (found_end_of_request_line)
            current_request_status = request_status::REQUEST_LINE_DONE;
    }
    catch (...)
    {
        throw;
    }
}

void http::HttpConnection::read_headers()
{
    try
    {
        long pos = buffer_cursor;
        bool found_end_of_headers{false};
        static long last_header_end = 0;
        static long header_start = buffer_cursor;
        for (; pos < (long)buffer.size() - 1; pos++)
        {

            if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
            {
                if (pos == last_header_end + 1)
                {
                    found_end_of_headers = true;
                    buffer_cursor = pos + 2;
                    break;
                }
                pos += 1;
                last_header_end = pos;
            }

            if ((pos - header_start) >= http::sizes::MAX_HEADER_SIZE)
            {
                throw http::exceptions::HeadersTooLarge();
            }
        }
        if (found_end_of_headers)
            current_request_status = request_status::HEADERS_DONE;
    }
    catch (...)
    {
        throw;
    }
}

void http::HttpConnection::read_body(long content_length)
{
    try
    {
        if ((long)buffer.size() - buffer_cursor >= content_length)
        {
            buffer_cursor += content_length;
            current_request_status = request_status::REQUEST_READING_DONE;
        }
    }
    catch (...)
    {
        throw;
    }
}

void http::HttpConnection::read_body() // For chunked transfer encoding
{
    try
    {
        long pos = buffer_cursor;
        while (true)
        {
            bool found_end_of_chunk_size_line{false};
            for (; pos < (long)buffer.size() - 1; pos++)
            {
                if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
                {
                    found_end_of_chunk_size_line = true;
                    break;
                }
            }
            if (!found_end_of_chunk_size_line)
                return;

            std::string chunk_size_str(buffer.begin() + buffer_cursor, buffer.begin() + pos);
            size_t chunk_size = std::stoul(chunk_size_str, nullptr, 16);
            pos += 2;

            if ((long)buffer.size() - pos < (long)(chunk_size + 2))
                return;

            pos += chunk_size + 2;

            if (chunk_size == 0)
            {
                buffer_cursor = pos;
                current_request_status = request_status::REQUEST_READING_DONE;
                return;
            }
            buffer_cursor = pos;
        }
    }
    catch (...)
    {
        throw;
    }
}

void http::HttpConnection::read_from_client()
{
    try
    {
        std::vector<char> temp_buffer;
        temp_buffer = client_socket.receive_data();
        buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());
    }
    catch (const tcp::exceptions::CanNotReceiveData &e)
    {
        current_request_status = request_status::CLIENT_ERROR;
        throw http::exceptions::UnexpectedEndOfStream(std::string(e.what()));
    }
    catch (...)
    {
        current_request_status = request_status::CLIENT_ERROR;
        throw http::exceptions::UnexpectedEndOfStream();
    }
}

std::string http::HttpServer::get_ip()
{
    return pimpl->server_socket.get_ip();
}

unsigned short http::HttpServer::get_port()
{
    return pimpl->server_socket.get_port();
}

void http::HttpConnection::log_info(const std::string &message)
{
    try
    {
        if (!logger__running)
            return;
        Logger::get_instance().log(std::string("[CONN] [" + get_ip() + ":" + std::to_string(get_port()) + "] " + message), Logger::LogLevel::INFO);
    }
    catch (...)
    {
    }
}

void http::HttpConnection::log_warning(const std::string &message)
{
    try
    {
        if (!logger__running)
            return;
        Logger::get_instance().log(std::string("[CONN] [" + get_ip() + ":" + std::to_string(get_port()) + "] " + message), Logger::LogLevel::WARNING);
    }
    catch (...)
    {
    }
}

void http::HttpConnection::log_error(const std::string &message)
{
    try
    {
        if (!logger__running)
            return;
        Logger::get_instance().log(std::string("[CONN] [" + get_ip() + ":" + std::to_string(get_port()) + "] " + message), Logger::LogLevel::ERROR);
    }
    catch (...)
    {
    }
}

void http::HttpServer::log_info(const std::string &message)
{
    try
    {
        if (!logger__running)
            return;
        Logger::get_instance().log(std::string("[SERVER] " + message), Logger::LogLevel::INFO);
    }
    catch (...)
    {
    }
}

void http::HttpServer::log_warning(const std::string &message)
{
    try
    {
        if (!logger__running)
            return;
        Logger::get_instance().log(std::string("[SERVER] " + message), Logger::LogLevel::WARNING);
    }
    catch (...)
    {
    }
}

void http::HttpServer::log_error(const std::string &message)
{
    try
    {
        if (!logger__running)
            return;
        Logger::get_instance().log(std::string("[SERVER] " + message), Logger::LogLevel::ERROR);
    }
    catch (...)
    {
    }
}
