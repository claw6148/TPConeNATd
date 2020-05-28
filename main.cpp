//
//  main.cpp
//  TPConeNATd
//
//  Created by claw6148 on 2020/5/25.
//  Copyright © 2020 claw6148. All rights reserved.
//

#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <exception>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <syslog.h>
#include <csignal>
#include <cinttypes>

using namespace std;

#ifndef IP_ORIGADDRS
#define IP_ORIGADDRS      20
#endif
#ifndef IP_RECVORIGADDRS
#define IP_RECVORIGADDRS  IP_ORIGADDRS
#endif

#define FUCK(expr) do { \
    if (!(expr)) break; \
    char str[0x20]{}; \
    snprintf(str, sizeof(str) - 1, "%d", __LINE__); \
    throw runtime_error(str); \
} while(0)

int run_as_daemon = 0;
uint8_t log_level = LOG_INFO;

char log_level_str[][8] = {
        "EMERG",
        "ALERT",
        "CRIT",
        "ERR",
        "WARNING",
        "NOTICE",
        "INFO",
        "DEBUG"
};

#define LOG(level, fmt, ...) do { \
    if (level > log_level) break; \
    if (run_as_daemon) \
        syslog(level, "[%s] " fmt"\n", log_level_str[level], ##__VA_ARGS__); \
    else \
        fprintf(level <= LOG_ERR ? stderr : stdout, "[%s] " fmt"\n", log_level_str[level], ##__VA_ARGS__); \
} while(0)

#define PERROR(msg) LOG(LOG_CRIT, "line = %s, err = %s", msg, strerror(errno))

#define PRINT_DEC(x) LOG(LOG_INFO, #x" = %d", x)
#define PRINT_IP(x) do { \
    uint32_t y = ntohl(x); \
    LOG(LOG_INFO, #x" = %d.%d.%d.%d", ((uint8_t*)&y)[3], ((uint8_t*)&y)[2], ((uint8_t*)&y)[1], ((uint8_t*)&y)[0]); \
} while(0)

#define UNUSED(x) (void)(x)

typedef struct {
    int fd;
    void *cb;
} ep_data_t;

typedef struct {
    time_t create_time;
    time_t active_time;
    bool reply;
    uint16_t nat_port;
    uint64_t rx;
    uint64_t tx;
    int fd;
    struct sockaddr_in src;
    ep_data_t ep_data;
} nat_item_t;

struct pair_hash {
    template<class T1, class T2>
    size_t operator()(const pair<T1, T2> &p) const {
        return hash<T1>{}(p.first) ^ hash<T2>{}(p.first);
    }
};

unordered_map<pair<uint32_t, uint16_t>, nat_item_t, pair_hash> src_nat_map;
unordered_map<int, nat_item_t *> dst_nat_map;
unordered_map<uint16_t, int> nat_port_fd;
unordered_map<int, unordered_set<pair<uint32_t, uint16_t>, pair_hash>> restricted_cone_set;
unordered_map<uint32_t, uint16_t> src_session_counter;

uint16_t tproxy_port = 0;
uint16_t min_port = 10240;
uint16_t max_port = 65535;
uint32_t nat_ip = 0;
uint32_t new_timeout = 30;
uint32_t est_timeout = 300;
uint32_t clean_interval = 10;
uint16_t session_per_src = 65535;
uint8_t nat_type = 1;
bool no_inbound_refresh = false;

int ep_fd;

void ep_add(int _ep_fd, ep_data_t *ep_data) {
    struct epoll_event ep_ev{};
    ep_ev.events = EPOLLIN;
    ep_ev.data.ptr = ep_data;
    FUCK(epoll_ctl(_ep_fd, EPOLL_CTL_ADD, ep_data->fd, &ep_ev) < 0);
}

void ep_del(int _ep_fd, int fd) {
    FUCK(epoll_ctl(_ep_fd, EPOLL_CTL_DEL, fd, nullptr) < 0);
}

static uint16_t update_check16(uint16_t check, uint16_t old_val, uint16_t new_val) {
    uint32_t x = ((uint16_t) ~check & 0xffffu) + ((uint16_t) ~old_val & 0xffffu) + new_val;
    x = (x >> 16u) + (x & 0xffffu);
    return ~(x + (x >> 16u));
}

static uint16_t update_check32(uint16_t check, uint32_t old_val, uint32_t new_val) {
    check = update_check16(check, old_val >> 16u, new_val >> 16u);
    check = update_check16(check, old_val & 0xffffu, new_val & 0xffffu);
    return check;
}

void icmp_recv(int fd,
               struct sockaddr_in *src,
               struct sockaddr_in *dst,
               uint8_t ttl,
               uint8_t tos,
               void *data,
               ssize_t data_len
) {
    UNUSED(fd);
    UNUSED(src);
    UNUSED(dst);
    UNUSED(ttl);
    UNUSED(tos);
#define RANGE_CHECK(x, y) FUCK((uint8_t *) x - (uint8_t *)data + y > data_len)
    auto *ip = (struct iphdr *) data;
    RANGE_CHECK(ip, (ip->ihl << 2u));
    auto *icmp = (struct icmphdr *) ((uint8_t *) ip + ((uint16_t) ip->ihl << 2u));
    RANGE_CHECK(icmp, sizeof(struct icmphdr));
    if (icmp->type != 11 || icmp->code != 0) return;
    auto *ip_inner = (struct iphdr *) ((uint8_t *) icmp + sizeof(struct icmphdr));
    RANGE_CHECK(ip_inner, sizeof(struct iphdr));
    if (ip_inner->protocol != IPPROTO_UDP) return;
    auto *udp_inner = (struct udphdr *) ((uint8_t *) ip_inner + ((uint16_t) ip_inner->ihl << 2u));
    RANGE_CHECK(udp_inner, sizeof(struct udphdr));
    auto it = nat_port_fd.find(udp_inner->source);
    if (it == nat_port_fd.end()) return;
    nat_item_t *nat_item = dst_nat_map[(*it).second];

    ip->check = update_check32(ip->check, ip->daddr, nat_item->src.sin_addr.s_addr);
    ip->daddr = nat_item->src.sin_addr.s_addr;
    ip_inner->check = update_check32(ip_inner->check, ip_inner->saddr, nat_item->src.sin_addr.s_addr);
    ip_inner->saddr = nat_item->src.sin_addr.s_addr;
    udp_inner->check = update_check16(udp_inner->check, udp_inner->source, nat_item->src.sin_port);
    udp_inner->source = nat_item->src.sin_port;

    FUCK(sendto(fd, data, data_len, 0, (struct sockaddr *) &nat_item->src,
                sizeof(struct sockaddr_in)) < 0);
}

static int bind_port(uint16_t port_net) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return 0;
    struct sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = port_net;
    serv.sin_addr.s_addr = nat_ip;
    int opt = 1;
    FUCK(setsockopt(fd, IPPROTO_IP, IP_RECVTTL, &opt, sizeof(opt)) < 0);
    FUCK(setsockopt(fd, IPPROTO_IP, IP_RECVTOS, &opt, sizeof(opt)) < 0);
    if (bind(fd, (const sockaddr *) &serv, sizeof(serv)) < 0) {
        close(fd);
        fd = 0;
    }
    return fd;
}

