#ifndef HTTP_PARSER
#define HTTP_PARSER

#include "includes/tcp.hpp"

#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace http
{
    namespace headers
    {
        const std::string CONTENT_LENGTH = "Content-Length";
        const std::string TRANSFER_ENCODING = "Transfer-Encoding";
        const std::string CONNECTION = "Connection";
        const std::string HOST = "Host";
        const std::string USER_AGENT = "User-Agent";
        const std::string ACCEPT = "Accept";
        const std::string CONTENT_TYPE = "Content-Type";
    }
    namespace methods
    {
        const std::string GET = "GET";
        const std::string POST = "POST";
        const std::string PUT = "PUT";
        const std::string DELETE = "DELETE";
        const std::string HEAD = "HEAD";
        const std::string OPTIONS = "OPTIONS";
        const std::string PATCH = "PATCH";
    }

    namespace versions
    {
        const std::string HTTP_1_0 = "HTTP/1.0";
        const std::string HTTP_1_1 = "HTTP/1.1";
        const std::string HTTP_2_0 = "HTTP/2.0";
    }

    namespace status_codes
    {
        const int OK = 200;
        const int CREATED = 201;
        const int NO_CONTENT = 204;
        const int BAD_REQUEST = 400;
        const int UNAUTHORIZED = 401;
        const int FORBIDDEN = 403;
        const int NOT_FOUND = 404;
        const int INTERNAL_SERVER_ERROR = 500;
        const int NOT_IMPLEMENTED = 501;
        const int BAD_GATEWAY = 502;
        const int SERVICE_UNAVAILABLE = 503;
    }

    namespace content_types
    {
        const std::string TEXT_HTML = "text/html";
        const std::string APPLICATION_JSON = "application/json";
        const std::string TEXT_PLAIN = "text/plain";
        const std::string APPLICATION_XML = "application/xml";
        const std::string MULTIPART_FORM_DATA = "multipart/form-data";
    }

    namespace constants
    {
        const size_t MAX_HEADER_SIZE = 8192; // 8 KB
        const size_t MAX_BODY_SIZE = 10485760; // 10 MB
        const size_t READ_BUFFER_SIZE = 4096; // 4 KB
        const size_t MAX_REQUEST_LINE = 8192; // 8 KB
    }

    class HttpRequestReader
    {
    private:
        std::vector<char> buffer;

    public:
        HttpRequestReader() = default;
        std::vector<char> read(tcp::ConnectionSocket &client_socket);
        void clear_buffer();
    };
}

#endif