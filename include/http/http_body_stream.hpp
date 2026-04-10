#ifndef HTTP_BODY_STREAM_HPP
#define HTTP_BODY_STREAM_HPP

#include <vector>
#include <functional>

namespace http
{
    class RequestBodyStream
    {
    private:
        struct Impl;
        Impl *pimpl;

    public:
        RequestBodyStream();
        ~RequestBodyStream();

        bool has_more_data() const;
        bool is_stream_closed() const;
        size_t get_next(std::vector<char> &buffer, size_t buffer_cursor = 0);
    };

    class ResponseBodyStream
    {
    private:
        struct Impl;
        Impl *pimpl;

    public:
        using WriterFunction = std::function<bool(std::vector<char> &data)>;

        ResponseBodyStream() = default;
        ResponseBodyStream(WriterFunction writer);
        ResponseBodyStream(const std::vector<char> &data);

        ~ResponseBodyStream();
    };
}

#endif // HTTP_BODY_STREAM_HPP