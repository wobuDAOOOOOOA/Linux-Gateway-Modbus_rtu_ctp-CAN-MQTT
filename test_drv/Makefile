obj-m += bmp280_drv.o

KDIR := /home/wdz/sdk/sysdrv/source/kernel
PWD  := $(shell pwd)

# 手动指定与板子匹配的内核配置
# EXTRA_CFLAGS += -mthumb -D__thumb2__ -D__LINUX_ARM_ARCH__=7

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-rockchip830-linux-uclibcgnueabihf- CONFIG_SMP=n CONFIG_THUMB2_KERNEL=y

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean