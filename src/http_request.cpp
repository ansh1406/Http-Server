#include "http/http_request.hpp"

#include "data_stream.hpp"
#include "http_request_builder.hpp"

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

    HttpRequest HttpRequestBuilder::build(const std::string &ip, const std::string &port)
    {
        HttpRequest request;
        request._ip = ip;
        request._port = port;
        return request;
    }

    void HttpRequestBuilder::set_method(HttpRequest &request, const std::string &method)
    {
        request._method = method;
    }

    void HttpRequestBuilder::set_uri(HttpRequest &request, const std::string &uri)
    {
        request._uri = uri;
    }

    void HttpRequestBuilder::set_version(HttpRequest &request, const std::string &version)
    {
        request._version = version;
    }

    void HttpRequestBuilder::set_header(HttpRequest &request, const std::string &key, const std::string &value)
    {
        request._headers[key] = value;
    }

    void HttpRequestBuilder::set_body_stream(HttpRequest &request, DataStream &&data_stream)
    {
        request._body = build_body_stream(std::move(data_stream));
    }

    HttpRequest::RequestBodyStream HttpRequestBuilder::build_body_stream(DataStream &&data_stream)
    {
        HttpRequest::RequestBodyStream stream;
        stream.pimpl = new HttpRequest::RequestBodyStream::Impl();
        stream.pimpl->data_stream = std::move(data_stream);
        return stream;
    }

    HttpRequest::RequestBodyStream HttpRequestBuilder::build_body_stream()
    {
        return HttpRequest::RequestBodyStream();
    }

    HttpRequest::HttpRequest() : _body(HttpRequestBuilder::build_body_stream()) {}

    const std::string &HttpRequest::ip() const noexcept { return _ip; }
    const std::string &HttpRequest::port() const noexcept { return _port; }
    const std::string &HttpRequest::method() const noexcept { return _method; }
    const std::string &HttpRequest::uri() const noexcept { return _uri; }
    const std::string &HttpRequest::version() const noexcept { return _version; }
    const std::map<std::string, std::string> &HttpRequest::headers() const noexcept { return _headers; }
    const HttpRequest::RequestBodyStream &HttpRequest::body() const noexcept { return _body; }
}