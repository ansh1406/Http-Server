#ifndef HTTP_BODY_STREAM_HPP
#define HTTP_BODY_STREAM_HPP

#include <vector>
#include <functional>

namespace http
{
    

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