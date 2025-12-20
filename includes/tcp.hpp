#ifndef TCP_HPP
#define TCP_HPP
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstdint>
#include <stdexcept>

#include <iostream>
#include <string>
#include <vector>
#include <cstring>

namespace tcp
{
    typedef int SocketHandle;
    typedef uint16_t Port;

    namespace constants
    {
        const SocketHandle INVALID_SOCKET = -1;
        const int SOCKET_ERROR = -1;
        const in_addr_t DEFAULT_ADDRESS = INADDR_ANY;
        const int BACKLOG = 128;
        const ssize_t MAX_BUFFER_SIZE = 4096;
        const int OPTION_TRUE = 1;
    }

    namespace exceptions
    {
        class CanNotCreateSocket : public std::runtime_error
        {
        public:
            explicit CanNotCreateSocket(const std::string error) : std::runtime_error(error) {}
        };
        class CanNotBindSocket : public std::runtime_error
        {
        public:
            explicit CanNotBindSocket(const std::string error) : std::runtime_error(error) {}
        };
        class CanNotSetSocketOptions : public std::runtime_error
        {
        public:
            explicit CanNotSetSocketOptions(const std::string error) : std::runtime_error(error) {}
        };
        class SocketNotCreated : public std::runtime_error
        {
        public:
            explicit SocketNotCreated(const std::string error) : std::runtime_error(error) {}
        };
        class CanNotListenOnSocket : public std::runtime_error
        {
        public:
            explicit CanNotListenOnSocket(const std::string error) : std::runtime_error(error) {}
        };
        class CanNotAcceptConnection : public std::runtime_error
        {
        public:
            explicit CanNotAcceptConnection(const std::string error) : std::runtime_error(error) {}
        };
        class CanNotSendData : public std::runtime_error
        {
        public:
            explicit CanNotSendData(const std::string error) : std::runtime_error(error) {}
        };
        class CanNotReceiveData : public std::runtime_error
        {
        public:
            explicit CanNotReceiveData(const std::string error) : std::runtime_error(error) {}
        };
    }

    class SocketFD
    {
    private:
        SocketHandle fd_;

    public:
        explicit SocketFD(SocketHandle handle = constants::INVALID_SOCKET) : fd_(handle) {}

        SocketFD(const SocketFD &) = delete;
        SocketFD &operator=(const SocketFD &) = delete;

        SocketFD(SocketFD &&other) noexcept : fd_(other.fd_)
        {
            other.fd_ = constants::INVALID_SOCKET;
        }
        SocketFD &operator=(SocketFD &&other) noexcept
        {
            if (this != &other)
            {
                close_fd();
                fd_ = other.fd_;
                other.fd_ = constants::INVALID_SOCKET;
            }
            return *this;
        }

        ~SocketFD()
        {
            close_fd();
        }

        SocketHandle fd() const { return fd_; }
        explicit operator bool() const { return fd_ != constants::INVALID_SOCKET; }

    private:
        void close_fd()
        {
            if (fd_ != constants::INVALID_SOCKET)
            {
                ::close(fd_);
                fd_ = constants::INVALID_SOCKET;
            }
        }
    };

    class ConnectionSocket
    {
    private:
        SocketFD socket_fd;

    public:
        explicit ConnectionSocket(const SocketHandle handle) : socket_fd(handle) {}

        ConnectionSocket(ConnectionSocket &&) = default;
        ConnectionSocket &operator=(ConnectionSocket &&) = default;

        ConnectionSocket(const ConnectionSocket &) = delete;
        ConnectionSocket &operator=(const ConnectionSocket &) = delete;

        SocketHandle fd() const
        {
            return socket_fd.fd();
        }
        void send_data(const std::vector<char> &data);
        std::vector<char> receive_data(const size_t max_size = constants::MAX_BUFFER_SIZE);
    };
    class ListeningSocket
    {
    private:
        sockaddr_in address;
        sockaddr_in create_address(const in_addr_t ip, const Port port);
        SocketFD create_socket();
        void bind_socket();
        void start_listening();
        SocketFD socket_fd;

    public:
        explicit ListeningSocket(const in_addr_t ip, const Port port);
        explicit ListeningSocket(const Port port) : ListeningSocket(constants::DEFAULT_ADDRESS, port) {}

        ListeningSocket() = default;

        ListeningSocket(ListeningSocket &&) = default;
        ListeningSocket &operator=(ListeningSocket &&) = default;

        ListeningSocket(const ListeningSocket &) = delete;
        ListeningSocket &operator=(const ListeningSocket &) = delete;
        
        SocketHandle fd() const
        {
            return socket_fd.fd();
        }
        ConnectionSocket accept_connection();
    };

}
#endif