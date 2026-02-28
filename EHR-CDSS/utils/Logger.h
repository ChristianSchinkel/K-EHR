#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <mutex>

namespace ehr {

/**
 * @brief Simple thread-safe logger utility.
 *
 * Writes timestamped entries to stdout and optionally to a log file.
 * Log levels: DEBUG < INFO < WARN < ERROR
 */
class Logger {
public:
    enum class Level { DEBUG = 0, INFO = 1, WARN = 2, ERR = 3 };

    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    void setLevel(Level level)          { minLevel_ = level; }
    void setLogFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fileStream_.is_open()) fileStream_.close();
        if (!path.empty()) fileStream_.open(path, std::ios::app);
    }

    void debug(const std::string& msg) { log(Level::DEBUG, msg); }
    void info (const std::string& msg) { log(Level::INFO,  msg); }
    void warn (const std::string& msg) { log(Level::WARN,  msg); }
    void error(const std::string& msg) { log(Level::ERR,   msg); }

private:
    Logger() = default;
    ~Logger() { if (fileStream_.is_open()) fileStream_.close(); }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(Level level, const std::string& msg) {
        if (level < minLevel_) return;
        std::string entry = timestamp() + " [" + levelStr(level) + "] " + msg;
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << entry << "\n";
        if (fileStream_.is_open()) { fileStream_ << entry << "\n"; fileStream_.flush(); }
    }

    static std::string timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return buf;
    }

    static const char* levelStr(Level l) {
        switch (l) {
            case Level::DEBUG: return "DEBUG";
            case Level::INFO:  return "INFO ";
            case Level::WARN:  return "WARN ";
            case Level::ERR:   return "ERROR";
        }
        return "?????";
    }

    Level         minLevel_  = Level::INFO;
    std::mutex    mutex_;
    std::ofstream fileStream_;
};

/* Convenience macros */
#define LOG_DEBUG(msg) ehr::Logger::instance().debug(msg)
#define LOG_INFO(msg)  ehr::Logger::instance().info(msg)
#define LOG_WARN(msg)  ehr::Logger::instance().warn(msg)
#define LOG_ERROR(msg) ehr::Logger::instance().error(msg)

} // namespace ehr
