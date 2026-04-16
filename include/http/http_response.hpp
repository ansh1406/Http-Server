/// @file http_response.hpp
/// @brief This file defines the HttpResponse class, which represents an HTTP response.

#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include "http_constants.hpp"

#include <string>
#include <map>
#include <vector>
#include <functional>

namespace http
{
    /// @brief Container for HTTP response data.
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
        std::string _reason_phrase;
        /// @brief A map of HTTP headers. The keys are header names (case-insensitive), and the values are header values.
        std::map<std::string, std::string> _headers;

    public:
        /// @brief A function type for generating the body of an HTTP response.
        /// This function should write the body content into the provided data vector and return the number of bytes written.
        /// If the body streaming is complete, the function should return -1.
        /// Successive calls are expected to continue where the previous call ended.
        using WriterFunction = std::function<long(std::vector<char> &data)>;

        /// @brief Default constructor for HttpResponse.
        /// Initializes an empty HTTP response with HTTP version set to HTTP/1.1.
        /// Verison is set to HTTP/1.1 by default and cannot be changed because of library constraints.
        HttpResponse();

        /// @brief Constructor for HttpResponse with status code and message.
        /// @param status_code The HTTP status code (e.g., 200, 404).
        /// @param reason_phrase The HTTP status message (e.g., "OK", "Not Found").
        explicit HttpResponse(int status_code,
                              const std::string &reason_phrase);

        ~HttpResponse();

        HttpResponse(const HttpResponse &) = delete;
        HttpResponse &operator=(const HttpResponse &) = delete;
        /// Move keeps response body stream state valid in the destination object.
        HttpResponse(HttpResponse &&other) noexcept;
        HttpResponse &operator=(HttpResponse &&other) noexcept;

        /// @return HTTP version as a std::string.
        const std::string &version() const noexcept;
        /// @return HTTP status code as an int.
        int status_code() const noexcept;
        /// @return HTTP status message as a std::string.
        const std::string &reason_phrase() const noexcept;
        /// @return HTTP headers as a map of Header key(std::string)-value(std::string) pairs.
        const std::map<std::string, std::string> &headers() const noexcept;

        /// @brief Sets the HTTP status code.
        void set_status_code(int status_code) noexcept;

        /// @brief Sets the HTTP status message.
        void set_status_message(const std::string &reason_phrase);

        /// @brief Sets the body generator function.
        /// The function should write the body content into the provided data vector and return the number of bytes written.
        /// If the body streaming is complete, the function should return -1.
        /// Replaces any previously configured body generator/body data source.
        /// @param writer A WriterFunction that generates the body content.
        void set_body_generator(WriterFunction writer);

        /// @brief Sets the body of the HTTP response.
        /// Replaces any previously configured streaming generator.
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