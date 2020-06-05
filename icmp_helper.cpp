//
// Created by claw6148 on 6/1/20.
//

#include "icmp_helper.h"
#include "util.h"
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <algorithm>

using namespace std;

icmp_helper::~icmp_helper() {
    if (this->done) {
        this->n->ep->del(this->ep_param.fd);
    }
    close(this->ep_param.fd);
}

void icmp_helper::init() {
    this->ep_param.fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    THROW_IF_NEG(this->ep_param.fd);
    set_nonblock(this->ep_param.fd);
    int opt = 1;
    THROW_IF_NEG(setsockopt(this->ep_param.fd, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)));
    this->ep_param.cb = (void *) icmp_helper::recv;
    this->ep_param.param = this;
    this->n->ep->add(&this->ep_param);
    this->done = true;
}

bool icmp_helper::recv(ep_param_t *param) {
    auto *_this = (icmp_helper *) param->param;
    dgram_data_t dgram_data;
    if (!dgram_read(param->fd, &dgram_data)) return false;
#define BOUND_CHECK(x, y) THROW_IF_NEG(dgram_data.data_len - ((uint8_t *) (x) - (uint8_t *)dgram_data.data + (y)))
    auto *ip = (struct iphdr *) dgram_data.data;
    BOUND_CHECK(ip, (ip->ihl << 2u));

    auto *icmp = (struct icmphdr *) ((uint8_t *) ip + ((uint16_t) ip->ihl << 2u));
    BOUND_CHECK(icmp, sizeof(struct icmphdr));

    if (find(
            _this->type_code,
            _this->type_code_end,
            ntohs(*(uint16_t *) &icmp->type)
    ) == _this->type_code_end) {
        return true;
    }

    auto *ip_inner = (struct iphdr *) ((uint8_t *) icmp + sizeof(struct icmphdr));
    BOUND_CHECK(ip_inner, sizeof(struct iphdr));
    if (ip_inner->protocol != IPPROTO_UDP) return true;

    auto *udp_inner = (struct udphdr *) ((uint8_t *) ip_inner + ((uint16_t) ip_inner->ihl << 2u));
    BOUND_CHECK(udp_inner, sizeof(struct udphdr));

    auto *out = _this->n->get_outbound(udp_inner->source);
    if (out == nullptr) return true;

    ip->check = update_check32(ip->check, ip->daddr, out->int_tuple.first);
    ip->daddr = out->int_tuple.first;

    ip_inner->check = update_check32(ip_inner->check, ip_inner->saddr, out->int_tuple.first);
    ip_inner->saddr = out->int_tuple.first;

    udp_inner->check = update_check16(udp_inner->check, udp_inner->source, out->int_tuple.second);
    udp_inner->source = out->int_tuple.second;

    struct sockaddr_in src{};
    src.sin_family = AF_INET;
    src.sin_addr.s_addr = out->int_tuple.first;
    src.sin_port = out->int_tuple.second;
    THROW_IF_NEG(sendto(
            _this->ep_param.fd,
            dgram_data.data,
            dgram_data.data_len,
            0,
            (struct sockaddr *) &src,
            sizeof(struct sockaddr_in)
    ));
    return true;
}

void icmp_helper::reply_ttl_exceed(dgram_data_t *dgram_data) {
    static uint8_t buffer[sizeof(struct icmphdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + 8]{};
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    THROW_IF_NEG(fd);
    int opt = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0) {
        close(fd);
        THROW_IF_NEG(ret);
    }

    const uint16_t init_icmp_check = htons(0xa149);
    auto *icmp = (struct icmphdr *) buffer;
    icmp->type = 11;
    icmp->code = 0;
    icmp->checksum = init_icmp_check;
    icmp->checksum = update_check32(icmp->checksum, 0, dgram_data->src.sin_addr.s_addr);
    icmp->checksum = update_check32(icmp->checksum, 0, dgram_data->dst.sin_addr.s_addr);

    const uint16_t init_ip_inner_check = htons(0xb9d2);
    auto *ip_inner = (struct iphdr *) ((uint8_t *) icmp + sizeof(struct icmphdr));
    ip_inner->saddr = dgram_data->src.sin_addr.s_addr;
    ip_inner->daddr = dgram_data->dst.sin_addr.s_addr;
    ip_inner->version = 4;
    ip_inner->ihl = sizeof(struct iphdr) >> 2u;
    ip_inner->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr));
    ip_inner->ttl = 1;
    ip_inner->tos = dgram_data->tos;
    ip_inner->protocol = IPPROTO_UDP;
    ip_inner->check = init_ip_inner_check;
    ip_inner->check = update_check32(ip_inner->check, 0, dgram_data->src.sin_addr.s_addr);
    ip_inner->check = update_check32(ip_inner->check, 0, dgram_data->dst.sin_addr.s_addr);
    icmp->checksum = update_check16(icmp->checksum, init_ip_inner_check, ip_inner->check);

    const uint16_t init_udp_inner_check = htons(0x53ae);
    auto *udp_inner = (struct udphdr *) ((uint8_t *) ip_inner + ((uint16_t) ip_inner->ihl << 2u));
    udp_inner->len = htons(8);
    udp_inner->check = init_udp_inner_check;
    udp_inner->source = dgram_data->src.sin_port;
    udp_inner->dest = dgram_data->dst.sin_port;
    udp_inner->check = update_check16(udp_inner->check, 0, dgram_data->src.sin_port);
    udp_inner->check = update_check16(udp_inner->check, 0, dgram_data->dst.sin_port);
    icmp->checksum = update_check16(icmp->checksum, 0, dgram_data->src.sin_port);
    icmp->checksum = update_check16(icmp->checksum, 0, dgram_data->dst.sin_port);
    icmp->checksum = update_check16(icmp->checksum, init_udp_inner_check, udp_inner->check);

    ssize_t send_len = sendto(
            fd,
            buffer,
            sizeof(buffer),
            0,
            (struct sockaddr *) &dgram_data->src,
            sizeof(struct sockaddr_in)
    );
    close(fd);
    THROW_IF_NEG(send_len);
}
