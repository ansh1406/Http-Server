#ifndef HTTP_PARSER
#define HTTP_PARSER

#include "includes/tcp.hpp"
#include "includes/http_request.hpp"
#include "includes/http_constants.hpp"

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

        class BodyTooLarge : public std::runtime_error
        {
        public:
            BodyTooLarge() : std::runtime_error("Payload too large") {}
        };

        class CanNotSendResponse : public std::runtime_error
        {
        public:
            CanNotSendResponse() : std::runtime_error("Unable to send HTTP response") {}
        };
    }

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
        static std::string path_from_uri(const std::string &uri);
    };

    
}

#endif