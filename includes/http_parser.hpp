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

    /// @brief Wrapper class for parsing methdos.
    class HttpRequestParser
    {
    private:
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

    public:
        /// @brief Parses a raw HTTP request into an HttpRequest object.
        /// @param raw_request vector of chars representing the raw HTTP request.
        /// @return Returns an HttpRequest object.
        static HttpRequest parse(const std::vector<char> &raw_request);
        /// @brief Extracts the path from a given URI.
        static std::string path_from_uri(const std::string &uri);
        /// @brief Validates the request line format.
        static bool validate_request_line(const std::vector<char> &request_line);
        /// @brief Checks if the header is a Content-Length header and returns its value.
        static long is_content_length_header(const std::vector<char> &header);
        /// @brief Checks if the header is a Transfer-Encoding: chunked header.
        static bool is_transfer_encoding_chunked_header(const std::vector<char> &header);
    };
}

#endif