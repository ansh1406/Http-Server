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

http::HttpResponse mock_response_generator()
{
    http::HttpResponse response;
    response.set_status_code(200);
    response.set_status_message("OK");
    response.set_headers({{"Content-Type", "text/plain"}, {"Connection", "close"}, {"Content-Length", "13"}});
    response.set_body(std::vector<char>{'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'});
    return response;
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