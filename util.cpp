//
// Created by root on 6/2/20.
//
#include "util.h"

const char *log_level_str[] = {
        "EMERG",
        "ALERT",
        "CRIT",
        "ERR",
        "WARNING",
        "NOTICE",
        "INFO",
        "DEBUG"
};

bool run_as_daemon = false;
uint8_t log_level = LOG_INFO;

void set_nonblock(int fd) {
    int fl;
    fl = fcntl(fd, F_GETFL);
    THROW_IF_NEG(fl);
    THROW_IF_NEG(fcntl(fd, F_SETFL, (uint32_t) fl | (uint32_t) O_NONBLOCK));
}

uint16_t update_check16(uint16_t check, uint16_t old_val, uint16_t new_val) {
    uint32_t x = ((uint16_t) ~check & 0xffffu) + ((uint16_t) ~old_val & 0xffffu) + new_val;
    x = (x >> 16u) + (x & 0xffffu);
    return ~(x + (x >> 16u));
}

uint16_t update_check32(uint16_t check, uint32_t old_val, uint32_t new_val) {
    check = update_check16(check, old_val >> 16u, new_val >> 16u);
    check = update_check16(check, old_val & 0xffffu, new_val & 0xffffu);
    return check;
}
