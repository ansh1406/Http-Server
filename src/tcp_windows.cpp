#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "tcp.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace tcp
{
    class WinsockManager
    {
    public:
        static void init()
        {
            if (ref_count++ == 0)
            {
                WSADATA wsaData;
                if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
                {
                    throw std::runtime_error("WSAStartup failed");
                }
            }
        }

        static void cleanup()
        {
            if (--ref_count == 0)
            {
                WSACleanup();
            }
        }

    private:
        static int ref_count;
    };

    int WinsockManager::ref_count = 0;

    std::string get_error_message()
    {
        int err = WSAGetLastError();
        return std::to_string(err);
    }

    ListeningSocket::ListeningSocket(const uint32_t ip, const Port port, const unsigned int max_pending)
    {
        max_pending_connections = max_pending;
        WinsockManager::init();
        try
        {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = ip;
            addr.sin_port = htons(port);

            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr.sin_addr, ip_str, sizeof(ip_str));
            ip_ = std::string(ip_str);
            port_ = port;

            // create socket
            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (static_cast<SocketHandle>(sock) == constants::INVALID_HANDLE)
            {
                throw exceptions::CanNotCreateSocket{std::string("TCP: ") + get_error_message()};
            }

            // set reuse addr
            int optionValue = constants::OPTION_TRUE;
            if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&optionValue), sizeof(int)) != 0)
            {
                closesocket(sock);
                throw exceptions::CanNotSetSocketOptions{std::string("TCP: ") + get_error_message()};
            }

            // set non-blocking
            u_long mode = 1;
            if (ioctlsocket(sock, FIONBIO, &mode) != 0)
            {
                closesocket(sock);
                throw exceptions::CanNotSetSocketOptions{std::string("TCP: ") + get_error_message()};
            }

            socket_fd = SocketFD(sock);

            if (bind(socket_fd.fd(), reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
            {
                throw exceptions::CanNotBindSocket{std::string("TCP: ") + get_error_message()};
            }

            if (listen(socket_fd.fd(), max_pending_connections) != 0)
            {
                throw exceptions::CanNotListenOnSocket{std::string("TCP: ") + get_error_message()};
            }
        }
        catch (const exceptions::CanNotCreateSocket &e)
        {
            throw exceptions::SocketNotCreated{"TCP: Cannot create socket: " + std::string(e.what())};
        }
        catch (const exceptions::CanNotSetSocketOptions &e)
        {
            throw exceptions::SocketNotCreated{"TCP: Cannot set socket options: " + std::string(e.what())};
        }
        catch (const exceptions::CanNotBindSocket &e)
        {
            throw exceptions::SocketNotCreated{"TCP: Cannot bind socket: " + std::string(e.what())};
        }
        catch (const exceptions::CanNotListenOnSocket &e)
        {
            throw exceptions::SocketNotCreated{"TCP: Cannot listen on socket: " + std::string(e.what())};
        }
        catch (...)
        {
            throw exceptions::SocketNotCreated{"TCP: Unknown error while setting up the socket."};
        }
    }

    std::vector<ConnectionSocket> ListeningSocket::accept_connections()
    {
        try
        {
            std::vector<ConnectionSocket> connections;
            while (true)
            {
                sockaddr_in client_addr{};
                int client_len = sizeof(client_addr);
                SOCKET sock = accept(socket_fd.fd(), reinterpret_cast<sockaddr *>(&client_addr), &client_len);
                if (static_cast<SocketHandle>(sock) == constants::INVALID_HANDLE)
                {
                    int err = WSAGetLastError();
                    if (err == WSAEWOULDBLOCK)
                    {
                        break;
                    }
                    throw exceptions::CanNotAcceptConnection{std::string("TCP: ") + get_error_message()};
                }

                // set non-blocking
                u_long mode = 1;
                if (ioctlsocket(sock, FIONBIO, &mode) != 0)
                {
                    closesocket(sock);
                    throw exceptions::CanNotSetSocketOptions{std::string("TCP: ") + get_error_message()};
                }

                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                Port client_port = ntohs(client_addr.sin_port);
                ConnectionSocket client_socket(static_cast<SocketHandle>(sock), std::string(client_ip), client_port);
                connections.push_back(std::move(client_socket));
            }
            return connections;
        }
        catch (const exceptions::CanNotAcceptConnection &e)
        {
            throw;
        }
        catch (const exceptions::CanNotSetSocketOptions &e)
        {
            throw exceptions::CanNotAcceptConnection{"TCP: Failed to set socket options: " + std::string(e.what())};
        }
        catch (const std::exception &e)
        {
            throw exceptions::CanNotAcceptConnection{"TCP: Failed to accept connection: " + std::string(e.what())};
        }
        catch (...)
        {
            throw exceptions::CanNotAcceptConnection{"TCP: Unknown error while accepting connection."};
        }
    }

    size_t ConnectionSocket::send_data(const std::vector<char> &data, size_t start_pos, size_t end_pos)
    {
        try
        {
            if (end_pos > data.size())
            {
                throw exceptions::CanNotSendData{"TCP: Send range exceeds buffer size."};
            }
            if (start_pos >= end_pos)
            {
                return 0;
            }

            int total_sent = 0;
            int data_length = static_cast<int>(end_pos - start_pos);
            const char *data_ptr = data.data() + start_pos;

            while (total_sent < data_length)
            {
                int bytes_sent = send(socket_fd.fd(), data_ptr + total_sent, data_length - total_sent, 0);
                if (bytes_sent == SOCKET_ERROR)
                {
                    int err = WSAGetLastError();
                    if (err == WSAEWOULDBLOCK)
                    {
                        break;
                    }
                    throw exceptions::CanNotSendData{get_error_message()};
                }
                total_sent += bytes_sent;
            }
            return total_sent;
        }
        catch (const std::exception &e)
        {
            throw exceptions::CanNotSendData{"TCP: Failed to send all data: " + std::string(e.what())};
        }
        catch (...)
        {
            throw exceptions::CanNotSendData{"TCP: Unknown error while sending data."};
        }
    }

    size_t ConnectionSocket::receive_data(std::vector<char> &buffer, size_t buffer_cursor, bool read_once)
    {
        try
        {
            size_t total_received = 0;
            while (true)
            {
                if (buffer_cursor + total_received >= buffer.size())
                {
                    break;
                }

                size_t remaining_space = buffer.size() - buffer_cursor - total_received;
                int bytes_to_read = static_cast<int>(std::min(remaining_space, static_cast<size_t>(INT_MAX)));
                int bytes_received = recv(socket_fd.fd(), buffer.data() + buffer_cursor + total_received, bytes_to_read, 0);

                if (bytes_received == 0)
                {
                    throw tcp::exceptions::CanNotReceiveData{"Connection closed by peer."};
                }
                if (bytes_received == SOCKET_ERROR)
                {
                    int err = WSAGetLastError();
                    if (err == WSAEWOULDBLOCK)
                    {
                        break;
                    }
                    else
                    {
                        throw tcp::exceptions::CanNotReceiveData{get_error_message()};
                    }
                }
                total_received += bytes_received;
                if (read_once)
                {
                    break;
                }
            }
            return total_received;
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

    void tcp::ConnectionSocket::set_socket_blocking(time_t blocking_timeout_in_milliseconds)
    {
        try
        {
            u_long mode = 0; // Blocking mode
            if (ioctlsocket(socket_fd.fd(), FIONBIO, &mode) != 0)
            {
                throw exceptions::CanNotSetSocketOptions{std::string("TCP: ") + get_error_message()};
            }
        }
        catch (const std::exception &e)
        {
            throw exceptions::CanNotSetSocketOptions{"TCP: Failed to set socket to blocking mode: " + std::string(e.what())};
        }
        catch (...)
        {
            throw exceptions::CanNotSetSocketOptions{"TCP: Unknown error while setting socket to blocking mode."};
        }

        if (blocking_timeout_in_milliseconds > 0)
        {
            DWORD timeout = static_cast<DWORD>(blocking_timeout_in_milliseconds);
            if (setsockopt(socket_fd.fd(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char *>(&timeout), sizeof(timeout)) != 0)
            {
                throw exceptions::CanNotSetSocketOptions{std::string("TCP: ") + get_error_message()};
            }
        }
    }

    void tcp::ConnectionSocket::set_socket_non_blocking()
    {
        try
        {
            u_long mode = 1; // Non-blocking mode
            if (ioctlsocket(socket_fd.fd(), FIONBIO, &mode) != 0)
            {
                throw exceptions::CanNotSetSocketOptions{std::string("TCP: ") + get_error_message()};
            }
        }
        catch (const std::exception &e)
        {
            throw exceptions::CanNotSetSocketOptions{"TCP: Failed to set socket to non-blocking mode: " + std::string(e.what())};
        }
        catch (...)
        {
            throw exceptions::CanNotSetSocketOptions{"TCP: Unknown error while setting socket to non-blocking mode."};
        }
    }

    void SocketFD::close_fd()
    {
        if (fd_ != constants::INVALID_HANDLE)
        {
            closesocket(static_cast<SOCKET>(fd_));
            fd_ = constants::INVALID_HANDLE;
            WinsockManager::cleanup();
        }
    }
}

#endif // _WIN32