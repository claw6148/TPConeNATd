//
// Created by root on 6/1/20.
//

#ifndef TPCONENATD_DGRAM_READ_H
#define TPCONENATD_DGRAM_READ_H

#include <cstdint>
#include <sys/types.h>
#include <netinet/in.h>

typedef struct {
    struct sockaddr_in src{};
    struct sockaddr_in dst{};
    uint8_t ttl = 0xff;
    uint8_t tos = 0x00;
    ssize_t data_len = 0;
    uint8_t data[0x10000]{};
}dgram_data_t;

bool dgram_read(int fd, dgram_data_t *dgram_data);

#endif //TPCONENATD_DGRAM_READ_H
