#pragma once
#include <stdint.h>
#include <unordered_set>
#include <cstdint>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

class asn_bank {

public:
    asn_bank() { load_asns_from_file(en_path); }

    bool is_ru_asn(uint32_t asn) {
        return RU_ASNs.contains(asn);
    }

    bool is_en_asn(uint32_t asn) {
        return EN_ASNs.contains(asn);
    }

    std::unordered_set<uint32_t> RU_ASNs =
    {
        47724, // head hunter
        59601, // head hunter office
        35237, // — Sberbank of Russia PJSC (основной ASN Сбербанка) 🇷🇺
        206673, // — Sberbank‑Telecom LLC 🇷🇺 (телеком‑подразделение Сбербанка)
        209701, // — Sberbank‑factoring Ltd. 🇷🇺 (факторинговое подразделение)
        45000, // — Sber‑EKB‑AS (Sberbank of Russia PJSC) 🇷🇺 (доп. ASN)
        44408, // — Sberbank of Russia PJSC (хостинг‑ASN) 🇷🇺
        201012, //  — зарегистрирован как Avito / KEH eCommerce LLC в RIPE NCC.
        213506, //  — VKUSVILL-AS, принадлежит VKUSVILL JSC (Вкусвилл).
        44386 // — OZON-AS / LLC Internet Solutions (используется для сети Ozon).
    };

    std::unordered_set<uint32_t> EN_ASNs =
    {
        15169,   // Google LLC (main)
        36040,   // YouTube
        36411,   // Google GEN-AS2
        36383,   // Google GEN-AS1
        36492,   // Google WiFi
        32381,   // Google Cloud Networking
        396982,  // Google Cloud Platform
        19527,   // Google
        139070,  // Google
        43515,   // AS43515 — YOUTUBE (Google Ireland Limited)
        36561,   // Google
        16591,    // Google Fiber
        32934,   // FACEBOOK – Meta (основной)
        54115,   // FACEBOOK-CORP – Meta corporate
        63293,   // FACEBOOK-OFFNET – Meta offnet
        149642,  // FACEBOOK-AS – Meta Singapore
        34825,    // META-AS34825 – Facebook Israel Ltd
        16509,  // AS16509 - AMAZON-02
        14618,  // AS14618 - AMAZON-AES
        7224,   // AS7224 - Amazon affiliated ASN
        8987,   // AS8987 - Amazon affiliated ASN
        62785,  // AS62785 - Amazon affiliated ASN
        19047,  // AS19047 - Amazon affiliated ASN
        39111,  // AS39111 - Amazon affiliated ASN
        9059,   // AS9059 - Amazon affiliated ASN
        38895,  // AS38895 - Amazon affiliated ASN
        10124,  // AS10124 - Amazon affiliated ASN
        17493,  // AS17493 - Amazon affiliated ASN
        46489   // AS46489 - Amazon affiliated ASN
    };

    std::string en_path = std::string("/opt/geoip/en.list");
    std::string ru_path = std::string("/opt/geoip/ru.list");

    void load_asns_from_file(const std::string& path)
    {
        std::unordered_set<uint32_t> &asns = EN_ASNs;
        std::ifstream file(path);

        if (!file.is_open()) {
            std::cout << "Failed to open ASN file: " << path << std::endl;
            return;
        }

        std::cout << "Loading en ASN from file: " << path << std::endl;

        EN_ASNs.clear();

        std::string line;
        while (std::getline(file, line)) {
            // rm comments
            auto comment_pos = line.find('#');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }

            // rm spaces from start and end
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty()) continue;

            uint32_t asn;
            std::istringstream iss(line);
            if (iss >> asn) {
                asns.insert(asn);
            }
        }
    }
};
