#ifndef HTTP_EXCEPTIONS_HPP
#define HTTP_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace http
{
    namespace exceptions
    {
        class CanNotCreateServer : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "Unable to create server.";
            }
        };

        class BadRequest : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "400 Bad Request";
            }
        };

        class URITooLong : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "414 URI Too Long";
            }
        };

        class HeaderFieldsTooLarge : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "431 Header Fields Too Large";
            }
        };

        class PayloadTooLarge : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "413 Payload Too Large";
            }
        };

        class InternalServerError : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "500 Internal Server Error";
            }
        };

        class NotFound : public std::exception
        {
        public:
            const char *what() const noexcept override
            {
                return "404 Not Found";
            }
        };
        
        class CanNotSendResponse : public std::runtime_error
        {
        public:
            CanNotSendResponse() : std::runtime_error("Unable to send HTTP response") {}
        };

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
    }
}

#endif // HTTP_EXCEPTIONS_HPP