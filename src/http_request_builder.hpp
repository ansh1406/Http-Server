/// @file http_request_builder.hpp
/// @brief Defines the HttpRequestBuilder class for constructing HttpRequest objects and managing their body streams.

#ifndef HTTP_REQUEST_BUILDER_HPP
#define HTTP_REQUEST_BUILDER_HPP

#include "http/http_request.hpp"

namespace http
{
    class DataStream;
    struct HttpRequestBuilder
    {
        /// @brief Builds an empty HttpRequest object.
        /// @return HttpRequest object with empty fields and an empty body stream.
        static HttpRequest build();

        static void set_ip(HttpRequest &request, const std::string &ip);
        static void set_port(HttpRequest &request, const std::string &port);
        static void set_method(HttpRequest &request, const std::string &method);
        static void set_uri(HttpRequest &request, const std::string &uri);
        static void set_version(HttpRequest &request, const std::string &version);
        static void set_header(HttpRequest &request, const std::string &key, const std::string &value);
        static void set_body_stream(HttpRequest &request, DataStream &&data_stream);

        /// @brief Move the DataStream into a RequestBodyStream.
        /// @param data_stream Source DataStream to be moved into the RequestBodyStream.
        /// @return A RequestBodyStream that takes ownership of the provided DataStream.
        static HttpRequest::RequestBodyStream build_body_stream(DataStream &&data_stream);

        /// @brief Builds a RequestBodyStream which pulls from an empty DataStream.
        /// @return An empty RequestBodyStream.
        static HttpRequest::RequestBodyStream build_body_stream();
    };
}

#endif // HTTP_REQUEST_BUILDER_HPP