#include "includes/http_parser.hpp"
#include "includes/http_exceptions.hpp"
#include "includes/http_request.hpp"
#include "includes/http_constants.hpp"

#include <cctype>

http::HttpRequestLine http::HttpRequestParser::parse_request_line(const std::vector<char> &raw_request, size_t &pos)
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

std::map<std::string, std::string> http::HttpRequestParser::parse_headers(const std::vector<char> &raw_request, size_t &pos)
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

std::vector<char> http::HttpRequestParser::parse_body(const std::vector<char> &raw_request, size_t &pos, const std::map<std::string, std::string> &headers)
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

http::HttpRequest http::HttpRequestParser::parse(const std::vector<char> &raw_request)
{
    size_t pos = 0;

    auto request_line = parse_request_line(raw_request, pos);
    auto headers = parse_headers(raw_request, pos);
    auto body = parse_body(raw_request, pos, headers);

    http::HttpRequest request(request_line.method, request_line.uri, request_line.version, headers, body);
    return request;
}

std::string http::HttpRequestParser::path_from_uri(const std::string &uri)
{
    size_t query_pos = uri.find('?');
    std::string path;
    if (query_pos != std::string::npos)
    {
        path = uri.substr(0, query_pos);
    }
    path = uri;

    path.push_back('/');

    std::vector<std::string> segments;
    size_t start = 0;
    size_t end = 0;
    while ((end = path.find('/', start)) != std::string::npos)
    {
        std::string segment = path.substr(start, end - start);
        if (segment == "..")
        {
            if (!segments.empty())
            {
                segments.pop_back();
            }
        }
        else if (segment != "." && !segment.empty())
        {
            segments.push_back(segment);
        }
        start = end + 1;
    }

    std::string normalized_path;
    for (const auto &segment : segments)
    {
        normalized_path += "/" + segment;
    }
    return normalized_path.empty() ? "/" : normalized_path;
}

bool http::HttpRequestParser::validate_request_line(const std::vector<char> &request_line)
{
    // Method SP Request-URI SP HTTP-Version CRLF
    // SP count should be 2 in a valid request line
    int space_count = 0;
    for (char c : request_line)
    {
        if (c == ' ')
        {
            ++space_count;
        }
    }
    return space_count == 2;
}

bool http::HttpRequestParser::is_transfer_encoding_chunked_header(const std::vector<char> &header)
{
    std::string header_line(header.begin(), header.end());
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

long http::HttpRequestParser::is_content_length_header(const std::vector<char> &header)
{
    std::string header_line(header.begin(), header.end());
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
        catch (const std::invalid_argument &)
        {
            throw http::exceptions::InvalidContentLength();
        }
        catch (const std::out_of_range &)
        {
            throw http::exceptions::InvalidContentLength();
        }
    }
    return -1;
}