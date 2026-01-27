#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <stdexcept>

class Logger
{
public:
    enum class LogLevel
    {
        INFO,
        WARNING,
        ERROR
    };

    class CanNotInitializeLogger : public std::runtime_error
    {
    public:
        explicit CanNotInitializeLogger() : std::runtime_error("Logger initialization failed") {};
    };

    ~Logger()
    {
        if (external_log && log_file.is_open())
        {
            log_file.close();
        }
    }

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
            case LogLevel::ERROR:
                level_str = "ERROR";
                break;
            }

            std::time_t now = std::time(nullptr);
            char time_buf[20];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

            if (external_log)
            {
                log_file << "[" << time_buf << "] [" << level_str << "] " << message << std::endl;
            }
            else
            {
                std::cout << "[" << time_buf << "] [" << level_str << "] " << message << std::endl;
            }
        }
        catch (...)
        {
            // Suppress all exceptions to avoid logging failures affecting main application flow
        }
    }

private:
    std::ofstream log_file;
    bool external_log = false;
    Logger() = default;
};