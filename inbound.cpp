//
// Created by root on 5/31/20.
//

#include <netinet/in.h>
#include <arpa/inet.h>
#include "inbound.h"
#include "util.h"

using namespace std;

void inbound::wd_cb(void *param) {
    delete (inbound *) param;
}

void inbound::init() {
    this->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    THROW_IF_NEG(this->fd);
    int opt = 1;
    THROW_IF_NEG(setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)));
    THROW_IF_NEG(setsockopt(this->fd, SOL_IP, IP_TRANSPARENT, &opt, sizeof(opt)));
    sockaddr_in src{};
    src.sin_family = AF_INET;
    src.sin_addr.s_addr = ext_tuple.first;
    src.sin_port = ext_tuple.second;
    THROW_IF_NEG(bind(this->fd, (struct sockaddr *) &src, sizeof(struct sockaddr_in)));
    this->wd = new watchdog(this->out->n->ep, (void *) inbound::wd_cb, this);
    this->done = true;
    this->create_time = time(nullptr);
    {
        string s;
        s += inet_ntoa(*reinterpret_cast<in_addr *>(&ext_tuple.first));
        s += ":";
        s += to_string(ntohs(ext_tuple.second));
        s += " -> ";
        s += inet_ntoa(*reinterpret_cast<in_addr *>(&this->out->n->config.nat_ip));
        s += ":";
        s += to_string(this->out->port);
        s += " -> ";
        s += inet_ntoa(*reinterpret_cast<in_addr *>(&this->out->int_tuple.first));
        s += ":";
        s += to_string(ntohs(this->out->int_tuple.second));
        printf("in-add %s\n", s.c_str());
    }
}

inbound::~inbound() {
    if (this->done) {
        delete this->wd;
        this->out->inbound_filter_set.erase(this->ext_tuple);
        this->out->inbound_map.erase(this->ext_tuple);
        {
            string s;
            s += inet_ntoa(*reinterpret_cast<in_addr *>(&ext_tuple.first));
            s += ":";
            s += to_string(ntohs(ext_tuple.second));
            s += " -> ";
            s += inet_ntoa(*reinterpret_cast<in_addr *>(&this->out->n->config.nat_ip));
            s += ":";
            s += to_string(this->out->port);
            s += " -> ";
            s += inet_ntoa(*reinterpret_cast<in_addr *>(&this->out->int_tuple.first));
            s += ":";
            s += to_string(ntohs(this->out->int_tuple.second));
            printf("in-del %s duration = %ld\n",
                   s.c_str(),
                   time(nullptr) - this->create_time - this->out->n->config.est_timeout
            );
        }
        if (this->out->inbound_map.empty()) delete this->out;
    }
    close(this->fd);
}

void inbound::send(dgram_data_t *dgram_data) {
    if (this->ttl != dgram_data->ttl) {
        THROW_IF_NEG(setsockopt(this->fd, IPPROTO_IP, IP_TTL, &dgram_data->ttl, sizeof(dgram_data->ttl)));
        this->ttl = dgram_data->ttl;
    }
    if (this->tos != dgram_data->tos) {
        THROW_IF_NEG(setsockopt(this->fd, IPPROTO_IP, IP_TOS, &dgram_data->tos, sizeof(dgram_data->tos)));
        this->tos = dgram_data->tos;
    }
    sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = this->out->int_tuple.first;
    dst.sin_port = this->out->int_tuple.second;
    THROW_IF_NEG(sendto(
            this->fd,
            dgram_data->data,
            dgram_data->data_len,
            0,
            (struct sockaddr *) &dst,
            sizeof(struct sockaddr_in)
    ));
    wd->feed(this->out->n->config.est_timeout);
}
