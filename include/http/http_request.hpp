/// @file http_request.hpp
/// @brief This file defines the HttpRequest class, which represents an HTTP request.

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <stdexcept>
namespace http
{
    /// @brief Container for HTTP request data.
    class HttpRequest
    {
    public:
        /// @brief A stream-like interface for reading the body of an HTTP request.
        struct RequestBodyStream
        {
        private:
            struct Impl;
            Impl *pimpl;

            RequestBodyStream();

        public:
            struct StreamError : public std::runtime_error
            {
                explicit StreamError(const std::string &message) : std::runtime_error(message) {}
            };

            ~RequestBodyStream();

            RequestBodyStream(const RequestBodyStream &) = delete;
            RequestBodyStream &operator=(const RequestBodyStream &) = delete;
            RequestBodyStream(RequestBodyStream &&other) noexcept;
            RequestBodyStream &operator=(RequestBodyStream &&other) noexcept;

            /// @brief Checks if there is more data available in the stream.
            /// @return True if there is more data, false otherwise.
            bool has_more_data() const;

            /// @brief Checks if the stream is closed.
            /// @return True if the stream is closed, false otherwise.
            bool is_stream_closed() const;

            /// @brief Reads the next chunk of data from the stream.
            /// @param buffer The buffer to read data into.
            /// @param buffer_cursor The position in the buffer to start reading from.
            /// @param max_size The maximum number of bytes to read.
            /// @return The number of bytes read.
            /// @throws StreamError if an error occurs while reading.
            size_t get_next(std::vector<char> &buffer, size_t buffer_cursor = 0, size_t max_size = static_cast<size_t>(-1)) const;

            friend struct HttpRequestBuilder;
        };

    private:
        /// @brief The IP address of the client making the request.
        std::string _ip;
        /// @brief The port number from which the client is connecting.
        std::string _port;
        ///@brief The HTTP method (e.g., GET, POST).
        std::string _method;
        /// @brief The requested URI (e.g., /index.html) It may also include query parameters , uri stored in the HttpRequest object is a URL encoded value.
        std::string _uri;
        /// @brief The HTTP version (e.g., HTTP/1.1).
        std::string _version;
        /// @brief A map of HTTP headers. The keys are header names (case-insensitive), and the values are header values.
        std::map<std::string, std::string> _headers;
        /// @brief The body of the HTTP request, stored as a stream of bytes. It will be empty for requests that do not have a body.
        RequestBodyStream _body;

        HttpRequest();

    public:
        ~HttpRequest() = default;

        HttpRequest(const HttpRequest &) = delete;
        HttpRequest &operator=(const HttpRequest &) = delete;
        HttpRequest(HttpRequest &&other) noexcept;
        HttpRequest &operator=(HttpRequest &&other) noexcept;

        /// @return The IP address of the client making the request as a std::string.
        const std::string &ip() const noexcept;

        /// @return The port number from which the client is connecting as a std::string.
        const std::string &port() const noexcept;

        /// @return The HTTP method (e.g., GET, POST) as a std::string.
        const std::string &method() const noexcept;

        /// @return The requested URI (e.g., /index.html) as a std::string. It is stored as a URL encoded value.
        const std::string &uri() const noexcept;

        /// @return The HTTP version (e.g., HTTP/1.1) as a std::string.
        const std::string &version() const noexcept;

        /// @return The HTTP headers as a map of header key(std::string)-value(std::string) pairs.
        const std::map<std::string, std::string> &headers() const noexcept;

        /// @return The body of the HTTP request.
        const RequestBodyStream &body() const noexcept;

        friend struct HttpRequestBuilder;
    };
}

#endif // HTTP_REQUEST_HPP