
LD = $(TARGET)-ld
CC = $(TARGET)-gcc
AR = $(TARGET)-ar

ifneq ($(KERNELRELEASE),)
	obj-m := syf-pwm.o

else

KERNELSRC ?= $(CROSS_KERNEL)
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELSRC) M=$(PWD) modules

endif

clean:
	rm *.o *.ko *.mod.c modules.order Module.symvers

