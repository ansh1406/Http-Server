#ifndef HTTP_EXCEPTIONS_HPP
#define HTTP_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

namespace http
{
    namespace exceptions
    {
        
        class CanNotSendResponse : public std::runtime_error
        {
        public:
            CanNotSendResponse(const std::string& message = "")
                : std::runtime_error("HTTP: Unable to send HTTP response" + (message.empty() ? "" : "\n" + message)) {}
        };

        class UnexpectedEndOfStream : public std::runtime_error
        {
        public:
            UnexpectedEndOfStream(const std::string& message = "")
                : std::runtime_error("HTTP: Unexpected end of stream" + (message.empty() ? "" : "\n" + message)) {}
        };

        class InvalidRequestLine : public std::runtime_error
        {
        public:
            InvalidRequestLine(const std::string& message = "")
                : std::runtime_error("HTTP: Invalid HTTP request line" + (message.empty() ? "" : "\n" + message)) {}
        };

        class RequestLineTooLong : public std::runtime_error
        {
        public:
            RequestLineTooLong(const std::string& message = "")
                : std::runtime_error("HTTP: HTTP request line too long" + (message.empty() ? "" : "\n" + message)) {}
        };

        class HeadersTooLarge : public std::runtime_error
        {
        public:
            HeadersTooLarge(const std::string& message = "")
                : std::runtime_error("HTTP: HTTP header too large" + (message.empty() ? "" : "\n" + message)) {}
        };

        class TransferEncodingWithoutChunked : public std::runtime_error
        {
        public:
            TransferEncodingWithoutChunked(const std::string& message = "")
                : std::runtime_error("HTTP: Transfer-Encoding header is present without 'chunked' value" + (message.empty() ? "" : "\n" + message)) {}
        };

        class InvalidContentLength : public std::runtime_error
        {
        public:
            InvalidContentLength(const std::string& message = "")
                : std::runtime_error("HTTP: Invalid Content-Length header value" + (message.empty() ? "" : "\n" + message)) {}
        };

        class MultipleContentLengthHeaders : public std::runtime_error
        {
        public:
            MultipleContentLengthHeaders(const std::string& message = "")
                : std::runtime_error("HTTP: Multiple Content-Length headers present" + (message.empty() ? "" : "\n" + message)) {}
        };

        class BothContentLengthAndChunked : public std::runtime_error
        {
        public:
            BothContentLengthAndChunked(const std::string& message = "")
                : std::runtime_error("HTTP: Both Content-Length and Transfer-Encoding headers present" + (message.empty() ? "" : "\n" + message)) {}
        };

        class InvalidChunkedEncoding : public std::runtime_error
        {
        public:
            InvalidChunkedEncoding(const std::string& message = "")
                : std::runtime_error("HTTP: Invalid chunked encoding" + (message.empty() ? "" : "\n" + message)) {}
        };

        class VersionNotSupported : public std::runtime_error
        {
        public:
            VersionNotSupported(const std::string& message = "")
                : std::runtime_error("HTTP: HTTP version not supported" + (message.empty() ? "" : "\n" + message)) {}
        };
    }
}

#endif // HTTP_EXCEPTIONS_HPP