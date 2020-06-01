//
// Created by root on 5/31/20.
//

#ifndef TPCONENATD_UTIL_H
#define TPCONENATD_UTIL_H

#include <fcntl.h>
#include <cstdint>
#include <stdexcept>
#include <string>

#define THROW_IF(expr) do { \
    if (!(expr)) break; \
    throw std::runtime_error(__FILE__":" + std::to_string(__LINE__)); \
} while(0)

#define THROW_IF_NEG(expr) THROW_IF((expr) < 0)

void inline set_nonblock(int fd) {
    int fl;
    fl = fcntl(fd, F_GETFL);
    THROW_IF_NEG(fl);
    THROW_IF_NEG(fcntl(fd, F_SETFL, (uint32_t) fl | (uint32_t) O_NONBLOCK));
}

#endif //TPCONENATD_UTIL_H
