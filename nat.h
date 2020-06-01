//
// Created by root on 6/1/20.
//

#ifndef TPCONENATD_NAT_H
#define TPCONENATD_NAT_H

#include "inbound.h"
#include "dgram_read.h"
#include <cstdint>
#include <map>
#include <queue>

class outbound;

typedef struct {
    uint16_t tproxy_port = 1010;
    uint16_t min_port = 10240;
    uint16_t max_port = 65535;
    uint32_t nat_ip = 0;
    uint32_t new_timeout = 10;
    uint32_t est_timeout = 15;
    uint16_t session_per_src = 65535;
    uint8_t nat_type = 1;
}nat_config_t;

class nat {
private:
    friend class tproxy;
    friend class inbound;
    friend class outbound;
    epoll_util *ep{};
    nat_config_t config;
    std::map<std::pair<uint32_t, uint16_t>, outbound*> outbound_map;

    uint16_t get_port();

    outbound *get_outbound(std::pair<uint32_t, uint16_t> int_tuple);

    std::queue<uint16_t> port_queue;
public:
    explicit nat(nat_config_t config);

    void run();
};


#endif //TPCONENATD_NAT_H
