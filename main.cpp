//
// Created by claw6148 on 5/25/20.
//

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include "nat.h"
#include "util.h"

using namespace std;

void write_pid(char *pid_file) {
    int fd = open(pid_file, (uint32_t) O_RDWR | (uint32_t) O_CREAT, 0666);
    struct flock lock{};
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    THROW_IF_NEG(fcntl(fd, F_SETLK, &lock));
    char pid_str[12];
    sprintf(pid_str, "%d\n", getpid());
    size_t len = strlen(pid_str);
    THROW_IF_NEG(write(fd, pid_str, len));
}

void usage_and_exit() {
    config cfg;
    printf("Usage:\n");
    printf("  -h Help\n");
    printf("  -p TPROXY port\n");
    printf("  -i Minimum port (default: %d)\n", cfg.min_port);
    printf("  -x Maximum port (default: %d)\n", cfg.max_port);
    printf("  -s NAT ip (default: %s)\n", inet_ntoa(*reinterpret_cast<in_addr *>(&cfg.nat_ip)));
    printf("  -n NEW(no reply) timeout (default: %d)\n", cfg.new_timeout);
    printf("  -e ESTABLISHED timeout (default: %d)\n", cfg.est_timeout);
    printf("  -o Session limit per source ip (default: %d)\n", cfg.session_per_src);
    printf("  -t NAT type, 1. full-cone, 2. restricted-cone, 3. port-restricted-cone (default: %d)\n", cfg.nat_type);
    printf("  -r Sender thread (default: %d)\n", cfg.sender_thread);
    printf("  -f PID file\n");
    printf("  -l Log level (default: %d)\n", log_level);
    printf("  -d Run as daemon, log to syslog\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    printf("TPConeNATd by claw6148\n");
    if (argc == 1) usage_and_exit();
    try {
        config cfg;
        char pid_file[0x100]{};
        for (int ch; (ch = getopt(argc, argv, "hp:i:x:s:n:e:o:t:r:f:l:d")) != -1;) {
            switch (ch) {
                case 'h':
                    usage_and_exit();
                    break;
                case 'p':
                    cfg.tproxy_port = stol(optarg);
                    break;
                case 'i':
                    cfg.min_port = stol(optarg);
                    break;
                case 'x':
                    cfg.max_port = stol(optarg);
                    break;
                case 's':
                    cfg.nat_ip = inet_addr(optarg);
                    break;
                case 'n':
                    cfg.new_timeout = stol(optarg);
                    break;
                case 'e':
                    cfg.est_timeout = stol(optarg);
                    break;
                case 'o':
                    cfg.session_per_src = stol(optarg);
                    break;
                case 't':
                    cfg.nat_type = stol(optarg);
                    break;
                case 'f':
                    strcpy(pid_file, optarg);
                    break;
                case 'l':
                    log_level = stol(optarg);
                    break;
                case 'r':
                    cfg.sender_thread = stol(optarg);
                    break;
                case 'd':
                    run_as_daemon = true;
                    break;
                default:
                    break;
            }
        }
        cfg.validate();
        if (run_as_daemon) THROW_IF_NEG(daemon(1, 1));
        if (strlen(pid_file)) write_pid(pid_file);
        LOG(LOG_INFO, "started");
        srandom(time(nullptr));
        cfg.print();
        nat(cfg).run();
    } catch (runtime_error &e) {
        PERROR(e.what());
        return -1;
    }
    return 0;
}
