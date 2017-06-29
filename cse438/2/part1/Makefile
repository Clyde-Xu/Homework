IOT_HOME = /opt/iot-devkit/1.7.3/sysroots
KDIR:=$(IOT_HOME)/i586-poky-linux/usr/src/kernel
PATH := $(PATH):$(IOT_HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux

CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-
SROOT=$(IOT_HOME)/i586-poky-linux/

obj-m = i2c_flash.o
APP = main

all:
	make ARCH=x86 CROSS_COMPILE=i586-poky-linux- -C $(KDIR) M=$(PWD) modules
	$(CC) -o $(APP) --sysroot=$(SROOT) -Wall main.c 

clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.c
	rm -f Module.symvers
	rm -f modules.order
	rm -f $(APP)
