//
// Created by claw6148 on 5/31/20.
//

#ifndef TPCONENATD_EPOLL_UTIL_H
#define TPCONENATD_EPOLL_UTIL_H


#include <sys/epoll.h>

#define MAX_EVENTS  0x10000


typedef struct {
    int fd;
    void *cb;
    void *param;
} ep_param_t;

typedef bool(*ep_cb_t)(ep_param_t*);

class epoll_util {
private:
    int fd;
public:
    epoll_util();

    void run();

    void del(int event_fd);

    void add(ep_param_t *param);
    void add(ep_param_t *param, uint32_t events);

};

#endif //TPCONENATD_EPOLL_UTIL_H
