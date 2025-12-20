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

    class HttpRequestParser
    {
    private:
        static HttpRequestLine parse_request_line(const std::vector<char> &raw_request, size_t &pos);
        static std::map<std::string, std::string> parse_headers(const std::vector<char> &raw_request, size_t &pos);
        static std::vector<char> parse_body(const std::vector<char> &raw_request, size_t &pos, const std::map<std::string, std::string> &headers);

    public:
        static HttpRequest parse(const std::vector<char> &raw_request);
        static std::string path_from_uri(const std::string &uri);
        static bool validate_request_line(const std::vector<char> &request_line);
        static long is_content_length_header(const std::vector<char> &header);
        static bool is_transfer_encoding_chunked_header(const std::vector<char> &header);
    };
}

#endif