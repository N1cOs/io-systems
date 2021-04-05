#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/moduleparam.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <net/arp.h>
#include <linux/icmp.h>
#include <linux/proc_fs.h>

#define PROC_NAME "var1"

#define INTERFACE_FORMAT_NAME "vni%d"
#define PARENT_INTERFACE_NAME "enp0s3"

#define BUF_SIZE 255
#define PACKET_SIZE 1500


struct priv {
    struct net_device *parent;
    struct net_device_stats stats;
};
