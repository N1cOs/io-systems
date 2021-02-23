#include "lab1.h"
#include "vector.h"

#define INIT_VECTOR_CAP 10

MODULE_AUTHOR("Nikita Karmatskikh");
MODULE_AUTHOR("Dmitriy Chistokhodov");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");

static dev_t num;
static struct class *dev_class;
static struct cdev device;

static struct proc_dir_entry* proc;
static uint32_t last_proc_read;

static vector *results;

static struct file_operations dev_fops = {
	.owner = THIS_MODULE,
	.read = dev_read,
	.write = dev_write
};

static struct file_operations proc_fops = {
	.owner = THIS_MODULE,
	.read = proc_read
};

static int __init mod_init(void) {
  results = new_vector(INIT_VECTOR_CAP);
  if (results == NULL) {
    return -1;
  }

  if (init_proc() < 0) {
    destroy_vector(results);
    return -1;
  }

  if (init_device() < 0) {
    destroy_proc();
    destroy_vector(results);
    return -1;
  }

  printk(KERN_INFO "%s: successfully started: device=%s, major=%d, proc=%s\n", 
    MODULE_NAME, DEVICE_NAME, MAJOR(num), PROC_NAME);
  return 0;
}

static void __exit mod_exit(void) {
  destroy_device();
  destroy_proc();
  destroy_vector(results);
  printk(KERN_INFO "%s: successfully release all resources\n", MODULE_NAME);
}

module_init(mod_init);
module_exit(mod_exit);

int init_device(void) {
  unsigned int minor = 0;
  if (alloc_chrdev_region(&num, minor, DEVICE_COUNT, DEVICE_NAME) < 0) {
    printk(KERN_ERR "%s: failed to allocate a char major number: device=%s\n", MODULE_NAME, DEVICE_NAME);
    return -1;
  }
  if ((dev_class = class_create(THIS_MODULE, "chardrv")) == NULL) {		
    printk(KERN_ERR "%s: failed to create device class: device=%s\n", MODULE_NAME, DEVICE_NAME);
    unregister_chrdev_region(num, DEVICE_COUNT);
    return -1;
  }
  if (device_create(dev_class, NULL, num, NULL, DEVICE_NAME) == NULL) {
    printk(KERN_ERR "%s: failed to create char device: device=%s\n", MODULE_NAME, DEVICE_NAME);
    class_destroy(dev_class);
    unregister_chrdev_region(num, DEVICE_COUNT);
    return -1;
  }
  cdev_init(&device, &dev_fops);
  if (cdev_add(&device, num, DEVICE_COUNT) < 0) {
    printk(KERN_ERR "%s: failed to add char device to the system: device=%s\n", MODULE_NAME, DEVICE_NAME);
    device_destroy(dev_class, num);
    class_destroy(dev_class);
    unregister_chrdev_region(num, DEVICE_COUNT);
    return -1;
  }
  return 0;
}

void destroy_device(void) {
  cdev_del(&device);
  device_destroy(dev_class, num);
  class_destroy(dev_class);
  unregister_chrdev_region(num, DEVICE_COUNT);
}

int init_proc(void) {
  if ((proc = proc_create(PROC_NAME, 0444, NULL, &proc_fops)) == NULL) {
    printk(KERN_ERR "%s: failed to create a proc entry: proc=%s\n", MODULE_NAME, PROC_NAME);
    return -1;
  }
  return 0;
}

void destroy_proc(void) {
  proc_remove(proc);
}

ssize_t dev_read(struct file *f, char __user *buf, size_t len, loff_t *off) {
  if (results == NULL) {
     printk(KERN_ERR "%s: failed to read results from null pointer\n", MODULE_NAME);
     return -1;
  }

  uint32_t i;
  for (i = 0; i < results->len; i++) {
    printk(KERN_DEBUG "%s: %d: result: %d\n", MODULE_NAME, i, results->buf[i]);
  }

  return 0;
}

ssize_t dev_write(struct file *f, const char __user *buf, size_t len, loff_t *off) {
  char str[len]; // note: limit maximum size of input buffer
  copy_from_user(str, buf, len);
  append(results, utf8_length(str, len));
  return len;
}

// note: kind of naive implementation because client must do syscall for each result
ssize_t proc_read(struct file *f, char __user *buf, size_t len, loff_t *off) {
  if (results == NULL) {
    printk(KERN_ERR "%s: failed to read values from null results\n", MODULE_NAME);
    return -1;
  }

  if (last_proc_read == results->len) {
    last_proc_read = 0;
    return 0;
  }

  char res[12]; // max 32-bit number (10 chars) + null-terminator + newline
  int res_len = sprintf(res, "%d\n", results->buf[last_proc_read]);
  
  copy_to_user(buf, res, res_len);
  last_proc_read++;
  return res_len;
}

uint32_t utf8_length(char *str, size_t bytes) {
  if (str == NULL) {
    printk(KERN_ERR "%s: failed to calculate length of null string\n", MODULE_NAME);
    return 0;
  }

  size_t cur = 0;
  uint32_t len = 0;
  while (cur != bytes) {
    /* 1 byte:  0xxxxxxx
       2 bytes: 110xxxxx 10xxxxxx
       3 bytes: 1110xxxx 10xxxxxx 10xxxxxx
       4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    */
    if ((str[cur] & 0xc0) != 0x80) {
      len++;
    }
    cur++;
  }
  return len;
}


vector* new_vector(uint32_t cap) {
  vector *v = kmalloc(sizeof(vector), GFP_USER);
  if (v == NULL) {
    printk(KERN_ERR "%s: failed to allocate memory for vector\n", MODULE_NAME);
    return NULL;
  }

  v->buf = kmalloc(VECTOR_ELEMENT_SIZE * cap, GFP_USER);
  if (v->buf == NULL) {
    printk(KERN_ERR "%s: failed to allocate memory for vector buffer\n", MODULE_NAME);
    kfree(v);
    return NULL;
  }

  v->len = 0;
  v->cap = cap;
  return v;
}

void append(vector *v, uint32_t val) {
  if (v == NULL) {
    printk(KERN_ERR "%s: failed to append to null vector\n", MODULE_NAME);
    return;
  }

  if (v->len == v->cap) {
    uint32_t cap = v->cap * 2;
    uint32_t *buf = kmalloc(cap * VECTOR_ELEMENT_SIZE, GFP_USER);
    if (buf == NULL) {
      printk(KERN_ERR "%s: failed to allocate memory for bigger vector buffer\n", MODULE_NAME);
      return;
    }
    
    memcpy(buf, v->buf, VECTOR_ELEMENT_SIZE * v->cap);
    kfree(v->buf);

    v->buf = buf;
    v->cap = cap;
  }

  v->buf[v->len] = val;
  v->len++;
}

void destroy_vector(vector *v) {
  if (v == NULL) {
    printk(KERN_ERR "%s: failed to destroy null vector\n", MODULE_NAME);
    return;
  }

  kfree(v->buf);
  kfree(v);
}
