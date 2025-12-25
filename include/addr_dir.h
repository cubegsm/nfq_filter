#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include "private_ip.h"

class addr_dir : public private_ip
{
public:
    enum pkt_dir { DIR_UNK = 0, DIR_IN, DIR_OUT, DIR_INT };

    pkt_dir get_diriction(struct iphdr* iphdr)
    {
        if (is_private_ip(iphdr->daddr)) {
            if (is_private_ip(iphdr->saddr))
                return DIR_INT;  // both is LAN

            return DIR_IN; // src = WAN, dst = LAN
        } else if (is_private_ip(iphdr->saddr)) {
            return DIR_OUT; // src = LAN, dst = WAN
        } else
            return DIR_UNK;
    }

    uint32_t get_addr(struct iphdr* iphdr)
    {
        if (is_private_ip(iphdr->daddr)) {
            if (is_private_ip(iphdr->saddr))
                return 0;  // both is LAN

            return iphdr->saddr; // src = WAN, dst = LAN
        } else if (is_private_ip(iphdr->saddr)) {
            return iphdr->daddr; // src = LAN, dst = WAN
        } else
            return 0;
    }

};
