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
            connection.handle();
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

void http::HttpConnection::handle()
{
    std::vector<char> raw_request;
    try
    {
        raw_request = request_reader.read(client_socket);
    }
    catch (const http::exceptions::UnexpectedEndOfStream &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"), client_socket);
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::InvalidRequestLine &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"), client_socket);
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::InvalidChunkedEncoding &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"), client_socket);
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::InvalidContentLength &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"), client_socket);
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::MultipleContentLengthHeaders &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"), client_socket);
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::BothContentLengthAndChunked &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"), client_socket);
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::TransferEncodingWithoutChunked &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request"), client_socket);
        throw http::exceptions::BadRequest();
    }
    catch (const http::exceptions::RequestLineTooLong &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::URI_TOO_LONG, "Invalid Request Line"), client_socket);
        throw http::exceptions::InvalidRequestLine();
    }
    catch (const http::exceptions::HeadersTooLarge &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::HEADERS_TOO_LARGE, "Header Fields Too Large"), client_socket);
        throw http::exceptions::HeaderFieldsTooLarge();
    }
    catch (const http::exceptions::BodyTooLarge &e)
    {
        std::cerr << "Error reading request: " << e.what() << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::PAYLOAD_TOO_LARGE, "Payload Too Large"), client_socket);
        throw http::exceptions::PayloadTooLarge();
    }
    catch (...)
    {
        std::cerr << "Unknown error reading request." << std::endl;
        HttpResponse::send(http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error"), client_socket);
        throw http::exceptions::InternalServerError();
    }

    HttpRequest request = request_parser.parse(raw_request);

    // Implement request handling logic here

    http::HttpResponse response = mock_response_generator();

    try
    {
        http::HttpResponse::send(response, client_socket);
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
