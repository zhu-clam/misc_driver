
PWD := $(shell pwd)
BIN := $(PWD)/../bin

SRCDIR :=$(PWD)

obj-m	+= gvclk.o
gvclk-objs := clock.o 

EXTRA_CFLAGS     += -Wall

default:
	$(MAKE) -C $(kdir)  M=$(SRCDIR) modules
	@cp gvclk.ko $(BIN)
clean:
	rm *.ko *.o *.mod* Module* modules* -rfv
