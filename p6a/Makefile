
PWD := $(shell pwd)
BIN := $(PWD)/../bin

SRCDIR :=$(PWD)

obj-m	+= p6airq.o
p6airq-objs := p6aevent.o 

EXTRA_CFLAGS     += -Wall

default:
	$(MAKE) -C $(kdir)  M=$(SRCDIR) modules
	@cp p6airq.ko $(BIN)
clean:
	rm *.ko *.o *.mod* -rfv
