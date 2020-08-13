#!/bin/sh -

EXEC_FILES=`ls -F | grep *$ | grep -v build.sh | grep -v gv_dma_api| sed "s/*//g" `
echo ${EXEC_FILES}
for i in $EXEC_FILES
do
	rm $i
done

LIB=-lpthread
CURDIR=`pwd`
LIBDIR=${CURDIR}/../../drv_api/target
INCDIR=${CURDIR}/../../drv_api/target

csky-linux-gnuabiv2-gcc -o dtbinfo dtbinfo.c $LIB
csky-linux-gnuabiv2-gcc -D_GNU_SOURCE -o p6airq p6airq.c $LIB
csky-linux-gnuabiv2-gcc -D_GNU_SOURCE -o p6birq p6birq.c $LIB
csky-linux-gnuabiv2-gcc -D_GNU_SOURCE -o c5airq c5airq.c $LIB
csky-linux-gnuabiv2-gcc -o axidma gv_dma_api.c $LIB -L$LIBDIR -I$INCDIR -lgvdma 
csky-linux-gnuabiv2-gcc -o testPolarisMemAlloc testPolarisMemAlloc.c $LIB

cp dtbinfo p6airq p6birq c5airq axidma testPolarisMemAlloc ../bin

