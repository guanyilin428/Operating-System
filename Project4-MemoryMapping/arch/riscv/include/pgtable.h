#ifndef PGTABLE_H
#define PGTABLE_H

#include <type.h>
#include <sbi.h>
#include <os/string.h>
#include <type.h>

#define SATP_MODE_SV39 8
#define SATP_MODE_SV48 9

#define SATP_ASID_SHIFT 44lu
#define SATP_MODE_SHIFT 60lu

#define NORMAL_PAGE_SHIFT 12lu
#define NORMAL_PAGE_SIZE (1lu << NORMAL_PAGE_SHIFT)
#define LARGE_PAGE_SHIFT 21lu
#define LARGE_PAGE_SIZE (1lu << LARGE_PAGE_SHIFT)

/*
 * Flush entire local TLB.  'sfence.vma' implicitly fences with the instruction
 * cache as well, so a 'fence.i' is not necessary.
 */
static inline void local_flush_tlb_all(void)
{
    __asm__ __volatile__ ("sfence.vma" : : : "memory");
}

/* Flush one page from local TLB */
static inline void local_flush_tlb_page(unsigned long addr)
{
    __asm__ __volatile__ ("sfence.vma %0" : : "r" (addr) : "memory");
}

static inline void local_flush_icache_all(void)
{
    asm volatile ("fence.i" ::: "memory");
}

static inline void flush_icache_all(void)
{
    local_flush_icache_all();
    sbi_remote_fence_i(NULL);
}

static inline void flush_tlb_all(void)
{
    local_flush_tlb_all();
    sbi_remote_sfence_vma(NULL, 0, -1);
}
static inline void flush_tlb_page_all(unsigned long addr)
{
    local_flush_tlb_page(addr);
    sbi_remote_sfence_vma(NULL, 0, -1);
}

static inline void set_satp(
    unsigned mode, unsigned asid, unsigned long ppn)
{
    unsigned long __v =
        (unsigned long)(((unsigned long)mode << SATP_MODE_SHIFT) | ((unsigned long)asid << SATP_ASID_SHIFT) | ppn);
    __asm__ __volatile__("sfence.vma\ncsrw satp, %0" : : "rK"(__v) : "memory");
}

#define PGDIR_PA 0x5e000000lu  // use bootblock's page as PGDIR

/*
 * PTE format:
 * | XLEN-1  10 | 9             8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *       PFN      reserved for SW   D   A   G   U   X   W   R   V
 */

#define _PAGE_ACCESSED_OFFSET 6

#define _PAGE_PRESENT (1 << 0)
#define _PAGE_READ (1 << 1)     /* Readable */
#define _PAGE_WRITE (1 << 2)    /* Writable */
#define _PAGE_EXEC (1 << 3)     /* Executable */
#define _PAGE_USER (1 << 4)     /* User */
#define _PAGE_GLOBAL (1 << 5)   /* Global */
#define _PAGE_ACCESSED (1 << 6) /* Set by hardware on any access \
                                 */
#define _PAGE_DIRTY (1 << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT (1 << 8)     /* Reserved for software */

#define _PAGE_PFN_SHIFT 10lu

#define VA_MASK ((1lu << 39) - 1)

#define PPN_BITS 9lu
#define NUM_PTE_ENTRY (1 << PPN_BITS)
#define KPA_OFFSET 0xffffffc000000000lu
typedef uint64_t PTE;

static inline uintptr_t kva2pa(uintptr_t kva)
{
    return kva - KPA_OFFSET;
}

static inline uintptr_t pa2kva(uintptr_t pa)
{
    return pa + KPA_OFFSET;
}


static inline uint64_t get_pa(PTE entry)
{
    return (uint64_t)(entry>>_PAGE_PFN_SHIFT)<<NORMAL_PAGE_SHIFT;
}

/*
static inline uint64_t get_pa(PTE entry)
{ 
    uint64_t mask = ~((~0) << 54);
    return ((entry & mask) >> 10) << 12;
}
*/

static inline uintptr_t get_kva_of(uintptr_t va, uintptr_t pgdir_va)
{
    //
}

/* Get/Set page frame number of the `entry` */
static inline long get_pfn(PTE entry)
{
    
}
static inline void set_pfn(PTE *entry, uint64_t pfn)
{
    *entry |= pfn<<_PAGE_PFN_SHIFT;
}

/* Get/Set attribute(s) of the `entry` */
static inline long get_attribute(PTE entry, uint64_t mask)
{
    //return ;
}
static inline void set_attribute(PTE *entry, uint64_t bits)
{
    *entry |= bits;
}

static inline void clear_pgdir(uintptr_t pgdir_addr)
{
    uint8_t *pgdir = pgdir_addr;
    kmemset(pgdir, 0, NORMAL_PAGE_SIZE);
}

#endif  // PGTABLE_H
