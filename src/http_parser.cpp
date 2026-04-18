#include "http_parser.hpp"
#include "http_exceptions.hpp"

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/http_constants.hpp"

#include <cctype>
#include <algorithm>
#include <unordered_set>
#include <stdexcept>
#include <cstring>

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

    template <typename HeaderContainer>
    bool has_transfer_encoding_chunked_header_impl(const HeaderContainer &headers)
    {
        auto it = headers.find(http::headers::TRANSFER_ENCODING);
        if (it != headers.end())
        {
            const std::string &value = it->second;

            size_t last_comma = value.rfind(',');

            std::string last_token;
            if (last_comma == std::string::npos)
            {
                last_token = value;
            }
            else
            {
                last_token = value.substr(last_comma + 1);
            }

            size_t start = last_token.find_first_not_of(" \t");
            size_t end = last_token.find_last_not_of(" \t");

            if (start != std::string::npos)
            {
                last_token = last_token.substr(start, end - start + 1);
            }
            else
            {
                last_token.clear();
            }

            if (last_token == "chunked")
            {
                return true;
            }
        }
        return false;
    }

    template <typename HeaderContainer>
    long has_content_length_header_impl(const HeaderContainer &headers)
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
}

http::HttpRequestLine http::HttpParser::parse_request_line(const std::vector<char> &raw_request, size_t cursor)
{
    http::HttpRequestLine request_line;
    size_t start = cursor;
    // Find method
    while (cursor < raw_request.size() && raw_request[cursor] != ' ')
    {
        cursor++;
    }
    request_line.method = std::string(raw_request.begin() + start, raw_request.begin() + cursor);
    cursor++;

    // Find URI
    start = cursor;
    while (cursor < raw_request.size() && raw_request[cursor] != ' ')
    {
        cursor++;
    }
    request_line.uri = std::string(raw_request.begin() + start, raw_request.begin() + cursor);
    cursor++;

    // Find HTTP version
    start = cursor;
    while (cursor < raw_request.size() && !(raw_request[cursor] == '\r' && raw_request[cursor + 1] == '\n'))
    {
        cursor++;
    }
    request_line.version = std::string(raw_request.begin() + start, raw_request.begin() + cursor);
    cursor += 2;

    return request_line;
}

std::map<std::string, std::string> http::HttpParser::parse_headers(const std::vector<char> &raw_request, size_t cursor)
{
    std::map<std::string, std::string> headers;
    while (cursor < raw_request.size())
    {
        if (raw_request[cursor] == '\r' && raw_request[cursor + 1] == '\n')
        {
            cursor += 2;
            break;
        }

        size_t start = cursor;
        while (cursor < raw_request.size() && raw_request[cursor] != ':')
        {
            cursor++;
        }
        std::string key = std::string(raw_request.begin() + start, raw_request.begin() + cursor);
        for (auto &c : key)
            c = std::tolower(c);
        cursor++;

        while (cursor < raw_request.size() && (raw_request[cursor] == ' ' || raw_request[cursor] == '\t'))
        {
            cursor++;
        }

        start = cursor;
        while (cursor < raw_request.size() && !(raw_request[cursor] == '\r' && raw_request[cursor + 1] == '\n'))
        {
            cursor++;
        }
        std::string value = std::string(raw_request.begin() + start, raw_request.begin() + cursor);
        cursor += 2;

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
    return has_transfer_encoding_chunked_header_impl(headers);
}

long http::HttpParser::has_content_length_header(const std::map<std::string, std::string> &headers)
{
    return has_content_length_header_impl(headers);
}

bool http::HttpParser::has_transfer_encoding_chunked_header(const std::unordered_map<std::string, std::string> &headers)
{
    return has_transfer_encoding_chunked_header_impl(headers);
}

long http::HttpParser::has_content_length_header(const std::unordered_map<std::string, std::string> &headers)
{
    return has_content_length_header_impl(headers);
}

size_t http::HttpParser::encode_response_status_line(const std::string &version, int status_code, const std::string &reason_phrase, std::vector<char> &buffer, size_t cursor)
{
    // version status-code reason_phrase\r\n
    std::string status_line = version + " " + std::to_string(status_code) + " " + reason_phrase + "\r\n";
    if (status_line.size() + cursor > buffer.size())
    {
        return 0;
    }

    memcpy(buffer.data() + cursor, status_line.data(), status_line.size());
    return status_line.size();
}

size_t http::HttpParser::encode_response_header(const std::string &header, const std::string &value, std::vector<char> &buffer, size_t cursor)
{
    // header:value\r\n
    const size_t required_size = header.size() + value.size() + 3; // ':' + "\r\n"
    if (cursor + required_size > buffer.size())
    {
        return 0;
    }

    memcpy(buffer.data() + cursor, header.data(), header.size());
    cursor += header.size();

    buffer[cursor++] = ':';
    memcpy(buffer.data() + cursor, value.data(), value.size());
    cursor += value.size();

    buffer[cursor++] = '\r';
    buffer[cursor++] = '\n';

    return required_size;
}

size_t http::HttpParser::encode_end_of_headers(std::vector<char> &buffer, size_t cursor)
{
    // \r\n
    if (cursor + 2 > buffer.size())
    {
        return 0;
    }

    buffer[cursor++] = '\r';
    buffer[cursor++] = '\n';

    return 2;
}

size_t http::HttpParser::encode_chunksize_line(size_t chunk_size, unsigned int width, std::vector<char> &buffer, size_t cursor)
{
    // XXXX\r\n X = Hex digit.
    size_t digits = width;
    size_t value = chunk_size;

    if (cursor + digits + 2 > buffer.size())
    {
        return 0;
    }

    for (size_t place_no = 0; place_no < digits; ++place_no)
    {
        unsigned int digit = static_cast<unsigned int>(chunk_size % 16);
        size_t place = cursor + digits - 1 - place_no;
        if (digit < 10)
        {
            buffer[place] = static_cast<char>('0' + digit);
        }
        else
        {
            buffer[place] = static_cast<char>('a' + digit - 10);
        }
        chunk_size /= 16;
    }

    cursor += digits;
    buffer[cursor++] = '\r';
    buffer[cursor++] = '\n';

    return digits + 2;
}

size_t http::HttpParser::encode_chunk_end(std::vector<char> &buffer, size_t cursor)
{
    // \r\n
    if (cursor + 2 > buffer.size())
    {
        return 0;
    }

    buffer[cursor++] = '\r';
    buffer[cursor++] = '\n';

    return 2;
}