#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <stdexcept>

#include <mutex>

namespace
{
    std::tm get_local_time(std::time_t t)
    {
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        return tm;
    }
}

class Logger
{
public:
    enum class LogLevel
    {
        INFO,
        WARNING,
        ERR
    };

    class CanNotInitializeLogger : public std::runtime_error
    {
    public:
        explicit CanNotInitializeLogger() : std::runtime_error("Logger initialization failed") {};
    };

    ~Logger() = default;

    static Logger &get_instance()
    {
        try
        {
            static Logger instance;
            return instance;
        }
        catch (const std::exception &e)
        {
            std::cout << "Logger initialization failed: " << e.what() << std::endl;
            throw CanNotInitializeLogger();
        }
        catch (...)
        {
            std::cout << "Logger initialization failed: Unknown error" << std::endl;
            throw CanNotInitializeLogger();
        }
    }

    static void set_external_logging(const std::string &filename)
    {
        Logger &logger = get_instance();
        logger.log_file.open(filename, std::ios::app);
        if (!logger.log_file.is_open())
        {
            throw std::runtime_error("Unable to open log file: " + filename);
        }
        logger.external_log = true;
    }

    void log(const std::string &message, LogLevel level)
    {
        try
        {
            std::string level_str;
            switch (level)
            {
            case LogLevel::INFO:
                level_str = "INFO";
                break;
            case LogLevel::WARNING:
                level_str = "WARNING";
                break;
            case LogLevel::ERR:
                level_str = "ERROR";
                break;
            }

            std::time_t now = std::time(nullptr);
            char time_buf[20];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &::get_local_time(now));

            {
                std::lock_guard<std::mutex> lock(log_mutex);
                std::string log_line = "[" + std::string(time_buf) + "] [" + level_str + "] " + message;
                if (external_log)
                {
                    log_file << log_line << std::endl;
                }
                else
                {
                    std::cout << log_line << std::endl;
                }
            }
        }
        catch (...)
        {
            // Suppress all exceptions to avoid logging failures affecting main application flow
        }
    }

    static void shutdown()
    {
        Logger &logger = get_instance();
        if (logger.external_log && logger.log_file.is_open())
        {
            logger.log_file.close();
        }
    }

private:
    std::ofstream log_file;
    bool external_log = false;
    std::mutex log_mutex;

    Logger() = default;
};