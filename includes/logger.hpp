#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <sstream>
#include <chrono>

namespace http
{
    class Logger
    {
    private:
        std::queue<std::string> log_queue;
        std::mutex logger_mutex;
        std::mutex logger_done_mutex;
        std::condition_variable logger_cv;
        std::condition_variable logger_done_cv;
        bool logger_done{false};
        std::thread logger_thread;
        bool stop{false};
        Logger()
        {
            try
            {
                logger_thread = std::thread([this]()
                                            {
                {
                    std::unique_lock<std::mutex> lock(logger_done_mutex);
                    logger_done = true;
                    logger_done_cv.notify_one();
                }
                while (true)
                {
                    std::string log_entry;
                    {
                        std::unique_lock<std::mutex> lock(logger_mutex);
                        logger_cv.wait(lock, [this]() { return !log_queue.empty() || stop; });
                        if (stop && log_queue.empty())
                            break;
                        log_entry = log_queue.front();
                        log_queue.pop();
                    }
                    std::cout << log_entry << std::endl;
                } });
                {
                    std::unique_lock<std::mutex> lock(logger_done_mutex);
                    logger_done_cv.wait(lock, [this]()
                                        { return logger_done; });
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Logger thread failed to start: " << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "Logger thread failed to start due to unknown error." << std::endl;
            }
        }
        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;

    public:
        static Logger &get_instance()
        {
            static Logger instance;
            return instance;
        }

        void add_log_entry(const std::string &entry)
        {
            {
                std::unique_lock<std::mutex> lock(logger_mutex);
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << "[ " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] " << entry;
                log_queue.push(ss.str());
            }
            logger_cv.notify_one();
        }
        ~Logger()
        {
            {
                std::unique_lock<std::mutex> lock(logger_mutex);
                stop = true;
            }
            logger_cv.notify_all();
            if (logger_thread.joinable())
                logger_thread.join();
        }
    };
}

#endif // LOGGER_HPP