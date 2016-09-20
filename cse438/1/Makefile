obj-m += ht530_drv.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -pthread -o main main.c
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm main
