//
// Created by root on 5/31/20.
//

#include <netinet/in.h>
#include "inbound.h"

using namespace std;

void inbound::wd_cb(void *param) {
    delete (inbound *) param;
}

inbound::inbound(outbound *out, pair<uint32_t, uint16_t> ext_tuple) {
    this->out = out;
    this->ext_tuple = ext_tuple;
    this->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int opt = 1;
    setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(this->fd, SOL_IP, IP_TRANSPARENT, &opt, sizeof(opt));
    sockaddr_in src{};
    src.sin_family = AF_INET;
    src.sin_addr.s_addr = ext_tuple.first;
    src.sin_port = ext_tuple.second;
    bind(this->fd, (struct sockaddr *) &src, sizeof(struct sockaddr_in));
    this->wd = new watchdog(this->out->n->ep, (void *) inbound::wd_cb, this);
}

void inbound::kill() {
    this->killed = true;
}

inbound::~inbound() {
    delete this->wd;
    if (!this->killed) this->out->inbound_map.erase(this->ext_tuple);
    this->out->inbound_filter_set.erase(this->ext_tuple);
    close(this->fd);
}

void inbound::send(dgram_data_t *dgram_data) {
    if (this->ttl != dgram_data->ttl) {
        setsockopt(this->fd, IPPROTO_IP, IP_TTL, &dgram_data->ttl, sizeof(dgram_data->ttl));
        this->ttl = dgram_data->ttl;
    }
    if (this->tos != dgram_data->tos) {
        setsockopt(this->fd, IPPROTO_IP, IP_TOS, &dgram_data->tos, sizeof(dgram_data->tos));
        this->tos = dgram_data->tos;
    }
    sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = this->out->int_tuple.first;
    dst.sin_port = this->out->int_tuple.second;
    sendto(
            this->fd,
            dgram_data->data,
            dgram_data->data_len,
            0,
            (struct sockaddr *) &dst,
            sizeof(struct sockaddr_in)
    );
    wd->feed(this->out->n->config.est_timeout);
}
