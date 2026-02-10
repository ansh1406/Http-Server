/// @file http_request.hpp
/// @brief This file defines the HttpRequest class, which represents an HTTP request.

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>

namespace http
{
    class HttpRequest
    {
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
        /// @brief The body of the HTTP request, stored as a vector of bytes. It will be empty for requests that do not have a body.
        std::vector<char> _body;

        /// @brief Default constructor for HttpRequest. Initializes an empty HTTP request.
        HttpRequest() = default;

        /// @brief Parameterized constructor for HttpRequest. Initializes the HTTP request with the provided method, URI, version, headers, and body.
        /// @param method The HTTP method (e.g., GET, POST).
        /// @param uri The requested URI (e.g., /index.html). Do not decode the URI here. It should be stored as a URL encoded value.
        /// @param version The HTTP version (e.g., HTTP/1.1).
        /// @param headers A map of HTTP headers. The keys are header names (case-insensitive), and the values are header values.
        /// @param body The body of the HTTP request, stored as a vector of bytes. It will be empty for requests that do not have a body
        HttpRequest(const std::string &method,
                    const std::string &uri,
                    const std::string &version,
                    const std::map<std::string, std::string> &headers,
                    const std::vector<char> &body)
            : _method(method), _uri(uri), _version(version), _headers(headers), _body(body) {}

    public:
        /// @return The IP address of the client making the request as a std::string.
        const std::string &ip() const noexcept { return _ip; }
        /// @return The port number from which the client is connecting as a std::string.
        const std::string &port() const noexcept { return _port; }
        /// @return The HTTP method (e.g., GET, POST) as a std::string.
        const std::string &method() const noexcept { return _method; }
        /// @return The requested URI (e.g., /index.html) as a std::string. It is stored as a URL encoded value.
        const std::string &uri() const noexcept { return _uri; }
        /// @return The HTTP version (e.g., HTTP/1.1) as a std::string.
        const std::string &version() const noexcept { return _version; }
        /// @return The HTTP headers as a map of header key(std::string)-value(std::string) pairs.
        const std::map<std::string, std::string> &headers() const noexcept { return _headers; }
        /// @return The body of the HTTP request as a vector of chars.
        const std::vector<char> &body() const noexcept { return _body; }

        friend class HttpConnection; // Allow HttpConnection to access private members of HttpRequest
    };
}

#endif // HTTP_REQUEST_HPP