//
// Created by claw6148 on 5/31/20.
//

#include "epoll_helper.h"
#include "util.h"

using namespace std;

epoll_helper::epoll_helper() {
    this->fd = epoll_create1(0);
    THROW_IF_NEG(this->fd);
}

void epoll_helper::add(ep_param_t *param) {
    add(param, EPOLLIN | EPOLLET);
}

void epoll_helper::add(ep_param_t *param, uint32_t event) {
    struct epoll_event ep_ev{};
    ep_ev.events = event;
    ep_ev.data.ptr = param;
    THROW_IF_NEG(epoll_ctl(this->fd, EPOLL_CTL_ADD, param->fd, &ep_ev));
}

void epoll_helper::del(int event_fd) {
    THROW_IF_NEG(epoll_ctl(this->fd, EPOLL_CTL_DEL, event_fd, nullptr));
}

void epoll_helper::run() {
    struct epoll_event ready_ev[MAX_EVENTS]{};
    for (;;) {
        int n = epoll_wait(this->fd, ready_ev, MAX_EVENTS, -1);
        THROW_IF_NEG(n);
        while (n--) {
            auto *p = (ep_param_t *) ready_ev[n].data.ptr;
            try {
                while (((ep_cb_t) p->cb)(p));
            } catch (runtime_error &e) {
                PERROR(e.what());
            }
        }
    }
}
