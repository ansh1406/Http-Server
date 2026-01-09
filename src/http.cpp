#include "includes/http.hpp"
#include "includes/http_connection.hpp"
#include "includes/http_exceptions.hpp"
#include "includes/http_parser.hpp"
#include "includes/http_constants.hpp"
#include "includes/http_request.hpp"
#include "includes/http_response.hpp"

#include <iostream>

struct http::HttpServer::Impl
{
    tcp::ListeningSocket server_socket;
    Impl(tcp::ListeningSocket &&sock) : server_socket(std::move(sock)) {}
};

http::HttpServer::HttpServer(HttpServerConfig config)
{
    try
    {
        pimpl = new Impl(std::move(tcp::ListeningSocket(config.port, config.max_pending_connections)));
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

http::HttpServer::~HttpServer()
{
    delete pimpl;
}

void http::HttpServer::start()
{
    while (true)
    {
        try
        {
            tcp::ConnectionSocket client_socket = pimpl->server_socket.accept_connection();
            HttpConnection connection(std::move(client_socket));
            std::cout << "Connection accepted." << std::endl;
            std::cout << "Ip: " << connection.get_ip() << std::endl;
            std::cout << "Port: " << connection.get_port() << std::endl;
            connection.handle(route_handlers);
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Unknown unexpected error." << std::endl;
        }
    }
}

void http::HttpConnection::handle(std::map<std::pair<std::string, std::string>, std::function<void(const http::HttpRequest &, http::HttpResponse &)>> &route_handlers)
{
    std::vector<char> raw_request;
    try
    {
        raw_request = read(client_socket);
    }
    catch (const http::exceptions::UnexpectedEndOfStream &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        return;
    }
    catch (const http::exceptions::InvalidRequestLine &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        return;
    }
    catch (const http::exceptions::InvalidChunkedEncoding &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        return;
    }
    catch (const http::exceptions::InvalidContentLength &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        return;
    }
    catch (const http::exceptions::MultipleContentLengthHeaders &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        return;
    }
    catch (const http::exceptions::BothContentLengthAndChunked &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        return;
    }
    catch (const http::exceptions::TransferEncodingWithoutChunked &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"));
        return;
    }
    catch (const http::exceptions::RequestLineTooLong &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::URI_TOO_LONG, "Invalid Request Line"));
        return;
    }
    catch (const http::exceptions::HeadersTooLarge &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::HEADERS_TOO_LARGE, "Header Fields Too Large"));
        return;
    }
    catch (const http::exceptions::BodyTooLarge &e)
    {
        std::cerr << e.what() << std::endl;
        send_response(http::HttpResponse(http::status_codes::PAYLOAD_TOO_LARGE, "Payload Too Large"));
        return;
    }
    catch (...)
    {
        std::cerr << "Unknown error during request handling." << std::endl;
        send_response(http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error"));
        return;
    }

    HttpRequest request = http::HttpRequestParser::parse(raw_request);

    std::string path = http::HttpRequestParser::path_from_uri(request.uri());

    try
    {
        if (route_handlers.find({request.method(), path}) == route_handlers.end())
        {
            std::cerr << "No handler found for " << request.method() << " " << path << std::endl;
            send_response(http::HttpResponse(http::status_codes::NOT_FOUND,"Not Found"));
            return;
        }

        http::HttpResponse response;
        route_handlers[{request.method(), path}](request, response);
        response.add_header(http::headers::CONNECTION, "close");
        response.add_header(http::headers::CONTENT_LENGTH, std::to_string(response.body().size()));

        send_response(response);
    }
    catch (const std::exception &e)
    {
        std::cerr<< e.what() << std::endl;
        return;
    }
    catch (...)
    {
        std::cerr << "Unknown error during response processing or sending." << std::endl;
        return;
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
        throw http::exceptions::CanNotSendResponse(std::string(e.what()));
    }
    catch (...)
    {
        throw http::exceptions::CanNotSendResponse();
    }
}

void http::HttpServer::add_route_handler(const std::string method, const std::string path, std::function<void(const http::HttpRequest &, http::HttpResponse &)> handler)
{
    route_handlers[{method, path}] = handler;
}

std::vector<char> http::HttpConnection::read(tcp::ConnectionSocket &client_socket)
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

        if (!http::HttpRequestParser::validate_request_line(request_data))
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
                    long content_length_from_header = http::HttpRequestParser::is_content_length_header(std::vector<char>(buffer.begin(), buffer.begin() + pos));
                    if (content_length != -1 && content_length_from_header != -1)
                    {
                        throw http::exceptions::MultipleContentLengthHeaders();
                    }
                    if (content_length_from_header != -1)
                    {
                        content_length = content_length_from_header;
                        request_has_body = true;
                    }
                    if (http::HttpRequestParser::is_transfer_encoding_chunked_header(std::vector<char>(buffer.begin(), buffer.begin() + pos)))
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

void http::HttpConnection::read_from_tcp(tcp::ConnectionSocket &client_socket)
{
    try
    {
        std::vector<char> temp_buffer;
        temp_buffer = client_socket.receive_data(http::constants::SINGLE_READ_SIZE);
        if (temp_buffer.empty())
        {
            throw;
        }
        buffer.insert(buffer.end(), temp_buffer.begin(), temp_buffer.end());
    }
    catch (const tcp::exceptions::CanNotReceiveData &e)
    {
        throw http::exceptions::UnexpectedEndOfStream(std::string(e.what()));
    }
    catch (...)
    {
        throw http::exceptions::UnexpectedEndOfStream();
    }
}

std::string http::HttpServer::get_ip(){
    return pimpl->server_socket.get_ip();
}

unsigned short http::HttpServer::get_port(){
    return pimpl->server_socket.get_port();
}