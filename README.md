# Network Filter & Routing Setup

## Dev Installation

```bash
sudo apt update
sudo apt install libnetfilter-queue-dev libnftnl-dev libmnl-dev libmaxminddb-dev
```

## Production Installation

```bash
sudo apt install libnetfilter-queue1
```

---

## Environment

### Network Interfaces

```
awgmy2: flags=209<UP,POINTOPOINT,RUNNING,NOARP>  mtu 1408
inet 192.168.196.72  netmask 255.255.255.255  destination 192.168.196.72

enp1s0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
inet 192.168.0.165  netmask 255.255.255.0  broadcast 192.168.0.255

wlx90de806d6fe6: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
inet 192.168.10.1  netmask 255.255.255.0  broadcast 192.168.10.255
```

```
awgmy2: flags=209<UP,POINTOPOINT,RUNNING,NOARP> mtu 1408
inet 192.168.196.72 netmask 255.255.255.255 destination 192.168.196.72

enp1s0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST> mtu 1500
inet 192.168.0.200 netmask 255.255.255.0 broadcast 192.168.0.255

wlx90de806d6fe6: flags=4163<UP,BROADCAST,RUNNING,MULTICAST> mtu 1500
inet 192.168.10.1 netmask 255.255.255.0 broadcast 0.0.0.0
```

### Interface Roles

- **enp1s0** — wired WAN interface
- **wlx90de806d6fe6** — Wi-Fi adapter in AP mode
    - connects clients
    - assigns IP addresses
    - network: `192.168.10.1/24`
- **awgmy2** — virtual VPN interface
    - Amnesia WireGuard client

---

## nftables Configuration

### Current Ruleset

```bash
sudo nft list ruleset
```

```
table inet filter {
    chain input {
        type filter hook input priority filter; policy accept;
    }

    chain forward {
        type filter hook forward priority filter; policy accept;
    }

    chain output {
        type filter hook output priority filter; policy accept;
    }
}

table ip nat {
    chain postrouting {
        type nat hook postrouting priority srcnat; policy accept;
        oif "awgmy2" masquerade
    }
}

table ip filter {
    chain forward {
        type filter hook forward priority filter; policy accept;
        iif "wlx90de806d6fe6" oif "awgmy2" accept
        iif "awgmy2" oif "wlx90de806d6fe6" ct state established,related accept
    }
}
```

---

## Routing

### Routing Tables

```bash
ip route show table 51820
```

```
default dev awgmy2 scope link
```

- **Main table** → default via `enp1s0`
- **Table 51820** → default via `awgmy2`

### Policy Routing Rules

```bash
ip rule show
```

```
0:      from all lookup local
32764:  from all lookup main suppress_prefixlength 0
32765:  not from all fwmark 0xca6c lookup 51820
32766:  from all lookup main
32767:  from all lookup default
```

---

## Traffic Flow (Wi-Fi Client → WAN)

### Wi-Fi Client

```
Client IP: 192.168.10.20
Gateway:   192.168.10.1
DNS:       192.168.10.1
```

Packet:

```
SRC=192.168.10.20
DST=8.8.8.8
```

### Routing Decision

```bash
ip route get 8.8.8.8 from 192.168.10.1
```

```
8.8.8.8 from 192.168.10.1 dev awgmy2 table 51820 uid 0
```

---

## PREROUTING Setup

```bash
nft add table inet mangle
nft add chain inet mangle prerouting '{ type filter hook prerouting priority mangle; policy accept; }'
nft add rule inet mangle prerouting iif "wlx90de806d6fe6" queue num 10 bypass
nft add rule ip nat postrouting oif "enp1s0" masquerade
```

---

## Policy Priority Override

```bash
ip rule add fwmark 0x1 lookup main priority 100
```

---

## Service Installation

```
/etc/systemd/system/filter.service
```

```bash
systemctl enable filter
```

---

## Traffic Logging

```bash
tcpdump -i wlx90de806d6fe6 host 192.168.10.71 -w int2.pcap
```

---

## ASN Lookup

```bash
whois -h whois.radb.net -- '-i origin AS15169' | grep ^route
```