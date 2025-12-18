#ifndef HTTP_PARSER
#define HTTP_PARSER

#include "includes/tcp.hpp"

#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <algorithm>

namespace http
{

    namespace exceptions{
        class UnexpectedEndOfStream : public std::runtime_error {
        public:
            UnexpectedEndOfStream() : std::runtime_error("Unexpected end of stream") {}
        };

        class InvalidRequestLine : public std::runtime_error {
        public:
            InvalidRequestLine() : std::runtime_error("Invalid HTTP request line") {}
        };

        class RequestLineTooLong : public std::runtime_error {
        public:
            RequestLineTooLong() : std::runtime_error("HTTP request line too long") {}
        };

        class HeadersTooLarge : public std::runtime_error {
        public:
            HeadersTooLarge() : std::runtime_error("HTTP header too large") {}
        };

        class TransferEncodingWithoutChunked : public std::runtime_error {
        public:
            TransferEncodingWithoutChunked() : std::runtime_error("Transfer-Encoding header is present without 'chunked' value") {}
        };

        class InvalidContentLength : public std::runtime_error {
        public:
            InvalidContentLength() : std::runtime_error("Invalid Content-Length header value") {}
        };

        class MultipleContentLengthHeaders : public std::runtime_error {
        public:
            MultipleContentLengthHeaders() : std::runtime_error("Multiple Content-Length headers present") {}
        };

        class BothContentLengthAndChunked : public std::runtime_error {
        public:
            BothContentLengthAndChunked() : std::runtime_error("Both Content-Length and Transfer-Encoding headers present") {}
        };

        class PayloadTooLarge : public std::runtime_error {
        public:
            PayloadTooLarge() : std::runtime_error("Payload too large") {}
        };
    }
    namespace headers
    {
        const std::string CONTENT_LENGTH = "content-length";
        const std::string TRANSFER_ENCODING = "transfer-encoding";
        const std::string CONNECTION = "connection";
        const std::string HOST = "host";
        const std::string USER_AGENT = "user-agent";
        const std::string ACCEPT = "accept";
        const std::string CONTENT_TYPE = "content-type";
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
        const size_t READ_BUFFER_SIZE = 16384; // 16 KB
        const size_t SINGLE_READ_SIZE = 4096; // 4 KB
        const size_t MAX_REQUEST_LINE = 8192; // 8 KB
    }

    class HttpRequestReader
    {
    private:
        std::vector<char> buffer;
        bool validate_request_line(const std::vector<char> &request_line);
        size_t is_content_length_header(const size_t header_end_index);
        bool is_transfer_encoding_header(const size_t header_end_index);
    public:
        HttpRequestReader() = default;
        std::vector<char> read(tcp::ConnectionSocket &client_socket);
        void clear_buffer();
    };
}

#endif