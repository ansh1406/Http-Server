#include "http_parser.hpp"
#include "http_exceptions.hpp"

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "http/http_constants.hpp"

#include <cctype>

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

        headers[key] = value;
    }
    return headers;
}

std::vector<char> http::HttpParser::parse_body(const std::vector<char> &raw_request, size_t &pos, const std::map<std::string, std::string> &headers)
{
    std::vector<char> body;
    auto it = headers.find(http::headers::CONTENT_LENGTH);
    if (it != headers.end())
    {
        size_t content_length = std::stoul(it->second);
        body.insert(body.end(), raw_request.begin() + pos, raw_request.begin() + pos + content_length);
        pos += content_length;
    }
    else if (headers.find(http::headers::TRANSFER_ENCODING) != headers.end())
    {
        while (true)
        {
            size_t start = pos;
            while (pos < raw_request.size() && !(raw_request[pos] == '\r' && raw_request[pos + 1] == '\n'))
            {
                pos++;
            }
            std::string chunk_size_str(raw_request.begin() + start, raw_request.begin() + pos);
            size_t chunk_size = std::stoul(chunk_size_str, nullptr, 16);
            pos += 2;

            if (chunk_size == 0)
            {
                break;
            }

            body.insert(body.end(), raw_request.begin() + pos, raw_request.begin() + pos + chunk_size);
            pos += chunk_size + 2; // Skip CRLF
        }
    }
    return body;
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

bool http::HttpParser::is_transfer_encoding_chunked_header(const std::vector<char> &header_byte_buffer)
{
    std::string header_line(header_byte_buffer.begin(), header_byte_buffer.end());
    size_t colon_pos = header_line.find(':');
    if (colon_pos == std::string::npos)
    {
        return false;
    }

    std::string key = header_line.substr(0, colon_pos);
    for (auto &c : key)
        c = std::tolower(c);
    std::string value = header_line.substr(colon_pos + 1);

    while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
        value.erase(value.begin());
    if (key == http::headers::TRANSFER_ENCODING)
    {
        size_t last_comma = value.find_last_of(',');
        std::string last_encoding = (last_comma == std::string::npos) ? value : value.substr(last_comma + 1);

        while (!last_encoding.empty() && (last_encoding.front() == ' ' || last_encoding.front() == '\t'))
            last_encoding.erase(last_encoding.begin());

        while (!last_encoding.empty() && (last_encoding.back() == ' ' || last_encoding.back() == '\t'))
            last_encoding.pop_back();

        if (last_encoding != "chunked")
        {
            throw http::exceptions::TransferEncodingWithoutChunked();
        }
        return true;
    }
    return false;
}

long http::HttpParser::is_content_length_header(const std::vector<char> &header_byte_buffer)
{
    std::string header_line(header_byte_buffer.begin(), header_byte_buffer.end());
    size_t colon_pos = header_line.find(':');
    if (colon_pos == std::string::npos)
    {
        return -1;
    }
    std::string key = header_line.substr(0, colon_pos);
    for (auto &c : key)
        c = std::tolower(c);
    std::string value = header_line.substr(colon_pos + 1);
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
        value.erase(value.begin());
    if (key == http::headers::CONTENT_LENGTH)
    {
        try
        {
            size_t content_length = std::stol(value);
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

std::vector<char> http::HttpParser::create_response_buffer(const http::HttpResponse &response)
{
    std::vector<char> buffer;
    std::string status_line = response.version() + " " + std::to_string(response.status_code()) + " " + response.status_message() + "\r\n";
    buffer.insert(buffer.end(), status_line.begin(), status_line.end());

    for (const auto &header : response.headers())
    {
        std::string header_line = header.first + ": " + header.second + "\r\n";
        buffer.insert(buffer.end(), header_line.begin(), header_line.end());
    }

    buffer.insert(buffer.end(), '\r');
    buffer.insert(buffer.end(), '\n');

    buffer.insert(buffer.end(), response.body().begin(), response.body().end());

    return buffer;
}