#include "http/http_request.hpp"

#include "http_request_body_stream_builder.hpp"
#include "data_stream.hpp"

#include <stdexcept>

namespace http
{
    struct HttpRequest::RequestBodyStream::Impl
    {
        DataStream data_stream;
    };

    HttpRequest::RequestBodyStream::RequestBodyStream()
    {
        pimpl = nullptr;
    }

    HttpRequest::RequestBodyStream::~RequestBodyStream()
    {
        delete pimpl;
    }

    size_t HttpRequest::RequestBodyStream::get_next(std::vector<char> &buffer, size_t buffer_cursor)
    {
        if (!pimpl)
        {
            throw std::runtime_error("RequestBodyStream: Attempted to read from an uninitialized stream.");
        }
        return pimpl->data_stream.get_next(buffer, buffer_cursor);
    }

    bool HttpRequest::RequestBodyStream::has_more_data() const
    {
        if (!pimpl)
        {
            throw std::runtime_error("RequestBodyStream: Attempted to check for more data on an uninitialized stream.");
        }
        return pimpl->data_stream.has_more_data();
    }

    bool HttpRequest::RequestBodyStream::is_stream_closed() const
    {
        if (!pimpl)
        {
            throw std::runtime_error("RequestBodyStream: Attempted to check if stream is closed on an uninitialized stream.");
        }
        return pimpl->data_stream.is_stream_closed();
    }

    HttpRequest::RequestBodyStream RequestBodyStreamBuilder::build(DataStream &&data_stream)
    {
        HttpRequest::RequestBodyStream stream;
        stream.pimpl = new HttpRequest::RequestBodyStream::Impl();
        stream.pimpl->data_stream = std::move(data_stream);
        return stream;
    }
}