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
    bool done = false;
    time_t create_time = 0;
    outbound *out;
    int fd = 0;
    std::pair<uint32_t, uint16_t> ext_tuple;
    uint8_t ttl{};
    uint8_t tos{};
    watchdog *wd{};

    static void wd_cb(void *param);

public:
    inbound(outbound *out, std::pair<uint32_t, uint16_t> ext_tuple);

    ~inbound();

    void send(dgram_data_t *dgram_data);

    void init();
};


#endif //TPCONENATD_INBOUND_H
