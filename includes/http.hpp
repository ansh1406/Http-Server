#ifndef HTTP_HPP
#define HTTP_HPP

#include "includes/tcp.hpp"
#include "includes/http_request.hpp"
#include "includes/http_response.hpp"

#include <map>
#include <functional>
#include <string>

namespace http
{
    class HttpServer
    {
    private:
        tcp::ListeningSocket server_socket;
        std::map<std::pair<std::string, std::string>, std::function<void(const http::HttpRequest &, http::HttpResponse &)>> route_handlers;

    public:
        explicit HttpServer(tcp::Port port);

        HttpServer(const HttpServer &) = delete;
        HttpServer &operator=(const HttpServer &) = delete;

        HttpServer(HttpServer &&) = default;
        HttpServer &operator=(HttpServer &&) = default;

        void start();
        void add_route_handler(const std::string method, const std::string path,
                               const std::function<void(const http::HttpRequest &, http::HttpResponse &)> handler);
    };
}
#endif // HTTP_HPP