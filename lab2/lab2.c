#include "lab2.h"

MODULE_AUTHOR("Nikita Karmatskikh");
MODULE_AUTHOR("Dmitriy Chistokhodov");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");

static struct partition_entry partitions[] = {
  {
    bootable: 0x00,
    start_head: 0x00,
    start_cyl_sec: 0x0000,
    part_type: 0x83, // primary
    end_head: 0x00,
    end_cyl_sec: 0x0000,
    abs_start_sec: 0x1,
    nr_sec: 0x4fff // 10 MB
  },
  {
    bootable: 0x00,
    start_head: 0x00,
    start_cyl_sec: 0x0000,
    part_type: 0x05, // extended
    end_head: 0x00,
    end_cyl_sec: 0x0000,
    abs_start_sec: 0x5000,
    nr_sec: 0x14000 // 40 MB
  },
};

static sector_t log_partitions_addrs[] = {0x5000, 0xf000};
static struct partition_entry log_partitions[][4] = {
  {
    {
      bootable: 0x00,
      start_head: 0x00,
      start_cyl_sec: 0x0000,
      part_type: 0x83,
      end_head: 0x00,
      end_cyl_sec: 0x0000,
      abs_start_sec: 0x1,
      nr_sec: 0x9fff // 20 MB
    },
    {
      bootable: 0x00,
      start_head: 0x00,
      start_cyl_sec: 0x0000,
      part_type: 0x05,
      end_head: 0x00,
      end_cyl_sec: 0x0000,
      abs_start_sec: 0xa000,
      nr_sec: 0x9fff // 20 MB
    },
  },
  {
    {
      bootable: 0x00,
      start_head: 0x00,
      start_cyl_sec: 0x0000,
      part_type: 0x83,
      end_head: 0x00,
      end_cyl_sec: 0x0000,
      abs_start_sec: 0x1,
      nr_sec: 0x9fff // 20 MB
    },
  }
};

static int major;
static struct disk_dev disk;

static void handle_disk_request(struct request_queue *queue) {
  struct request *req;
  while((req = blk_fetch_request(queue)) != NULL) {
    struct bio_vec bv;
    struct req_iterator iter;
    sector_t sector_offset = 0;
    sector_t start_sector = blk_rq_pos(req);
    
    rq_for_each_segment(bv, req, iter) {
      u8 *buf = page_address(bv.bv_page) + bv.bv_offset;
      unsigned int sectors = bv.bv_len / SECTOR_SIZE;

      int dir = rq_data_dir(req);
      sector_t pos = (start_sector + sector_offset) * SECTOR_SIZE;
      switch (dir) {
      case READ:
        memcpy(buf, disk.data + pos, sectors * SECTOR_SIZE);
        break;
      case WRITE:
        memcpy(disk.data + pos, buf, sectors * SECTOR_SIZE);	
        break;
      default:
        printk(KERN_ERR "%s: unknown disk operation: op=%d\n", THIS_MODULE->name, dir);
        continue;
      }

      sector_offset += sectors;
    }

    __blk_end_request_all(req, 0);
  }
}

static void write_mbr(u8 *buf) {
  memset(buf, 0, BR_SIZE);
  memcpy(buf + BR_PARTITION_TABLE_OFFSET, &partitions, sizeof(partitions));

  u16 br_signature = BR_SIGNATURE;
  memcpy(buf + BR_SIGNATURE_OFFSET, &br_signature, sizeof(br_signature));
}

static void write_ebr(u8 *buf) {
  u16 br_signature = BR_SIGNATURE;
  int i;
  for (i = 0; i < ARRAY_SIZE(log_partitions); i++) {
    u8 *addr = buf + log_partitions_addrs[i] * SECTOR_SIZE;
    memset(addr, 0, BR_SIZE);
    memcpy(addr + BR_PARTITION_TABLE_OFFSET, &log_partitions[i], sizeof(log_partitions[i]));
    memcpy(addr + BR_SIGNATURE_OFFSET, &br_signature, sizeof(br_signature));
  }
}

static int disk_open(struct block_device *bdev, fmode_t mode) {
    printk(KERN_INFO "%s: disk open\n", THIS_MODULE->name);
    return 0;
}

void disk_release(struct gendisk *gd, fmode_t mode) {
    printk(KERN_INFO "%s: disk release\n", THIS_MODULE->name);
}

struct block_device_operations disk_ops = {
    .owner = THIS_MODULE,
    .open = disk_open,
    .release = disk_release
};

static int __init mod_init(void) {
  disk.data = vmalloc(DISK_SIZE);
  if (!disk.data) {
    printk(KERN_ERR "%s: failed to allocate memory for disk: size=%d\n", THIS_MODULE->name, DISK_SIZE);
    return -1;
  }
  write_mbr(disk.data);
  write_ebr(disk.data);

  if ((major = register_blkdev(0, DISK_NAME)) < 0) {
    printk(KERN_ERR "%s: failed to get major number\n", THIS_MODULE->name);
    vfree(disk.data);
    return -1;
  }

  spin_lock_init(&disk.lock);
  disk.queue = blk_init_queue(handle_disk_request, &disk.lock);
  if (!disk.queue) {
    printk(KERN_ERR "%s: failed to init blk queue\n", THIS_MODULE->name);
    unregister_blkdev(major, DISK_NAME);
    vfree(disk.data);
    return -1;
  }

  u8 minors = 7; // for each partition we need individual minor number
  disk.gd = alloc_disk(minors);
  if (!disk.gd) {
    printk(KERN_ERR "%s: failed to allocate disk\n", THIS_MODULE->name);
    blk_cleanup_queue(disk.queue);
    unregister_blkdev(major, DISK_NAME);
    vfree(disk.data);
    return -1;
  }

  disk.gd->major = major;
  disk.gd->first_minor = 0;
  disk.gd->fops = &disk_ops;
  disk.gd->queue = disk.queue;
  strcpy(disk.gd->disk_name, DISK_NAME); 
  set_capacity(disk.gd, DISK_NR_SECTORS);
  add_disk(disk.gd);

  printk(KERN_INFO "%s: successfully loaded: disk=%s, major=%d\n", 
    THIS_MODULE->name, DISK_NAME, major);
  return 0;
}

static void __exit mod_exit(void) {
  del_gendisk(disk.gd);
  blk_cleanup_queue(disk.queue);
  unregister_blkdev(major, DISK_NAME);
  vfree(disk.data);
  printk(KERN_INFO "%s: successfully release all resources\n", THIS_MODULE->name);
}

module_init(mod_init);
module_exit(mod_exit);
