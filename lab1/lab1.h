#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/fs.h>

#define MODULE_NAME "lab1"
#define PROC_NAME "var1"
#define DEVICE_NAME "var1"
#define DEVICE_COUNT 1

int init_device(void);
int init_proc(void);
void destroy_device(void);
void destroy_proc(void);

ssize_t dev_read(struct file *f, char __user *buf, size_t len, loff_t *off);
ssize_t dev_write(struct file *f, const char __user *buf, size_t len, loff_t *off);

ssize_t proc_read(struct file *f, char __user *buf, size_t len, loff_t *off);

uint32_t utf8_length(char *str, size_t bytes);
