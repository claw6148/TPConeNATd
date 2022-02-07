//
// Created by claw6148 on 6/2/20.
//

#ifndef TPCONENATD_CONFIG_H
#define TPCONENATD_CONFIG_H


#include <cstdint>

class config {
public:
    uint16_t tproxy_port = 100;
    uint16_t min_port = 10240;
    uint16_t max_port = 65535;
    uint32_t nat_ip = 0;
    uint32_t new_timeout = 30;
    uint32_t est_timeout = 300;
    uint16_t port_per_src = 65535;
    uint8_t nat_type = 1;
    uint16_t sender_thread = 1;
    uint32_t socket_mark = 0;

    void validate();

    void print();
};


#endif //TPCONENATD_CONFIG_H
