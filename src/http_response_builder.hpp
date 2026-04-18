#ifndef HTTP_RESPONSE_BUILDER_HPP
#define HTTP_RESPONSE_BUILDER_HPP

#include "http/http_response.hpp"

namespace http
{
    struct HttpResponseBuilder
    {
        static HttpResponse build();
        static HttpResponse build(int status_code, const std::string &reason_phrase);
    };
}

#endif // HTTP_RESPONSE_BUILDER_HPP