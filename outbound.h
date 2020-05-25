//
// Created by claw6148 on 5/31/20.
//

#ifndef TPCONENATD_OUTBOUND_H
#define TPCONENATD_OUTBOUND_H

#include "watchdog.h"
#include "inbound.h"
#include "dgram_read.h"
#include "nat.h"
#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <netinet/in.h>

class inbound;

class nat;

class outbound {
private:
    friend class inbound;

    friend class icmp_helper;

    bool done = false;
    time_t create_time = 0;
    nat *n;
    uint16_t port;
    uint64_t rx = 0;
    uint64_t tx = 0;
    uint8_t ttl = 0;
    uint8_t tos = 0;
    std::pair<uint32_t, uint16_t> int_tuple;
    std::map<std::pair<uint32_t, uint16_t>, inbound *> inbound_map;
    std::set<std::pair<uint32_t, uint16_t>> inbound_filter_set;

    watchdog *wd = nullptr;

    static void wd_cb(void *param);

    ep_param_t ep_param{};

    static bool dst_nat(ep_param_t *param);

public:
    outbound(nat *n, uint16_t port, std::pair<uint32_t, uint16_t> int_tuple) :
            n(n),
            port(port),
            int_tuple(std::move(int_tuple)) {};

    ~outbound();

    void src_nat(dgram_data_t *dgram_data);

    void init();
};

#endif //TPCONENATD_OUTBOUND_H
