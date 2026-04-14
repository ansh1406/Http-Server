#include "http/http.hpp"

#include "http_internal.hpp"
#include "logger.hpp"

std::string http::HttpServer::Impl::get_ip() const
{
    return server_socket.get_ip();
}

unsigned short http::HttpServer::Impl::get_port() const noexcept
{
    return server_socket.get_port();
}

void http::HttpConnection::log_info(const std::string &message) const
{
    try
    {
        if (!Logger::logger_running)
            return;
        Logger::get_instance().log(std::string("[CONN] [" + get_ip() + ":" + std::to_string(get_port()) + "] " + message), Logger::LogLevel::INFO);
    }
    catch (...)
    {
    }
}

void http::HttpConnection::log_warning(const std::string &message) const
{
    try
    {
        if (!Logger::logger_running)
            return;
        Logger::get_instance().log(std::string("[CONN] [" + get_ip() + ":" + std::to_string(get_port()) + "] " + message), Logger::LogLevel::WARNING);
    }
    catch (...)
    {
    }
}

void http::HttpConnection::log_error(const std::string &message) const
{
    try
    {
        if (!Logger::logger_running)
            return;
        Logger::get_instance().log(std::string("[CONN] [" + get_ip() + ":" + std::to_string(get_port()) + "] " + message), Logger::LogLevel::ERR);
    }
    catch (...)
    {
    }
}

void http::HttpServer::Impl::log_info(const std::string &message) const
{
    try
    {
        if (!Logger::logger_running)
            return;
        Logger::get_instance().log(std::string("[SERVER] " + message), Logger::LogLevel::INFO);
    }
    catch (...)
    {
    }
}

void http::HttpServer::Impl::log_warning(const std::string &message) const
{
    try
    {
        if (!Logger::logger_running)
            return;
        Logger::get_instance().log(std::string("[SERVER] " + message), Logger::LogLevel::WARNING);
    }
    catch (...)
    {
    }
}

void http::HttpServer::Impl::log_error(const std::string &message) const
{
    try
    {
        if (!Logger::logger_running)
            return;
        Logger::get_instance().log(std::string("[SERVER] " + message), Logger::LogLevel::ERR);
    }
    catch (...)
    {
    }
}