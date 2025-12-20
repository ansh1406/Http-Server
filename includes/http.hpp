#ifndef HTTP_HPP
#define HTTP_HPP

#include "http_constants.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <map>
#include <functional>
#include <string>

namespace http
{
    class HttpRequest;
    class HttpResponse;
    class HttpServer
    {
    private:
        struct Impl;
        Impl *pimpl;
        std::map<std::pair<std::string, std::string>, std::function<void(const http::HttpRequest &, http::HttpResponse &)>> route_handlers;

    public:
        explicit HttpServer(unsigned short port);

        HttpServer(const HttpServer &) = delete;
        HttpServer &operator=(const HttpServer &) = delete;

        HttpServer(HttpServer &&) = default;
        HttpServer &operator=(HttpServer &&) = default;

        ~HttpServer();

        void start();
        void add_route_handler(const std::string method, const std::string path,
                               const std::function<void(const http::HttpRequest &, http::HttpResponse &)> handler);
    };
}
#endif // HTTP_HPP