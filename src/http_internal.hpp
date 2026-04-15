#ifndef HTTP_INTERNAL_HPP
#define HTTP_INTERNAL_HPP

#include "http/http.hpp"
#include "http_connection.hpp"
#include "event_manager.hpp"
#include "logger.hpp"

#include <map>
#include <queue>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

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
        tcp::EventManager response_event_manager;
        HttpServerConfig config;
        RequestHandler request_handler;
        std::map<int, HttpConnection> connections;
        std::map<int, int> response_sending_connections;

        std::queue<int> waiting_for_handler_connections;
        std::queue<int> waiting_to_send_response;

        std::mutex completed_connections_mutex;
        std::queue<int> completed_connections;

        std::mutex handler_mutex;
        std::vector<std::thread> handler_threads;
        std::condition_variable handler_cv;

        std::mutex response_mutex;
        std::thread response_thread;
        std::condition_variable response_cv;

        void initialize_handler_threads();
        void initialize_response_thread();

        void start_event_loop();
        void accept_new_connections();
        void mark_inactive_connections();
        void remove_completed_connections();

        void log_info(const std::string &message) const;
        void log_warning(const std::string &message) const;
        void log_error(const std::string &message) const;

        std::string get_ip() const;
        unsigned short get_port() const noexcept;

        Impl(tcp::ListeningSocket &&sock,
             tcp::EventManager &&req_em,
             tcp::EventManager &&resp_em,
             HttpServerConfig _config,
             RequestHandler handler) : server_socket(std::move(sock)),
                                       request_event_manager(std::move(req_em)),
                                       response_event_manager(std::move(resp_em)),
                                       config(_config), request_handler(handler) {}
    };
}
#endif // HTTP_INTERNAL_HPP