void perform_dst_nat(int fd,
                     struct sockaddr_in *src,
                     struct sockaddr_in *dst,
                     uint8_t ttl,
                     uint8_t tos,
                     void *data,
                     ssize_t data_len
) {
    UNUSED(dst);
    nat_item_t *nat_item = dst_nat_map[fd];
    if (nat_type > 1 &&
        restricted_cone_set[fd].find(make_pair(src->sin_addr.s_addr, nat_type > 2 ? src->sin_port : 0)) ==
        restricted_cone_set[fd].end()) {
        return;
    }
    nat_item->reply = true;
    if (!no_inbound_refresh) nat_item->active_time = time(nullptr);

    int tmp_fd = 0;
    FUCK((tmp_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0);
    int opt = 1;
    FUCK(setsockopt(tmp_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0);
    FUCK(setsockopt(tmp_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(ttl)) < 0);
    FUCK(setsockopt(tmp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0);
    FUCK(setsockopt(tmp_fd, SOL_IP, IP_TRANSPARENT, &opt, sizeof(opt)) < 0);
    FUCK(bind(tmp_fd, (struct sockaddr *) src, sizeof(struct sockaddr_in)) < 0);
    FUCK(sendto(tmp_fd, data, data_len, 0, (struct sockaddr *) &nat_item->src, sizeof(struct sockaddr_in)) < 0);
    nat_item->rx += data_len;
    close(tmp_fd);
}

void perform_src_nat(int fd,
                     struct sockaddr_in *src,
                     struct sockaddr_in *dst,
                     uint8_t ttl,
                     uint8_t tos,
                     void *data,
                     ssize_t data_len
) {
    UNUSED(fd);
    uint32_t src_addr_host = ntohl(src->sin_addr.s_addr);
    uint32_t dst_addr_host = ntohl(dst->sin_addr.s_addr);
    if ((src_addr_host == 0 || dst_addr_host == 0) ||
        (src_addr_host >= 0xE0000000 || dst_addr_host >= 0xE0000000) ||
        ((src_addr_host & 0xFFu) == 0xFF || (dst_addr_host & 0xFFu) == 0xFF)) {
        return;
    }
    auto key = make_pair(src->sin_addr.s_addr, src->sin_port);
    nat_item_t *nat_item;
    auto it = src_nat_map.find(key);
    if (it == src_nat_map.end()) {
        nat_item = &src_nat_map[key];
        memset(nat_item, 0, sizeof(nat_item_t));
        if (src_session_counter[src->sin_addr.s_addr] + 1 > session_per_src) {
            char src_ip_str[0x20];
            inet_ntop(AF_INET, &src->sin_addr, src_ip_str, sizeof(src_ip_str));
            LOG(LOG_ERR, "%s session limit reached!", src_ip_str);
            return;
        }

        set<uint16_t> visited_port;
        for (int i = 0; i < (max_port - min_port); ++i) {
            uint16_t nat_port;
            do {
                nat_port = min_port + (random() % (max_port - min_port + 1));
            } while (visited_port.find(nat_port) != visited_port.end());
            visited_port.insert(nat_port);
            nat_port = htons(nat_port);
            int bind_fd = bind_port(nat_port);
            if (bind_fd) {
                nat_item->fd = bind_fd;
                nat_item->nat_port = nat_port;
                break;
            }
        }
        if (nat_item->fd == 0) {
            LOG(LOG_ERR, "No ports available!");
            src_nat_map.erase(key);
            return;
        }
        src_session_counter[src->sin_addr.s_addr]++;

        char src_ip_str[0x20];
        inet_ntop(AF_INET, &src->sin_addr, src_ip_str, sizeof(src_ip_str));
        char nat_ip_str[0x20];
        inet_ntop(AF_INET, &nat_ip, nat_ip_str, sizeof(nat_ip_str));
        LOG(LOG_INFO, "add, src = %s:%d, nat = %s:%d",
            src_ip_str, ntohs(src->sin_port),
            nat_ip_str, ntohs(nat_item->nat_port)
        );

        memcpy(&nat_item->src, src, sizeof(struct sockaddr_in));
        dst_nat_map[nat_item->fd] = nat_item;
        nat_port_fd[nat_item->nat_port] = nat_item->fd;

        nat_item->ep_data.fd = nat_item->fd;
        nat_item->ep_data.cb = (void *) perform_dst_nat;
        ep_add(ep_fd, &nat_item->ep_data);

        nat_item->active_time = time(nullptr);
        nat_item->create_time = nat_item->active_time;
    } else {
        nat_item = &(*it).second;
        nat_item->active_time = time(nullptr);
    }
    if (nat_type > 1) {
        restricted_cone_set[nat_item->fd].insert(make_pair(dst->sin_addr.s_addr, nat_type > 2 ? dst->sin_port : 0));
    }
    FUCK(setsockopt(nat_item->fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0);
    FUCK(setsockopt(nat_item->fd, IPPROTO_IP, IP_TOS, &tos, sizeof(ttl)) < 0);
    FUCK(sendto(nat_item->fd, data, data_len, 0, (struct sockaddr *) dst, sizeof(struct sockaddr_in)) < 0);
    nat_item->tx += data_len;
}

void ep_recv(int fd, void *cb) {
    struct sockaddr_in src{}, dst{};

    static uint8_t data[0x10000];
    struct iovec iov{
            .iov_base = data,
            .iov_len = sizeof(data),
    };

    uint8_t ctl[0x100];
    struct msghdr msg{
            .msg_name = &src,
            .msg_namelen = sizeof(src),
            .msg_iov = &iov,
            .msg_iovlen = 1,
            .msg_control = ctl,
            .msg_controllen = sizeof(ctl),
    };

    ssize_t data_len;
    FUCK((data_len = recvmsg(fd, &msg, 0)) < 0);

    uint16_t ttl = 0xFF;
    uint16_t tos = 0x00;
    for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVORIGADDRS) {
            memcpy(&dst, CMSG_DATA(cmsg), sizeof(struct sockaddr_in));
            dst.sin_family = AF_INET;
        }
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
            ttl = *CMSG_DATA(cmsg);
        }
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TOS) {
            tos = *CMSG_DATA(cmsg);
        }
    }
    ((void (*)(int,
               struct sockaddr_in *,
               struct sockaddr_in *,
               uint8_t,
               uint8_t,
               void *,
               ssize_t
    )) cb)(fd,
           &src,
           &dst,
           ttl,
           tos,
           data,
           data_len
    );
}

