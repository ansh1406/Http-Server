#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <ctime>
#include <string>

namespace tcp
{
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

    class EventManager
    {
        struct Impl;
        Impl *pimpl;

        std::unordered_map<int, int> status;
        int max_events;
        time_t timeout;

    public:
        explicit EventManager(const int max_events, const time_t timeout);
        EventManager(const EventManager &) = delete;
        EventManager &operator=(const EventManager &) = delete;

        EventManager(EventManager &&other) noexcept;
        EventManager &operator=(EventManager &&other) noexcept;

        int register_for_read(const int fd);
        int register_for_write(const int fd);
        void remove_socket(const int id);

        ~EventManager();

        std::vector<int> wait_for_events();

        const bool is_readable(const int id) const;
        const bool is_writable(const int id) const;
        void clear_status(const int id);
    };
}

#endif // EVENT_MANAGER_HPP
