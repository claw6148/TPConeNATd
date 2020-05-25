//
// Created by claw6148 on 6/1/20.
//

#include <netinet/in.h>
#include <cstring>
#include <cerrno>
#include "dgram_read.h"
#include "util.h"

#ifndef IP_ORIGADDRS
#define IP_ORIGADDRS      20
#endif
#ifndef IP_RECVORIGADDRS
#define IP_RECVORIGADDRS  IP_ORIGADDRS
#endif

bool dgram_read(int fd, dgram_data_t *dgram_data) {
    struct iovec iov{
            .iov_base = dgram_data->data,
            .iov_len = sizeof(dgram_data->data),
    };

    uint8_t ctl[0x100];
    struct msghdr msg{
            .msg_name = &dgram_data->src,
            .msg_namelen = sizeof(dgram_data->src),
            .msg_iov = &iov,
            .msg_iovlen = 1,
            .msg_control = ctl,
            .msg_controllen = sizeof(ctl),
    };
    dgram_data->data_len = recvmsg(fd, &msg, 0);
    if (dgram_data->data_len < 0 && errno == EAGAIN) return false;
    THROW_IF_NEG(dgram_data->data_len);

    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVORIGADDRS) {
            memcpy(&dgram_data->dst, CMSG_DATA(cmsg), sizeof(struct sockaddr_in));
            dgram_data->dst.sin_family = AF_INET;
        }
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
            dgram_data->ttl = *CMSG_DATA(cmsg);
        }
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TOS) {
            dgram_data->tos = *CMSG_DATA(cmsg);
        }
    }
    return true;
}
