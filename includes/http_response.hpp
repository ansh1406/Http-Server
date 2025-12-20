#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "http_constants.hpp"

#include <string>
#include <map>
#include <vector>

namespace http
{
    class HttpResponse
    {
    private:
        std::string _version;
        int _status_code;
        std::string _status_message;
        std::map<std::string, std::string> _headers;
        std::vector<char> _body;

    public:
        HttpResponse(int status_code,
                     const std::string &status_message,
                     const std::map<std::string, std::string> &headers,
                     const std::vector<char> &body)
            : _version(versions::HTTP_1_1), _status_code(status_code), _status_message(status_message), _headers(headers), _body(body) {}

        HttpResponse() : _version(versions::HTTP_1_1) {}

        explicit HttpResponse(int status_code,
                              const std::string &status_message)
            : _version(versions::HTTP_1_1), _status_code(status_code), _status_message(status_message) {}

        const std::string &version() const { return _version; }
        int status_code() const { return _status_code; }
        const std::string &status_message() const { return _status_message; }
        const std::map<std::string, std::string> &headers() const { return _headers; }
        const std::vector<char> &body() const { return _body; }

        void set_status_code(int status_code) { _status_code = status_code; }
        void set_status_message(const std::string &status_message) { _status_message = status_message; }
        void set_headers(const std::map<std::string, std::string> &headers) { _headers = headers; }
        void set_body(const std::vector<char> &body) { _body = body; }
    };
}

#endif // HTTP_RESPONSE_HPP