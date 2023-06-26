obj-m:= unimem_main.o
unimem_main-objs:= unimem_main2.o unimem_ops.o
KDIR    := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

