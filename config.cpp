//
// Created by root on 6/2/20.
//

#include "config.h"
#include "util.h"
#include <arpa/inet.h>

using namespace std;

void config::validate() {
    THROW_IF_FALSE(this->tproxy_port > 0);
    THROW_IF_FALSE(this->min_port > 0);
    THROW_IF_FALSE(this->max_port > 0);
    THROW_IF_FALSE(this->min_port < this->max_port);
    THROW_IF_FALSE(ntohl(this->nat_ip) < 0xe0000000);
    THROW_IF_FALSE(this->new_timeout > 0);
    THROW_IF_FALSE(this->est_timeout > 0);
    THROW_IF_FALSE(this->session_per_src > 0);
    THROW_IF_FALSE(this->nat_type >= 1);
    THROW_IF_FALSE(this->nat_type <= 3);
}

void config::print() {
#define PRINT_DEC(x) LOG(LOG_INFO, #x" = %d", x)
#define PRINT_IP(x) do { \
    uint32_t y = ntohl(x); \
    LOG(LOG_INFO, #x" = %d.%d.%d.%d", ((uint8_t*)&y)[3], ((uint8_t*)&y)[2], ((uint8_t*)&y)[1], ((uint8_t*)&y)[0]); \
} while(0)
    PRINT_DEC(this->tproxy_port);
    PRINT_DEC(this->min_port);
    PRINT_DEC(this->max_port);
    PRINT_IP(this->nat_ip);
    PRINT_DEC(this->new_timeout);
    PRINT_DEC(this->est_timeout);
    PRINT_DEC(this->session_per_src);
    PRINT_DEC(this->nat_type);
}
