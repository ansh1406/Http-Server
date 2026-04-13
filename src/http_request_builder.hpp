#ifndef HTTP_REQUEST_BUILDER_HPP
#define HTTP_REQUEST_BUILDER_HPP

#include "http/http_request.hpp"

namespace http
{
    class DataStream;
    struct HttpRequestBuilder
    {
        static HttpRequest build(const std::string &ip,
                                 const std::string &port);

        static void set_method(HttpRequest &request, const std::string &method);
        static void set_uri(HttpRequest &request, const std::string &uri);
        static void set_version(HttpRequest &request, const std::string &version);
        static void set_header(HttpRequest &request, const std::string &key, const std::string &value);
        static void set_body_stream(HttpRequest &request, DataStream &&data_stream);

        static HttpRequest::RequestBodyStream build_body_stream(DataStream &&data_stream);
        static HttpRequest::RequestBodyStream build_body_stream();
    };
}

#endif // HTTP_REQUEST_BUILDER_HPP