#include "includes/http.hpp"

http::HttpServer::HttpServer(tcp::Port port)
{
    try
    {
        server_socket = tcp::ListeningSocket(port);
    }
    catch (const tcp::exceptions::CanNotCreateSocket &e)
    {
        std::cerr << "Error opening server socket: " << e.what() << std::endl;
        throw http::exceptions::CanNotCreateServer();
    }
    catch (...)
    {
        std::cerr << "Unknown error opening server socket." << std::endl;
        throw http::exceptions::CanNotCreateServer();
    }
}

void http::HttpServer::start()
{
    while (true)
    {
        try
        {
            tcp::ConnectionSocket client_socket = server_socket.accept_connection();
            HttpConnection connection(std::move(client_socket));
            std::cout << "Connection accepted." << std::endl;
            std::cout << "Ip: " << connection.get_ip() << std::endl;
            std::cout << "Port: " << connection.get_port() << std::endl;
            connection.handle(route_handlers);
        }
        catch (const tcp::exceptions::CanNotAcceptConnection &e)
        {
            std::cerr << "Error accepting connection: " << e.what() << std::endl;
            continue;
        }
        catch (const tcp::exceptions::CanNotSendData &e)
        {
            std::cerr << "Error sending response: " << e.what() << std::endl;
            continue;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Unexpected error: " << e.what() << std::endl;
            continue;
        }
        catch (...)
        {
            std::cerr << "Unknown unexpected error." << std::endl;
            continue;
        }
    }
}

void http::HttpConnection::handle(std::map<std::pair<std::string, std::string>, std::function<void(const http::HttpRequest &, http::HttpResponse &)>> &route_handlers)
{
    std::vector<char> raw_request;
    try
    {
        raw_request = request_reader.read(client_socket);
    }
    catch (const http::exceptions::UnexpectedEndOfStream &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::InvalidRequestLine &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::InvalidChunkedEncoding &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::InvalidContentLength &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::MultipleContentLengthHeaders &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::BothContentLengthAndChunked &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::TransferEncodingWithoutChunked &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::RequestLineTooLong &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::URI_TOO_LONG, "Invalid Request Line"));
        throw http::exceptions::InvalidRequestLine();
    }
    catch (const http::exceptions::HeadersTooLarge &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::HEADERS_TOO_LARGE, "Header Fields Too Large"));
        throw http::exceptions::HeaderFieldsTooLarge();
    }
    catch (const http::exceptions::BodyTooLarge &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::PAYLOAD_TOO_LARGE, "Payload Too Large"));
        throw http::exceptions::PayloadTooLarge();
    }
    catch (...)
    {
        std::cerr << "Unknown error reading request." << std::endl;
        send_response(http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error"));
        throw http::exceptions::InternalServerError();
    }

    HttpRequest request = request_parser.parse(raw_request);

    std::string path = http::HttpRequestParser::path_from_uri(request.uri());
    if (route_handlers.find({request.method(), path}) == route_handlers.end())
    {
        send_response(http::HttpResponse(http::status_codes::NOT_FOUND, "Not Found"));
        throw http::exceptions::NotFound();
    }

    http::HttpResponse response;
    route_handlers[{request.method(), path}](request, response);

    try
    {
        send_response(response);
    }
    catch (const http::exceptions::CanNotSendResponse &e)
    {
        throw;
    }
    catch (...)
    {
        throw;
    }
}

void http::HttpConnection::send_response(const http::HttpResponse &response)
{
    try
    {
        std::string status_line = response.version() + " " + std::to_string(response.status_code()) + " " + response.status_message() + "\r\n";
        client_socket.send_data(std::vector<char>(status_line.begin(), status_line.end()));

        for (const auto &header : response.headers())
        {
            std::string header_line = header.first + ": " + header.second + "\r\n";
            client_socket.send_data(std::vector<char>(header_line.begin(), header_line.end()));
        }

        client_socket.send_data(std::vector<char>({'\r', '\n'}));

        if (!response.body().empty())
        {
            client_socket.send_data(response.body());
        }
    }
    catch (const tcp::exceptions::CanNotSendData &e)
    {
        std::cerr << "Error sending data: " << e.what() << std::endl;
        throw http::exceptions::CanNotSendResponse();
    }
}

void http::HttpServer::add_route_handler(const std::string method, const std::string path, std::function<void(const http::HttpRequest &, http::HttpResponse &)> handler)
{
    route_handlers[{method, path}] = handler;
}

