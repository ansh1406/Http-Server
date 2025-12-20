#include "includes/http_parser.hpp"

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
