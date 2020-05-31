//
// Created by root on 5/31/20.
//

#include <cassert>
#include <cstdio>
#include "epoll_util.h"

epoll_util::epoll_util() {
    this->fd = epoll_create1(0);
}

void epoll_util::add(ep_param_t *param) {
    add(param, EPOLLIN | EPOLLET);
}

void epoll_util::add(ep_param_t *param, uint32_t event) {
    struct epoll_event ep_ev{};
    ep_ev.events = event;
    ep_ev.data.ptr = param;
    epoll_ctl(this->fd, EPOLL_CTL_ADD, param->fd, &ep_ev);
}

void epoll_util::del(int event_fd) {
    epoll_ctl(this->fd, EPOLL_CTL_DEL, event_fd, nullptr);
}

void epoll_util::run() {
    struct epoll_event ready_ev[MAX_EVENTS]{};
    for (;;) {
        int n = epoll_wait(this->fd, ready_ev, MAX_EVENTS, -1);
        assert(n >= 0);
        while (n--) {
            auto *p = (ep_param_t *) ready_ev[n].data.ptr;
            while (((ep_cb_t) p->cb)(p));
        }
    }

}
