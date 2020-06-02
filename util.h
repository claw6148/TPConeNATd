//
// Created by root on 5/31/20.
//

#ifndef TPCONENATD_UTIL_H
#define TPCONENATD_UTIL_H

#include <fcntl.h>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <syslog.h>
#include <cstring>

class retry_exception : public std::runtime_error {
public:
    explicit retry_exception(const std::string &s) : std::runtime_error(s) {}
};

class warn_exception : public std::runtime_error {
public:
    explicit warn_exception(const std::string &s) : std::runtime_error(s) {}
};

#define THROW_IF_EX(expr, exception, str) do { \
    if (!(expr)) break; \
    std::string exc_str; \
    exc_str += strchr(__FILE__, '/') ? strrchr(__FILE__ , '/') + 1 : __FILE__; \
    exc_str += ":" + std::to_string(__LINE__) + " "#expr; \
    if(strlen(str)) { \
        exc_str += " "; \
        exc_str += (str); \
    } \
    throw exception(exc_str); \
} while(0)

#define THROW_IF(expr) THROW_IF_EX(expr, std::runtime_error, "")
#define THROW_IF_FALSE(expr) THROW_IF_EX((expr) == false, std::runtime_error, "")
#define THROW_IF_NEG(expr) THROW_IF((expr) < 0)
#define THROW_RETRY_IF_NEG(expr) THROW_IF_EX((expr) < 0, retry_exception, "")

extern const char *log_level_str[];
extern bool run_as_daemon;
extern uint8_t log_level;

#define LOG_REQUIRED(level) if((level) <= log_level)

#define LOG(level, fmt, ...) do { \
    if (level > log_level) break; \
    if (run_as_daemon) \
        syslog(level, "[%s] " fmt"\n", log_level_str[level], ##__VA_ARGS__); \
    else \
        fprintf(level <= LOG_ERR ? stderr : stdout, "[%s] " fmt"\n", log_level_str[level], ##__VA_ARGS__); \
} while(0)

#define PERROR(msg) do { \
    if(errno) \
        LOG(LOG_CRIT, "%s, %d (%s)", msg, errno, strerror(errno)); \
    else \
        LOG(LOG_CRIT, "%s", msg); \
} while(0)

void set_nonblock(int fd);

uint16_t update_check16(uint16_t check, uint16_t old_val, uint16_t new_val);

uint16_t update_check32(uint16_t check, uint32_t old_val, uint32_t new_val);

#endif //TPCONENATD_UTIL_H
