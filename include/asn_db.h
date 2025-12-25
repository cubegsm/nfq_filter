#pragma once
#include <iostream>
#include <csignal>
#include <string>
#include <netinet/in.h>
#include <maxminddb.h>

class asn_db {

public:
    asn_db() {
        const char *db_path = "/opt/geoip/GeoLite2-ASN.mmdb";

        int status = MMDB_open(db_path, MMDB_MODE_MMAP, &mmdb);
        if (status != MMDB_SUCCESS) {
            std::cerr << "MMDB_open failed: " << MMDB_strerror(status) << std::endl;
        }
    }

    ~asn_db() {
        MMDB_close(&mmdb);
    }

     MMDB_lookup_result_s lookup(
                         const struct sockaddr *const sockaddr,
                         int *const mmdb_error) {
        return MMDB_lookup_sockaddr(&mmdb, sockaddr, mmdb_error);
    }

protected:
    MMDB_s mmdb;
};
