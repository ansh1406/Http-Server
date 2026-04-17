#ifndef HTTP_INTERNAL_HPP
#define HTTP_INTERNAL_HPP

#include "http/http.hpp"
#include "http_connection.hpp"
#include "event_manager.hpp"
#include "logger.hpp"

#include <map>
#include <unordered_map>
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
        /// Maximum accepted request-line size before request is rejected.
        const size_t MAX_REQUEST_LINE_SIZE = 8192; // 8 KB
        /// Maximum accepted aggregate header bytes.
        const size_t MAX_HEADER_SIZE = 8192;
        /// Per-connection socket read buffer size.
        const size_t READ_BUFFER_SIZE = 8192;
    }

    /// Private runtime state for HttpServer.
    /// Owns sockets, event managers, connection maps, and worker coordination queues.
    struct HttpServer::Impl
    {
        tcp::ListeningSocket server_socket;
        tcp::EventManager request_event_manager;
        tcp::EventManager response_event_manager;
        HttpServerConfig config;
        RequestHandler request_handler;
        // fd -> active connection state.
        std::map<int, HttpConnection> connections;
        // active-connection pointer -> fd (reverse index for O(1) cleanup lookup).
        std::unordered_map<HttpConnection *, int> connection_ids;
        // event-manager id -> connection currently scheduled for response writes.
        std::map<int, HttpConnection *> response_sending_connections;

        // connections ready to run request handler logic.
        std::queue<HttpConnection *> waiting_for_handler_connections;
        // connections with responses ready for the response thread.
        std::queue<HttpConnection *> waiting_to_send_response;

        std::mutex completed_connections_mutex;
        // connections finished or failed and pending cleanup in event loop thread.
        std::queue<HttpConnection *> completed_connections;

        std::mutex handler_mutex;
        std::vector<std::thread> handler_threads;
        std::condition_variable handler_cv;

        std::mutex response_mutex;
        std::thread response_thread;
        std::condition_variable response_cv;

        /// Spawns worker threads that consume waiting_for_handler_connections.
        void initialize_handler_threads();
        /// Spawns response thread that consumes waiting_to_send_response.
        void initialize_response_thread();

        /// Main accept/poll/dispatch loop.
        void start_event_loop();
        /// Accepts new TCP peers and inserts them into connection/event maps.
        void accept_new_connections();
        /// Marks connections inactive when idle timeout is exceeded.
        void mark_inactive_connections();
        /// Removes and closes connections queued in completed_connections.
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