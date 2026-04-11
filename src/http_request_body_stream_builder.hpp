#ifndef HTTP_REQEST_BODY_STREAM_BUILDER_HPP
#define HTTP_REQEST_BODY_STREAM_BUILDER_HPP

#include "http/http_request.hpp"

namespace http
{
    class DataStream;

    struct RequestBodyStreamBuilder
    {
        static HttpRequest::RequestBodyStream build(DataStream &&data_stream);
    };
}

#endif // HTTP_REQEST_BODY_STREAM_BUILDER_HPP