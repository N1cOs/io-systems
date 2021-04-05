#include "lab3.h"

MODULE_AUTHOR("Nikita Karmatskikh");
MODULE_AUTHOR("Dmitriy Chistokhodov");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");

static struct net_device *ndev;

static u16 packets_len;
static u8 packets[BUF_SIZE][PACKET_SIZE];

static u8 last_proc_read;
static struct proc_dir_entry* proc;

static int check_frame(struct sk_buff *skb) {
  struct iphdr *ip = (struct iphdr *)skb_network_header(skb);
  if (ip->protocol == IPPROTO_ICMP) {
    u16 ip_hdr_len = ip->ihl * 4;
    struct icmphdr *icmp = (struct icmphdr*)((u8*)ip + ip_hdr_len);
    if (icmp->type == ICMP_ECHO) {
      u16 off = ip_hdr_len + sizeof(struct icmphdr);
      u16 data_len = ntohs(ip->tot_len) - off;

      u8 *packet = packets[packets_len % BUF_SIZE];
      packets_len++;

      u8 *icmp_data = (skb->data + off);
      memcpy(packet, icmp_data, data_len);
      packet[data_len] = '\0';

      return 1;
    }
  }
  return 0;
}

static rx_handler_result_t handle_frame(struct sk_buff **pskb) {
  struct sk_buff *skb = *pskb;
  struct priv *priv = netdev_priv(ndev);

  if (check_frame(skb)) {
    priv->stats.rx_packets++;
    priv->stats.rx_bytes += skb->len;
  }
  return RX_HANDLER_PASS;
}

static struct net_device_stats *ndev_stats(struct net_device *dev) {
  struct priv *priv = netdev_priv(dev);
  return &priv->stats;
}

static int ndev_open(struct net_device *dev) {
  netif_start_queue(dev);
  printk(KERN_INFO "%s: device opened: name=%s\n", THIS_MODULE->name, dev->name);
  return 0; 
} 

static int ndev_stop(struct net_device *dev) {
  netif_stop_queue(dev);
  printk(KERN_INFO "%s: device closed: name=%s\n", THIS_MODULE->name, dev->name);
  return 0; 
}

static netdev_tx_t ndev_start_xmit(struct sk_buff *skb, struct net_device *dev) {
  struct priv *priv = netdev_priv(dev);
  priv->stats.tx_packets++;
  priv->stats.tx_bytes += skb->len;

  if (priv->parent) {
    skb->dev = priv->parent;
    skb->priority = 1;
    dev_queue_xmit(skb);
  }

  return NETDEV_TX_OK;
}

static struct net_device_ops nops = {
  .ndo_open=ndev_open,
  .ndo_stop=ndev_stop,
  .ndo_get_stats=ndev_stats,
  .ndo_start_xmit=ndev_start_xmit,
};

static void ndev_setup(struct net_device *dev) {
  ether_setup(dev);
  memset(netdev_priv(dev), 0, sizeof(struct priv));
  dev->netdev_ops = &nops;
}

ssize_t proc_read(struct file *f, char __user *buf, size_t len, loff_t *off) {
  u8 packs = min(packets_len, BUF_SIZE);
  if (last_proc_read == packs) {
    last_proc_read = 0;
    return 0;
  }

  copy_to_user(buf, packets[last_proc_read], PACKET_SIZE);
  last_proc_read++;
  
  return PACKET_SIZE;
}

static struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.read = proc_read
};


int __init vni_init(void) {
  ndev = alloc_netdev(sizeof(struct priv), INTERFACE_FORMAT_NAME, NET_NAME_UNKNOWN, ndev_setup);
  if (ndev == NULL) {
    printk(KERN_ERR "%s: failed to allocate net device\n", THIS_MODULE->name);
    return -ENOMEM;
  }

  struct priv *priv = netdev_priv(ndev);
  priv->parent = __dev_get_by_name(&init_net, PARENT_INTERFACE_NAME);
  if (!priv->parent) {
    printk(KERN_ERR "%s: no such parent net device: %s\n", THIS_MODULE->name, PARENT_INTERFACE_NAME);
    free_netdev(ndev);
    return -ENODEV;
  }

  if (priv->parent->type != ARPHRD_ETHER && priv->parent->type != ARPHRD_LOOPBACK) {
    printk(KERN_ERR "%s: illegal type of parent net device\n", THIS_MODULE->name); 
    free_netdev(ndev);
    return -EINVAL;
  }

  memcpy(ndev->dev_addr, priv->parent->dev_addr, ETH_ALEN);
  memcpy(ndev->broadcast, priv->parent->broadcast, ETH_ALEN);
  
  int err = dev_alloc_name(ndev, ndev->name);
  if (err < 0) {
    printk(KERN_ERR "%s: failed to allocate name, error %i\n", THIS_MODULE->name, err);
    free_netdev(ndev);
    return -EIO;
  }

  if ((proc = proc_create(PROC_NAME, 0444, NULL, &proc_fops)) == NULL) {
    printk(KERN_ERR "%s: failed to create a proc entry: proc=%s\n", THIS_MODULE->name, PROC_NAME);
    free_netdev(ndev);
    return -EIO;
  }

  register_netdev(ndev);
  rtnl_lock();
  netdev_rx_handler_register(priv->parent, handle_frame, NULL);
  rtnl_unlock();
  
  printk(KERN_INFO "%s: successfully loaded\n", THIS_MODULE->name);
  return 0; 
}

void __exit vni_exit(void) {
  struct priv *priv = netdev_priv(ndev);
  if (priv->parent) {
    rtnl_lock();
    netdev_rx_handler_unregister(priv->parent);
    rtnl_unlock();
    printk(KERN_INFO "%s: unregister rx handler for %s\n", THIS_MODULE->name, priv->parent->name);
  }
  unregister_netdev(ndev);
  free_netdev(ndev);
  proc_remove(proc);
  printk(KERN_INFO "%s: successfully unloaded\n", THIS_MODULE->name); 
}

module_init(vni_init);
module_exit(vni_exit);
