#ifndef HTTP_HPP
#define HTTP_HPP

#include "includes/tcp.hpp"
#include "includes/http_parser.hpp"

#include <string>
#include <map>
#include <vector>

namespace http {

    class HttpServer {
        private:
            tcp::ListeningSocket server_socket;
        public:
            explicit HttpServer(tcp::Port port) : server_socket(port){}

            HttpServer(const HttpServer&) = delete;
            HttpServer& operator=(const HttpServer&) = delete;

            HttpServer(HttpServer&&) = default;
            HttpServer& operator=(HttpServer&&) = default;

            void start();
    };

    class HttpConnection {
        private:
            tcp::ConnectionSocket client_socket;
            http::HttpRequestReader request_reader;
            http::HttpRequestParser request_parser;

        public:
            explicit HttpConnection(tcp::ConnectionSocket &&socket) 
                : client_socket(std::move(socket)) , request_reader() , request_parser() {}

            HttpConnection(const HttpConnection&) = delete;
            HttpConnection& operator=(const HttpConnection&) = delete;

            HttpConnection(HttpConnection&&) = default;
            HttpConnection& operator=(HttpConnection&&) = default;

            void handle();
    };
}



#endif // HTTP_HPP