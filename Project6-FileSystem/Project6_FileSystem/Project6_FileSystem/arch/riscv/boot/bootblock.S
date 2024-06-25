#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>
// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50301000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50301000


.text
.global main

main:
	// fence on all memory and I/O
	fence
	// a0 is mhartid, a1 is the pointer of fdt
	move s0,a0
	move s1,a1

	.option push
    .option norelax
    la gp, __global_pointer$
    .option pop
	
	// print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR
	
	// The core whose mhartid=0 is selected to boot the kernel.
	// Other cores should wait until the kernel's initialization
	// has been finished.
 	move a0, s0
	bnez a0, secondary

	// 1) task1 call BIOS print string "It's bootblock!"
	la a0, msg
	SBI_CALL SBI_CONSOLE_PUTSTR


	// 2) task2 call BIOS read kernel in SD card and jump to kernel start
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

secondary:

	//1. Mask all interrupts
	csrw CSR_SIE, zero
	csrw CSR_SIP, zero
	//2. let stvec pointer to kernel_main
	la t0, kernel_main
    csrw CSR_STVEC, t0
	//3. enable software interrupt for ipi
	li t0, SIE_SSIE
    csrs CSR_SIE, t0 
    li t0, SR_SIE
    csrw CSR_SSTATUS, t0
	
	fence.i
wait_for_wakeup:
	wfi
	j wait_for_wakeup
	

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

