#include "includes/tcp.hpp"
tcp::ListeningSocket::ListeningSocket(const in_addr_t ip, const tcp::Port port)
{
    try
    {
        address = create_address(ip, port);
        socket_fd = create_socket();
        bind_socket();
        start_listening();
    }
    catch (const tcp::exceptions::CanNotCreateSocket &e)
    {
        std::cerr << e.what() << std::endl;
        throw tcp::exceptions::SocketNotCreated{"Cannot create socket."};
    }
    catch (const tcp::exceptions::CanNotSetSocketOptions &e)
    {
        std::cerr << e.what() << std::endl;
        throw tcp::exceptions::SocketNotCreated{"Cannot set socket options."};
    }
    catch (const tcp::exceptions::CanNotBindSocket &e)
    {
        std::cerr << e.what() << std::endl;
        throw tcp::exceptions::SocketNotCreated{"Cannot bind socket."};
    }
    catch (const tcp::exceptions::CanNotListenOnSocket &e)
    {
        std::cerr << e.what() << std::endl;
        throw tcp::exceptions::SocketNotCreated{"Cannot listen on socket."};
    }
    catch (...)
    {
        std::cerr << "Unknown error occurred during socket creation" << std::endl;
        throw tcp::exceptions::SocketNotCreated{"Unknown error while setting up the socket."};
    }
}

tcp::SocketFD tcp::ListeningSocket::create_socket()
{
    SocketFD sock(socket(AF_INET, SOCK_STREAM, 0));
    if (sock.fd() < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotCreateSocket{std::string(strerror(err))};
    }
    int optionValue = tcp::constants::OPTION_TRUE;
    if (setsockopt(sock.fd(), SOL_SOCKET, SO_REUSEADDR, &optionValue, sizeof(int)) < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotSetSocketOptions{std::string(strerror(err))};
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
        throw tcp::exceptions::CanNotBindSocket{std::string(strerror(err))};
    }
}

void tcp::ListeningSocket::start_listening()
{
    if (listen(socket_fd.fd(), tcp::constants::BACKLOG) < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotListenOnSocket{std::string(strerror(err))};
    }
}

tcp::ConnectionSocket tcp::ListeningSocket::accept_connection()
{
    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    ConnectionSocket client_socket(accept(socket_fd.fd(), reinterpret_cast<struct sockaddr *>(&client_addr), &client_len));
    if (client_socket.fd() < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotAcceptConnection{std::string(strerror(err))};
    }
    return client_socket;
}

void tcp::ConnectionSocket::send_data(const std::vector<char> &data)
{
    ssize_t total_sent = 0;
    ssize_t data_length = data.size();
    const char *data_ptr = data.data();

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

std::vector<char> tcp::ConnectionSocket::receive_data(const size_t max_size)
{
    std::vector<char> buffer(max_size);
    ssize_t bytes_received = recv(socket_fd.fd(), buffer.data(), max_size, 0);
    if (bytes_received < 0)
    {
        int err = errno;
        throw tcp::exceptions::CanNotReceiveData{std::string(strerror(err))};
    }
    /// @todo : Watch for EINTR and EAGAIN
    return buffer;
}