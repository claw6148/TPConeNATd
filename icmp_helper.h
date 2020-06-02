//
// Created by claw6148 on 6/1/20.
//

#ifndef TPCONENATD_ICMP_HELPER_H
#define TPCONENATD_ICMP_HELPER_H


#include "nat.h"

class icmp_helper {
    bool done = false;
    nat *n;
    ep_param_t ep_param{};

    // rfc777
    const uint16_t type_code[9] = {
            // dest. unreachable
            0x0300,
            0x0301,
            0x0302,
            0x0303,
            0x0304,
            // src. quench
            0x0400,
            // ttl exceed
            0x0b00,
            0x0b01,
            // param. problem
            0x0c00,
    };
    const uint16_t *type_code_end = type_code + (sizeof(type_code) / sizeof(uint16_t));

    static bool recv(ep_param_t *param);

public:
    ~icmp_helper();

    explicit icmp_helper(nat *n) : n(n) {};

    static void reply_ttl_exceed(dgram_data_t *dgram_data);

    void init();
};


#endif //TPCONENATD_ICMP_HELPER_H
