/// @file http_constants.hpp
/// @brief This file contains constants for HTTP headers, methods, versions, and status codes.

#ifndef HTTP_CONSTANTS_HPP
#define HTTP_CONSTANTS_HPP

#include <string>

namespace http
{
    namespace headers
    {
        const std::string CONTENT_LENGTH = "content-length";
        const std::string TRANSFER_ENCODING = "transfer-encoding";
        const std::string CONNECTION = "connection";
        const std::string HOST = "host";
        const std::string EXPECT = "expect";
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
        const std::string TRACE = "TRACE";
        const std::string CONNECT = "CONNECT";
    }

    namespace versions
    {
        const std::string HTTP_1_1 = "HTTP/1.1";
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
        const int URI_TOO_LONG = 414;
        const int PAYLOAD_TOO_LARGE = 413;
        const int HEADERS_TOO_LARGE = 431;
        const int HTTP_VERSION_NOT_SUPPORTED = 505;
    }
}

#endif // HTTP_CONSTANTS_HPP