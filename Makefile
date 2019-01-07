obj-m += test.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

ins:
	make
	sudo dmesg -C
	sudo insmod test.ko

rm:
	dmesg
	sudo rmmod test