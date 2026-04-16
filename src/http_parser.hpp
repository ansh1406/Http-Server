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
        /// @return Returns a map of header key-value pairs. All keys are converted to lowercase.
        static std::map<std::string, std::string> parse_headers(const std::vector<char> &raw_request, size_t &pos);

        /// @brief Validates the request line format.
        /// @param request_line_byte_buffer vector of chars representing the request line.
        /// @return Returns true if the request line is valid, false otherwise.
        static bool validate_request_line(const std::vector<char> &request_line_byte_buffer);

        /// @brief Checks if the header is a Content-Length header and returns its value if it is.
        /// @param header_byte_buffer vector of chars representing the header line.
        /// @return Returns the value of the Content-Length header if it exists, -1 otherwise.
        /// @throws http::exceptions::InvalidContentLength if the header is Content-Length but Content-Length value is invalid (e.g., negative or non-numeric).
        static long has_content_length_header(const std::map<std::string, std::string> &headers);

        /// @brief Checks if the header is a Transfer-Encoding header with chunked value.
        /// @param header_byte_buffer vector of chars representing the header line.
        /// @return Returns true if the Transfer-Encoding header is chunked, false otherwise.
        /// @throws http::exceptions::TransferEncodingWithoutChunked if the header is Transfer-Encoding but value is not chunked.
        static bool has_transfer_encoding_chunked_header(const std::map<std::string, std::string> &headers);

        static size_t encode_response_status_line(const std::string &version, int status_code, const std::string &reason_phrase, std::vector<char> &buffer, size_t cursor = 0);

        static size_t encode_response_header(const std::string &header, const std::string &value, std::vector<char> &buffer, size_t cursor = 0);

        static size_t encode_end_of_headers(std::vector<char> &buffer, size_t cursor = 0);

        static size_t encode_chunksize_line(size_t chunk_size, unsigned int width, std::vector<char> &buffer, size_t cursor = 0);

        static size_t encode_chunk_end(std::vector<char> &buffer, size_t cursor = 0);
    };
}

#endif