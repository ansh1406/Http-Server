#include "http_connection.hpp"
#include "http_request_builder.hpp"
#include "http_exceptions.hpp"
#include "http_internal.hpp"
#include "http_parser.hpp"
#include "http_response_reader.hpp"
#include "data_stream.hpp"

#include <cstring>
#include <vector>

http::HttpConnection::CurrentRequest::CurrentRequest() : request(std::move(HttpRequestBuilder::build())), status(RequestStatus::CONNECTION_ESTABLISHED) {}

http::HttpConnection::HttpConnection(tcp::ConnectionSocket &&socket) : client_socket(std::move(socket)), current_request(), last_activity_time(time(nullptr)) {}

void http::HttpConnection::handle_request(std::function<void(const http::HttpRequest &, http::HttpResponse &)> &request_handler) noexcept
{
    try
    {
        if (current_request.has_chunked_body && current_request.content_length != -1)
        {
            log_error("Both Content-Length and chunked Transfer-Encoding headers present in request.");
            current_request.status = RequestStatus::CLIENT_ERROR;
            current_response.response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
            return;
        }
        if (!current_request.has_chunked_body && current_request.content_length == -1)
        {
            current_request.status = RequestStatus::REQUEST_READING_DONE;
        }
        else
        {
            client_socket.set_socket_blocking(100); // 100 is placeholder value.
            current_request.status = RequestStatus::READING_BODY;
        }

        DataStream body_stream;

        body_stream.set_stream_updater(
            [this]()
            {
                if (buffer_cursor == buffer_size)
                {
                    buffer_cursor = 0;
                    buffer_size = 0;
                    read_from_client();
                }
                read_body();
            });

        body_stream.set_stream_view_provider(
            [this]() -> DataStream::StreamView
            {
                DataStream::StreamView view;
                view.data = buffer.data();
                view.size = current_request.body_end_cursor;
                view.cursor = current_request.body_stream_cursor;
                view.is_closed = current_request.status == RequestStatus::REQUEST_READING_DONE;
                view.error = current_request.status == RequestStatus::CLIENT_ERROR || current_request.status == RequestStatus::SERVER_ERROR || inactive;
                return view;
            });

        body_stream.set_cursor_advancer(
            [this](size_t bytes)
            {
                current_request.body_stream_cursor += bytes;

                if (current_request.body_stream_cursor == current_request.body_end_cursor)
                {
                    current_request.body_stream_cursor = 0;
                    current_request.body_end_cursor = 0;
                }
            });

        HttpRequestBuilder::set_body_stream(current_request.request, std::move(body_stream));

        try
        {
            request_handler(current_request.request, current_response.response);
            current_request.status = RequestStatus::REQUEST_HANDLING_DONE;
        }
        catch (const std::exception &e)
        {
            log_error(std::string("Error handling request: ") + e.what());
            current_request.status = RequestStatus::SERVER_ERROR;
            current_response.response = http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error");
        }
        catch (...)
        {
            log_error("Unknown error handling request.");
            current_request.status = RequestStatus::SERVER_ERROR;
            current_response.response = http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error");
        }
    }
    catch (...)
    {
        // Suppress all.
    }
}

