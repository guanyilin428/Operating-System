CC = riscv64-unknown-linux-gnu-gcc
HOST_CC = gcc
# CFLAGS = -O2  -fno-pic -fno-builtin -nostdinc -N -o bootblock bootblock.s -nostdlib -e main -Wl,-m -T riscv.lds
CFLAGS =  -O2  -fno-builtin -nostdlib -T riscv.lds -Iinclude -Wall -mcmodel=medany
DISK = /dev/sdb

BOOTLOADER_ENTRYPOINT = 0x50200000
KERNEL_ENTRYPOINT = 0x50201000

all: image

bootblock: bootblock.S riscv.lds
	${CC} ${CFLAGS} -o bootblock bootblock.S -e main -Ttext=${BOOTLOADER_ENTRYPOINT}

kernel0: kernel0.c head.S riscv.lds
	${CC} ${CFLAGS} -o kernel0 kernel0.c head.S -Ttext=${KERNEL_ENTRYPOINT}

kernel1: kernel1.c head.S riscv.lds
	${CC} ${CFLAGS} -o kernel1 kernel1.c head.S -Ttext=${KERNEL_ENTRYPOINT}

createimage_ccore: createimage_ccore.c
	${HOST_CC} createimage_ccore.c -o createimage_ccore -ggdb -Wall

image: bootblock kernel0 kernel1 createimage_ccore
	./createimage_ccore --extended bootblock kernel0 kernel1

clean:
	rm -rf bootblock image kernel1 kernel0 *.o createimage_ccore

floppy:
	sudo fdisk -l ${DISK}
	sudo dd if=image of=${DISK}2 conv=notrunc
