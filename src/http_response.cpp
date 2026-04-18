#include "http/http_response.hpp"
#include "http/http_constants.hpp"

#include "http_response_reader.hpp"
#include "http_response_builder.hpp"
#include "data_stream.hpp"

#include <vector>
#include <stdexcept>
#include <cstring>
#include <utility>

namespace http
{
    struct HttpResponse::Impl
    {
        struct ResponseBodyStream
        {
        private:
            struct Impl;
            Impl *pimpl;

        public:
            ResponseBodyStream();
            ResponseBodyStream(WriterFunction writer);
            ResponseBodyStream(const std::vector<char> &data);

            ResponseBodyStream(const ResponseBodyStream &) = delete;
            ResponseBodyStream &operator=(const ResponseBodyStream &) = delete;
            ResponseBodyStream(ResponseBodyStream &&other) noexcept;
            ResponseBodyStream &operator=(ResponseBodyStream &&other) noexcept;

            ~ResponseBodyStream();

            friend struct HttpResponseReader;
        };

        ResponseBodyStream body_stream;
    };

    HttpResponse::HttpResponse() : _version(http::versions::HTTP_1_1), _status_code(0), _reason_phrase(""), pimpl(new Impl()) {}

    HttpResponse::HttpResponse(int status_code, const std::string &reason_phrase)
        : _version(http::versions::HTTP_1_1), _status_code(status_code), _reason_phrase(reason_phrase), pimpl(new Impl()) {}

    HttpResponse::HttpResponse(HttpResponse &&other) noexcept
        : _version(std::move(other._version)),
          _status_code(other._status_code),
          _reason_phrase(std::move(other._reason_phrase)),
          _headers(std::move(other._headers)),
          pimpl(other.pimpl)
    {
        other.pimpl = nullptr;
        other._status_code = 0;
    }

    HttpResponse &HttpResponse::operator=(HttpResponse &&other) noexcept
    {
        if (this != &other)
        {
            delete pimpl;
            _version = std::move(other._version);
            _status_code = other._status_code;
            _reason_phrase = std::move(other._reason_phrase);
            _headers = std::move(other._headers);
            pimpl = other.pimpl;

            other.pimpl = nullptr;
            other._status_code = 0;
        }

        return *this;
    }

    HttpResponse::~HttpResponse() { delete pimpl; }

    const std::string &HttpResponse::version() const noexcept { return _version; }

    int HttpResponse::status_code() const noexcept { return _status_code; }

    const std::string &HttpResponse::reason_phrase() const noexcept { return _reason_phrase; }

    const std::map<std::string, std::string> &HttpResponse::headers() const noexcept { return _headers; }

    void HttpResponse::set_status_code(int status_code) noexcept { _status_code = status_code; }

    void HttpResponse::set_reason_phrase(const std::string &reason_phrase) { _reason_phrase = reason_phrase; }

    void HttpResponse::set_header(const std::string &key, const std::string &value)
    {
        std::string lower_key = key;
        for (char &c : lower_key)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        _headers[lower_key] = value;
    }

    void HttpResponse::set_body_generator(WriterFunction writer)
    {
        pimpl->body_stream = Impl::ResponseBodyStream(writer);
    }

    void HttpResponse::set_body(const std::vector<char> &data)
    {
        pimpl->body_stream = Impl::ResponseBodyStream(data);
    }

    struct HttpResponse::Impl::ResponseBodyStream::Impl
    {
        DataStream data_stream;

        DataStream::StreamView current_view;

        std::vector<char> buffer;
        size_t buffer_cursor = 0;
        size_t buffer_size = 0;
        bool is_stream_closed = false;

        void set_stream_functions(WriterFunction writer);
    };

    HttpResponse::Impl::ResponseBodyStream::ResponseBodyStream()
        : pimpl(new Impl())
    {
        pimpl->buffer.resize(8192); // Placeholder buffer size
    }

    HttpResponse::Impl::ResponseBodyStream::ResponseBodyStream(WriterFunction writer)
        : ResponseBodyStream()
    {
        pimpl->set_stream_functions(writer);
    }

    HttpResponse::Impl::ResponseBodyStream::ResponseBodyStream(const std::vector<char> &data) : ResponseBodyStream()
    {
        size_t bytes_left = data.size();
        auto writer =
            [data, bytes_left](std::vector<char> &buffer) mutable -> long
        {
            if (bytes_left == 0)
            {
                return -1; // Indicate end of stream
            }
            size_t chunk_size = std::min(bytes_left, buffer.size());
            std::memcpy(buffer.data(), data.data() + (data.size() - bytes_left), chunk_size);
            bytes_left -= chunk_size;
            return chunk_size;
        };
        pimpl->set_stream_functions(writer);
    }

    HttpResponse::Impl::ResponseBodyStream::ResponseBodyStream(ResponseBodyStream &&other) noexcept
        : pimpl(other.pimpl)
    {
        other.pimpl = nullptr;
    }

    HttpResponse::Impl::ResponseBodyStream &HttpResponse::Impl::ResponseBodyStream::operator=(ResponseBodyStream &&other) noexcept
    {
        if (this != &other)
        {
            delete pimpl;
            pimpl = other.pimpl;
            other.pimpl = nullptr;
        }

        return *this;
    }

    HttpResponse::Impl::ResponseBodyStream::~ResponseBodyStream()
    {
        delete pimpl;
    }

    void HttpResponse::Impl::ResponseBodyStream::Impl::set_stream_functions(WriterFunction writer)
    {
        this->data_stream.set_stream_updater(
            [this, writer]()
            {
                long bytes_written = writer(this->buffer);
                if (bytes_written == -1)
                {
                    this->is_stream_closed = true;
                }
                else
                {
                    this->buffer_size += bytes_written;
                }
            });

        this->data_stream.set_stream_view_provider(
            [this]()
            {
                this->current_view.data = this->buffer.data();
                this->current_view.size = this->buffer_size;
                this->current_view.cursor = this->buffer_cursor;
                this->current_view.is_closed = this->is_stream_closed;
                return this->current_view;
            });

        this->data_stream.set_cursor_advancer(
            [this](size_t bytes)
            {
                this->buffer_cursor += bytes;
                if (this->buffer_cursor == this->buffer_size)
                {
                    this->buffer_cursor = 0;
                    this->buffer_size = 0;
                }
                if (this->buffer_cursor > this->buffer_size)
                {
                    throw std::overflow_error("ResponseBodyStream: Cursor advanced beyond buffer size.");
                }
            });
    }

    HttpResponse HttpResponseBuilder::build()
    {
        return HttpResponse();
    }

    HttpResponse HttpResponseBuilder::build(int status_code, const std::string &reason_phrase)
    {
        return HttpResponse(status_code, reason_phrase);
    }

    long HttpResponseReader::read_body_stream(const HttpResponse &response, std::vector<char> &buffer, size_t buffer_pointer, size_t max_size)
    {
        if (response.pimpl->body_stream.pimpl->data_stream.is_stream_closed())
        {
            return -1; // Indicate end of stream
        }
        return response.pimpl->body_stream.pimpl->data_stream.get_next(buffer, buffer_pointer, max_size);
    }
}