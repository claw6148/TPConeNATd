cmake_minimum_required(VERSION 3.15)
project(TPConeNATd)

set(CMAKE_CXX_STANDARD 11)

add_executable(TPConeNATd main.cpp epoll_helper.cpp epoll_helper.h watchdog.cpp watchdog.h inbound.cpp inbound.h tproxy.cpp tproxy.h util.h outbound.cpp outbound.h dgram_read.cpp dgram_read.h nat.cpp nat.h icmp_helper.cpp icmp_helper.h config.cpp config.h util.cpp)