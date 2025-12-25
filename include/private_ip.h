#pragma once
#include <stdint.h>

class private_ip
{
public:
    bool is_private_ip(uint32_t ip_net_order)
    {
        uint32_t ip = ntohl(ip_net_order); // to host byte order

        if ((ip & 0xFF000000) == 0x0A000000) // 10.0.0.0/8
            return true;

        if ((ip & 0xFFF00000) == 0xAC100000) // 172.16.0.0/12
            return true;

        if ((ip & 0xFFFF0000) == 0xC0A80000) // 192.168.0.0/16
            return true;

        return false;
    }

};
