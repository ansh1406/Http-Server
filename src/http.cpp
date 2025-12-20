#include "includes/http.hpp"

void http::HttpServer::start()
{
    while (true)
    {
        tcp::ConnectionSocket client_socket = server_socket.accept_connection();
        HttpConnection connection(std::move(client_socket));
        connection.handle();
    }
}

http::HttpResponse mock_response_generator(){
    http::HttpResponse response;
    response.set_version(http::versions::HTTP_1_1);
    response.set_status_code(200);
    response.set_status_message("OK");
    response.set_headers({{"Content-Type", "text/plain"} , {"Connection", "close"} , {"Content-Length", "13"}});
    response.set_body(std::vector<char>{'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!'});
    return response;
}

void http::HttpConnection::handle()
{
    std::vector<char> raw_request = request_reader.read(client_socket);
    HttpRequest request = request_parser.parse(raw_request);

    /// @todo: Implement request handling logic here

    http::HttpResponse response = mock_response_generator();

    response.send(client_socket);

}
