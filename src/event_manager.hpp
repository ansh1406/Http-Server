#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

#include <sys/epoll.h>
#include <unistd.h>

#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <cstring>

namespace tcp
{
    namespace socket_status
    {
        const int IDLE = 0;
        const int READABLE = 1;
        const int WRITABLE = 2;
    }

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
        using Event = struct epoll_event;
        int epoll_fd;
        std::unordered_map<int, int> status;
        int max_events;
        time_t timeout;

    public:
        explicit EventManager(const int max_events, const time_t timeout) : max_events(max_events), timeout(timeout)
        {
            epoll_fd = epoll_create1(0);
            if (epoll_fd == -1)
            {
                int error = errno;
                throw exceptions::CanNotCreateEventManager("Failed to create epoll instance: " + std::string(strerror(error)));
            }
        }
        EventManager(const EventManager &) = delete;
        EventManager &operator=(const EventManager &) = delete;

        EventManager(EventManager &&other) noexcept : epoll_fd(other.epoll_fd), status(std::move(other.status)), max_events(other.max_events), timeout(other.timeout)
        {
            other.epoll_fd = -1;
        }

        EventManager &operator=(EventManager &&other) noexcept
        {
            if (this != &other)
            {
                close(epoll_fd);
                epoll_fd = other.epoll_fd;
                status = std::move(other.status);
                max_events = other.max_events;
                timeout = other.timeout;
                other.epoll_fd = -1;
            }
            return *this;
        }

        int register_socket(const int fd)
        {
            try
            {
                Event ev;
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
                {
                    int error = errno;
                    throw exceptions::CanNotRegisterSocket("Failed to register socket: " + std::string(strerror(error)));
                }
                status[fd] = socket_status::WRITABLE;
                return fd;
            }
            catch (tcp::exceptions::CanNotRegisterSocket &)
            {
                throw;
            }
            catch (std::exception &e)
            {
                throw tcp::exceptions::CanNotRegisterSocket("Failed to register socket: " + std::string(e.what()));
            }
            catch (...)
            {
                throw tcp::exceptions::CanNotRegisterSocket("Failed to register socket: Unknown error");
            }
        }

        void add_to_write_monitoring(const int id)
        {
            try
            {
                Event ev;
                ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                ev.data.fd = id;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, id, &ev) == -1)
                {
                    int error = errno;
                    throw exceptions::CanNotModifySocket("Failed to modify socket for write monitoring: " + std::string(strerror(error)));
                }
            }
            catch (tcp::exceptions::CanNotModifySocket &)
            {
                throw;
            }
            catch (std::exception &e)
            {
                throw tcp::exceptions::CanNotModifySocket("Failed to modify socket for write monitoring: " + std::string(e.what()));
            }
            catch (...)
            {
                throw tcp::exceptions::CanNotModifySocket("Failed to modify socket for write monitoring: Unknown error");
            }
        }

        void remove_socket(const int id)
        {
            try
            {
                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, id, nullptr) == -1)
                {
                    int error = errno;
                    throw exceptions::CanNotRemoveSocket("Failed to remove socket: " + std::string(strerror(error)));
                }
                status.erase(id);
            }
            catch (tcp::exceptions::CanNotRemoveSocket &)
            {
                throw;
            }
            catch (std::exception &e)
            {
                throw tcp::exceptions::CanNotRemoveSocket("Failed to remove socket: " + std::string(e.what()));
            }
            catch (...)
            {
                throw tcp::exceptions::CanNotRemoveSocket("Failed to remove socket: Unknown error");
            }
        }

        ~EventManager()
        {
            close(epoll_fd);
        }

        std::vector<int> wait_for_events()
        {
            try
            {
                std::vector<Event> events(max_events);
                int num_events = epoll_wait(epoll_fd, events.data(), max_events, timeout);
                if (num_events == -1)
                {
                    int error = errno;
                    throw exceptions::CanNotWaitForEvents("Failed to wait for events: " + std::string(strerror(error)));
                }
                std::vector<int> fds;
                for (int i = 0; i < num_events; ++i)
                {
                    int fd = events[i].data.fd;
                    if (events[i].events & EPOLLIN)
                    {
                        status[fd] |= socket_status::READABLE;
                    }
                    if (events[i].events & EPOLLOUT)
                    {
                        status[fd] |= socket_status::WRITABLE;
                    }
                    fds.push_back(fd);
                }
                return fds;
            }
            catch (tcp::exceptions::CanNotWaitForEvents &)
            {
                throw;
            }
            catch (std::exception &e)
            {
                throw tcp::exceptions::CanNotWaitForEvents("Failed to wait for events: " + std::string(e.what()));
            }
            catch (...)
            {
                throw tcp::exceptions::CanNotWaitForEvents("Failed to wait for events: Unknown error");
            }
        }

        const bool is_readable(const int id) const
        {
            return status.at(id) & socket_status::READABLE;
        }

        const bool is_writable(const int id) const
        {
            return status.at(id) & socket_status::WRITABLE;
        }

        void clear_status(const int id)
        {
            status[id] = socket_status::IDLE;
        }
    };
}

#endif // EVENT_MANAGER_HPP
