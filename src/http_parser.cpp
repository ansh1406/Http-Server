#include "http_parser.hpp"
#include "http_exceptions.hpp"

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/http_constants.hpp"

#include <cctype>
#include <algorithm>
#include <unordered_set>

namespace
{
    const std::unordered_set<std::string> repeatable_headers = {
        http::headers::ACCEPT,
        http::headers::ACCEPT_ENCODING,
        http::headers::ACCEPT_LANGUAGE,
        http::headers::CACHE_CONTROL,
        http::headers::CONNECTION,
        http::headers::VIA,
        http::headers::WARNING,
        http::headers::IF_MATCH,
        http::headers::IF_NONE_MATCH,
    };
}

http::HttpRequestLine http::HttpParser::parse_request_line(const std::vector<char> &raw_request, size_t &pos)
{
    http::HttpRequestLine request_line;
    size_t start = pos;
    // Find method
    while (pos < raw_request.size() && raw_request[pos] != ' ')
    {
        pos++;
    }
    request_line.method = std::string(raw_request.begin() + start, raw_request.begin() + pos);
    pos++;

    // Find URI
    start = pos;
    while (pos < raw_request.size() && raw_request[pos] != ' ')
    {
        pos++;
    }
    request_line.uri = std::string(raw_request.begin() + start, raw_request.begin() + pos);
    pos++;

    // Find HTTP version
    start = pos;
    while (pos < raw_request.size() && !(raw_request[pos] == '\r' && raw_request[pos + 1] == '\n'))
    {
        pos++;
    }
    request_line.version = std::string(raw_request.begin() + start, raw_request.begin() + pos);
    pos += 2;

    return request_line;
}

std::map<std::string, std::string> http::HttpParser::parse_headers(const std::vector<char> &raw_request, size_t &pos)
{
    std::map<std::string, std::string> headers;
    while (pos < raw_request.size())
    {
        if (raw_request[pos] == '\r' && raw_request[pos + 1] == '\n')
        {
            pos += 2;
            break;
        }

        size_t start = pos;
        while (pos < raw_request.size() && raw_request[pos] != ':')
        {
            pos++;
        }
        std::string key = std::string(raw_request.begin() + start, raw_request.begin() + pos);
        for (auto &c : key)
            c = std::tolower(c);
        pos++;

        while (pos < raw_request.size() && (raw_request[pos] == ' ' || raw_request[pos] == '\t'))
        {
            pos++;
        }

        start = pos;
        while (pos < raw_request.size() && !(raw_request[pos] == '\r' && raw_request[pos + 1] == '\n'))
        {
            pos++;
        }
        std::string value = std::string(raw_request.begin() + start, raw_request.begin() + pos);
        pos += 2;

        if (headers.find(key) != headers.end())
        {
            if (repeatable_headers.find(key) != repeatable_headers.end())
            {
                headers[key] += "," + value;
            }
            else
            {
                throw http::exceptions::InvalidDuplicateHeaders("Duplicate header: " + key);
            }
        }
        else
        {
            headers[key] = value;
        }
    }
    return headers;
}

bool http::HttpParser::validate_request_line(const std::vector<char> &request_line_byte_buffer)
{
    // Method SP Request-URI SP HTTP-Version CRLF
    // SP count should be 2 in a valid request line
    int space_count = 0;
    for (long pos = 0; pos < (long)request_line_byte_buffer.size(); pos++)
    {
        if (request_line_byte_buffer[pos] == ' ')
        {
            ++space_count;
        }
        if (request_line_byte_buffer[pos] == '\r' && request_line_byte_buffer[pos + 1] == '\n')
        {
            break;
        }
    }
    return space_count == 2;
}

bool http::HttpParser::has_transfer_encoding_chunked_header(const std::map<std::string, std::string> &headers)
{
    auto it = headers.find(http::headers::TRANSFER_ENCODING);
    if (it != headers.end())
    {
        const std::string &value = it->second;
        size_t chunked_pos = value.find("chunked");
        if (chunked_pos != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

long http::HttpParser::has_content_length_header(const std::map<std::string, std::string> &headers)
{
    auto it = headers.find(http::headers::CONTENT_LENGTH);
    if (it != headers.end())
    {
        try
        {
            size_t content_length = std::stol(it->second);
            if (content_length < 0)
            {
                throw http::exceptions::InvalidContentLength();
            }
            return content_length;
        }
        catch (...)
        {
            throw http::exceptions::InvalidContentLength();
        }
    }
    return -1;
}