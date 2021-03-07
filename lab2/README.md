# Лабораторная работа 2

**Название:** Разработка драйверов блочных устройств

**Цель работы:** Получить знания и навыки разработки драйверов блочных устройств для операционной системы Linux.

## Описание функциональности драйвера
При загрузке модуля создается блочное устройство с одними первичным разделом размером 10Мбайт и одним расширенным разделом, содержащих два логических раздела размером 20Мбайт каждый.

## Инструкция по сборке
Для сборки драйвера выполнить:
```bash
make
```

## Инструкция пользователя
После успешной сборки загрузить полученный модуль:
```bash
insmod lab2.ko
```
Проверить, что драйвер загрузился без ошибок с помощью команды `dmesg`, в выводе должно быть подобное:
```
lab2: successfully loaded: disk=lab2, major=252
lab2: disk open
lab2: disk release
```

## Примеры использования
После загрузки можно проверить, что драйвер создал блочное устройство согласно заданию
c помощью `fdisk -l /dev/lab2`:
```
Disk /dev/lab2: 50 MiB, 52428800 bytes, 102400 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x00000000

Device      Boot Start    End Sectors Size Id Type
/dev/lab2p1          1  20479   20479  10M 83 Linux
/dev/lab2p2      20480 102399   81920  40M  5 Extended
/dev/lab2p5      20481  61439   40959  20M 83 Linux
/dev/lab2p6      61441 102399   40959  20M 83 Linux

```
После можно отформатировать новые разделы и смонтировать их в директорию `/mnt`. Выполним команду `make setup` и убедимся что все сработало без ошибок выполнив `lsblk -l /dev/lab2`:
```
NAME   MAJ:MIN RM SIZE RO TYPE MOUNTPOINT
lab2   252:0    0  50M  0 disk 
lab2p1 252:1    0  10M  0 part /mnt/lab2p1
lab2p2 252:2    0   1K  0 part 
lab2p5 252:5    0  20M  0 part /mnt/lab2p5
lab2p6 252:6    0  20M  0 part /mnt/lab2p6
```
Далее можно провести бенчмарки, которые измеряют скорость передачи файла 9MB (размер файла можно регулировать переменной окружения `BENCH_FILE_SIZE`) между различными разделами созданного блочного устройства и реального SSD выполнив `make bench`:
```
src=/mnt/lab2p1, dst=/mnt/lab2p5, median speed=315.0, units=MB/s
src=/mnt/lab2p1, dst=/mnt/lab2p6, median speed=316.0, units=MB/s
src=/mnt/lab2p1, dst=/root, median speed=307.0, units=MB/s
src=/mnt/lab2p5, dst=/mnt/lab2p1, median speed=316.0, units=MB/s
src=/mnt/lab2p5, dst=/mnt/lab2p6, median speed=306.0, units=MB/s
src=/mnt/lab2p5, dst=/root, median speed=301.0, units=MB/s
src=/mnt/lab2p6, dst=/mnt/lab2p1, median speed=317.0, units=MB/s
src=/mnt/lab2p6, dst=/mnt/lab2p5, median speed=295.0, units=MB/s
src=/mnt/lab2p6, dst=/root, median speed=260.0, units=MB/s
src=/root, dst=/mnt/lab2p1, median speed=315.0, units=MB/s
src=/root, dst=/mnt/lab2p5, median speed=306.0, units=MB/s
src=/root, dst=/mnt/lab2p6, median speed=321.0, units=MB/s
```
Видно, что при чтении/записи файла в созданное блочное устройтво скорость записи в районе 300 MB/s.
Однако при записи файла в директорию, которая смонтирована на реальный SSD можно в одном из
случаем увидеть падение скорости до 260 MB/s, что подтверждает теорию о том, что SSD обладает меньшими скоростями на чтение/запись нежели RAM. Скорее всего этот факт был бы более явным, если проводить бенчмарки на файле большего размера.

После звершения работы можно отмонтировать разделы и выгрузить модуль из ядра:
```bash
make umount
rmmod lab2
```
