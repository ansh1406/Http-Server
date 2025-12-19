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
#include <cctype>

namespace http
{
    namespace exceptions
    {
        class UnexpectedEndOfStream : public std::runtime_error
        {
        public:
            UnexpectedEndOfStream() : std::runtime_error("Unexpected end of stream") {}
        };

        class InvalidRequestLine : public std::runtime_error
        {
        public:
            InvalidRequestLine() : std::runtime_error("Invalid HTTP request line") {}
        };

        class RequestLineTooLong : public std::runtime_error
        {
        public:
            RequestLineTooLong() : std::runtime_error("HTTP request line too long") {}
        };

        class HeadersTooLarge : public std::runtime_error
        {
        public:
            HeadersTooLarge() : std::runtime_error("HTTP header too large") {}
        };

        class TransferEncodingWithoutChunked : public std::runtime_error
        {
        public:
            TransferEncodingWithoutChunked() : std::runtime_error("Transfer-Encoding header is present without 'chunked' value") {}
        };

        class InvalidContentLength : public std::runtime_error
        {
        public:
            InvalidContentLength() : std::runtime_error("Invalid Content-Length header value") {}
        };

        class MultipleContentLengthHeaders : public std::runtime_error
        {
        public:
            MultipleContentLengthHeaders() : std::runtime_error("Multiple Content-Length headers present") {}
        };

        class BothContentLengthAndChunked : public std::runtime_error
        {
        public:
            BothContentLengthAndChunked() : std::runtime_error("Both Content-Length and Transfer-Encoding headers present") {}
        };

        class InvalidChunkedEncoding : public std::runtime_error
        {
        public:
            InvalidChunkedEncoding() : std::runtime_error("Invalid chunked encoding") {}
        };

        class PayloadTooLarge : public std::runtime_error
        {
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

    namespace constants
    {
        const size_t MAX_HEADER_SIZE = 8192;   // 8 KB
        const size_t MAX_BODY_SIZE = 10485760; // 10 MB
        const size_t READ_BUFFER_SIZE = 16384; // 16 KB
        const size_t SINGLE_READ_SIZE = 4096;  // 4 KB
        const size_t MAX_REQUEST_LINE = 8192;  // 8 KB
    }

    class HttpRequestReader
    {
    private:
        std::vector<char> buffer;
        bool validate_request_line(const std::vector<char> &request_line);
        long is_content_length_header(const size_t header_end_index);
        bool is_transfer_encoding_header(const size_t header_end_index);
        void read_from_tcp(tcp::ConnectionSocket &client_socket);

    public:
        HttpRequestReader() = default;
        std::vector<char> read(tcp::ConnectionSocket &client_socket);
    };

    class HttpRequest
    {
    private:
        std::string _method;
        std::string _uri;
        std::string _version;
        std::map<std::string, std::string> _headers;
        std::vector<char> _body;

    public:
        HttpRequest(const std::string &method,
                    const std::string &uri,
                    const std::string &version,
                    const std::map<std::string, std::string> &headers,
                    const std::vector<char> &body)
            : _method(method), _uri(uri), _version(version), _headers(headers), _body(body) {}

        const std::string &method() const { return _method; }
        const std::string &uri() const { return _uri; }
        const std::string &version() const { return _version; }
        const std::map<std::string, std::string> &headers() const { return _headers; }
        const std::vector<char> &body() const { return _body; }
    };

    struct HttpRequestLine
    {
        std::string method;
        std::string uri;
        std::string version;
    };

    class HttpRequestParser
    {
    private:
        static HttpRequestLine parse_request_line(const std::vector<char> &raw_request, size_t &pos);
        static std::map<std::string, std::string> parse_headers(const std::vector<char> &raw_request, size_t &pos);
        static std::vector<char> parse_body(const std::vector<char> &raw_request, size_t &pos, const std::map<std::string, std::string> &headers);

    public:
        static HttpRequest parse(const std::vector<char> &raw_request);
    };

    class HttpResponse
    {
    private:
        std::string _version;
        int _status_code;
        std::string _status_message;
        std::map<std::string, std::string> _headers;
        std::vector<char> _body;

    public:
        HttpResponse(const std::string &version,
                     int status_code,
                     const std::string &status_message,
                     const std::map<std::string, std::string> &headers,
                     const std::vector<char> &body)
            : _version(version), _status_code(status_code), _status_message(status_message), _headers(headers), _body(body) {}

        const std::string &version() const { return _version; }
        int status_code() const { return _status_code; }
        const std::string &status_message() const { return _status_message; }
        const std::map<std::string, std::string> &headers() const { return _headers; }
        const std::vector<char> &body() const { return _body; }
        void send(tcp::ConnectionSocket &client_socket);
    };
}

#endif