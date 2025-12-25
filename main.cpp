#include <iostream>
#include <csignal>
#include <string>
#include <arpa/inet.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include "asn_filter.h"

volatile sig_atomic_t stop_flag = 0;
asn_filter afilter;
struct timespec last, now;
uint32_t pkts_wan = 0;
uint32_t pkts_vpn = 0;

void handle_sigint(int signum)
{
    stop_flag = 1;
}

int main()
{
    std::signal(SIGINT, handle_sigint);

    std::cout << "build: " << __DATE__ << " " << __TIME__ << std::endl;

    struct nfq_handle *h;
    struct nfq_q_handle *qh;

    clock_gettime(CLOCK_MONOTONIC, &last);

    h = nfq_open();
    if (!h) {
        std::cerr << "Error: nfq_open() failed\n";
        return 1;
    }

    if (nfq_unbind_pf(h, AF_INET) < 0) {
        std::cerr << "Error: nfq_unbind_pf()\n";
    }

    if (nfq_bind_pf(h, AF_INET) < 0) {
        std::cerr << "Error: nfq_bind_pf()\n";
        return 1;
    }

    qh = nfq_create_queue(h, 10, [](struct
                nfq_q_handle *qh,
                struct nfgenmsg *,
                struct nfq_data *nfa, void *) -> int {
        bool route_to_wan = false;
        uint32_t mark = nfq_get_nfmark(nfa);
        struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
        uint32_t id = ph ? ntohl(ph->packet_id) : 0;
        unsigned char *payload;
        int len = nfq_get_payload(nfa, &payload);

        if (len >= (int) sizeof(struct iphdr)) {
            struct iphdr *iph = (struct iphdr *) payload;

            // схема 1. известные RU ASN пускаем на WAN, остальное VPN
            //route_to_wan = get_wan_decision(iph);

            // схема 2. известные EN ASN пускаем на VPN, остальное WAN
            route_to_wan = afilter.get_wan_verdict_en(iph);
        }

        if (route_to_wan) {
            // accept and set new mark = 0x1 (route tp wan)
            nfq_set_verdict2(qh, id, NF_ACCEPT, 0x1, 0, nullptr);
            pkts_wan++;
        }
        else {
            //accept packet without mark changing (default route to vpn)
            nfq_set_verdict(qh, id, NF_ACCEPT, 0, nullptr);
            pkts_vpn++;
        }

        clock_gettime(CLOCK_MONOTONIC, &now);

        long diff_ms = (now.tv_sec - last.tv_sec) * 1000 +
            (now.tv_nsec - last.tv_nsec) / 1000000;

        if (diff_ms >= 10000) {
            printf("pkts_vpn %u, pkts_wan %u\n", pkts_vpn, pkts_wan);
            last = now;
            fflush(stdout);
        }

        return 0;
    }, nullptr);

    if (!qh) {
        std::cerr << "Error: nfq_create_queue()\n";
        nfq_close(h);
        return 1;
    }

    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        std::cerr << "Error: can't set packet copy mode\n";
        nfq_destroy_queue(qh);
        nfq_close(h);
        return 1;
    }

    int fd = nfq_fd(h);
    char buf[4096] __attribute__ ((aligned));

    while (!stop_flag) {
        int rv = recv(fd, buf, sizeof(buf), 0);
        if (rv >= 0) {
            nfq_handle_packet(h, buf, rv);
        } else if (rv < 0 && errno == EINTR) {
            break;
        }
    }

    std::cout << "\nbye!\n";

    nfq_destroy_queue(qh);
    nfq_close(h);

    return 0;
}
