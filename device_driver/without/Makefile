obj-m :=both_without.o
both_without-objs := without_init.o without_exit.o


KDIR:=/lib/modules/3.13.0-93-generic/build

PWD:=$(shell pwd)

default : 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD)
clean : 
	$(MAKE) clean -C $(KDIR) SUBDIRS=$(PWD)
