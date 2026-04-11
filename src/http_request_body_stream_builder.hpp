#ifndef HTTP_REQEST_BODY_STREAM_BUILDER_HPP
#define HTTP_REQEST_BODY_STREAM_BUILDER_HPP

namespace http
{
    class DataStream;
    struct RequestBodyStream;

    struct RequestBodyStreamBuilder
    {
        static RequestBodyStream build(DataStream &&data_stream);
    };
}

#endif // HTTP_REQEST_BODY_STREAM_BUILDER_HPP