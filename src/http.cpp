#include "includes/http.hpp"
#include "includes/tcp.hpp"
#include "includes/http_connection.hpp"
#include "includes/http_exceptions.hpp"
#include "includes/http_parser.hpp"
#include "includes/http_constants.hpp"
#include "includes/http_request.hpp"
#include "includes/http_response.hpp"
#include "includes/logger.hpp"

#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>

struct http::HttpServer::Impl
{
    tcp::ListeningSocket server_socket;
    std::vector<std::thread> worker_threads;
    std::queue<std::unique_ptr<http::HttpConnection>> connection_queue;
    std::mutex worker_mutex;
    std::mutex ready_mutex;
    std::condition_variable worker_cv;
    std::condition_variable ready_cv;
    int ready_threads{0};
    bool stop{false};

    Impl(tcp::ListeningSocket &&sock) : server_socket(std::move(sock)) {}
};

http::HttpServer::HttpServer(HttpServerConfig config)
{
    try
    {
        pimpl = new Impl(std::move(tcp::ListeningSocket(config.port, config.max_pending_connections)));

        if (config.max_parallel_connections == 0)
            config.max_parallel_connections = std::thread::hardware_concurrency();

        pimpl->worker_threads.resize(config.max_parallel_connections);
        for (unsigned int i = 0; i < config.max_parallel_connections; ++i)
        {
            pimpl->worker_threads[i] = std::thread([this, config]()
                                                   {
                {
                    std::unique_lock<std::mutex> lock(pimpl->ready_mutex);
                    pimpl->ready_threads++;
                    pimpl->ready_cv.notify_one();
                }
                while(true){
                    try{
                    std::unique_ptr<http::HttpConnection> connection;
                    {
                        std::unique_lock<std::mutex> lock(pimpl->worker_mutex);
                        pimpl->worker_cv.wait(lock, [this]() { return !pimpl->connection_queue.empty() || pimpl->stop; });
                        if(pimpl->stop && pimpl->connection_queue.empty())
                            break;
                        connection = std::move(pimpl->connection_queue.front());
                        pimpl->connection_queue.pop();
                    }
                    if(connection)
                        connection->handle(route_handlers);
                }
                catch(const std::exception& e){
                    http::Logger::get_instance().add_log_entry(std::string("Worker thread error: ") + e.what());
                }
                catch(...){
                    http::Logger::get_instance().add_log_entry("Unknown worker thread error.");
                } } });
        }

        // Wait for all threads to be ready
        {
            std::unique_lock<std::mutex> lock(pimpl->ready_mutex);
            int expected_ready = config.max_parallel_connections; // worker threads
            pimpl->ready_cv.wait(lock, [this, expected_ready]()
                                 { return pimpl->ready_threads == expected_ready; });
        }
        http::Logger::get_instance().set_external_logging(config.external_logging);
        log_info("HTTP Server created successfully on port " + std::to_string(get_port()));
    }
    catch (const tcp::exceptions::CanNotCreateSocket &e)
    {
        http::Logger::get_instance().add_log_entry(std::string("Error opening server socket: ") + e.what());
        throw http::exceptions::CanNotCreateServer();
    }
    catch (...)
    {
        http::Logger::get_instance().add_log_entry("Unknown error opening server socket.");
        throw http::exceptions::CanNotCreateServer();
    }
}

http::HttpServer::~HttpServer()
{
    {
        std::unique_lock<std::mutex> lock(pimpl->worker_mutex);
        pimpl->stop = true;
    }
    pimpl->worker_cv.notify_all();
    for (auto &thread : pimpl->worker_threads)
    {
        if (thread.joinable())
            thread.join();
    }
    delete pimpl;
}

