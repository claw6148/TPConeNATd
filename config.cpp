//
// Created by root on 6/2/20.
//

#include "config.h"
#include "util.h"

using namespace std;

void config::validate() {
    THROW_IF(tproxy_port == 0);
    THROW_IF(min_port == 0);
    THROW_IF(max_port == 0);
    THROW_IF(min_port >= max_port);
    THROW_IF(nat_ip == 0);
    THROW_IF(new_timeout == 0);
    THROW_IF(est_timeout == 0);
    THROW_IF(session_per_src == 0);
    THROW_IF(nat_type < 1 || nat_type > 3);
    THROW_IF(log_level > 7);
}

void config::print() {
#define PRINT
}
