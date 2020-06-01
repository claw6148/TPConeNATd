//
// Created by root on 5/31/20.
//

#ifndef TPCONENATD_UTIL_H
#define TPCONENATD_UTIL_H

#include <fcntl.h>
#include <cstdint>
#include <stdexcept>
#include <string>

class retry_exception : public std::runtime_error {
public:
    explicit retry_exception(const std::string &s) : std::runtime_error(s) {}
};

#define THROW_IF_EX(expr, exc) do { \
    if (!(expr)) break; \
    throw exc(__FILE__":" + std::to_string(__LINE__)); \
} while(0)

#define THROW_IF_NEG(expr) THROW_IF_EX((expr) < 0, std::runtime_error)
#define THROW_RETRY_IF_NEG(expr) THROW_IF_EX((expr) < 0, retry_exception)

void inline set_nonblock(int fd) {
    int fl;
    fl = fcntl(fd, F_GETFL);
    THROW_IF_NEG(fl);
    THROW_IF_NEG(fcntl(fd, F_SETFL, (uint32_t) fl | (uint32_t) O_NONBLOCK));
}

#endif //TPCONENATD_UTIL_H
