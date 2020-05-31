//
// Created by root on 6/1/20.
//

#ifndef TPCONENATD_DGRAM_READ_H
#define TPCONENATD_DGRAM_READ_H

#include <cstdint>
#include <sys/types.h>
#include <netinet/in.h>

typedef struct {
    struct sockaddr_in src;
    struct sockaddr_in dst;
    uint8_t ttl;
    uint8_t tos;
    ssize_t data_len;
    uint8_t data[0x10000];
}dgram_data_t;

bool dgram_read(int fd, dgram_data_t *dgram_data);

#endif //TPCONENATD_DGRAM_READ_H
