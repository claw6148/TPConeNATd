//
// Created by root on 5/31/20.
//

#ifndef TPCONENATD_WATCHDOG_H
#define TPCONENATD_WATCHDOG_H


#include <sys/timerfd.h>
#include <unistd.h>
#include "epoll_helper.h"

typedef struct {
    void *wd;
    void *cb;
    void *param;
} wd_param_t;

typedef void(*wd_cb_t)(wd_param_t *);

class watchdog {
private:
    time_t last_feed = 0;
    epoll_helper *ep;
    wd_param_t wd_param{};
    ep_param_t ep_param{};

    static bool ep_cb(ep_param_t *param);

public:
    watchdog(epoll_helper *ep, void *cb, void *param);

    void feed(time_t timeout);

    void feed(time_t timeout, bool force);


    ~watchdog();

};

#endif //TPCONENATD_WATCHDOG_H
