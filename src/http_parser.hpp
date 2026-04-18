/// @file http_parser.hpp
/// @brief Header file for the HttpParser class, which provides static methods for parsing HTTP requests

#ifndef HTTP_PARSER
#define HTTP_PARSER

#include <string>
#include <map>
#include <unordered_map>
#include <vector>

namespace http
{
    /// @brief Struct representing the components of an HTTP request line.
    struct HttpRequestLine
    {
        /// @brief The HTTP method (e.g., GET, POST).
        std::string method;
        /// @brief The URI of the request (e.g., /index.html).
        std::string uri;
        /// @brief The HTTP version (e.g., HTTP/1.1).
        std::string version;
    };

    class HttpResponse;

    /// @brief Wrapper class for parsing methods.
    class HttpParser
    {
    public:
        /// @brief Parses the request line from the raw HTTP request
        /// @param raw_request vector of chars representing the raw HTTP request.
        /// @param cursor Starting position for parsing.
        /// @return Returns an HttpRequestLine struct containing method, uri, and version.
        /// Parsing stops at CRLF for the request line.
        static HttpRequestLine parse_request_line(const std::vector<char> &raw_request, size_t cursor = 0);

        /// @brief Parses the headers from the raw HTTP request
        /// @param raw_request vector of chars representing the raw HTTP request.
        /// @param cursor Starting position for parsing.
        /// @return Returns a map of header key-value pairs. Header names are normalized to lowercase.
        static std::map<std::string, std::string> parse_headers(const std::vector<char> &raw_request, size_t cursor = 0);

        /// @brief Validates the request line format.
        /// @param request_line_byte_buffer vector of chars representing the request line.
        /// @return Returns true if the request line is valid, false otherwise.
        static bool validate_request_line(const std::vector<char> &request_line_byte_buffer);

        /// @brief Checks if the header list contains a Content-Length header and returns its value if present.
        /// @param headers map of header key-value pairs.
        /// @return Returns parsed Content-Length value, or -1 if header is absent.
        static long has_content_length_header(const std::map<std::string, std::string> &headers);
        static long has_content_length_header(const std::unordered_map<std::string, std::string> &headers);

        /// @brief Checks if the header list contains a Transfer-Encoding header with chunked value as it's last value.
        /// @param headers map of header key-value pairs.
        /// @return Returns true only if the final Transfer-Encoding token is "chunked".
        static bool has_transfer_encoding_chunked_header(const std::map<std::string, std::string> &headers);
        static bool has_transfer_encoding_chunked_header(const std::unordered_map<std::string, std::string> &headers);

        /// @brief Encodes the response status line into the buffer.
        /// @param version Http version string (e.g., "HTTP/1.1").
        /// @param status_code Status code for the response (e.g., 200, 404).
        /// @param reason_phrase Reason phrase (e.g., "OK", "Not Found").
        /// @param buffer Vector of chars to which the encoded status line will be appended.
        /// @param cursor Position in the buffer where the encoded status line should be written. Defaults to 0.
        /// @return No of bytes written to the buffer.
        static size_t encode_response_status_line(const std::string &version, int status_code, const std::string &reason_phrase, std::vector<char> &buffer, size_t cursor = 0);

        /// @brief Encodes a single header line into the buffer.
        /// @param header Header name (e.g., "Content-Type").
        /// @param value Header value (e.g., "text/html").
        /// @param buffer Vector of chars to which the encoded header will be appended.
        /// @param cursor Position in the buffer where the encoded header should be written. Defaults to 0.
        /// @return Number of bytes written to the buffer.
        static size_t encode_response_header(const std::string &header, const std::string &value, std::vector<char> &buffer, size_t cursor = 0);

        /// @brief Encodes the end of headers marker into the buffer.
        /// @param buffer Vector of chars to which the marker will be appended.
        /// @param cursor Position in the buffer where the marker should be written. Defaults to 0.
        /// @return Number of bytes written to the buffer.
        static size_t encode_end_of_headers(std::vector<char> &buffer, size_t cursor = 0);

        /// @brief Encodes a chunk size line into the buffer.
        /// @param chunk_size Size of the chunk as non-negative decimal integer.
        /// @param width Width of the chunk size when converted to hexadecimal.
        /// @param buffer Vector of chars to which the encoded line will be appended.
        /// @param cursor Position in the buffer where the encoded line should be written. Defaults to 0.
        /// @return Number of bytes written to the buffer.
        static size_t encode_chunksize_line(size_t chunk_size, unsigned int width, std::vector<char> &buffer, size_t cursor = 0);

        /// @brief Encodes the end of a chunk into the buffer.
        /// @param buffer Vector of chars to which the marker will be appended.
        /// @param cursor Position in the buffer where the marker should be written. Defaults to 0.
        /// @return Number of bytes written to the buffer.
        static size_t encode_chunk_end(std::vector<char> &buffer, size_t cursor = 0);
    };
}

#endif