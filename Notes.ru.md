

## Dev installation

sudo apt update
sudo apt install libnetfilter-queue-dev libnftnl-dev libmnl-dev libmaxminddb-dev

## Prod installation:
sudo apt install libnetfilter-queue1


# Среда

awgmy2: flags=209<UP,POINTOPOINT,RUNNING,NOARP>  mtu 1408
inet 192.168.196.72  netmask 255.255.255.255  destination 192.168.196.72

enp1s0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
inet 192.168.0.165  netmask 255.255.255.0  broadcast 192.168.0.255

wlx90de806d6fe6: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
inet 192.168.10.1  netmask 255.255.255.0  broadcast 192.168.10.255


awgmy2: flags=209<UP,POINTOPOINT,RUNNING,NOARP> mtu 1408 inet 192.168.196.72 netmask 255.255.255.255 destination 192.168.196.72 enp1s0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST> mtu 1500 inet 192.168.0.200 netmask 255.255.255.0 broadcast 192.168.0.255 wlx90de806d6fe6: flags=4163<UP,BROADCAST,RUNNING,MULTICAST> mtu 1500 inet 192.168.10.1 netmask 255.255.255.0 broadcast 0.0.0.0


enp1s0 - это проводной сетевой канал WAN
wlx90de806d6fe6 - это wifi адаптер, который настроен в режим AP и подключает клиентов, выдает им адреса
192.168.10.1/24
awgmy2 - виртуальный VPN адаптер, клиент amnesia WG, серевер запущен на внешнов ip 


sudo nft list ruleset
----------------------------------
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
----------------------------------

ip route show table 51820
default dev awgmy2 scope link


# Routing

Основная таблица (main)  → default через enp1s0
Таблица 51820  → default через awgmy2

ip rule show  - проходят все пакеты от всех интерфейсов
`
0:	from all lookup local  (таблица для локальных адресов)
32764:	from all lookup main suppress_prefixlength 0  
32765:	not from all fwmark 0xca6c lookup 51820 -- «Если пакет имеет fwmark = 0xca6c — смотри таблицу 51820»
32766:	from all lookup main
32767:	from all lookup default
`



# схему обработки трафика от wifi клиента до WAN

1. Клиент в Wi-Fi сети
   Client: 192.168.10.20
   GW:     192.168.10.1
   DNS:    192.168.10.1

формирует пакет:
SRC=192.168.10.20
DST=8.8.8.8

и отправляет его на шлюз 192.168.10.1, т.е. на wlx90de806d6fe6

2. Пакет приходит на AP - wlx90de806d6fe6

3. IP routing
Пакет проходит через ip rule

----
ip route get 8.8.8.8 from 192.168.10.1
8.8.8.8 from 192.168.10.1 dev awgmy2 table 51820 uid 0
cache
------
срабатывает правило -- dev awgmy2  (table 51820)


======================================================================================

## Добавляем PREROUTING

### Создаём таблицу inet mangle
nft add table inet mangle

### Добавляем цепочку:
nft add chain inet mangle prerouting '{ type filter hook prerouting priority mangle; policy accept; }'

### Добавляем правило ТОЛЬКО для Wi-Fi:
nft add rule inet mangle prerouting iif "wlx90de806d6fe6" queue num 10 bypass

### добавляем SNAT enp1s0
nft add rule ip nat postrouting oif "enp1s0" masquerade

nft list table ip nat
table ip nat {
    chain postrouting {
        type nat hook postrouting priority srcnat; policy accept;
        oif "awgmy2" masquerade
        oif "enp1s0" masquerade
    }
}

### добавляем таблицу перед всеми
ip rule add fwmark 0x1 lookup main priority 100
====================================
ip rule show
0:	from all lookup local
100:	from all fwmark 0x1 lookup main
32764:	from all lookup main suppress_prefixlength 0
32765:	not from all fwmark 0xca6c lookup 51820
32766:	from all lookup main
32767:	from all lookup default
===================================


изменнеия сохраняются сразу

-- nft list chain inet mangle prerouting
=====================
    table inet mangle {
        chain prerouting {
        type filter hook prerouting priority mangle; policy accept;
        iif "wlx90de806d6fe6" queue flags bypass to 10
    }
}
====================


# install as service

/etc/systemd/system/filter.service
systemctl enable filter


# log traffic

tcpdump -i wlx90de806d6fe6 host 192.168.10.71 -w int2.pcap

whois -h whois.radb.net -- '-i origin AS15169' | grep ^route
