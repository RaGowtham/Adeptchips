obj-m=blink_led.o


CROSS_KDIR=/home/gowtham/adeptchips/uboot/linux-4.1

KDIR = /lib/modules/$(shell uname -r)/build

PWD=`pwd`

default:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean 
cross:
	make -C $(CROSS_KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabi-  modules 