void http::HttpConnection::read_and_build_request_head()
{
    try
    {
        read_from_client();
        if (current_request.status == RequestStatus::CONNECTION_ESTABLISHED)
        {
            HttpRequestBuilder::set_ip(current_request.request, get_ip());
            HttpRequestBuilder::set_port(current_request.request, std::to_string(get_port()));
            current_request.status = RequestStatus::READING_REQUEST_LINE;
            buffer.resize(std::max(sizes::MAX_HEADER_SIZE, sizes::MAX_REQUEST_LINE_SIZE));
        }
        if (current_request.status == RequestStatus::READING_REQUEST_LINE)
        {
            read_request_line();
        }
        if (current_request.status == RequestStatus::REQUEST_LINE_DONE)
        {
            if (!http::HttpParser::validate_request_line(buffer))
            {
                throw http::exceptions::InvalidRequestLine();
            }
            else
            {
                http::HttpRequestLine req_line = http::HttpParser::parse_request_line(buffer, parser_cursor);
                HttpRequestBuilder::set_method(current_request.request, req_line.method);
                HttpRequestBuilder::set_uri(current_request.request, req_line.uri);
                HttpRequestBuilder::set_version(current_request.request, req_line.version);
                if (current_request.request.version() != http::versions::HTTP_1_1)
                {
                    throw http::exceptions::VersionNotSupported{};
                }

                current_request.status = RequestStatus::READING_HEADERS;
            }
        }
        if (current_request.status == RequestStatus::READING_HEADERS)
        {
            read_headers();
        }
        if (current_request.status == RequestStatus::HEADERS_DONE)
        {
            auto headers = http::HttpParser::parse_headers(buffer, parser_cursor);
            for (auto &header : headers)
            {
                HttpRequestBuilder::set_header(current_request.request, header.first, header.second);
            }
        }
    }
    catch (const http::exceptions::UnexpectedEndOfStream &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::InvalidRequestLine &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::InvalidChunkedEncoding &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::InvalidContentLength &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::MultipleContentLengthHeaders &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::BothContentLengthAndChunked &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::TransferEncodingWithoutChunked &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (const http::exceptions::RequestLineTooLong &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::URI_TOO_LONG, "Invalid Request Line");
        return;
    }
    catch (const http::exceptions::HeadersTooLarge &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::HEADERS_TOO_LARGE, "Header Fields Too Large");
        return;
    }
    catch (const http::exceptions::VersionNotSupported &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::HTTP_VERSION_NOT_SUPPORTED, "HTTP Version Not Supported");
        return;
    }
    catch (const http::exceptions::InvalidDuplicateHeaders &e)
    {
        log_error(std::string(e.what()));
        current_request.status = RequestStatus::CLIENT_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::BAD_REQUEST, "Bad Request");
        return;
    }
    catch (std::exception &e)
    {
        log_error(std::string("Unknown error reading request: ") + e.what());
        current_request.status = RequestStatus::SERVER_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error");
        return;
    }
    catch (...)
    {
        log_error("Unknown error reading request.");
        current_request.status = RequestStatus::SERVER_ERROR;
        current_response.response = http::HttpResponse(http::status_codes::INTERNAL_SERVER_ERROR, "Internal Server Error");
        return;
    }
}

void http::HttpConnection::read_request_line()
{
    try
    {
        long pos = buffer_cursor;
        bool found_end_of_request_line{false};
        for (; pos < buffer_size - 1; pos++)
        {
            if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
            {
                found_end_of_request_line = true;
                buffer_cursor = pos + 2;
                break;
            }

            if (pos >= http::sizes::MAX_REQUEST_LINE_SIZE)
            {
                throw http::exceptions::RequestLineTooLong();
            }
        }
        if (found_end_of_request_line)
            current_request.status = RequestStatus::REQUEST_LINE_DONE;
    }
    catch (...)
    {
        throw;
    }
}

void http::HttpConnection::read_headers()
{
    try
    {
        long pos = buffer_cursor;
        bool found_end_of_headers{false};
        static long last_header_end = 0;
        static long header_start = buffer_cursor;
        for (; pos < buffer_size - 1; pos++)
        {

            if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
            {
                if (pos == last_header_end + 1)
                {
                    found_end_of_headers = true;
                    buffer_cursor = pos + 2;
                    break;
                }
                pos += 1;
                last_header_end = pos;
            }

            if ((pos - header_start) >= http::sizes::MAX_HEADER_SIZE)
            {
                throw http::exceptions::HeadersTooLarge();
            }
        }
        if (found_end_of_headers)
            current_request.status = RequestStatus::HEADERS_DONE;
    }
    catch (...)
    {
        throw;
    }
}

