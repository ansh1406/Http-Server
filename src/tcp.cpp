#include "includes/tcp.hpp"
#include <csignal>

tcp::ListeningSocket::ListeningSocket(const in_addr_t ip, const tcp::Port port , const unsigned int max_pending)
{
    max_pending_connections = max_pending;
    try
    {
        signal(SIGPIPE, SIG_IGN);
        address = create_address(ip, port);
        socket_fd = create_socket();
        bind_socket();
        start_listening();
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

tcp::SocketFD tcp::ListeningSocket::create_socket()
{
    SocketFD sock(socket(AF_INET, SOCK_STREAM, 0));
    if (sock.fd() < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotCreateSocket{std::string("TCP: ") + std::string(strerror(err))};
    }
    int optionValue = tcp::constants::OPTION_TRUE;
    if (setsockopt(sock.fd(), SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(int)) < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotSetSocketOptions{std::string("TCP: ") + std::string(strerror(err))};
    }
    return sock;
}

sockaddr_in tcp::ListeningSocket::create_address(const in_addr_t ip, const tcp::Port port)
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);
    return addr;
}

void tcp::ListeningSocket::bind_socket()
{
    if (bind(socket_fd.fd(), reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotBindSocket{std::string("TCP: ") + std::string(strerror(err))};
    }
}

void tcp::ListeningSocket::start_listening()
{
    if (listen(socket_fd.fd(), max_pending_connections) < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotListenOnSocket{std::string("TCP: ") + std::string(strerror(err))};
    }
}

tcp::ConnectionSocket tcp::ListeningSocket::accept_connection()
{
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    tcp::SocketHandle sock = accept(socket_fd.fd(), reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);
    if (sock < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotAcceptConnection{std::string("TCP: ") + std::string(strerror(err))};
    }
    ConnectionSocket client_socket(sock, client_addr);
    return client_socket;
}

void tcp::ConnectionSocket::send_data(const std::vector<char> &data)
{
    ssize_t total_sent = 0;
    ssize_t data_length = data.size();
    const char *data_ptr = data.data();

    try
    {
        while (total_sent < data_length)
        {
            ssize_t bytes_sent = send(socket_fd.fd(), data_ptr + total_sent, data_length - total_sent, 0);
            if (bytes_sent < 0)
            {
                int err = errno;
                throw tcp::exceptions::CanNotSendData{std::string(strerror(err))};
            }
            total_sent += bytes_sent;
        }
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

std::vector<char> tcp::ConnectionSocket::receive_data(const size_t max_size)
{
    std::vector<char> buffer(max_size);
    try
    {
        ssize_t bytes_received = recv(socket_fd.fd(), buffer.data(), max_size, 0);
        if (bytes_received < 0)
        {
            int err = errno;
            throw tcp::exceptions::CanNotReceiveData{std::string(strerror(err))};
        }
        buffer.resize(bytes_received);
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