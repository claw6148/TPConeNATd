//
// Created by root on 5/31/20.
//

#ifndef TPCONENATD_TPROXY_H
#define TPCONENATD_TPROXY_H


#include <cstdint>
#include <map>
#include "epoll_util.h"
#include "util.h"
#include "outbound.h"
#include "nat.h"

class tproxy {
private:
    nat *n;
    ep_param_t ep_param{};
    static bool recv(ep_param_t *param);
public:
    explicit tproxy(nat *n);
};


#endif //TPCONENATD_TPROXY_H
