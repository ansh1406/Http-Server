/// @file http_response.hpp
/// @brief This file defines the HttpResponse class, which represents an HTTP response.

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
        /// @brief  The HTTP version (e.g., HTTP/1.1). It is set to HTTP/1.1 by default and cannot be changed because of library constraints.
        std::string _version;
        /// @brief The HTTP status code (e.g., 200, 404).
        int _status_code;
        /// @brief The HTTP resaon phrase (e.g., "OK", "Not Found").
        std::string _status_message;
        /// @brief A map of HTTP headers. The keys are header names (case-insensitive), and the values are header values.
        std::map<std::string, std::string> _headers;
        /// @brief The body of the HTTP response, stored as a vector of bytes. It will be empty for responses that do not have a body.
        std::vector<char> _body;

    public:
        /// @brief Constructor for HttpResponse with all parameters.
        /// @param status_code The HTTP status code (e.g., 200, 404).
        /// @param status_message The HTTP status message (e.g., "OK", "Not Found").
        /// @param headers A map of HTTP headers. The keys are header names (case-insensitive), and the values are header values.
        /// @param body The body of the HTTP response, stored as a vector of bytes. It will be empty for responses that do not have a body.
        HttpResponse(int status_code,
                     const std::string &status_message,
                     const std::map<std::string, std::string> &headers,
                     const std::vector<char> &body)
            : _version(versions::HTTP_1_1), _status_code(status_code), _status_message(status_message), _headers(headers), _body(body) {}

        /// @brief Default constructor for HttpResponse. Initializes an empty HTTP response with HTTP version set to HTTP/1.1. Verison is set to HTTP/1.1 by default and cannot be changed because of library constraints.
        HttpResponse() : _version(versions::HTTP_1_1) {}

        /// @brief Constructor for HttpResponse with status code and message.
        /// @param status_code The HTTP status code (e.g., 200, 404).
        /// @param status_message The HTTP status message (e.g., "OK", "Not Found").
        explicit HttpResponse(int status_code,
                              const std::string &status_message)
            : _version(versions::HTTP_1_1), _status_code(status_code), _status_message(status_message) {}

        /// @return HTTP version as a std::string.
        const std::string &version() const noexcept { return _version; }
        /// @return HTTP status code as an int.
        int status_code() const noexcept { return _status_code; }
        /// @return HTTP status message as a std::string.
        const std::string &status_message() const noexcept { return _status_message; }
        /// @return HTTP headers as a map of Header key(std::string)-value(std::string) pairs.
        const std::map<std::string, std::string> &headers() const noexcept { return _headers; }
        /// @return HTTP body as a vector of chars.
        const std::vector<char> &body() const noexcept { return _body; }

        /// @brief Sets the HTTP status code.
        void set_status_code(int status_code) noexcept { _status_code = status_code; }
        /// @brief Sets the HTTP status message.
        void set_status_message(const std::string &status_message) { _status_message = status_message; }
        /// @brief Adds or updates body of the HTTP response.
        /// @param body vector of chars representing the body content.
        void set_body(const std::vector<char> &body) { _body = body; }
        /// @brief Adds or updates a header in the HTTP response.
        /// @param key Header key as a std::string.
        /// @param value Header value as a std::string.
        void add_header(const std::string key, const std::string value) { _headers[key] = value; }
    };
}

#endif // HTTP_RESPONSE_HPP