void clean_expired(time_t now) {
    for (auto it = src_nat_map.begin(); it != src_nat_map.end();) {
        nat_item_t *nat_item = &(*it).second;
        time_t time_lapsed = now - nat_item->active_time;
        if ((nat_item->reply && time_lapsed >= est_timeout) ||
            (!nat_item->reply && time_lapsed >= new_timeout)) {
            {
                char src_ip_str[0x20];
                inet_ntop(AF_INET, &nat_item->src.sin_addr, src_ip_str, sizeof(src_ip_str));
                char nat_ip_str[0x20];
                inet_ntop(AF_INET, &nat_ip, nat_ip_str, sizeof(nat_ip_str));
                LOG(LOG_INFO, "del, src = %s:%d, nat = %s:%d, duration = %lu, tx = %"
                        PRIu64
                        ", rx = %"
                        PRIu64,
                    src_ip_str, ntohs(nat_item->src.sin_port),
                    nat_ip_str, ntohs(nat_item->nat_port),
                    nat_item->active_time - nat_item->create_time,
                    nat_item->tx, nat_item->rx
                );
            }
            src_session_counter[nat_item->src.sin_addr.s_addr]--;
            ep_del(ep_fd, nat_item->fd);
            nat_port_fd.erase(nat_item->nat_port);
            close(nat_item->fd);
            restricted_cone_set.erase(nat_item->fd);
            dst_nat_map.erase(nat_item->fd);
            it = src_nat_map.erase(it);
            continue;
        }
        ++it;
    }
    LOG(LOG_DEBUG, "live = %lu", src_nat_map.size());
}

