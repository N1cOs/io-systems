KVERSION = $(shell uname -r)

obj-m += lab1.o

all:
	make --directory /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make --directory /lib/modules/$(KVERSION)/build M=$(PWD) clean
