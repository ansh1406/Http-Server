#include <iostream>
#include "includes/http.hpp"
#include <vector>
#include <string>
#include <functional>

int main()
{
    http::HttpServer server(51234);
    server.add_route_handler(http::methods::GET, "/", [](const http::HttpRequest &req, http::HttpResponse &res)
                             {
        res.set_status_code(200);
        res.set_status_message("OK");
        res.set_headers({{"Content-Type", "text/plain"}, {"Connection", "close"}, {"Content-Length", "13"}});

        std::vector<char> body = {'H','e','l','l','o',',',' ','W','o','r','l','d','!'};
        res.set_body(body); });

    server.add_route_handler(http::methods::GET, "/favicon.ico", [](const http::HttpRequest &req, http::HttpResponse &res)
                             {
        res.set_status_code(200);
        res.set_status_message("OK");
        res.set_headers({{"Content-Type", "text/plain"}, {"Connection", "close"}, {"Content-Length", "21"}});

        std::string favicon = "No favicon available";
        std::vector<char> body(favicon.begin(), favicon.end());
        body.push_back('\0');
        res.set_body(body); });
    server.start();
}