#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "http_constants.hpp"

#include <string>
#include <map>
#include <vector>

namespace http
{
    /// @brief Class representing an HTTP response.
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

        HttpResponse() : _version(versions::HTTP_1_1) , _status_code(0) {}

        explicit HttpResponse(int status_code,
                              const std::string &status_message)
            : _version(versions::HTTP_1_1), _status_code(status_code), _status_message(status_message) {}

        /// @return HTTP version as a std::string.
        const std::string &version() const { return _version; }
        /// @return HTTP status code as an int.
        int status_code() const { return _status_code; }
        /// @return HTTP status message as a std::string.
        const std::string &status_message() const { return _status_message; }
        /// @return HTTP headers as a map of Header key(std::string)-value(std::string) pairs.
        const std::map<std::string, std::string> &headers() const { return _headers; }
        /// @return HTTP body as a vector of chars.
        const std::vector<char> &body() const { return _body; }

        /// @brief Sets the HTTP status code.
        void set_status_code(int status_code) { _status_code = status_code; }
        /// @brief Sets the HTTP status message.
        void set_status_message(const std::string &status_message) { _status_message = status_message; }
        /// @brief Adds or updates body of the HTTP response.
        /// @param body vector of chars representing the body content.
        void set_body(const std::vector<char> &body) { _body = body; }
        /// @brief Adds or updates a header in the HTTP response.
        /// @param key Header key as a std::string.
        /// @param value Header value as a std::string.
        void add_header(const std::string key, const std::string value) { _headers[key] = value; }

        explicit operator bool() const{
            return _status_code != 0;
        }
    };
}

#endif // HTTP_RESPONSE_HPP