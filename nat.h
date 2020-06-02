//
// Created by root on 6/1/20.
//

#ifndef TPCONENATD_NAT_H
#define TPCONENATD_NAT_H

#include "inbound.h"
#include "dgram_read.h"
#include "config.h"
#include <cstdint>
#include <map>
#include <queue>

class outbound;

class nat {
private:
    friend class tproxy;

    friend class inbound;

    friend class outbound;

    friend class icmp_helper;

    epoll_helper *ep{};
    config cfg;
    std::map<uint32_t, uint16_t> session_counter;
    std::map<std::pair<uint32_t, uint16_t>, outbound *> outbound_map;
    std::map<uint16_t, outbound *> port_outbound_map;

    uint16_t get_port();

    outbound *get_outbound(std::pair<uint32_t, uint16_t> int_tuple);

    outbound *get_outbound(uint16_t nat_port);

    std::queue<uint16_t> port_queue;
public:
    explicit nat(config cfg);

    void run();
};


#endif //TPCONENATD_NAT_H
