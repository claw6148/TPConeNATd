//
// Created by root on 5/31/20.
//

#ifndef TPCONENATD_UTIL_H
#define TPCONENATD_UTIL_H

#include <fcntl.h>
#include <cstdint>

#define FUCK(expr) do { \
    if (!(expr)) break; \
    char str[0x20]{}; \
    snprintf(str, sizeof(str) - 1, "%d", __LINE__); \
    throw runtime_error(str); \
} while(0)

void inline set_nonblock(int fd) {
    int fl;
    fl = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, (uint32_t) fl | (uint32_t) O_NONBLOCK);
}

#endif //TPCONENATD_UTIL_H
