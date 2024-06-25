#include <asm/sbidef.h>
#include <asm/sbiasm.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50201000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50201000

// 3. move bootloader to this address
.equ new_bootloader, 0x5eff0000

.text
.global main

main:
	// fence on all memory and I/O
	fence

	// print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// 1) task1 call BIOS print string "It's bootblock!"
	la a0, msg
	SBI_CALL SBI_CONSOLE_PUTSTR


	// 2) task2 call BIOS read kernel in SD card and jump to kernel start
	lui  a0,     %hi(new_bootloader)
	addi a0, a0, %lo(new_bootloader)
	li	 a1, 1
	li	 a2, 0
	SBI_CALL SBI_SD_READ
	fence.i
	la	 t0, main
	la	 t1, jmp
	sub  t1, t1, t0
	la	 t2, new_bootloader
	add  t1, t1, t2
	jr   t1

jmp:
	lui  a0,     %hi(kernel)
	addi a0, a0, %lo(kernel)
	la	 a3, os_size_loc
	lh	 a1, (a3)
	li	 a2, 1
	SBI_CALL SBI_SD_READ
	fence.i
	lui	 t0, 	 %hi(kernel_main)
	addi t0, t0, %lo(kernel_main)
	jr	 t0

	

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

