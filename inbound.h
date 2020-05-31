//
// Created by root on 5/31/20.
//

#ifndef TPCONENATD_INBOUND_H
#define TPCONENATD_INBOUND_H

#include "watchdog.h"
#include "outbound.h"
#include "dgram_read.h"

class outbound;

class inbound {
private:
    outbound *out;
    int fd;
    std::pair<uint32_t, uint16_t> ext_tuple;
    uint8_t ttl{};
    uint8_t tos{};
    watchdog *wd{};

    bool killed = false;

    static void wd_cb(void *param);

public:
    inbound(outbound *out, std::pair<uint32_t, uint16_t> ext_tuple);

    ~inbound();

    void kill();

    void send(dgram_data_t *dgram_data);

};


#endif //TPCONENATD_INBOUND_H
