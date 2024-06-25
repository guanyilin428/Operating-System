#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* TODO: 
   * save exception_handler_entry into STVEC
   * enable global exceptions */
  la a0, exception_handler_entry
  csrw CSR_STVEC, a0
  li t0, SIE_STIE
  csrw CSR_SIE, t0
  jr ra
ENDPROC(setup_exception)
