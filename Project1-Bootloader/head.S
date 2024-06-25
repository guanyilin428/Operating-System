/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#define CSR_SIE			0x104
#define CSR_SIP			0x144
#define KERNEL_STACK		0x50500000

.section ".entry_function","ax"
.global _start
_start:
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop
  
  /* Clear BSS for flat non-ELF images */
  la t0, __bss_start
  la t1, __BSS_END__
  addi t2, t0, 4      /*t2 functions as i */
  li t3, 0            /*t3 functions as const_0 to be stored*/
another_loop: 
  sw   t3, (t2)
  addi t2, t2, 4
  bltu t2, t1, another_loop
  /* setup C environment (set sp register)*/
  lui   sp, %hi(KERNEL_STACK)
  addi  sp, sp,  %lo(KERNEL_STACK)
  /* Jump to the code in kernel.c*/
  j    main
loop:
  wfi
  j loop

