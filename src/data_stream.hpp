#ifndef DATA_STREAM_HPP
#define DATA_STREAM_HPP

#include <vector>
#include <functional>
#include <stdexcept>
#include <string>
#include <cstring>
#include <limits>

namespace http
{

    /// Stateful byte stream wrapper built from provider callbacks.
    ///
    /// A stream is represented by three callbacks:
    /// - updater: pulls or advances producer state
    /// - view provider: returns current readable window
    /// - cursor advancer: commits consumed bytes to producer state
    class DataStream
    {
    public:
        template <typename T>
        using ProviderFunction = std::function<T()>;

        struct StreamView
        {
            /// Pointer to the current readable region.
            char *data;
            /// Total bytes available in the current region.
            size_t size;
            /// Read cursor inside the current region.
            size_t cursor;
            /// True when producer will provide no more data.
            bool is_closed;
            /// True when producer encountered an unrecoverable stream error.
            bool error;
        };
        struct StreamPipelineBroken : public std::runtime_error
        {
            explicit StreamPipelineBroken(const std::string &error) : std::runtime_error(error) {}
        };

    private:
        ProviderFunction<void> stream_updater;
        ProviderFunction<StreamView> stream_view_provider;
        std::function<void(size_t)> cursor_advancer;

        void read_more()
        {
            stream_updater();
        }

        StreamView get_stream_view() const
        {
            return stream_view_provider();
        }

        void advance_cursor(size_t bytes)
        {
            cursor_advancer(bytes);
        }

    public:
        DataStream()
        {
            stream_updater = []()
            {
                // Default no-op updater
            };
            stream_view_provider = []() -> StreamView
            {
                // Default empty stream view
                return StreamView{nullptr, 0, 0, true, false};
            };
            cursor_advancer = [](size_t)
            {
                // Default no-op cursor advancer
            };
        }

        DataStream(const DataStream &) = delete;
        DataStream &operator=(const DataStream &) = delete;

        DataStream(DataStream &&) = default;
        DataStream &operator=(DataStream &&) = default;

        bool has_more_data() const
        {
            auto view = get_stream_view();
            return view.cursor < view.size || !view.is_closed;
        }

        bool is_stream_closed() const
        {
            auto view = get_stream_view();
            return view.is_closed;
        }

        size_t get_next(std::vector<char> &buffer, size_t buffer_cursor = 0, size_t max_size = std::numeric_limits<size_t>::max())
        {
            try
            {
                auto view = get_stream_view();

                if (view.is_closed)
                {
                    return 0;
                }

                if (view.error)
                {
                    throw StreamPipelineBroken("DataStream: Stream pipeline is broken.");
                }

                size_t available_data_size = view.size;
                size_t current_cursor = view.cursor;

                if (current_cursor >= available_data_size)
                {
                    // Request producer to refresh stream view when current window is exhausted.
                    read_more();
                    view = get_stream_view();
                    available_data_size = view.size;
                    current_cursor = view.cursor;
                }

                if (current_cursor >= available_data_size)
                {
                    return 0; // No more data available after provider read
                }

                if (buffer_cursor >= buffer.size())
                {
                    throw std::out_of_range("DataStream: Buffer cursor is out of bounds.");
                }

                if (max_size == 0)
                {
                    return 0;
                }

                size_t bytes_to_read = std::min(buffer.size() - buffer_cursor, available_data_size - current_cursor);
                bytes_to_read = std::min(bytes_to_read, max_size);

                std::memcpy(buffer.data() + buffer_cursor, view.data + current_cursor, bytes_to_read);
                advance_cursor(bytes_to_read);
                return bytes_to_read;
            }
            catch (const std::exception &e)
            {
                throw StreamPipelineBroken(std::string("DataStream: Error while reading data: ") + e.what());
            }
            catch (...)
            {
                throw StreamPipelineBroken("DataStream: Unknown error while reading data.");
            }
        }

        /// Keep consuming until producer signals end of stream. Throws if producer signals an error.
        void flush()
        {
            while (true)
            {
                auto view = get_stream_view();
                if (view.is_closed)
                {
                    break;
                }
                if (view.error)
                {
                    throw StreamPipelineBroken("DataStream: Stream pipeline is broken.");
                }
                // Keep requesting producer updates until stream is closed.
                read_more();
            }
        }

        /// @brief Sets the provider for the stream view.
        /// @param provider A function that returns the current state of input stream.
        void set_stream_view_provider(ProviderFunction<StreamView> provider)
        {
            stream_view_provider = provider;
        }

        /// @brief Sets the updater function for the stream.
        /// @param updater A function that signals the producer to advance its state or provide more data.
        void set_stream_updater(ProviderFunction<void> updater)
        {
            stream_updater = updater;
        }

        /// @brief Sets the cursor advancer function for the stream.
        /// @param advancer A function that advances the cursor position within the stream.
        void set_cursor_advancer(std::function<void(size_t)> advancer)
        {
            cursor_advancer = advancer;
        }

        /// @brief Sets the provider stream for this stream.
        /// @param other The other data stream to use as a provider.
        void set_provider_stream(DataStream &other)
        {
            /// Copies provider callbacks from another stream.
            /// Both streams will then observe and advance the same provider state.
            stream_updater = other.stream_updater;
            stream_view_provider = other.stream_view_provider;
            cursor_advancer = other.cursor_advancer;
        }
    };
}

#endif // DATA_STREAM_HPP