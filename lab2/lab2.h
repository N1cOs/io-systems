#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/bio.h>

#define DISK_NAME "lab2"

#define SECTOR_SIZE 512
#define DISK_SIZE 50 * 0x100000
#define DISK_NR_SECTORS (DISK_SIZE / SECTOR_SIZE)

#define BR_SIZE SECTOR_SIZE
#define BR_PARTITION_TABLE_OFFSET 446
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE 0xAA55


struct disk_dev {
    spinlock_t lock;
    struct request_queue *queue;
    struct gendisk *gd;
    u8 *data;
};

struct partition_entry {
    u8 bootable;
    u8 start_head;
    u16 start_cyl_sec;
    u8 part_type;
	u8 end_head;
	u16 end_cyl_sec;
	u32 abs_start_sec;
	u32 nr_sec;
};
