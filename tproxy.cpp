//
// Created by root on 5/31/20.
//

#include <netinet/in.h>
#include <cerrno>
#include "tproxy.h"
#include "inbound.h"
#include "dgram_read.h"

using namespace std;

tproxy::tproxy(nat *n) {
    this->n = n;
    this->ep_param.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int opt = 1;
    THROW_IF_NEG(setsockopt(this->ep_param.fd, IPPROTO_IP, IP_RECVTTL, &opt, sizeof(opt)));
    THROW_IF_NEG(setsockopt(this->ep_param.fd, IPPROTO_IP, IP_RECVTOS, &opt, sizeof(opt)));
    THROW_IF_NEG(setsockopt(this->ep_param.fd, SOL_IP, IP_TRANSPARENT, &opt, sizeof(opt)));
    THROW_IF_NEG(setsockopt(this->ep_param.fd, SOL_IP, IP_RECVORIGDSTADDR, &opt, sizeof(opt)));
    set_nonblock(this->ep_param.fd);
    struct sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(n->config.tproxy_port);
    THROW_IF_NEG(bind(this->ep_param.fd, (const struct sockaddr *) &serv, sizeof(serv)));
    this->ep_param.cb = (void *) tproxy::recv;
    this->ep_param.param = this;
    this->n->ep->add(&this->ep_param);
}

bool tproxy::recv(ep_param_t *param) {
    auto *_this = (tproxy *) param->param;
    dgram_data_t dgram_data;
    if (!dgram_read(param->fd, &dgram_data)) return false;

    uint32_t src_addr_host = ntohl(dgram_data.src.sin_addr.s_addr);
    uint32_t dst_addr_host = ntohl(dgram_data.dst.sin_addr.s_addr);
    if ((src_addr_host == 0 || dst_addr_host == 0) ||
        (src_addr_host >= 0xE0000000 || dst_addr_host >= 0xE0000000) ||
        ((src_addr_host & 0xFFu) == 0xFF || (dst_addr_host & 0xFFu) == 0xFF)) {
        return true;
    }
    _this->n->get_outbound(
            make_pair(
                    dgram_data.src.sin_addr.s_addr,
                    dgram_data.src.sin_port
            )
    )->src_nat(&dgram_data);
    return true;
}
