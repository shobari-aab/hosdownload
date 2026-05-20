#include <cstdio>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

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
        fprintf(stderr, "[%s.%03ld] [ APP_LOG_" #level " ] " format "\n",      \
                timeBuffer, now_ms.count(), ##__VA_ARGS__);                    \
    } while (0)
