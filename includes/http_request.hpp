#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>

namespace http
{
    /// @brief Represents an HTTP request.
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

        /// @return The HTTP method as a string.
        const std::string &method() const { return _method; }
        /// @return The request URI as a string.
        const std::string &uri() const { return _uri; }
        /// @return The HTTP version as a string.
        const std::string &version() const { return _version; }
        /// @return The headers as a map of strings.
        const std::map<std::string, std::string> &headers() const { return _headers; }
        /// @return The body as a vector of characters.
        const std::vector<char> &body() const { return _body; }
    };
}

#endif // HTTP_REQUEST_HPP