void http::HttpConnection::read_body()
{
    if (current_request.status == RequestStatus::REQUEST_READING_DONE)
    {
        return;
    }

    if (current_request.has_chunked_body)
    {
        if (current_request.remaining_content_length == 0)
        {
            long chunk_size = read_chunksize_line();
            if (chunk_size == 0)
            {
                current_request.status = RequestStatus::REQUEST_READING_DONE;
                return;
            }
            else if (chunk_size > 0)
            {
                current_request.remaining_content_length = chunk_size;
            }
        }
        if (current_request.remaining_content_length > 0)
        {
            long bytes_read = read_body_chunk();
            current_request.remaining_content_length -= bytes_read;
            current_request.body_end_cursor += bytes_read;
            if (current_request.remaining_content_length == 0)
            {
                buffer_cursor += 2; // To skip the \r\n after chunk data
                read_body();
            }
        }
    }
    else if (current_request.content_length != -1 && current_request.remaining_content_length > 0)
    {
        long bytes_read = read_fixed_body();
        current_request.remaining_content_length -= bytes_read;
        current_request.body_end_cursor += bytes_read;
        if (current_request.remaining_content_length == 0)
        {
            current_request.status = RequestStatus::REQUEST_READING_DONE;
        }
    }
}

long http::HttpConnection::read_fixed_body()
{
    size_t bytes_to_read = std::min((size_t)(buffer.size() - buffer_cursor), (size_t)current_request.remaining_content_length);
    buffer_cursor += bytes_to_read;
    return bytes_to_read;
}

long http::HttpConnection::read_chunksize_line() // For chunked transfer encoding
{
    try
    {
        long pos = buffer_cursor;
        while (true)
        {
            bool found_end_of_chunk_size_line{false};
            for (; pos < (long)buffer.size() - 1; pos++)
            {
                if (buffer[pos] == '\r' && buffer[pos + 1] == '\n')
                {
                    found_end_of_chunk_size_line = true;
                    break;
                }
            }
            if (!found_end_of_chunk_size_line)
                return -1;

            std::string chunk_size_str(buffer.begin() + buffer_cursor, buffer.begin() + pos);
            size_t chunk_size = std::stoul(chunk_size_str, nullptr, 16);

            buffer_cursor = pos + 2;

            return chunk_size;
        }
    }
    catch (...)
    {
        throw;
    }
}

long http::HttpConnection::read_body_chunk()
{
    size_t bytes_to_read = std::min((size_t)(buffer.size() - buffer_cursor), (size_t)current_request.remaining_content_length);
    memmove(buffer.data() + current_request.body_end_cursor, buffer.data() + buffer_cursor, bytes_to_read);
    buffer_cursor += bytes_to_read;
    return bytes_to_read;
}

void http::HttpConnection::read_from_client()
{
    try
    {
        // While reading request body client socket will not be managed by epoll and there will be no need to empty the socket while reading.
        bool read_once = current_request.status == RequestStatus::READING_BODY;
        auto bytes_received = client_socket.receive_data(buffer, buffer_cursor, read_once);
        if (bytes_received > 0)
        {
            last_activity_time = time(nullptr);
            buffer_size += bytes_received;
        }
    }
    catch (const tcp::exceptions::CanNotReceiveData &e)
    {
        current_request.status = RequestStatus::CLIENT_ERROR;
        throw http::exceptions::UnexpectedEndOfStream(std::string(e.what()));
    }
    catch (...)
    {
        current_request.status = RequestStatus::CLIENT_ERROR;
        throw http::exceptions::UnexpectedEndOfStream();
    }
}