void write_pid(char *pid_file) {
    int fd;
    size_t len;
    struct flock lock{};
    FUCK((fd = open(pid_file, (uint32_t) O_RDWR | (uint32_t) O_CREAT, 0666)) < 0);
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    FUCK(fcntl(fd, F_SETLK, &lock) < 0);
    char pid_str[12];
    sprintf(pid_str, "%d\n", getpid());
    len = strlen(pid_str);
    FUCK(write(fd, pid_str, len) != len);
}

void usage_and_exit() {
    printf("Usage:\n");
    printf("  -h Help\n");
    printf("  -p TPROXY port\n");
    printf("  -i Minimum port (default: 10240)\n");
    printf("  -x Maximum port (default: 65535)\n");
    printf("  -s NAT ip (default: 0.0.0.0, depends on system)\n");
    printf("  -n NEW(no reply) timeout (default: 30)\n");
    printf("  -e ESTABLISHED timeout (default: 300)\n");
    printf("  -o Session limit per source ip (default: 65535, unlimited)\n");
    printf("  -t NAT type, 1. full-cone, 2. restricted-cone, 3. port-restricted-cone (default: 1)\n");
    printf("  -c Clean interval (default: 10)\n");
    printf("  -b No inbound refresh\n");
    printf("  -f PID file\n");
    printf("  -l Log level (default: 6. INFO)\n");
    printf("  -d Run as daemon, log to syslog\n");
    exit(0);
}

