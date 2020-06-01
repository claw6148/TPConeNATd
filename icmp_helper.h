//
// Created by root on 6/1/20.
//

#ifndef TPCONENATD_ICMP_HELPER_H
#define TPCONENATD_ICMP_HELPER_H


#include "nat.h"

class icmp_helper {
    nat *n;
    ep_param_t ep_param{};

    static bool recv(ep_param_t *param);

public:
    explicit icmp_helper(nat *n);

    static void reply_ttl_exceed(dgram_data_t *dgram_data);
};


#endif //TPCONENATD_ICMP_HELPER_H
