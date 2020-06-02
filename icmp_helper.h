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

    static bool recv(ep_param_t *param);

public:
    ~icmp_helper();

    explicit icmp_helper(nat *n) : n(n) {};

    static void reply_ttl_exceed(dgram_data_t *dgram_data);

    void init();
};


#endif //TPCONENATD_ICMP_HELPER_H
