//
// Created by claw6148 on 6/3/20.
//

#ifndef TPCONENATD_MT_SENDER_H
#define TPCONENATD_MT_SENDER_H

#include <thread>
#include <atomic>
#include <semaphore.h>
#include "dgram_read.h"

typedef struct {
    int fd;
    struct sockaddr_in dst;
    uint8_t data[0x10000];
    ssize_t data_len;
} send_param_t;

typedef struct {
    uint8_t state;
    sem_t sem;
    send_param_t send_param;
} sender_thread_t;

class mt_sender {
private:
    uint16_t thread_count;

    sender_thread_t *sender_thread;

    int i{};

    std::atomic<uint16_t> ready_thread{};

    static void worker(mt_sender *_this, uint16_t t);

public:

    explicit mt_sender(uint16_t thread_count);

    ~mt_sender();

    void send(int fd, void *data, size_t data_len, sockaddr_in *dst);
};


#endif //TPCONENATD_MT_SENDER_H
