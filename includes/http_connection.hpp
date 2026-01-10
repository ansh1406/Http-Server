#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

#include "tcp.hpp"

#include <string>
#include <map>
#include <vector>
#include <functional>

namespace http
{
    class HttpRequest;
    class HttpResponse;
    
    class HttpConnection
    {
    private:
        std::vector<char> buffer;
        tcp::ConnectionSocket client_socket;
        void read_from_tcp(tcp::ConnectionSocket &client_socket);
        std::vector<char> read(tcp::ConnectionSocket &client_socket);
        void log_info(const std::string &entry);
        void log_error(const std::string &entry);

    public:
        /// @brief Construct a new Http Connection object
        /// @param socket The TCP connection socket associated with this HTTP connection
        explicit HttpConnection(tcp::ConnectionSocket &&socket)
            : client_socket(std::move(socket)) {}

        HttpConnection(const HttpConnection &) = delete;
        HttpConnection &operator=(const HttpConnection &) = delete;

        HttpConnection(HttpConnection &&) = default;
        HttpConnection &operator=(HttpConnection &&) = default;

        /// @brief Generate and send HTTP response based on the request and route handlers
        /// @param route_handlers map of (method, path) pairs to their corresponding handler functions (callbacks)
        void handle(std::map<std::pair<std::string, std::string>,
                             std::function<void(const http::HttpRequest &, http::HttpResponse &)>> &route_handlers) noexcept;
        void send_response(const http::HttpResponse &response);

        /// @return IP address of the connected client
        std::string get_ip() const
        {
            return client_socket.get_ip();
        }

        /// @return Port number of the connected client
        tcp::Port get_port() const
        {
            return client_socket.get_port();
        }
    };
}

#endif // HTTP_CONNECTION_HPP