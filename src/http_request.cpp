#include "http/http_request.hpp"

#include "data_stream.hpp"
#include "http_request_builder.hpp"

namespace http
{
    struct HttpRequest::RequestBodyStream::Impl
    {
        DataStream data_stream;
    };

    HttpRequest::RequestBodyStream::RequestBodyStream() : pimpl(new Impl()) {}

    HttpRequest::RequestBodyStream::RequestBodyStream(RequestBodyStream &&other) noexcept : pimpl(other.pimpl)
    {
        other.pimpl = nullptr;
    }

    HttpRequest::RequestBodyStream &HttpRequest::RequestBodyStream::operator=(RequestBodyStream &&other) noexcept
    {
        if (this != &other)
        {
            delete pimpl;
            pimpl = other.pimpl;
            other.pimpl = nullptr;
        }

        return *this;
    }

    HttpRequest::RequestBodyStream::~RequestBodyStream() { delete pimpl; }

    size_t HttpRequest::RequestBodyStream::get_next(std::vector<char> &buffer, size_t buffer_cursor, size_t max_size) const
    {
        try
        {
            return pimpl->data_stream.get_next(buffer, buffer_cursor, max_size);
        }
        catch (...)
        {
            throw StreamError("Failed to read from request body stream.");
        }
    }

    bool HttpRequest::RequestBodyStream::has_more_data() const
    {
        return pimpl->data_stream.has_more_data();
    }

    bool HttpRequest::RequestBodyStream::is_stream_closed() const
    {
        return pimpl->data_stream.is_stream_closed();
    }

    HttpRequest HttpRequestBuilder::build()
    {
        return HttpRequest();
    }

    HttpRequest::HttpRequest(HttpRequest &&other) noexcept
        : _ip(std::move(other._ip)),
          _port(std::move(other._port)),
          _method(std::move(other._method)),
          _uri(std::move(other._uri)),
          _version(std::move(other._version)),
          _headers(std::move(other._headers)),
          _body(std::move(other._body))
    {
    }

    HttpRequest &HttpRequest::operator=(HttpRequest &&other) noexcept
    {
        if (this != &other)
        {
            _ip = std::move(other._ip);
            _port = std::move(other._port);
            _method = std::move(other._method);
            _uri = std::move(other._uri);
            _version = std::move(other._version);
            _headers = std::move(other._headers);
            _body = std::move(other._body);
        }

        return *this;
    }

    void HttpRequestBuilder::set_ip(HttpRequest &request, const std::string &ip)
    {
        request._ip = ip;
    }

    void HttpRequestBuilder::set_port(HttpRequest &request, const std::string &port)
    {
        request._port = port;
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
        stream.pimpl->data_stream = std::move(data_stream);
        return stream;
    }

    HttpRequest::RequestBodyStream HttpRequestBuilder::build_body_stream()
    {
        return HttpRequest::RequestBodyStream();
    }

    HttpRequest::HttpRequest() : _body(std::move(HttpRequestBuilder::build_body_stream())) {}

    const std::string &HttpRequest::ip() const noexcept { return _ip; }
    const std::string &HttpRequest::port() const noexcept { return _port; }
    const std::string &HttpRequest::method() const noexcept { return _method; }
    const std::string &HttpRequest::uri() const noexcept { return _uri; }
    const std::string &HttpRequest::version() const noexcept { return _version; }
    const std::map<std::string, std::string> &HttpRequest::headers() const noexcept { return _headers; }
    const HttpRequest::RequestBodyStream &HttpRequest::body() const noexcept { return _body; }
}