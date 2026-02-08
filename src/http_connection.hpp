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
    namespace request_status
    {
        const int CONNECTION_ESTABLISHED = 0;
        const int READING_REQUEST_LINE = 1;
        const int REQUEST_LINE_DONE = 2;
        const int READING_HEADERS = 3;
        const int HEADERS_DONE = 4;
        const int READING_BODY = 5;
        const int REQUEST_READING_DONE = 6;
        const int SENDING_RESPONSE = 7;
        const int COMPLETED = 8;
        const int CLIENT_ERROR = 9;
        const int SERVER_ERROR = 10;
    }

    namespace connection_status
    {
        const int IDLE = 0;
        const int READING = 1;
        const int WRITING = 2;
    }

    class HttpConnection
    {
    private:
        std::vector<char> buffer;
        tcp::ConnectionSocket client_socket;
        http::HttpRequest current_request;
        http::HttpResponse current_response;
        time_t last_activity_time = 0;
        int current_request_status = request_status::CONNECTION_ESTABLISHED;
        long buffer_cursor = 0;
        size_t parser_cursor = 0;
        int peer_status = connection_status::IDLE;

        void read_from_client();
        void read_request_line();
        void read_headers();
        void read_body(long content_length);
        void read_body(); // For chunked transfer encoding
        void log_info(const std::string &message) const;
        void log_warning(const std::string &message) const;
        void log_error(const std::string &message) const;

    public:
        /// @brief Construct a new Http Connection object
        /// @param socket The TCP connection socket associated with this HTTP connection
        explicit HttpConnection(tcp::ConnectionSocket &&socket)
            : client_socket(std::move(socket))
        {
            last_activity_time = time(nullptr);
        }

        HttpConnection(const HttpConnection &) = delete;
        HttpConnection &operator=(const HttpConnection &) = delete;

        HttpConnection(HttpConnection &&) = default;
        HttpConnection &operator=(HttpConnection &&) = default;

        void read_request();
        void handle_request(std::function<void(const http::HttpRequest &, http::HttpResponse &)> &request_handler) noexcept;

        void send_response();

        void set_peer_idle() noexcept
        {
            peer_status = connection_status::IDLE;
            last_activity_time = time(nullptr);
        }

        void set_peer_reading() noexcept
        {
            peer_status |= connection_status::READING;
        }

        void set_peer_writing() noexcept
        {
            peer_status |= connection_status::WRITING;
        }

        const bool peer_is_readable() const noexcept
        {
            return peer_status & connection_status::WRITING;
        }

        const bool peer_is_writable() const noexcept
        {
            return peer_status & connection_status::READING;
        }

        const int status() const noexcept
        {
            return current_request_status;
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