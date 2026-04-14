#ifndef HTTP_INTERNAL_HPP
#define HTTP_INTERNAL_HPP

#include "http/http.hpp"
#include "http_connection.hpp"
#include "event_manager.hpp"
#include "logger.hpp"

namespace http
{
    namespace sizes
    {
        const size_t MAX_REQUEST_LINE_SIZE = 8192; // 8 KB
        const size_t MAX_HEADER_SIZE = 8192;
        const size_t READ_BUFFER_SIZE = 8192;
    }

    struct HttpServer::Impl
    {
        tcp::ListeningSocket server_socket;
        tcp::EventManager request_event_manager;
        HttpServerConfig config;
        std::map<int, HttpConnection> connections;
        RequestHandler request_handler;

        void check_and_remove_inactive_connections();
        void accept_new_connections();
        void start_event_loop();

        void log_info(const std::string &message) const;
        void log_warning(const std::string &message) const;
        void log_error(const std::string &message) const;

        std::string get_ip() const;
        unsigned short get_port() const noexcept;

        Impl(tcp::ListeningSocket &&sock, tcp::EventManager &&em, HttpServerConfig _config, RequestHandler handler) : server_socket(std::move(sock)), request_event_manager(std::move(em)), config(_config), request_handler(handler) {}
    };
}
#endif // HTTP_INTERNAL_HPP