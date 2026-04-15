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
        CONNECTION_ESTABLISHED,
        READING_REQUEST_LINE,
        REQUEST_LINE_DONE,
        READING_HEADERS,
        HEADERS_DONE,
        READING_BODY,
        REQUEST_READING_DONE,
        REQUEST_HANDLING_DONE,
        SENDING_STATUS_LINE,
        SENDING_HEADERS,
        SENDING_RESPONSE_HEAD_DONE,
        SENDING_BODY,
        SENDING_BUFFER_FLUSHING,
        COMPLETED,
        CLIENT_ERROR,
        SERVER_ERROR
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
            RequestStatus status;

            bool has_chunked_body = false;
            long content_length = -1;
            long remaining_content_length = -1;
            long body_stream_cursor = 0;
            long body_end_cursor = 0;

        public:
            CurrentRequest();

            const RequestStatus get_status() const noexcept
            {
                return status;
            }

            friend class HttpConnection;
        };

        struct CurrentResponse
        {
        private:
            HttpResponse response;
            bool has_chunked_body = false;
            long content_length = -1;
            long remaining_content_length = -1;

            std::map<std::string, std::string>::const_iterator currently_sending_header;

            bool has_fixed_length_body() const
            {
                return content_length != -1;
            }

        public:
            CurrentResponse() = default;
            friend class HttpConnection;
        };

    private:
        std::vector<char> buffer;
        tcp::ConnectionSocket client_socket;
        CurrentRequest current_request;
        CurrentResponse current_response;
        time_t last_activity_time = 0;
        long buffer_cursor = 0;
        long buffer_size = 0;
        size_t parser_cursor = 0;
        int peer_status = ConnectionStatus::IDLE;

        void read_from_client();
        void read_request_line();
        void read_headers();
        void read_body();
        long read_fixed_body();
        long read_chunksize_line();
        long read_body_chunk();
        void log_info(const std::string &message) const;
        void log_warning(const std::string &message) const;
        void log_error(const std::string &message) const;

        void send_to_client();

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

        void read_and_build_request_head();
        void handle_request(std::function<void(const http::HttpRequest &, http::HttpResponse &)> &request_handler) noexcept;

        void send_response();

        int fd() const noexcept
        {
            return client_socket.fd();
        }

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