void setup_tproxy() {
    int fd;
    FUCK((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0);
    int opt = 1;
    FUCK(setsockopt(fd, IPPROTO_IP, IP_RECVTTL, &opt, sizeof(opt)) < 0);
    FUCK(setsockopt(fd, IPPROTO_IP, IP_RECVTOS, &opt, sizeof(opt)) < 0);
    FUCK(setsockopt(fd, SOL_IP, IP_TRANSPARENT, &opt, sizeof(opt)) < 0);
    FUCK(setsockopt(fd, SOL_IP, IP_RECVORIGDSTADDR, &opt, sizeof(opt)) < 0);
    struct sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(tproxy_port);
    FUCK(bind(fd, (const struct sockaddr *) &serv, sizeof(serv)) < 0);

    auto *ep_data = (ep_data_t *) malloc(sizeof(ep_data_t));
    ep_data->fd = fd;
    ep_data->cb = (void *) perform_src_nat;
    ep_add(ep_fd, ep_data);
}

void setup_icmp() {
    int fd;
    FUCK((fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0);
    int opt = 1;
    FUCK(setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0);
    auto *ep_data = (ep_data_t *) malloc(sizeof(ep_data_t));
    ep_data->fd = fd;
    ep_data->cb = (void *) icmp_recv;
    ep_add(ep_fd, ep_data);
}

void bye(int) {
}

void adjust_sched() {
    int p = sched_get_priority_max(SCHED_FIFO);
    FUCK(sched_setscheduler(getpid(), SCHED_FIFO, (struct sched_param *) &p) < 0);
}

int main(int argc, char *argv[]) {
    printf("TPConeNATd by claw6148\n\n");
    if (argc == 1) usage_and_exit();
    try {
        char pid_file[0x100]{};
        for (int ch; (ch = getopt(argc, argv, "hp:i:x:s:n:e:o:t:c:bf:l:d")) != -1;) {
            switch (ch) {
                case 'h':
                    usage_and_exit();
                    break;
                case 'p':
                    tproxy_port = stol(optarg);
                    break;
                case 'i':
                    min_port = stol(optarg);
                    break;
                case 'x':
                    max_port = stol(optarg);
                    break;
                case 's':
                    nat_ip = inet_addr(optarg);
                    break;
                case 'n':
                    new_timeout = stol(optarg);
                    break;
                case 'e':
                    est_timeout = stol(optarg);
                    break;
                case 'o':
                    session_per_src = stol(optarg);
                    break;
                case 't':
                    nat_type = stol(optarg);
                    break;
                case 'c':
                    clean_interval = stol(optarg);
                    break;
                case 'b':
                    no_inbound_refresh = true;
                    break;
                case 'f':
                    strcpy(pid_file, optarg);
                    break;
                case 'l':
                    log_level = stol(optarg);
                    break;
                case 'd':
                    run_as_daemon = 1;
                    break;
                default:
                    break;
            }
        }
        if (tproxy_port == 0) {
            printf("Invalid TPROXY port!\n");
            return 1;
        }

        if (min_port >= max_port) {
            printf("Invalid port range!\n");
            return 1;
        }

        if (nat_type < 1 || nat_type > 3) {
            printf("Invalid NAT type!\n");
            return 1;
        }

        if (clean_interval <= 0) {
            printf("Invalid clean interval!\n");
            return 1;
        }

        PRINT_DEC(tproxy_port);
        PRINT_DEC(min_port);
        PRINT_DEC(max_port);
        PRINT_IP(nat_ip);
        PRINT_DEC(new_timeout);
        PRINT_DEC(est_timeout);
        PRINT_DEC(clean_interval);
        PRINT_DEC(session_per_src);
        PRINT_DEC(nat_type);
        PRINT_DEC(no_inbound_refresh);
        PRINT_DEC(log_level);

        LOG(LOG_INFO, "Started!");

        if (run_as_daemon) FUCK(daemon(1, 1) < 0);
        if (strlen(pid_file)) write_pid(pid_file);

        signal(SIGINT, bye);
        signal(SIGTERM, bye);

        adjust_sched();

        srandom(time(nullptr));
        ep_fd = epoll_create1(0);

        setup_icmp();
        setup_tproxy();

        time_t last_tick = 0;
        for (;;) {
            static struct epoll_event ready_ev[0x10000];
            int n = epoll_wait(ep_fd, ready_ev, sizeof(ready_ev) / sizeof(struct epoll_event),
                               (int) clean_interval * 1000);
            FUCK(n < 0);
            while (n--) {
                try {
                    auto *p = (ep_data_t *) ready_ev[n].data.ptr;
                    ep_recv(p->fd, p->cb);
                } catch (exception const &e) {
                    PERROR(e.what());
                }
            }
            time_t now = time(nullptr);
            if (now - last_tick >= clean_interval) {
                clean_expired(now);
                last_tick = now;
            }
        }
    } catch (exception const &e) {
        PERROR(e.what());
    }
    return 0;
}
