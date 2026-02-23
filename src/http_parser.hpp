#ifndef HTTP_PARSER
#define HTTP_PARSER

#include <string>
#include <map>
#include <vector>

namespace http
{
    struct HttpRequestLine
    {
        std::string method;
        std::string uri;
        std::string version;
    };

    class HttpRequest;
    class HttpResponse;

    /// @brief Wrapper class for parsing methdos.
    class HttpParser
    {
    public:
        /// @brief Parses the request line from the raw HTTP request
        /// @param raw_request vector of chars representing the raw HTTP request.
        /// @param pos Starting position for parsing, will be updated to the position after the request line.
        /// @return Returns an HttpRequestLine struct containing method, uri, and version.
        static HttpRequestLine parse_request_line(const std::vector<char> &raw_request, size_t &pos);

        /// @brief Parses the headers from the raw HTTP request
        /// @param raw_request vector of chars representing the raw HTTP request.
        /// @param pos Starting position for parsing, will be updated to the position after the headers.
        /// @return Returns a map of header key-value pairs.
        static std::map<std::string, std::string> parse_headers(const std::vector<char> &raw_request, size_t &pos);

        /// @brief Parses the body from the raw HTTP request
        /// @param raw_request vector of chars representing the raw HTTP request.
        /// @param pos Starting position for parsing, will be updated to the position after the body
        /// @param headers Map of header key-value pairs to determine body length.
        /// @return Returns a vector of chars representing the body.
        static std::vector<char> parse_body(const std::vector<char> &raw_request, size_t &pos, const std::map<std::string, std::string> &headers);

        /// @brief Validates the request line format.
        /// @param request_line_byte_buffer vector of chars representing the request line.
        /// @return Returns true if the request line is valid, false otherwise.
        static bool validate_request_line(const std::vector<char> &request_line_byte_buffer);

        /// @brief Checks if the header is a Content-Length header and returns its value if it is.
        /// @param header_byte_buffer vector of chars representing the header line.
        /// @return Returns the value of the Content-Length header if it exists, -1 otherwise.
        /// @throws http::exceptions::InvalidContentLength if the header is Content-Length but Content-Length value is invalid (e.g., negative or non-numeric).
        static long is_content_length_header(const std::vector<char> &header_byte_buffer);

        /// @brief Checks if the header is a Transfer-Encoding header with chunked value.
        /// @param header_byte_buffer vector of chars representing the header line.
        /// @return Returns true if the Transfer-Encoding header is chunked, false otherwise.
        /// @throws http::exceptions::TransferEncodingWithoutChunked if the header is Transfer-Encoding but value is not chunked.
        static bool is_transfer_encoding_chunked_header(const std::vector<char> &header_byte_buffer);

        /// @brief Creates a response buffer from an HttpResponse object.
        /// @param response The HttpResponse object to be converted into a response buffer.
        /// @return Returns a vector of chars representing the raw HTTP response that can be sent to the client.
        static std::vector<char> create_response_buffer(const http::HttpResponse &response);
    };
}

#endif