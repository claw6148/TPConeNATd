//
// Created by root on 5/31/20.
//

#include <cstdio>
#include "watchdog.h"
#include "util.h"

bool watchdog::ep_cb(ep_param_t *param) {
    uint64_t u;
    read(param->fd, &u, sizeof(u));
    ((wd_cb_t) ((wd_param_t *) param->param)->cb)((wd_param_t *) ((wd_param_t *) param->param)->param);
    return false;
}

watchdog::watchdog(epoll_util *ep, void *cb, void *param) {
    wd_param.wd = this;
    wd_param.cb = cb;
    wd_param.param = param;
    this->ep = ep;
    this->ep_param.fd = timerfd_create(CLOCK_MONOTONIC, 0);
    THROW_IF_NEG(this->ep_param.fd);
    this->ep_param.cb = (void *) watchdog::ep_cb;
    this->ep_param.param = &this->wd_param;
    ep->add(&this->ep_param);
}

watchdog::~watchdog() {
    this->ep->del(this->ep_param.fd);
    close(this->ep_param.fd);
}

void watchdog::feed(time_t timeout, bool force) {
    if (!force) {
        time_t now = time(nullptr);
        if (last_feed == now) return;
        last_feed = now;
    }
    struct itimerspec timer{};
    timer.it_interval.tv_sec = 0;
    timer.it_value.tv_sec = timeout;
    THROW_IF_NEG(timerfd_settime(this->ep_param.fd, 0, &timer, nullptr));
}

void watchdog::feed(time_t timeout) {
    feed(timeout, false);
}
