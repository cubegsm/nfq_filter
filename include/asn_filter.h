#pragma once
#include <iostream>
#include <csignal>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include "lru_cache.h"
#include "asn_bank.h"
#include "asn_db.h"
#include "addr_dir.h"

class asn_filter : public asn_bank, addr_dir, protected asn_db {
public:
    asn_filter() : verict_cache(1048576, 86400) { }
    ~asn_filter() { }

    bool get_wan_decision(struct iphdr* iphdr) {

        uint32_t ip = get_addr(iphdr);

        if (ip == 0)
            return true; // local && unknown addresses routes to WAN

        int mmdb_error = 0;
        struct sockaddr_in sa4;
        sa4.sin_family = AF_INET;
        sa4.sin_addr.s_addr = ip;

        MMDB_lookup_result_s result = MMDB_lookup_sockaddr(&mmdb, (struct sockaddr*) &sa4, &mmdb_error);

        if (mmdb_error != MMDB_SUCCESS) {
            std::cerr << "MMDB lookup error: " << MMDB_strerror(mmdb_error) << std::endl;
            return false; // default route to VPN
        }

        if (!result.found_entry) {
            // is not error
            //std::cerr << "IP not found in database" << std::endl;
            return false; // default route to VPN
        }

        MMDB_entry_data_s asn;
        int status = MMDB_get_value(&result.entry, &asn, "autonomous_system_number", NULL);

        if (status == MMDB_SUCCESS && asn.has_data) {
            //std::cout << "ASN: AS" << asn.uint32 << std::endl;
        } else {
            std::cerr << "can't resolve ASN" << std::endl;
            return false; // default route to VPN
        }

        if (is_ru_asn(asn.uint32))
            return true; // route to WAN

        return false; // default route to VPN
    }


    // true - route to WAN (return false)
    // false - default route to VPN (return true)
    bool get_wan_verdict_en(struct iphdr* iphdr) {

        uint32_t ip = get_addr(iphdr);

        if (ip == 0)
            return true; // local && unknown addresses routes to WAN


        // try to get cached value
        bool* cached_verdict = verict_cache.get(ip);
        if (cached_verdict != nullptr)
            return *cached_verdict;

        int mmdb_error = 0;
        struct sockaddr_in sa4;
        sa4.sin_family = AF_INET;
        sa4.sin_addr.s_addr = ip;

        MMDB_lookup_result_s result = MMDB_lookup_sockaddr(&mmdb, (struct sockaddr*) &sa4, &mmdb_error);

        if (mmdb_error != MMDB_SUCCESS) {
            std::cerr << "MMDB lookup error: " << MMDB_strerror(mmdb_error) << std::endl;
            return false; // default route to VPN
        }

        if (!result.found_entry) {
            // is not error
            //std::cerr << "IP not found in database" << std::endl;
            return false; // default route to VPN
        }

        MMDB_entry_data_s asn;
        int status = MMDB_get_value(&result.entry, &asn, "autonomous_system_number", NULL);

        if (status == MMDB_SUCCESS && asn.has_data) {
            //std::cout << "ASN: AS" << asn.uint32 << std::endl;
        } else {
            std::cerr << "can't resolve ASN" << std::endl;
            return false; // default route to VPN
        }

        // is_en_asn = true ? verdict -> false- default route to VPN
        // is_en_asn = false ? verdict -> true - route to WAN
        bool verdict = !is_en_asn(asn.uint32);

        // cache value
        verict_cache.put(ip,verdict);

        return verdict;
    }

private:
    Cache::lru_cache<uint32_t, bool> verict_cache; //(1048576, 86400);
};
