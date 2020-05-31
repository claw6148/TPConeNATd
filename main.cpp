//
//  main.cpp
//  TPConeNATd
//
//  Created by claw6148 on 2020/5/25.
//  Copyright © 2020 claw6148. All rights reserved.
//


#include "nat.h"

int main() {
    nat_config_t config;
    nat n(config);
    n.run();
}


//#include <cstdio>
//#include <cstring>
//#include <map>
//#include <set>
//#include <exception>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <unistd.h>
//#include <sys/epoll.h>
//#include <fcntl.h>
//#include <netinet/ip_icmp.h>
//#include <netinet/ip.h>
//#include <netinet/udp.h>
//#include <syslog.h>
//#include <csignal>
//#include <cinttypes>
//#include <sys/timerfd.h>
//#include "epoll_util.h"
//#include "util.h"
//
//using namespace std;
//
//
//
//int run_as_daemon = 0;
//uint8_t log_level = LOG_INFO;
//
//char log_level_str[][8] = {
//        "EMERG",
//        "ALERT",
//        "CRIT",
//        "ERR",
//        "WARNING",
//        "NOTICE",
//        "INFO",
//        "DEBUG"
//};
//
//#define LOG(level, fmt, ...) do { \
//    if (level > log_level) break; \
//    if (run_as_daemon) \
//        syslog(level, "[%s] " fmt"\n", log_level_str[level], ##__VA_ARGS__); \
//    else \
//        fprintf(level <= LOG_ERR ? stderr : stdout, "[%s] " fmt"\n", log_level_str[level], ##__VA_ARGS__); \
//} while(0)
//
//#define PERROR(msg) LOG(LOG_CRIT, "line = %s, err = %d (%s)", msg, errno, strerror(errno))
//
//#define PRINT_DEC(x) LOG(LOG_INFO, #x" = %d", x)
//#define PRINT_IP(x) do { \
//    uint32_t y = ntohl(x); \
//    LOG(LOG_INFO, #x" = %d.%d.%d.%d", ((uint8_t*)&y)[3], ((uint8_t*)&y)[2], ((uint8_t*)&y)[1], ((uint8_t*)&y)[0]); \
//} while(0)
//
//#define UNUSED(x) (void)(x)
//
//typedef struct {
//    uint8_t ttl = 0;
//    uint8_t tos = 0;
//    int socket_fd = 0;
//    time_t timer_last_set_time = 0;
//    ep_param_t timer_ep_param{};
//    void *nat = nullptr;
//    pair<uint32_t, uint16_t> key;
//} nat_inbound_t;
//
//typedef struct {
//    time_t create_time = 0;
//    bool reply = false;
//    uint16_t nat_port = 0;
//    uint64_t rx = 0;
//    uint64_t tx = 0;
//    uint8_t ttl = 0;
//    uint8_t tos = 0;
//    ep_param_t socket_ep_param{};
//    time_t timer_last_set_time{};
//    ep_param_t timer_ep_param{};
//    //struct sockaddr_in src{};
//    map<pair<uint32_t, uint16_t>, nat_inbound_t> inbound_map;
//    set<pair<uint32_t, uint16_t>> inbound_filter_set;
//} nat_t;
//
//epoll_util ep;
//
//map<pair<uint32_t, uint16_t>, nat_t> src_nat_map;
//map<uint16_t, nat_t *> dst_nat_map;
//map<uint32_t, uint32_t> src_session_counter;
//
//uint16_t tproxy_port = 0;
//uint16_t min_port = 10240;
//uint16_t max_port = 65535;
//uint32_t nat_ip = 0;
//uint32_t new_timeout = 30;
//uint32_t est_timeout = 300;
//uint16_t session_per_src = 65535;
//uint8_t nat_type = 1;
//
//uint16_t update_check16(uint16_t check, uint16_t old_val, uint16_t new_val) {
//    uint32_t x = ((uint16_t) ~check & 0xffffu) + ((uint16_t) ~old_val & 0xffffu) + new_val;
//    x = (x >> 16u) + (x & 0xffffu);
//    return ~(x + (x >> 16u));
//}
//
//uint16_t update_check32(uint16_t check, uint32_t old_val, uint32_t new_val) {
//    check = update_check16(check, old_val >> 16u, new_val >> 16u);
//    check = update_check16(check, old_val & 0xffffu, new_val & 0xffffu);
//    return check;
//}
//
//
//
//void icmp_recv(int fd,
//               struct sockaddr_in *src,
//               struct sockaddr_in *dst,
//               uint8_t ttl,
//               uint8_t tos,
//               void *data,
//               ssize_t data_len,
//               void *param
//) {
//    UNUSED(fd);
//    UNUSED(src);
//    UNUSED(dst);
//    UNUSED(ttl);
//    UNUSED(tos);
//    UNUSED(param);
//#define RANGE_CHECK(x, y) FUCK((uint8_t *) (x) - (uint8_t *)data + (y) > data_len)
//    auto *ip = (struct iphdr *) data;
//    RANGE_CHECK(ip, (ip->ihl << 2u));
//    auto *icmp = (struct icmphdr *) ((uint8_t *) ip + ((uint16_t) ip->ihl << 2u));
//    RANGE_CHECK(icmp, sizeof(struct icmphdr));
//    if (icmp->type != 11 || icmp->code != 0) return;
//    auto *ip_inner = (struct iphdr *) ((uint8_t *) icmp + sizeof(struct icmphdr));
//    RANGE_CHECK(ip_inner, sizeof(struct iphdr));
//    if (ip_inner->protocol != IPPROTO_UDP) return;
//    auto *udp_inner = (struct udphdr *) ((uint8_t *) ip_inner + ((uint16_t) ip_inner->ihl << 2u));
//    RANGE_CHECK(udp_inner, sizeof(struct udphdr));
//    auto it = dst_nat_map.find(udp_inner->source);
//    if (it == dst_nat_map.end()) return;
//    nat_t *nat = (*it).second;
//
//    ip->check = update_check32(ip->check, ip->daddr, nat->src.sin_addr.s_addr);
//    ip->daddr = nat->src.sin_addr.s_addr;
//    ip_inner->check = update_check32(ip_inner->check, ip_inner->saddr, nat->src.sin_addr.s_addr);
//    ip_inner->saddr = nat->src.sin_addr.s_addr;
//    udp_inner->check = update_check16(udp_inner->check, udp_inner->source, nat->src.sin_port);
//    udp_inner->source = nat->src.sin_port;
//
//    FUCK((sendto(fd, data, data_len, 0, (struct sockaddr *) &nat->src, sizeof(struct sockaddr_in))) < 0 &&
//         errno != EAGAIN);
//}
//
//void icmp_reply_ttl_exceed(int fd,
//                           struct sockaddr_in *src,
//                           struct sockaddr_in *dst,
//                           uint8_t ttl,
//                           uint8_t tos,
//                           void *data,
//                           ssize_t data_len,
//                           void *param
//) {
//    UNUSED(data);
//    UNUSED(data_len);
//    UNUSED(param);
//    static uint8_t buffer[sizeof(struct icmphdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + 8]{};
//    int tmp_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
//    set_nonblock(fd);
//    int opt = 1;
//    FUCK(setsockopt(tmp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0);
//
//    auto *icmp = (struct icmphdr *) buffer;
//    icmp->type = 11;
//    icmp->code = 0;
//    icmp->checksum = htons(0xa149);
//    icmp->checksum = update_check32(icmp->checksum, 0, src->sin_addr.s_addr);
//    icmp->checksum = update_check32(icmp->checksum, 0, dst->sin_addr.s_addr);
//
//    auto *ip_inner = (struct iphdr *) ((uint8_t *) icmp + sizeof(struct icmphdr));
//    ip_inner->saddr = src->sin_addr.s_addr;
//    ip_inner->daddr = dst->sin_addr.s_addr;
//    ip_inner->version = 4;
//    ip_inner->ihl = sizeof(struct iphdr) >> 2u;
//    ip_inner->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr));
//    ip_inner->ttl = ++ttl;
//    ip_inner->tos = tos;
//    ip_inner->protocol = IPPROTO_UDP;
//    ip_inner->check = htons(0xb9d2);
//    ip_inner->check = update_check32(ip_inner->check, 0, src->sin_addr.s_addr);
//    ip_inner->check = update_check32(ip_inner->check, 0, dst->sin_addr.s_addr);
//    icmp->checksum = update_check16(icmp->checksum, htons(0xb9d2), ip_inner->check);
//
//    auto *udp_inner = (struct udphdr *) ((uint8_t *) ip_inner + ((uint16_t) ip_inner->ihl << 2u));
//    udp_inner->len = htons(8);
//    udp_inner->check = htons(0x53ae);
//    udp_inner->source = src->sin_port;
//    udp_inner->dest = dst->sin_port;
//    udp_inner->check = update_check16(udp_inner->check, 0, src->sin_port);
//    udp_inner->check = update_check16(udp_inner->check, 0, dst->sin_port);
//    icmp->checksum = update_check16(icmp->checksum, 0, src->sin_port);
//    icmp->checksum = update_check16(icmp->checksum, 0, dst->sin_port);
//    icmp->checksum = update_check16(icmp->checksum, htons(0x53ae), udp_inner->check);
//
//    ssize_t send_len = sendto(tmp_fd, buffer, sizeof(buffer), 0, (struct sockaddr *) src,
//                              sizeof(struct sockaddr_in));
//    close(tmp_fd);
//    FUCK(send_len < 0 && errno != EAGAIN);
//}
//
//int bind_port(uint16_t port_net) {
//    int fd = socket(AF_INET, SOCK_DGRAM, 0);
//    if (fd < 0) return 0;
//    struct sockaddr_in serv{};
//    serv.sin_family = AF_INET;
//    serv.sin_port = port_net;
//    serv.sin_addr.s_addr = nat_ip;
//    int opt = 1;
//    FUCK(setsockopt(fd, IPPROTO_IP, IP_RECVTTL, &opt, sizeof(opt)) < 0);
//    FUCK(setsockopt(fd, IPPROTO_IP, IP_RECVTOS, &opt, sizeof(opt)) < 0);
//    set_nonblock(fd);
//    if (bind(fd, (const sockaddr *) &serv, sizeof(serv)) < 0) {
//        close(fd);
//        fd = 0;
//    }
//    return fd;
//}
//
//void set_timeout(int fd, time_t sec, time_t *last_set_time) {
//    time_t now = time(nullptr);
//    if (now == *last_set_time) return;
//    struct itimerspec timer{};
//    timer.it_interval.tv_sec = 0;
//    timer.it_value.tv_sec = sec;
//    timerfd_settime(fd, 0, &timer, nullptr);
//    *last_set_time = now;
//}
//
//void close_inbound_socket(int fd, void *param) {
//    auto *inbound = (nat_inbound_t *) param;
//    if (inbound->socket_fd == 0) return;
//    close(inbound->socket_fd);
//    inbound->socket_fd = 0;
//    ep.del(inbound->timer_ep_param.fd);
//    close(inbound->timer_ep_param.fd);
//
//    if (fd != -1) {
//        auto *nat = (nat_t *) inbound->nat;
//        nat->inbound_map.erase(inbound->key);
//    }
//}
//
//void delete_nat(int fd, void *param) {
//    auto *nat = (nat_t *) param;
//    src_session_counter[nat->src.sin_addr.s_addr]--;
//    ep.del(nat->socket_ep_param.fd);
//    close(nat->socket_ep_param.fd);
//    dst_nat_map.erase(nat->nat_port);
//    for (auto &it : nat->inbound_map) {
//        close_inbound_socket(-1, &it.second);
//    }
//    ep.del(fd);
//    close(fd);
//
//    char src_ip_str[0x20];
//    inet_ntop(AF_INET, &nat->src.sin_addr, src_ip_str, sizeof(src_ip_str));
//    char nat_ip_str[0x20];
//    inet_ntop(AF_INET, &nat_ip, nat_ip_str, sizeof(nat_ip_str));
//    LOG(LOG_INFO, "del, src = %s:%d, tproxy = %s:%d, duration = %lu, tx = %"
//            PRIu64
//            ", rx = %"
//            PRIu64,
//        src_ip_str, ntohs(nat->src.sin_port),
//        nat_ip_str, ntohs(nat->nat_port),
//        time(nullptr) - nat->create_time,
//        nat->tx, nat->rx
//    );
//}
//
//void perform_dst_nat(int fd,
//                     struct sockaddr_in *src,
//                     struct sockaddr_in *dst,
//                     uint8_t ttl,
//                     uint8_t tos,
//                     void *data,
//                     ssize_t data_len,
//                     void *param
//) {
//    UNUSED(fd);
//    UNUSED(dst);
//    auto *nat = (nat_t *) param;
//    if (nat_type > 1) {
//        if (nat->inbound_filter_set.find(make_pair(src->sin_addr.s_addr, nat_type == 3 ? src->sin_port : 0)) ==
//            nat->inbound_filter_set.end()) {
//            return;
//        }
//    }
//
//    auto key = make_pair(src->sin_addr.s_addr, src->sin_port);
//    nat_inbound_t *inbound = &nat->inbound_map[key];
//    if (inbound->socket_fd == 0) {
//        int inbound_fd;
//        FUCK((inbound_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0);
//        int opt = 1;
//        FUCK(setsockopt(inbound_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0);
//        FUCK(setsockopt(inbound_fd, SOL_IP, IP_TRANSPARENT, &opt, sizeof(opt)) < 0);
//        FUCK(bind(inbound_fd, (struct sockaddr *) src, sizeof(struct sockaddr_in)) < 0);
//        inbound->socket_fd = inbound_fd;
//
//        inbound->nat = (void *) nat;
//        inbound->key = key;
//        inbound->timer_ep_param.fd = timerfd_create(CLOCK_MONOTONIC, 0);
////        inbound->timer_ep_param.is_timer = true;
//        FUCK(inbound->timer_ep_param.fd < 0);
//        inbound->timer_ep_param.cb = (void *) close_inbound_socket;
//        inbound->timer_ep_param.param = (void *) inbound;
//        ep.add(&inbound->timer_ep_param);
//    }
//
//    nat->reply = true;
//    set_timeout(nat->timer_ep_param.fd, est_timeout, &nat->timer_last_set_time);
//    set_timeout(inbound->timer_ep_param.fd, est_timeout, &inbound->timer_last_set_time);
//
//    if (inbound->ttl != ttl) {
//        FUCK(setsockopt(inbound->socket_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0);
//        inbound->ttl = ttl;
//    }
//    if (inbound->tos != tos) {
//        FUCK(setsockopt(inbound->socket_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(ttl)) < 0);
//        inbound->tos = tos;
//    }
//    FUCK((sendto(inbound->socket_fd, data, data_len, 0, (struct sockaddr *) &nat->src,
//                 sizeof(struct sockaddr_in))) < 0 && errno != EAGAIN);
//    nat->rx += data_len;
//}
//
//void perform_src_nat(int fd,
//                     struct sockaddr_in *src,
//                     struct sockaddr_in *dst,
//                     uint8_t ttl,
//                     uint8_t tos,
//                     void *data,
//                     ssize_t data_len,
//                     void *param
//) {
//    UNUSED(fd);
//    UNUSED(param);
//    uint32_t src_addr_host = ntohl(src->sin_addr.s_addr);
//    uint32_t dst_addr_host = ntohl(dst->sin_addr.s_addr);
//    if ((src_addr_host == 0 || dst_addr_host == 0) ||
//        (src_addr_host >= 0xE0000000 || dst_addr_host >= 0xE0000000) ||
//        ((src_addr_host & 0xFFu) == 0xFF || (dst_addr_host & 0xFFu) == 0xFF)) {
//        return;
//    }
//    auto key = make_pair(src->sin_addr.s_addr, src->sin_port);
//    nat_t *nat = &src_nat_map[key];
//    if (nat->create_time == 0) {
//        if (src_session_counter[src->sin_addr.s_addr] + 1 > session_per_src) {
//            char src_ip_str[0x20];
//            inet_ntop(AF_INET, &src->sin_addr, src_ip_str, sizeof(src_ip_str));
//            LOG(LOG_ERR, "%s session limit reached!", src_ip_str);
//            src_nat_map.erase(key);
//            return;
//        }
//
//        set<uint16_t> visited_port;
//        for (int i = 0; i < (max_port - min_port); ++i) {
//            uint16_t nat_port;
//            do {
//                nat_port = min_port + (random() % (max_port - min_port + 1));
//            } while (visited_port.find(nat_port) != visited_port.end());
//            visited_port.insert(nat_port);
//            nat_port = htons(nat_port);
//            int bind_fd = bind_port(nat_port);
//            if (bind_fd) {
//                nat->socket_ep_param.fd = bind_fd;
//                nat->nat_port = nat_port;
//                break;
//            }
//        }
//        if (nat->socket_ep_param.fd == 0) {
//            LOG(LOG_ERR, "No ports available!");
//            src_nat_map.erase(key);
//            return;
//        }
//        src_session_counter[src->sin_addr.s_addr]++;
//
//        char src_ip_str[0x20];
//        inet_ntop(AF_INET, &src->sin_addr, src_ip_str, sizeof(src_ip_str));
//        char nat_ip_str[0x20];
//        inet_ntop(AF_INET, &nat_ip, nat_ip_str, sizeof(nat_ip_str));
//        LOG(LOG_INFO, "add, src = %s:%d, tproxy = %s:%d",
//            src_ip_str, ntohs(src->sin_port),
//            nat_ip_str, ntohs(nat->nat_port)
//        );
//
//        memcpy(&nat->src, src, sizeof(struct sockaddr_in));
//        dst_nat_map[nat->nat_port] = nat;
//
//        nat->socket_ep_param.cb = (void *) perform_dst_nat;
//        nat->socket_ep_param.param = (void *) nat;
//        ep.add(&nat->socket_ep_param);
//
//        nat->timer_ep_param.fd = timerfd_create(CLOCK_MONOTONIC, 0);
//        FUCK(nat->timer_ep_param.fd < 0);
//        nat->timer_ep_param.is_timer = true;
//        nat->timer_ep_param.cb = (void *) delete_nat;
//        nat->timer_ep_param.param = (void *) nat;
//        ep.add(&nat->timer_ep_param);
//
//        nat->create_time = time(nullptr);
//    }
//    set_timeout(nat->timer_ep_param.fd, nat->reply ? est_timeout : new_timeout, &nat->timer_last_set_time);
//
//    if (nat_type > 1) {
//        nat->inbound_filter_set.insert(make_pair(dst->sin_addr.s_addr, nat_type == 3 ? dst->sin_port : 0));
//    }
//    if (nat->ttl != ttl) {
//        FUCK(setsockopt(nat->socket_ep_param.fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0);
//        nat->ttl = ttl;
//    }
//    if (nat->tos != tos) {
//        FUCK(setsockopt(nat->socket_ep_param.fd, IPPROTO_IP, IP_TOS, &tos, sizeof(ttl)) < 0);
//        nat->tos = tos;
//    }
//    FUCK(sendto(nat->socket_ep_param.fd, data, data_len, 0, (struct sockaddr *) dst, sizeof(struct sockaddr_in)) < 0 &&
//         errno != EAGAIN);
//    nat->tx += data_len;
//}
//
//bool ep_recv(ep_param_t *ep_param) {
//    if (ep_param->is_timer) {
//        ((void (*)(int,
//                   void *
//        )) ep_param->cb)(ep_param->fd,
//                        ep_param->param);
//        return false;
//    } else {
//        struct sockaddr_in src{}, dst{};
//        static uint8_t data[0x10000];
//        struct iovec iov{
//                .iov_base = data,
//                .iov_len = sizeof(data),
//        };
//
//        uint8_t ctl[0x100];
//        struct msghdr msg{
//                .msg_name = &src,
//                .msg_namelen = sizeof(src),
//                .msg_iov = &iov,
//                .msg_iovlen = 1,
//                .msg_control = ctl,
//                .msg_controllen = sizeof(ctl),
//        };
//
//        ssize_t data_len = recvmsg(ep_param->fd, &msg, 0);
//        if (data_len < 0 && errno == EAGAIN) return false;
//        FUCK(data_len < 0);
//
//        uint16_t ttl = 0xFF;
//        uint16_t tos = 0x00;
//
//        for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
//            if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVORIGADDRS) {
//                memcpy(&dst, CMSG_DATA(cmsg), sizeof(struct sockaddr_in));
//                dst.sin_family = AF_INET;
//            }
//            if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
//                ttl = *CMSG_DATA(cmsg);
//            }
//            if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TOS) {
//                tos = *CMSG_DATA(cmsg);
//            }
//        }
//        void *cb = ep_param->cb;
//        if (!--ttl) cb = (void *) icmp_reply_ttl_exceed;
//        ((void (*)(int,
//                   struct sockaddr_in *,
//                   struct sockaddr_in *,
//                   uint8_t,
//                   uint8_t,
//                   void *,
//                   ssize_t,
//                   void *
//        )) cb)(ep_param->fd,
//               &src,
//               &dst,
//               ttl,
//               tos,
//               data,
//               data_len,
//               ep_param->param
//        );
//        return true;
//    }
//}
//
//void write_pid(char *pid_file) {
//    int fd;
//    size_t len;
//    struct flock lock{};
//    FUCK((fd = open(pid_file, (uint32_t) O_RDWR | (uint32_t) O_CREAT, 0666)) < 0);
//    lock.l_type = F_WRLCK;
//    lock.l_whence = SEEK_SET;
//    FUCK(fcntl(fd, F_SETLK, &lock) < 0);
//    char pid_str[12];
//    sprintf(pid_str, "%d\n", getpid());
//    len = strlen(pid_str);
//    FUCK(write(fd, pid_str, len) != len);
//}
//
//void usage_and_exit() {
//    printf("Usage:\n");
//    printf("  -h Help\n");
//    printf("  -p TPROXY port\n");
//    printf("  -i Minimum port (default: %d)\n", min_port);
//    printf("  -x Maximum port (default: %d)\n", max_port);
//    printf("  -s NAT ip (default: 0.0.0.0, depends on system)\n");
//    printf("  -n NEW(no reply) timeout (default: %d)\n", new_timeout);
//    printf("  -e ESTABLISHED timeout (default: %d)\n", est_timeout);
//    printf("  -o Session limit per source ip (default: %d)\n", session_per_src);
//    printf("  -t NAT type, 1. full-cone, 2. restricted-cone, 3. port-restricted-cone (default: %d)\n", nat_type);
//    printf("  -f PID file\n");
//    printf("  -l Log level (default: %d)\n", log_level);
//    printf("  -d Run as daemon, log to syslog\n");
//    exit(0);
//}
//
//void setup_tproxy() {
//    int fd;
//    FUCK((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0);
//    int opt = 1;
//    FUCK(setsockopt(fd, IPPROTO_IP, IP_RECVTTL, &opt, sizeof(opt)) < 0);
//    FUCK(setsockopt(fd, IPPROTO_IP, IP_RECVTOS, &opt, sizeof(opt)) < 0);
//    FUCK(setsockopt(fd, SOL_IP, IP_TRANSPARENT, &opt, sizeof(opt)) < 0);
//    FUCK(setsockopt(fd, SOL_IP, IP_RECVORIGDSTADDR, &opt, sizeof(opt)) < 0);
//    set_nonblock(fd);
//    struct sockaddr_in serv{};
//    serv.sin_family = AF_INET;
//    serv.sin_addr.s_addr = htonl(INADDR_ANY);
//    serv.sin_port = htons(tproxy_port);
//    FUCK(bind(fd, (const struct sockaddr *) &serv, sizeof(serv)) < 0);
//
//    auto *ep_param = (ep_param_t *) malloc(sizeof(ep_param_t));
//    ep_param->fd = fd;
//    ep_param->cb = (void *) perform_src_nat;
//    ep.add(ep_param, EPOLLIN | EPOLLET);
//}
//
//void setup_icmp() {
//    int fd;
//    FUCK((fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0);
//    int opt = 1;
//    FUCK(setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0);
//    set_nonblock(fd);
//    auto *ep_param = (ep_param_t *) malloc(sizeof(ep_param_t));
//    ep_param->fd = fd;
//    ep_param->cb = (void *) icmp_recv;
//    ep.add(ep_param, EPOLLIN | EPOLLET);
//}
//
//void bye(int) {
//    exit(0);
//}
//
//void adjust_sched() {
//    int p = sched_get_priority_max(SCHED_FIFO);
//    FUCK(sched_setscheduler(getpid(), SCHED_FIFO, (struct sched_param *) &p) < 0);
//}
//
//int main(int argc, char *argv[]) {
//    printf("TPConeNATd by claw6148\n\n");
//    if (argc == 1) usage_and_exit();
//    try {
//        char pid_file[0x100]{};
//        for (int ch; (ch = getopt(argc, argv, "hp:i:x:s:n:e:o:t:f:l:d")) != -1;) {
//            switch (ch) {
//                case 'h':
//                    usage_and_exit();
//                    break;
//                case 'p':
//                    tproxy_port = stol(optarg);
//                    break;
//                case 'i':
//                    min_port = stol(optarg);
//                    break;
//                case 'x':
//                    max_port = stol(optarg);
//                    break;
//                case 's':
//                    nat_ip = inet_addr(optarg);
//                    break;
//                case 'n':
//                    new_timeout = stol(optarg);
//                    break;
//                case 'e':
//                    est_timeout = stol(optarg);
//                    break;
//                case 'o':
//                    session_per_src = stol(optarg);
//                    break;
//                case 't':
//                    nat_type = stol(optarg);
//                    break;
//                case 'f':
//                    strcpy(pid_file, optarg);
//                    break;
//                case 'l':
//                    log_level = stol(optarg);
//                    break;
//                case 'd':
//                    run_as_daemon = 1;
//                    break;
//                default:
//                    break;
//            }
//        }
//        if (tproxy_port == 0) {
//            printf("Invalid TPROXY port!\n");
//            return 1;
//        }
//
//        if (min_port >= max_port) {
//            printf("Invalid port range!\n");
//            return 1;
//        }
//
//        if (nat_type < 1 || nat_type > 3) {
//            printf("Invalid NAT type!\n");
//            return 1;
//        }
//
//        PRINT_DEC(tproxy_port);
//        PRINT_DEC(min_port);
//        PRINT_DEC(max_port);
//        PRINT_IP(nat_ip);
//        PRINT_DEC(new_timeout);
//        PRINT_DEC(est_timeout);
//        PRINT_DEC(session_per_src);
//        PRINT_DEC(nat_type);
//        PRINT_DEC(log_level);
//
//        LOG(LOG_INFO, "Started!");
//
//        if (run_as_daemon) FUCK(daemon(1, 1) < 0);
//        if (strlen(pid_file)) write_pid(pid_file);
//
//        signal(SIGINT, bye);
//        signal(SIGTERM, bye);
//
//        adjust_sched();
//
//        srandom(time(nullptr));
//
//        setup_icmp();
//        setup_tproxy();
//
//        for (;;) {
//            ep.run();
//            auto *p = ep.get_ready_event();
//            while (p) {
//                try {
//                    while (ep_recv(p));
//                } catch (exception const &e) {
//                    if (!p->is_timer) {
//                        static uint8_t junk[0x10000];
//                        socklen_t sock_len = sizeof(struct sockaddr);
//                        while (recvfrom(p->fd, junk, sizeof(junk), 0, (struct sockaddr *) junk, &sock_len) > 0);
//                    }
//                    PERROR(e.what());
//                }
//            }
//        }
//    } catch (exception const &e) {
//        PERROR(e.what());
//    }
//    return 0;
//}
