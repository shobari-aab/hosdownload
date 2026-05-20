#pragma once

#include <cstdio>
#include <ctime>
#include <mutex>

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void open(const char* filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!file_) {
            file_ = fopen(filename, "a");
        }
    }

    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_) {
            fclose(file_);
            file_ = nullptr;
        }
    }

    void log(const char* msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_) {
            fprintf(file_, "%s", msg);
            fflush(file_);
        }
    }

private:
    Logger() : file_(nullptr) {}
    ~Logger() {
        close();
    }

    FILE* file_;
    std::mutex mutex_;
};

enum {ERROR = -1, INFO = 0, WARNING, FATAL};

#define AG_LOG(level, format, ...)                                             \
    do {                                                                       \
        auto now = std::chrono::system_clock::now();                           \
        auto now_time_t = std::chrono::system_clock::to_time_t(now);           \
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(   \
                          now.time_since_epoch()) % 1000;                      \
                                                                               \
        std::tm timeInfo;                                                      \
        localtime_r(&now_time_t, &timeInfo);                                   \
                                                                               \
        char timeBuffer[64];                                                   \
        std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &timeInfo); \
                                                                               \
        char logBuffer[1024];                                                  \
        snprintf(logBuffer, sizeof(logBuffer),                                 \
            "[%s.%03ld] [ APP_LOG_" #level " ] " format "\n",                  \
            timeBuffer, now_ms.count(), ##__VA_ARGS__);                        \
        Logger::instance().log(logBuffer);                                     \
    } while (0)
