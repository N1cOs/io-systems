KVERSION = $(shell uname -r)

obj-m += lab2.o

all:
	make --directory /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make --directory /lib/modules/$(KVERSION)/build M=$(PWD) clean


PARTITIONS = lab2p1 lab2p5 lab2p6

define setup_partition
	mkfs.vfat /dev/$(1)
	mkdir /mnt/$(1)
	mount /dev/$(1) /mnt/$(1);
endef

define umount_partition
	umount /mnt/$(1)
	rmdir /mnt/$(1);
endef


.PHONY: setup
setup:
	$(foreach part,$(PARTITIONS),$(call setup_partition,$(part)))

.PHONY: umount
umount:
	$(foreach part,$(PARTITIONS),$(call umount_partition,$(part)))
