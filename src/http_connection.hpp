#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include "tcp.hpp"

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <ctime>

namespace http
{
    enum RequestStatus
    {
        CONNECTION_ESTABLISHED = 0,
        READING_REQUEST_LINE = 1,
        REQUEST_LINE_DONE = 2,
        READING_HEADERS = 3,
        HEADERS_DONE = 4,
        READING_BODY = 5,
        REQUEST_READING_DONE = 6,
        SENDING_RESPONSE = 7,
        COMPLETED = 8,
        CLIENT_ERROR = 9,
        SERVER_ERROR = 10
    };

    enum ConnectionStatus
    {
        IDLE = 0,
        READING = 1,
        WRITING = 2
    };

    class HttpConnection
    {
    public:
        struct CurrentRequest
        {
        private:
            HttpRequest request;
            HttpResponse response;
            RequestStatus status;
            bool has_chunked_body = false;
            long content_length = -1;
            long remaining_content_length = -1;
            long body_cursor = 0;

        public:
            CurrentRequest();

            const RequestStatus get_status() const noexcept
            {
                return status;
            }

            friend class HttpConnection;
        };

    private:
        std::vector<char> buffer;
        tcp::ConnectionSocket client_socket;
        CurrentRequest current_request;
        time_t last_activity_time = 0;
        long buffer_cursor = 0;
        long buffer_size = 0;
        size_t parser_cursor = 0;
        int peer_status = ConnectionStatus::IDLE;

        void read_from_client();
        void read_request_line();
        void read_headers();
        void read_fixed_body();
        void read_chunksize_line();
        void read_body_chunk();
        void log_info(const std::string &message) const;
        void log_warning(const std::string &message) const;
        void log_error(const std::string &message) const;

        void reposition_buffer();

    public:
        /// @brief Construct a new Http Connection object
        /// @param socket The TCP connection socket associated with this HTTP connection
        explicit HttpConnection(tcp::ConnectionSocket &&socket);

        HttpConnection(const HttpConnection &) = delete;
        HttpConnection &operator=(const HttpConnection &) = delete;

        HttpConnection(HttpConnection &&) = default;
        HttpConnection &operator=(HttpConnection &&) = default;

        bool inactive = false;

        void read_request();
        void handle_request(std::function<void(const http::HttpRequest &, http::HttpResponse &)> &request_handler) noexcept;

        void send_response();

        void set_peer_idle() noexcept
        {
            peer_status = ConnectionStatus::IDLE;
            last_activity_time = time(nullptr);
        }

        void set_peer_reading() noexcept
        {
            peer_status |= ConnectionStatus::READING;
        }

        void set_peer_writing() noexcept
        {
            peer_status |= ConnectionStatus::WRITING;
        }

        const bool peer_is_readable() const noexcept
        {
            return peer_status & ConnectionStatus::WRITING;
        }

        const bool peer_is_writable() const noexcept
        {
            return peer_status & ConnectionStatus::READING;
        }

        const CurrentRequest &get_current_request() const noexcept
        {
            return current_request;
        }

        const time_t idle_time() const noexcept
        {
            return time(nullptr) - last_activity_time;
        }

        /// @return IP address of the connected client
        std::string get_ip() const
        {
            return client_socket.get_ip();
        }

        /// @return Port number of the connected client
        tcp::Port get_port() const noexcept
        {
            return client_socket.get_port();
        }
    };
}

#endif // HTTP_CONNECTION_HPP