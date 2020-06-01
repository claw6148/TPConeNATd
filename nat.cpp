//
// Created by root on 6/1/20.
//

#include <netinet/in.h>
#include <random>
#include <set>
#include "nat.h"
#include "tproxy.h"
#include "icmp_helper.h"

using namespace std;

nat::nat(nat_config_t config) {
    this->config = config;

    set<uint16_t> visited_port;
    uint16_t total_port = this->config.max_port - this->config.min_port;
    for (int i = 0; i < total_port; ++i) {
        uint16_t nat_port;
        do {
            nat_port = this->config.min_port + (random() % (total_port + 1));
        } while (visited_port.find(nat_port) != visited_port.end());
        visited_port.insert(nat_port);
        this->port_queue.push(nat_port);
    }
}

uint16_t nat::get_port() {
    if (port_queue.empty()) return 0;
    uint16_t port = this->port_queue.front();
    this->port_queue.pop();
    this->port_queue.push(port);
    return port;
}

outbound *nat::get_outbound(pair<uint32_t, uint16_t> int_tuple) {
    outbound *out;
    auto it = this->outbound_map.find(int_tuple);
    if (it == this->outbound_map.end()) {
        uint16_t nat_port;
        while (true) {
            nat_port = ntohs(this->get_port());
            out = new outbound(this, nat_port, int_tuple);
            try {
                out->init();
                break;
            } catch (retry_exception &e) {
                delete out;
                perror(e.what());
            } catch (runtime_error &e) {
                delete out;
                throw e;
            }
        }
        this->outbound_map[int_tuple] = out;
        this->port_outbound_map[nat_port] = out;
    } else {
        out = (*it).second;
    }
    return out;
}

outbound *nat::get_outbound(uint16_t nat_port) {
    auto it = this->port_outbound_map.find(nat_port);
    return it == this->port_outbound_map.end() ? nullptr : (*it).second;
}

void nat::run() {
    this->ep = new epoll_helper();
    new icmp_helper(this);
    new tproxy(this);
    this->ep->run();
}
