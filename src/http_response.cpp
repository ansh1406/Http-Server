#include "http/http_response.hpp"

#include "http_response_reader.hpp"
#include "data_stream.hpp"

#include <vector>
#include <stdexcept>
#include <cstring>

namespace http
{
    struct HttpResponse::Impl
    {
        struct ResponseBodyStream
        {
        private:
            struct Impl;
            Impl *pimpl;

        public:
            ResponseBodyStream() = default;
            ResponseBodyStream(WriterFunction writer);
            ResponseBodyStream(const std::vector<char> &data);

            ~ResponseBodyStream();

            friend struct HttpResponseReader;
        };

        ResponseBodyStream body_stream;
    };

    HttpResponse::HttpResponse() : _version("HTTP/1.1"), _status_code(0), _reason_phrase(""), pimpl(new Impl()) {}

    HttpResponse::HttpResponse(int status_code, const std::string &reason_phrase)
        : _version("HTTP/1.1"), _status_code(status_code), _reason_phrase(reason_phrase), pimpl(new Impl()) {}

    HttpResponse::~HttpResponse() { delete pimpl; }

    const std::string &HttpResponse::version() const noexcept { return _version; }

    int HttpResponse::status_code() const noexcept { return _status_code; }

    const std::string &HttpResponse::reason_phrase() const noexcept { return _reason_phrase; }

    const std::map<std::string, std::string> &HttpResponse::headers() const noexcept { return _headers; }

    void HttpResponse::set_status_code(int status_code) noexcept { _status_code = status_code; }

    void HttpResponse::set_status_message(const std::string &reason_phrase) { _reason_phrase = reason_phrase; }

    void HttpResponse::set_header(const std::string &key, const std::string &value) { _headers[key] = value; }

    void HttpResponse::set_body_generator(WriterFunction writer)
    {
        pimpl->body_stream = Impl::ResponseBodyStream(writer);
    }

    void HttpResponse::set_body(const std::vector<char> &data)
    {
        pimpl->body_stream = Impl::ResponseBodyStream(data);
    }

    struct HttpResponse::Impl::ResponseBodyStream::Impl
    {
        DataStream data_stream;

        DataStream::StreamView current_view;

        std::vector<char> buffer;
        size_t buffer_cursor = 0;
        size_t buffer_size = 0;
        bool is_stream_closed = false;
    };

    HttpResponse::Impl::ResponseBodyStream::ResponseBodyStream(WriterFunction writer)
    {
        pimpl = new Impl();
        pimpl->buffer.resize(8192); /// Placeholder value.
        pimpl->data_stream.set_stream_updater(
            [this, writer]()
            {
                long bytes_written = writer(pimpl->buffer);
                if (bytes_written == -1)
                {
                    pimpl->is_stream_closed = true;
                }
                else
                {
                    pimpl->buffer_size += bytes_written;
                }
            });

        pimpl->data_stream.set_stream_view_provider(
            [this]()
            {
                pimpl->current_view.data = pimpl->buffer.data();
                pimpl->current_view.size = pimpl->buffer_size;
                pimpl->current_view.cursor = pimpl->buffer_cursor;
                pimpl->current_view.is_closed = pimpl->is_stream_closed;
                return pimpl->current_view;
            });

        pimpl->data_stream.set_cursor_advancer(
            [this](size_t bytes)
            {
                pimpl->buffer_cursor += bytes;
                if (pimpl->buffer_cursor > pimpl->buffer_size)
                {
                    throw std::overflow_error("ResponseBodyStream: Cursor advanced beyond buffer size.");
                }
            });
    }

    HttpResponse::Impl::ResponseBodyStream::ResponseBodyStream(const std::vector<char> &data)
    {
        size_t bytes_left = data.size();
        ResponseBodyStream(
            [data, bytes_left](std::vector<char> &buffer) mutable -> long
            {
                if (bytes_left == 0)
                {
                    return -1; // Indicate end of stream
                }
                size_t bytes_to_write = std::min(bytes_left, buffer.size());
                std::memcpy(buffer.data(), data.data(), bytes_to_write);
                bytes_left -= bytes_to_write;
                return static_cast<long>(bytes_to_write);
            });
    }

    HttpResponse::Impl::ResponseBodyStream::~ResponseBodyStream()
    {
        delete pimpl;
    }

    long HttpResponseReader::read_body_stream(const HttpResponse &response, std::vector<char> &buffer, size_t buffer_pointer)
    {
        if (response.pimpl->body_stream.pimpl->data_stream.is_stream_closed())
        {
            return -1; // Indicate end of stream
        }
        return response.pimpl->body_stream.pimpl->data_stream.get_next(buffer, buffer_pointer);
    }
}