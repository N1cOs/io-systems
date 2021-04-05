# Лабораторная работа 3

**Название:** "Разработка драйверов сетевых устройств"

**Цель работы:** получить знания и навыки разработки драйверов сетевых интерфейсов для операционной системы Linux

## Описание функциональности драйвера
Создать виртуальный интерфейс, который перехватывает пакеты протокола ICMP (Internet Control Message Protocol) – только тип 8. Сохранять данные.  
Состояние разбора пакетов необходимо выводить в файл `/proc/var1`

## Инструкция по сборке
Для сборки драйвера выполнить:
```bash
make
```

## Инструкция пользователя
После успешной сборки загрузить полученный модуль:
```bash
insmod lab3.ko
```
Проверить, что драйвер загрузился без ошибок с помощью команды `dmesg`, в выводе должно быть подобное:
```
lab3: successfully loaded
lab3: device opened: name=vni0
```

## Примеры использования
После загрузки можно проверить, что драйвер создал сетевой интерфейс c помощью `ip a`:
```
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host 
       valid_lft forever preferred_lft forever
2: enp0s3: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 08:00:27:fc:c8:88 brd ff:ff:ff:ff:ff:ff
    inet 192.168.8.56/24 brd 192.168.8.255 scope global dynamic noprefixroute enp0s3
       valid_lft 77837sec preferred_lft 77837sec
    inet6 fe80::e675:536:b664:f96a/64 scope link noprefixroute 
       valid_lft forever preferred_lft forever
20: vni0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN group default qlen 1000
    link/ether 08:00:27:fc:c8:88 brd ff:ff:ff:ff:ff:ff
```
Далее начнем отправлять ICMP пакеты на родительский интерфейс `enp0s3`. Выполним серию команд:
```
ping -c 2 -p 68656c6c6f0a -s 6 192.168.8.56
ping -c 3 -p 776f726c640a -s 6 192.168.8.56
```
Проверим содержимое файла `/proc/var1`:
```
hello
hello
world
world
world
```
Далее посмотрим статистику по принятым пакетам выполнив `ip -s link vni0`:
```
20: vni0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN mode DEFAULT group default qlen 1000
    link/ether 08:00:27:fc:c8:88 brd ff:ff:ff:ff:ff:ff
    RX: bytes  packets  errors  dropped overrun mcast   
    230        5        0       0       0       0       
```
Видно, что интерфейс получил ровно 5 ICMP пакетов, которые мы отправили ранее.

После звершения работы можно выгрузить модуль из ядра:
```bash
rmmod lab3
```
