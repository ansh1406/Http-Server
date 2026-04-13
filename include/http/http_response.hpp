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
        struct Impl;
        Impl *pimpl;

        /// @brief  The HTTP version (e.g., HTTP/1.1). It is set to HTTP/1.1 by default and cannot be changed because of library constraints.
        std::string _version;
        /// @brief The HTTP status code (e.g., 200, 404).
        int _status_code;
        /// @brief The HTTP reason phrase (e.g., "OK", "Not Found").
        std::string _status_message;
        /// @brief A map of HTTP headers. The keys are header names (case-insensitive), and the values are header values.
        std::map<std::string, std::string> _headers;

    public:
        using WriterFunction = std::function<long(std::vector<char> &data)>;

        /// @brief Default constructor for HttpResponse. Initializes an empty HTTP response with HTTP version set to HTTP/1.1. Verison is set to HTTP/1.1 by default and cannot be changed because of library constraints.
        HttpResponse();

        /// @brief Constructor for HttpResponse with status code and message.
        /// @param status_code The HTTP status code (e.g., 200, 404).
        /// @param status_message The HTTP status message (e.g., "OK", "Not Found").
        explicit HttpResponse(int status_code,
                              const std::string &status_message);

        ~HttpResponse();

        /// @return HTTP version as a std::string.
        const std::string &version() const noexcept;
        /// @return HTTP status code as an int.
        int status_code() const noexcept;
        /// @return HTTP status message as a std::string.
        const std::string &status_message() const noexcept;
        /// @return HTTP headers as a map of Header key(std::string)-value(std::string) pairs.
        const std::map<std::string, std::string> &headers() const noexcept;

        /// @brief Sets the HTTP status code.
        void set_status_code(int status_code) noexcept;
        /// @brief Sets the HTTP status message.
        void set_status_message(const std::string &status_message);
        /// @brief Sets or updates the body of the HTTP response.
        /// @param body ResponseBodyStream representing the body content.
        void set_body_generator(WriterFunction writer);
        /// @brief Sets the body of the HTTP response.
        /// @param data std::vector<char> representing the body content.
        void set_body(const std::vector<char> &data);
        /// @brief Sets or updates a header in the HTTP response.
        /// @param key Header key as a std::string.
        /// @param value Header value as a std::string.
        void set_header(const std::string &key, const std::string &value);

        friend struct HttpResponseReader;
    };
}

#endif // HTTP_RESPONSE_HPP