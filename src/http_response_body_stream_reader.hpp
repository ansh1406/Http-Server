#ifndef HTTP_RESPONSE_BODY_STREAM_READER_HPP
#define HTTP_RESPONSE_BODY_STREAM_READER_HPP

#include "http/http_response.hpp"

namespace http
{
    struct ResponseBodyStreamReader
    {
        static long read(const HttpResponse::ResponseBodyStream &body_stream, std::vector<char> &buffer, size_t buffer_pointer = 0);
    };
}

#endif // HTTP_RESPONSE_BODY_STREAM_READER_HPP