void http::HttpServer::start()
{
    log_info("HTTP Server started is listening on port: " + std::to_string(get_port()));
    while (true)
    {
        try
        {
            tcp::ConnectionSocket client_socket = pimpl->server_socket.accept_connection();
            std::unique_ptr<http::HttpConnection> connection(new http::HttpConnection(std::move(client_socket)));
            log_info("Connection accepted: " + connection->get_ip() + ":" + std::to_string(connection->get_port()));
            {
                std::unique_lock<std::mutex> lock(pimpl->worker_mutex);
                pimpl->connection_queue.push(std::move(connection));
            }
            pimpl->worker_cv.notify_one();
        }
        catch (const std::exception &e)
        {
            log_error(e.what());
        }
        catch (...)
        {
            log_error("Unknown unexpected error.");
        }
    }
}

void http::HttpConnection::handle(std::map<std::pair<std::string, std::string>, std::function<void(const http::HttpRequest &, http::HttpResponse &)>> &route_handlers) noexcept
{
    try
    {
        std::vector<char> raw_request;
        http::HttpResponse response;
        try
        {
            raw_request = read(client_socket);
        }
        catch (const http::exceptions::UnexpectedEndOfStream &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
            return;
        }
        catch (const http::exceptions::InvalidRequestLine &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
            return;
        }
        catch (const http::exceptions::InvalidChunkedEncoding &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
            return;
        }
        catch (const http::exceptions::InvalidContentLength &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
            return;
        }
        catch (const http::exceptions::MultipleContentLengthHeaders &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
            return;
        }
        catch (const http::exceptions::BothContentLengthAndChunked &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
            return;
        }
        catch (const http::exceptions::TransferEncodingWithoutChunked &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
            return;
        }
        catch (const http::exceptions::RequestLineTooLong &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::URI_TOO_LONG, "Invalid Request Line");
            return;
        }
        catch (const http::exceptions::HeadersTooLarge &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::HEADERS_TOO_LARGE, "Header Fields Too Large");
            return;
        }
        catch (const http::exceptions::BodyTooLarge &e)
        {
            log_error(e.what());
            response = http::HttpResponse(http::status_codes::PAYLOAD_TOO_LARGE, "Payload Too Large");
            return;
        }
        catch (...)
        {
            log_error("Unknown error during request handling.");
            response = http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error");
            return;
        }

        try
        {
            if (response)
            {
                send_response(response);
                return;
            }

            HttpRequest request = http::HttpRequestParser::parse(raw_request);

            std::string path = http::HttpRequestParser::path_from_uri(request.uri());
            log_info("Received request: " + request.method() + " " + path);

            if (route_handlers.find({request.method(), path}) == route_handlers.end())
            {
                log_error(std::string("No handler found for ") + request.method() + " " + path);
                response = http::HttpResponse(http::status_codes::NOT_FOUND, "Not Found");
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
            log_error(e.what());
            return;
        }
        catch (...)
        {
            log_error("Unknown error during response processing or sending.");
            return;
        }
    }
    catch(...){
        try{
            log_error("Internal error in connection handling.");
            send_response(http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error"));
        }
        catch(...){
            // Suppress all exceptions
        }
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
        log_info("Response sent with status code: " + std::to_string(response.status_code()));
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

void http::HttpServer::log_info(const std::string &entry)
{
    Logger::get_instance().add_log_entry("[INFO] [SERVER] " + entry);
}

void http::HttpServer::log_error(const std::string &entry)
{
    Logger::get_instance().add_log_entry("[ERROR] [SERVER] " + entry);
}

void http::HttpConnection::log_info(const std::string &entry)
{
    Logger::get_instance().add_log_entry("[INFO] [CONN] [" + get_ip() + ":" + std::to_string(get_port()) + "] " + entry);
}

void http::HttpConnection::log_error(const std::string &entry)
{
    Logger::get_instance().add_log_entry("[ERROR] [CONN] [" + get_ip() + ":" + std::to_string(get_port()) + "] " + entry);
}

std::string http::HttpServer::get_ip()
{
    return pimpl->server_socket.get_ip();
}

unsigned short http::HttpServer::get_port()
{
    return pimpl->server_socket.get_port();
}