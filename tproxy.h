//
// Created by claw6148 on 5/31/20.
//

#ifndef TPCONENATD_TPROXY_H
#define TPCONENATD_TPROXY_H


#include <cstdint>
#include <map>
#include "epoll_helper.h"
#include "util.h"
#include "outbound.h"
#include "nat.h"

class tproxy {
private:
    bool done = false;
    nat *n;
    ep_param_t ep_param{};

    static bool recv(ep_param_t *param);

public:
    explicit tproxy(nat *n) : n(n) {};

    ~tproxy();

    void init();
};


#endif //TPCONENATD_TPROXY_H
