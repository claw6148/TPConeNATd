//
// Created by root on 5/31/20.
//

#include "inbound.h"
#include "outbound.h"
#include "util.h"
#include "dgram_read.h"
#include <string>
#include <arpa/inet.h>

using namespace std;

outbound::outbound(nat *n, uint16_t port, pair<uint32_t, uint16_t> int_tuple) {
    this->n = n;
    this->port = port;
    this->int_tuple = int_tuple;
    this->wd = new watchdog(n->ep, (void *) outbound::wd_cb, this);

    this->ep_param.fd = socket(AF_INET, SOCK_DGRAM, 0);
    this->ep_param.cb = (void *) outbound::dst_nat;
    this->ep_param.param = this;

    int opt = 1;
    setsockopt(this->ep_param.fd, IPPROTO_IP, IP_RECVTTL, &opt, sizeof(opt));
    setsockopt(this->ep_param.fd, IPPROTO_IP, IP_RECVTOS, &opt, sizeof(opt));
    set_nonblock(this->ep_param.fd);

    sockaddr_in nat{};
    nat.sin_family = AF_INET;
    nat.sin_addr.s_addr = this->n->config.nat_ip;
    nat.sin_port = htons(this->port);
    bind(this->ep_param.fd, (const sockaddr *) &nat, sizeof(struct sockaddr_in));

    n->ep->add(&this->ep_param);

    {
        string s;
        s += inet_ntoa(*reinterpret_cast<in_addr *>(&int_tuple.first));
        s += ":";
        s += to_string(ntohs(int_tuple.second));
        s += " <-> ";
        s += inet_ntoa(*reinterpret_cast<in_addr *>(&this->n->config.nat_ip));
        s += ":";
        s += to_string(this->port);
        printf("out-add %s\n", s.c_str());
    }
}

outbound::~outbound() {
    this->n->outbound_map.erase(this->int_tuple);
    this->n->ep->del(this->ep_param.fd);
    close(this->ep_param.fd);
    {
        string s;
        s += inet_ntoa(*reinterpret_cast<in_addr *>(&int_tuple.first));
        s += ":";
        s += to_string(ntohs(int_tuple.second));
        s += " <-> ";
        s += inet_ntoa(*reinterpret_cast<in_addr *>(&this->n->config.nat_ip));
        s += ":";
        s += to_string(this->port);
        printf("out-del %s\n", s.c_str());
    }
}

void outbound::wd_cb(void *param) {
    delete (outbound *) param;
}

bool outbound::dst_nat(ep_param_t *param) {
    auto *_this = (outbound *) param->param;
    dgram_data_t dgram_data;
    if (!dgram_read(param->fd, &dgram_data)) {
        if (errno == EAGAIN) return false;
        else {
            perror("?");
            return false;
        }
    }

    if (_this->n->config.nat_type > 1) {
        if (_this->inbound_filter_set.find(
                make_pair(
                        dgram_data.src.sin_addr.s_addr,
                        _this->n->config.nat_type == 2 ? 0 : dgram_data.src.sin_port
                )
        ) == _this->inbound_filter_set.end()) {
            return true;
        }
    }

    inbound *in;
    auto key = make_pair(dgram_data.src.sin_addr.s_addr, dgram_data.src.sin_port);
    auto it = _this->inbound_map.find(key);
    if (it == _this->inbound_map.end()) {
        in = new inbound(_this, key);
        _this->inbound_map[key] = in;
    } else {
        in = (*it).second;
    }
    in->send(&dgram_data);
    if (_this->wd) {
        delete _this->wd;
        _this->wd = nullptr;
    }
    _this->rx += dgram_data.data_len;
    return true;
}

void outbound::src_nat(dgram_data_t *dgram_data) {
    if (this->ttl != dgram_data->ttl) {
        setsockopt(this->ep_param.fd, IPPROTO_IP, IP_TTL, &dgram_data->ttl, sizeof(dgram_data->ttl));
        this->ttl = dgram_data->ttl;
    }
    if (this->tos != dgram_data->tos) {
        setsockopt(this->ep_param.fd, IPPROTO_IP, IP_TOS, &dgram_data->tos, sizeof(dgram_data->tos));
        this->tos = dgram_data->tos;
    }
    if (this->n->config.nat_type > 1) {
        this->inbound_filter_set.insert(
                make_pair(
                        dgram_data->dst.sin_addr.s_addr,
                        this->n->config.nat_type == 2 ? 0 : dgram_data->dst.sin_port
                )
        );
    }
    sendto(
            this->ep_param.fd,
            dgram_data->data,
            dgram_data->data_len,
            0,
            (struct sockaddr *) &dgram_data->dst,
            sizeof(struct sockaddr_in)
    );
    if (this->wd) this->wd->feed(this->n->config.new_timeout);
    this->tx += dgram_data->data_len;
}
