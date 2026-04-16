/// @file http_response_reader.hpp
/// @brief A utility class for reading the body stream of an HTTP response.

#ifndef HTTP_RESPONSE_READER_HPP
#define HTTP_RESPONSE_READER_HPP

#include "http/http_response.hpp"

namespace http
{
    /// @brief A utility class for reading the body stream of an HTTP response.
    struct HttpResponseReader
    {
        /// @brief Reads the body stream of an HTTP response into a buffer.
        /// Repeated calls continue consuming the same underlying response body stream.
        /// @param response The HTTP response from which to read the body.
        /// @param buffer The buffer into which to read the body data.
        /// @param buffer_pointer The position in the buffer where reading should start.
        /// @param max_size The maximum number of bytes to read.
        /// @return Bytes read for this call. Returns -1 when the response body is fully consumed.
        static long read_body_stream(const HttpResponse &response, std::vector<char> &buffer, size_t buffer_pointer = 0, size_t max_size = static_cast<size_t>(-1));
    };
}

#endif // HTTP_RESPONSE_READER_HPP