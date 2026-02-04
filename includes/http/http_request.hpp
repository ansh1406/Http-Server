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
    public:
        std::string method;
        std::string uri;
        std::string version;
        std::map<std::string, std::string> headers;
        std::vector<char> body;

        HttpRequest() = default;
        HttpRequest(const std::string &method,
                    const std::string &uri,
                    const std::string &version,
                    const std::map<std::string, std::string> &headers,
                    const std::vector<char> &body)
            : method(method), uri(uri), version(version), headers(headers), body(body) {}
    };
}

#endif // HTTP_REQUEST_HPP