std::vector<char> http::HttpRequestReader::read(tcp::ConnectionSocket &client_socket)
{
    try
    {
        std::vector<char> request_data;
        std::vector<char> temp_buffer;
        bool request_has_body{false};
        bool has_chunked_transfer_encoding{false};
        long content_length{-1};
        // For reading the request line
        while (true) // Read until we find the end of the request line
        {
            long pos{0};
            bool found_end_of_request_line{false};
            for (; pos < (long)buffer.size() - 1; pos++)
            {
                if (buffer[pos] == '\r' && buffer[pos + 1] == '\n') // End of request line found
                {
                    request_data.insert(request_data.end(), buffer.begin(), buffer.begin() + pos + 2);
                    buffer.erase(buffer.begin(), buffer.begin() + pos + 2);
                    found_end_of_request_line = true;
                    break;
                }

                if (pos >= http::constants::MAX_REQUEST_LINE)
                {
                    throw http::exceptions::RequestLineTooLong();
                }
            }
            if (found_end_of_request_line)
                break;
            read_from_tcp(client_socket);
        }

        if (!validate_request_line(request_data))
        {
            throw http::exceptions::InvalidRequestLine();
        }

        // Read the headers
        long header_start = request_data.size();
        while (true)
        {
            long pos{0};
            bool found_end_of_headers{false};
            for (; pos < (long)buffer.size() - 1; pos++)
            {

                if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
                {
                    long content_length_from_header = is_content_length_header(pos);
                    if (content_length != -1 && content_length_from_header != -1)
                    {
                        throw http::exceptions::MultipleContentLengthHeaders();
                    }
                    if (content_length_from_header != -1)
                    {
                        content_length = content_length_from_header;
                        request_has_body = true;
                    }
                    if (is_transfer_encoding_header(pos))
                    {
                        has_chunked_transfer_encoding = true;
                        request_has_body = true;
                    }
                    request_data.insert(request_data.end(), buffer.begin(), buffer.begin() + pos + 2);
                    buffer.erase(buffer.begin(), buffer.begin() + pos + 2);
                    if (pos == 0) // Empty line found, end of headers
                    {
                        found_end_of_headers = true;
                        break;
                    }
                    pos = -1; // Reset pos for next header line
                }

                if ((long)request_data.size() - header_start >= http::constants::MAX_HEADER_SIZE)
                {
                    throw http::exceptions::HeadersTooLarge();
                }
            }
            if (found_end_of_headers)
                break;
            read_from_tcp(client_socket);
        }

        // Read the body if present
        if (request_has_body)
        {
            if (has_chunked_transfer_encoding && content_length != -1)
            {
                throw http::exceptions::BothContentLengthAndChunked();
            }

            if (content_length != -1)
            {
                if (content_length > http::constants::MAX_BODY_SIZE)
                {
                    throw http::exceptions::BodyTooLarge();
                }
                size_t remaining = content_length;
                while (remaining > 0)
                {
                    size_t to_read = std::min(remaining, buffer.size());
                    request_data.insert(request_data.end(), buffer.begin(), buffer.begin() + to_read);
                    buffer.erase(buffer.begin(), buffer.begin() + to_read);
                    remaining -= to_read;

                    if (remaining == 0)
                        break;

                    read_from_tcp(client_socket);
                }
            }
            else if (has_chunked_transfer_encoding)
            {
                while (true)
                {
                    // Read chunk size line
                    std::vector<char> chunk_size_line;
                    while (true)
                    {
                        long pos{0};
                        bool found_end_of_chunk_size_line{false};
                        for (; pos < (long)buffer.size() - 1; pos++)
                        {
                            if (buffer[pos] == '\r' && buffer[pos + 1] == '\n') // End of chunk size line found
                            {
                                chunk_size_line.insert(chunk_size_line.end(), buffer.begin(), buffer.begin() + pos + 2);
                                buffer.erase(buffer.begin(), buffer.begin() + pos + 2);
                                found_end_of_chunk_size_line = true;
                                break;
                            }
                        }
                        if (found_end_of_chunk_size_line)
                            break;
                        read_from_tcp(client_socket);
                    }
                    request_data.insert(request_data.end(), chunk_size_line.begin(), chunk_size_line.end());

                    std::string chunk_size_str(chunk_size_line.begin(), chunk_size_line.end());
                    size_t chunk_size = std::stoul(chunk_size_str, nullptr, 16);

                    // Read the chunk data plus the trailing CRLF
                    size_t remaining = chunk_size + 2; // +2 for CRLF
                    while (remaining > 0)
                    {
                        size_t to_read = std::min(remaining, buffer.size());
                        request_data.insert(request_data.end(), buffer.begin(), buffer.begin() + to_read);
                        buffer.erase(buffer.begin(), buffer.begin() + to_read);
                        remaining -= to_read;
                        if (remaining == 0)
                            break;
                        read_from_tcp(client_socket);
                    }
                    if (chunk_size == 0)
                    {
                        break; // Last chunk
                    }
                }
            }
        }
        return request_data;
    }
    catch (...)
    {
        throw;
    }
}

bool http::HttpRequestReader::validate_request_line(const std::vector<char> &request_line)
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

bool http::HttpRequestReader::is_transfer_encoding_header(const size_t header_end_index)
{
    size_t header_start = 0;
    std::string header_line(buffer.begin() + header_start, buffer.begin() + header_end_index);
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

long http::HttpRequestReader::is_content_length_header(const size_t header_end_index)
{
    size_t header_start = 0;
    std::string header_line(buffer.begin() + header_start, buffer.begin() + header_end_index);
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

void http::HttpRequestReader::read_from_tcp(tcp::ConnectionSocket &client_socket)
{
    try
    {
        std::vector<char> temp_buffer;
        temp_buffer = client_socket.receive_data(http::constants::SINGLE_READ_SIZE);
        if (temp_buffer.empty())
        {
            throw http::exceptions::UnexpectedEndOfStream();
        }
        buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());
    }
    catch (const tcp::exceptions::CanNotReceiveData &e)
    {
        std::cerr << "Error receiving data: " << e.what() << std::endl;
        throw http::exceptions::UnexpectedEndOfStream();
    }
}