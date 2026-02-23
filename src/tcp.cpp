#include "tcp.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>

#include <csignal>
#include <cstring>
#include <cerrno>

tcp::ListeningSocket::ListeningSocket(const uint32_t ip, const tcp::Port port, const unsigned int max_pending)
{
    max_pending_connections = max_pending;
    try
    {
        signal(SIGPIPE, SIG_IGN);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ip;
        addr.sin_port = htons(port);

        ip_ = std::string(inet_ntoa(addr.sin_addr));
        port_ = port;

        // create socket
        SocketFD sock(::socket(AF_INET, SOCK_STREAM, 0));
        if (sock.fd() < 0)
        {
            int err = errno;
            throw tcp::exceptions::CanNotCreateSocket{std::string("TCP: ") + std::string(strerror(err))};
        }

        // set reuse addr
        int optionValue = tcp::constants::OPTION_TRUE;
        if (setsockopt(sock.fd(), SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(int)) < 0)
        {
            int err = errno;
            throw tcp::exceptions::CanNotSetSocketOptions{std::string("TCP: ") + std::string(strerror(err))};
        }

        int flags = fcntl(sock.fd(), F_GETFL, 0);
        if (flags == -1)
            flags = 0;
        flags |= O_NONBLOCK;
        if (fcntl(sock.fd(), F_SETFL, flags) < 0)
        {
            int err = errno;
            throw tcp::exceptions::CanNotSetSocketOptions{std::string("TCP: ") + std::string(strerror(err))};
        }

        socket_fd = std::move(sock);

        if (bind(socket_fd.fd(), reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
        {
            int err = errno;
            throw tcp::exceptions::CanNotBindSocket{std::string("TCP: ") + std::string(strerror(err))};
        }

        if (listen(socket_fd.fd(), max_pending_connections) < 0)
        {
            int err = errno;
            throw tcp::exceptions::CanNotListenOnSocket{std::string("TCP: ") + std::string(strerror(err))};
        }
    }
    catch (const tcp::exceptions::CanNotCreateSocket &e)
    {
        throw tcp::exceptions::SocketNotCreated{"TCP: Cannot create socket: " + std::string(e.what())};
    }
    catch (const tcp::exceptions::CanNotSetSocketOptions &e)
    {
        throw tcp::exceptions::SocketNotCreated{"TCP: Cannot set socket options: " + std::string(e.what())};
    }
    catch (const tcp::exceptions::CanNotBindSocket &e)
    {
        throw tcp::exceptions::SocketNotCreated{"TCP: Cannot bind socket: " + std::string(e.what())};
    }
    catch (const tcp::exceptions::CanNotListenOnSocket &e)
    {
        throw tcp::exceptions::SocketNotCreated{"TCP: Cannot listen on socket: " + std::string(e.what())};
    }
    catch (...)
    {
        throw tcp::exceptions::SocketNotCreated{"TCP: Unknown error while setting up the socket."};
    }
}

std::vector<tcp::ConnectionSocket> tcp::ListeningSocket::accept_connections()
{
    try
    {
        std::vector<ConnectionSocket> connections;
        while (true)
        {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            tcp::SocketHandle sock = accept(socket_fd.fd(), reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);
            if (sock < 0)
            {
                int err = errno;
                if (err == EAGAIN || err == EWOULDBLOCK)
                {
                    break;
                }
                throw tcp::exceptions::CanNotAcceptConnection{std::string("TCP: ") + std::string(strerror(err))};
            }
            int flags = fcntl(sock, F_GETFL, 0);
            if (flags == -1)
                flags = 0;
            flags |= O_NONBLOCK;
            if (fcntl(sock, F_SETFL, flags) < 0)
            {
                int err = errno;
                throw tcp::exceptions::CanNotSetSocketOptions{std::string("TCP: ") + std::string(strerror(err))};
            }
            std::string client_ip = std::string(inet_ntoa(client_addr.sin_addr));
            tcp::Port client_port = ntohs(client_addr.sin_port);
            ConnectionSocket client_socket(sock, client_ip, client_port);
            connections.push_back(std::move(client_socket));
        }
        return connections;
    }
    catch (const tcp::exceptions::CanNotAcceptConnection &e)
    {
        throw;
    }
    catch (const tcp::exceptions::CanNotSetSocketOptions &e)
    {
        throw tcp::exceptions::CanNotAcceptConnection{"TCP: Failed to set socket options: " + std::string(e.what())};
    }
    catch (const std::exception &e)
    {
        throw tcp::exceptions::CanNotAcceptConnection{"TCP: Failed to accept connection: " + std::string(e.what())};
    }
    catch (...)
    {
        throw tcp::exceptions::CanNotAcceptConnection{"TCP: Unknown error while accepting connection."};
    }
}

size_t tcp::ConnectionSocket::send_data(const std::vector<char> &data, size_t start_pos)
{
    ssize_t total_sent = 0;
    ssize_t data_length = data.size() - start_pos;
    const char *data_ptr = data.data() + start_pos;
    try
    {
        while (total_sent < data_length)
        {
            ssize_t bytes_sent = send(socket_fd.fd(), data_ptr + total_sent, data_length - total_sent, 0);
            if (bytes_sent < 0)
            {
                int err = errno;
                if (err == EAGAIN || err == EWOULDBLOCK)
                {
                    break;
                }
                throw tcp::exceptions::CanNotSendData{std::string(strerror(err))};
            }
            total_sent += bytes_sent;
        }
        return total_sent;
    }
    catch (const std::exception &e)
    {
        throw tcp::exceptions::CanNotSendData{"TCP: Failed to send all data: " + std::string(e.what())};
    }
    catch (...)
    {
        throw tcp::exceptions::CanNotSendData{"TCP: Unknown error while sending data."};
    }
}

std::vector<char> tcp::ConnectionSocket::receive_data()
{
    try
    {
        std::vector<char> buffer;
        size_t total_received = 0;
        while (true)
        {
            int remaining_space = buffer.size() - total_received;
            if (remaining_space < constants::BUFFER_EXPANTION_SIZE)
            {
                buffer.resize(buffer.size() + constants::BUFFER_EXPANTION_SIZE);
                remaining_space = buffer.size() - total_received;
            }
            long bytes_received = recv(socket_fd.fd(), buffer.data() + total_received, remaining_space, 0);
            if (bytes_received == 0)
            {
                throw tcp::exceptions::CanNotReceiveData{"Connection closed by peer."};
            }
            if (bytes_received < 0)
            {
                int err = errno;
                if (err == EAGAIN || err == EWOULDBLOCK)
                {
                    break;
                }
                else
                {
                    throw tcp::exceptions::CanNotReceiveData{std::string(strerror(err))};
                }
            }
            total_received += bytes_received;
        }
        buffer.resize(total_received);
        return buffer;
    }
    catch (const std::exception &e)
    {
        throw tcp::exceptions::CanNotReceiveData{"TCP: Failed to receive data: " + std::string(e.what())};
    }
    catch (...)
    {
        throw tcp::exceptions::CanNotReceiveData{"TCP: Unknown error while receiving data."};
    }
}

void tcp::SocketFD::close_fd()
{
    if (fd_ != constants::INVALID_SOCKET)
    {
        ::close(fd_);
        fd_ = constants::INVALID_SOCKET;
    }
}