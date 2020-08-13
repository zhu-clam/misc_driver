
PWD := $(shell pwd)
BIN := $(PWD)/../bin

SRCDIR :=$(PWD)

obj-m	+= dtbinfo.o
dtbinfo-objs := devicetree.o 

EXTRA_CFLAGS     += -Wall

default:
	$(MAKE) -C $(kdir)  M=$(SRCDIR) modules
	@cp dtbinfo.ko $(BIN)
clean:
	rm *.ko *.o *.mod* -rfv
