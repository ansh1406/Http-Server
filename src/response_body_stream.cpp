#include "http/http_response.hpp"

#include "http_response_body_stream_reader.hpp"
#include "data_stream.hpp"

#include <vector>
#include <stdexcept>
#include <cstring>

namespace http
{
    struct HttpResponse::ResponseBodyStream::Impl
    {
        DataStream data_stream;

        DataStream::StreamView current_view;

        std::vector<char> buffer;
        size_t buffer_cursor = 0;
        size_t buffer_size = 0;
        bool is_stream_closed = false;
    };

    HttpResponse::ResponseBodyStream::ResponseBodyStream(WriterFunction writer)
    {
        pimpl = new Impl();
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

    HttpResponse::ResponseBodyStream::ResponseBodyStream(const std::vector<char> &data)
    {
        size_t bytes_left = data.size();
        WriterFunction writer = [data, bytes_left](std::vector<char> &buffer) mutable -> long
        {
            if (bytes_left == 0)
            {
                return -1; // Indicate end of stream
            }
            buffer.resize(data.size());
            std::memcpy(buffer.data(), data.data(), data.size());
            bytes_left = 0;
            return static_cast<long>(data.size());
        };

        ResponseBodyStream(writer);
    }

    HttpResponse::ResponseBodyStream::~ResponseBodyStream()
    {
        delete pimpl;
    }

    long ResponseBodyStreamReader::read(const HttpResponse::ResponseBodyStream &body_stream, std::vector<char> &buffer, size_t buffer_pointer)
    {
        if (body_stream.pimpl->data_stream.is_stream_closed())
        {
            return -1; // Indicate end of stream
        }
        return body_stream.pimpl->data_stream.get_next(buffer, buffer_pointer);
    }
}