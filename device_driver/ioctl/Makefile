obj-m = ioctl.o

KDIR = /lib/modules/$(shell uname -r)/build

PWD =`pwd`

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
