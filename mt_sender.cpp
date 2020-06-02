//
// Created by claw6148 on 6/3/20.
//

#include "mt_sender.h"
#include "util.h"
#include <thread>
#include <cstring>

using namespace std;

#define THREAD_STATE_READY      100
#define THREAD_STATE_BUSY       101
#define THREAD_STATE_KILL       102
#define THREAD_STATE_KILLED     103

mt_sender::mt_sender(uint16_t thread_count) {
    this->thread_count = thread_count;
    this->sender_thread = new sender_thread_t[this->thread_count];
    for (int t = 0; t < this->thread_count; ++t) {
        auto *tt = &this->sender_thread[t];
        sem_init(&tt->sem, 0, 0);
        thread(mt_sender::worker, this, t).detach();
        while (tt->state != THREAD_STATE_READY);
    }
}

mt_sender::~mt_sender() {
    for (int t = 0; t < this->thread_count; ++t) {
        auto *tt = &this->sender_thread[t];
        tt->state = THREAD_STATE_KILL;
        sem_post(&tt->sem);
        while (tt->state != THREAD_STATE_KILLED);
    }
    delete this->sender_thread;
}

void mt_sender::worker(mt_sender *_this, uint16_t t) {
    auto *tt = &_this->sender_thread[t];
    while (true) {
        tt->state = THREAD_STATE_READY;
        _this->ready_thread++;
        sem_wait(&tt->sem);
        if (tt->state == THREAD_STATE_KILL) {
            tt->state = THREAD_STATE_KILLED;
            break;
        }
        send_param_t *p = &tt->send_param;
        try {
            THROW_IF_NEG(sendto(
                    p->fd,
                    p->data,
                    p->data_len,
                    0,
                    (struct sockaddr *) &p->dst,
                    sizeof(struct sockaddr_in)
            ));
        } catch (runtime_error &e) {
            PERROR(e.what());
        }
    }
}

void mt_sender::send(int fd, void *data, size_t data_len, struct sockaddr_in *dst) {
    while (this->ready_thread == 0);
    this->ready_thread--;

    while (true) {
        auto *tt = &this->sender_thread[this->i];
        this->i = (this->i + 1) % thread_count;
        if (tt->state == THREAD_STATE_READY) {
            tt->state = THREAD_STATE_BUSY;
            tt->send_param.fd = fd;
            tt->send_param.dst = *dst;
            tt->send_param.data_len = data_len;
            memcpy(&tt->send_param.data, data, data_len);
            sem_post(&tt->sem);
            break;
        }
    }
}
