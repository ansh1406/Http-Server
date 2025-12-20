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
}

#endif // HTTP_REQUEST_HPP