#ifndef TCP_HPP
#define TCP_HPP
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstdint>
#include <stdexcept>

#include <string>
#include <vector>

namespace tcp
{
    typedef int SocketHandle;
    typedef uint16_t Port;

    namespace constants
    {
        const SocketHandle INVALID_SOCKET = -1;
        const int SOCKET_ERROR = -1;
        const in_addr_t DEFAULT_ADDRESS = INADDR_ANY;
        const int BACKLOG = 10;
        const size_t BUFFER_EXPANTION_SIZE = 4096;
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

    /// @brief RAII class for managing socket file descriptor
    class SocketFD
    {
    private:
        SocketHandle fd_;

    public:
        explicit SocketFD(SocketHandle handle = constants::INVALID_SOCKET) noexcept : fd_(handle) {}

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

        SocketHandle fd() const noexcept { return fd_; }
        explicit operator bool() const noexcept { return fd_ != constants::INVALID_SOCKET; }

    private:
        void close_fd();
    };

    /// @brief Class representing a TCP connection socket
    class ConnectionSocket
    {
    private:
        SocketFD socket_fd;
        sockaddr_in address;

    public:
        explicit ConnectionSocket(const SocketHandle handle, const sockaddr_in &addr) noexcept : socket_fd(handle), address(addr) {}

        ConnectionSocket(ConnectionSocket &&) = default;
        ConnectionSocket &operator=(ConnectionSocket &&) = default;

        ConnectionSocket(const ConnectionSocket &) = delete;
        ConnectionSocket &operator=(const ConnectionSocket &) = delete;

        SocketHandle fd() const noexcept
        {
            return socket_fd.fd();
        }
        size_t send_data(const std::vector<char> &data, size_t start_pos);
        std::vector<char> receive_data();

        /// @return IP address of the connected peer as a string
        std::string get_ip() const
        {
            return std::string(inet_ntoa(address.sin_addr));
        }

        /// @return Port number of the connected peer
        Port get_port() const noexcept
        {
            return ntohs(address.sin_port);
        }
    };

    /// @brief Class representing a TCP listening socket
    class ListeningSocket
    {
    private:
        sockaddr_in address;
        sockaddr_in create_address(const in_addr_t ip, const Port port);
        SocketFD create_socket();
        void bind_socket();
        void start_listening();
        SocketFD socket_fd;
        unsigned int max_pending_connections;

    public:
        ListeningSocket(const in_addr_t ip, const Port port, const unsigned int max_pending);
        explicit ListeningSocket(const Port port, const unsigned int max_pending) : ListeningSocket(constants::DEFAULT_ADDRESS, port, max_pending) {}
        explicit ListeningSocket(const Port port) : ListeningSocket(constants::DEFAULT_ADDRESS, port, constants::BACKLOG) {}

        ListeningSocket(ListeningSocket &&) = default;
        ListeningSocket &operator=(ListeningSocket &&) = default;

        ListeningSocket(const ListeningSocket &) = delete;
        ListeningSocket &operator=(const ListeningSocket &) = delete;

        SocketHandle fd() const noexcept
        {
            return socket_fd.fd();
        }
        /// @brief Accepts an incoming connection and returns a ConnectionSocket object
        /// @return ConnectionSocket representing the accepted connection
        std::vector<ConnectionSocket> accept_connections();

        /// @return IP address the socket is bound to as a string
        std::string get_ip() const
        {
            return std::string(inet_ntoa(address.sin_addr));
        }

        /// @return Port number the socket is bound to
        Port get_port() const noexcept
        {
            return ntohs(address.sin_port);
        }
    };

}
#endif