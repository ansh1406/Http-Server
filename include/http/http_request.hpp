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
    public:
        ///@brief The HTTP method (e.g., GET, POST).
        std::string method;
        /// @brief The requested URI (e.g., /index.html) It may also include query parameters , uri stored in the HttpRequest object is a URL encoded value.
        std::string uri;
        /// @brief The HTTP version (e.g., HTTP/1.1).
        std::string version;
        /// @brief A map of HTTP headers. The keys are header names (case-insensitive), and the values are header values.
        std::map<std::string, std::string> headers;
        /// @brief The body of the HTTP request, stored as a vector of bytes. It will be empty for requests that do not have a body.
        std::vector<char> body;

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
            : method(method), uri(uri), version(version), headers(headers), body(body) {}
    };
}

#endif // HTTP_REQUEST_HPP