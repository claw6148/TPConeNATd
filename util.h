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
    throw exc(__FILE__":" + std::to_string(__LINE__) + " "#expr); \
} while(0)

#define THROW_IF(expr) THROW_IF_EX(expr, std::runtime_error)
#define THROW_IF_NEG(expr) THROW_IF(expr < 0)
#define THROW_RETRY_IF_NEG(expr) THROW_IF_EX(expr < 0, retry_exception)

void inline set_nonblock(int fd) {
    int fl;
    fl = fcntl(fd, F_GETFL);
    THROW_IF_NEG(fl);
    THROW_IF_NEG(fcntl(fd, F_SETFL, (uint32_t) fl | (uint32_t) O_NONBLOCK));
}

uint16_t inline update_check16(uint16_t check, uint16_t old_val, uint16_t new_val) {
    uint32_t x = ((uint16_t) ~check & 0xffffu) + ((uint16_t) ~old_val & 0xffffu) + new_val;
    x = (x >> 16u) + (x & 0xffffu);
    return ~(x + (x >> 16u));
}

uint16_t inline update_check32(uint16_t check, uint32_t old_val, uint32_t new_val) {
    check = update_check16(check, old_val >> 16u, new_val >> 16u);
    check = update_check16(check, old_val & 0xffffu, new_val & 0xffffu);
    return check;
}

#endif //TPCONENATD_UTIL_H