void http::HttpConnection::send_response()
{
    if (current_request.status == RequestStatus::CLIENT_ERROR || current_request.status == RequestStatus::SERVER_ERROR || current_request.status == RequestStatus::REQUEST_HANDLING_DONE)
    {
        client_socket.set_socket_non_blocking();
        buffer_size = 0;
        buffer_cursor = 0;
        current_response.response.set_header("Connection", "close");
        current_request.status = RequestStatus::SENDING_STATUS_LINE;
    }

    if (current_request.status == RequestStatus::SENDING_STATUS_LINE)
    {
        size_t bytes_written = HttpParser::encode_response_status_line(current_response.response.version(), current_response.response.status_code(), current_response.response.reason_phrase(), buffer, buffer_size);
        if (bytes_written != 0)
        {
            buffer_size += bytes_written;
            current_request.status = RequestStatus::SENDING_HEADERS;
            current_response.currently_sending_header = current_response.response.headers().begin();
        }
        else
        {
            log_error("Status line too large.");
            throw http::exceptions::StautsLineTooLong();
        }
    }

    if (current_request.status == RequestStatus::SENDING_HEADERS)
    {
        while (current_response.currently_sending_header != current_response.response.headers().end())
        {
            size_t bytes_written = HttpParser::encode_response_header(current_response.currently_sending_header->first, current_response.currently_sending_header->second, buffer, buffer_size);
            if (bytes_written != 0)
            {
                buffer_size += bytes_written;
                ++current_response.currently_sending_header;
            }
            else
            {
                break;
            }
        }

        if (current_response.currently_sending_header == current_response.response.headers().end())
        {
            size_t bytes_written = HttpParser::encode_end_of_headers(buffer, buffer_size);
            if (bytes_written != 0)
            {
                buffer_size += bytes_written;
                current_request.status = RequestStatus::HEADERS_DONE;
            }
        }
    }

    if (current_request.status == RequestStatus::HEADERS_DONE)
    {
        long content_length = HttpParser::has_content_length_header(current_response.response.headers());
        bool has_chunked_encoding = HttpParser::has_transfer_encoding_chunked_header(current_response.response.headers());

        if (content_length != -1 && has_chunked_encoding)
        {
            throw http::exceptions::BothContentLengthAndChunked();
        }

        if (content_length == -1 && !has_chunked_encoding)
        {
            current_request.status = RequestStatus::COMPLETED;
        }
        else
        {
            current_response.has_chunked_body = has_chunked_encoding;
            current_response.content_length = content_length;
            current_response.remaining_content_length = content_length;
            current_request.status = RequestStatus::SENDING_BODY;
        }
    }

    if (current_request.status == RequestStatus::SENDING_BODY)
    {
        if (current_response.has_fixed_length_body())
        {
            long bytes_read = HttpResponseReader::read_body_stream(current_response.response, buffer, buffer_size);
            if (bytes_read == -1)
            {
                if (current_response.remaining_content_length != 0)
                {
                    throw http::exceptions::UnexpectedEndOfStream();
                }
            }
            buffer_size += bytes_read;
            current_response.remaining_content_length -= bytes_read;
            if (current_response.remaining_content_length == 0)
            {
                current_request.status = RequestStatus::SENDING_BUFFER_FLUSHING;
            }
        }
        else if (current_response.has_chunked_body)
        {
            // Read only if a certain minimum buffer size is available.
            if (buffer.size() - buffer_size > 128) // Placeholder
            {
                long bytes_read = HttpResponseReader::read_body_stream(current_response.response, buffer, buffer_size + 6); // 6 is Empty space for chunk size in hex and \r\n.
                if (bytes_read > 0)
                {
                    size_t bytes_encoded = HttpParser::encode_chunksize_line(bytes_read, 4, buffer, buffer_size); // in HHHH format.
                    buffer_size += bytes_read + bytes_encoded;
                }
                if (bytes_read == -1)
                {
                    size_t bytes_encoded = HttpParser::encode_chunksize_line(0, 1, buffer, buffer_size); // Last chunk with size 0.
                    buffer_size += bytes_encoded;
                    current_request.status = RequestStatus::SENDING_BUFFER_FLUSHING;
                }
            }
        }
    }
    send_to_client();

    if (current_request.status == RequestStatus::SENDING_BUFFER_FLUSHING && buffer_cursor == buffer_size)
    {
        current_request.status = RequestStatus::COMPLETED;
    }
}

void http::HttpConnection::send_to_client()
{
    try
    {
        size_t bytes_sent = client_socket.send_data(buffer, buffer_cursor, buffer_size);
        if (bytes_sent > 0)
        {
            last_activity_time = time(nullptr);
            buffer_cursor += bytes_sent;
            if (buffer_cursor == buffer_size)
            {
                buffer_cursor = 0;
                buffer_size = 0;
            }
        }
    }
    catch (const tcp::exceptions::CanNotSendData &e)
    {
        throw http::exceptions::UnexpectedEndOfStream(std::string(e.what()));
    }
    catch (...)
    {
        throw http::exceptions::UnexpectedEndOfStream();
    }
}

void http::HttpConnection::reposition_buffer()
{
    long remaining_data = buffer_size - buffer_cursor;
    memmove(buffer.data(), buffer.data() + buffer_cursor, remaining_data);
    buffer_cursor = 0;
    buffer_size = remaining_data;
    parser_cursor = 0;
}
