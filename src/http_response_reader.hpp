#ifndef HTTP_RESPONSE_READER_HPP
#define HTTP_RESPONSE_READER_HPP

#include "http/http_response.hpp"

namespace http
{
    struct HttpResponseReader
    {
        static long read_body_stream(const HttpResponse &response, std::vector<char> &buffer, size_t buffer_pointer = 0);
    };
}

#endif // HTTP_RESPONSE_READER_HPP