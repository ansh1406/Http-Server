#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <ctime>
#include <string>

namespace tcp
{
    /// Bit flags describing readiness returned by the OS event backend.
    enum socket_status
    {
        IDLE = 0,
        READABLE = 1,
        WRITABLE = 2
    };

    namespace exceptions
    {
        class CanNotCreateEventManager : public std::runtime_error
        {
        public:
            explicit CanNotCreateEventManager(const std::string error) : std::runtime_error(error) {}
        };

        class CanNotRegisterSocket : public std::runtime_error
        {
        public:
            explicit CanNotRegisterSocket(const std::string error) : std::runtime_error(error) {}
        };

        class CanNotModifySocket : public std::runtime_error
        {
        public:
            explicit CanNotModifySocket(const std::string error) : std::runtime_error(error) {}
        };

        class CanNotRemoveSocket : public std::runtime_error
        {
        public:
            explicit CanNotRemoveSocket(const std::string error) : std::runtime_error(error) {}
        };

        class CanNotWaitForEvents : public std::runtime_error
        {
        public:
            explicit CanNotWaitForEvents(const std::string error) : std::runtime_error(error) {}
        };
    }

    /// Cross-platform event poller wrapper (epoll/wepoll via pimpl).
    class EventManager
    {
        struct Impl;
        Impl *pimpl;

        std::unordered_map<int, int> status;
        int max_events;
        time_t timeout;

    public:
        /// @param max_events Maximum number of events returned in one wait call.
        /// @param timeout Wait timeout used by the backend (platform-specific unit in implementation).
        explicit EventManager(const int max_events, const time_t timeout);
        EventManager(const EventManager &) = delete;
        EventManager &operator=(const EventManager &) = delete;

        EventManager(EventManager &&other) noexcept;
        EventManager &operator=(EventManager &&other) noexcept;

        /// Registers socket for readability notifications and returns backend registration id.
        int register_for_read(const int fd);
        /// Registers socket for writability notifications and returns backend registration id.
        int register_for_write(const int fd);
        /// Removes a previously registered socket id from backend polling.
        void remove_socket(const int id);

        ~EventManager();

        /// Blocks until events are available or timeout expires, then returns ready ids.
        std::vector<int> wait_for_events();

        /// True if last wait reported readable readiness for this id.
        const bool is_readable(const int id) const;
        /// True if last wait reported writable readiness for this id.
        const bool is_writable(const int id) const;
        /// Clears cached readiness flags for this id after event handling.
        void clear_status(const int id);
    };
}

#endif // EVENT_MANAGER_HPP
