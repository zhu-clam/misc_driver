CFLAGS += -O0
CROSS_COMPILE=csky-linux-gnuabiv2-
CC=${CROSS_COMPILE}gcc
SRCS	:= $(wildcard ./*.c)
OBJS	:= $(patsubst %.c,%.o,${SRCS})
LIBNAME :=libgvdma.so
all: $(OBJS)
	$(CC) -fPIC -shared -o $(LIBNAME) ${SRCS}
	cp $(LIBNAME) ../target
	cp *.h ../target
%.o:%.c
	@$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -rf *.o *.so *.a