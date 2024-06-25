#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // map phys_addr to a virtual address
    // then return the virtual address
    
    uintptr_t pgdir = 0xffffffc05e000000;
    uint64_t ret_val = io_base;
    while(size != 0){
        uintptr_t va = io_base;

        va &= VA_MASK;
        uint64_t vpn2 = va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
        uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
        uint64_t vpn0 = ((vpn1 << PPN_BITS) | (vpn2 << 2*PPN_BITS)) ^
                    (va >> (NORMAL_PAGE_SHIFT));
    
        PTE *fst_pg = (PTE *)pgdir + vpn2;
        PTE *snd_pg = NULL;
        PTE *thr_pg = NULL;
        int vpn2_valid = (*fst_pg) & _PAGE_PRESENT;
        int vpn1_valid;
        int vpn0_valid;

        if (!vpn2_valid){
            ptr_t new_page_vaddr = allocPage(NULL);
            ptr_t new_page_paddr = kva2pa(new_page_vaddr);
            set_pfn(fst_pg, new_page_paddr >> NORMAL_PAGE_SHIFT);
            set_attribute(fst_pg, _PAGE_PRESENT);
            clear_pgdir(pa2kva(get_pa(*fst_pg)));
        }
        snd_pg = (PTE*)pa2kva(get_pa(*fst_pg)) + vpn1;
        vpn1_valid = (*snd_pg) & _PAGE_PRESENT;
        if (!vpn1_valid){
            ptr_t new_page_vaddr = allocPage(NULL);
            ptr_t new_page_paddr = kva2pa(new_page_vaddr);
            set_pfn(snd_pg, new_page_paddr >> NORMAL_PAGE_SHIFT);
            set_attribute(snd_pg, _PAGE_PRESENT);
            clear_pgdir(pa2kva(get_pa(*snd_pg)));
        }
        thr_pg = (PTE*)pa2kva(get_pa(*snd_pg)) + vpn0;
        set_pfn(thr_pg, phys_addr >> NORMAL_PAGE_SHIFT);
        set_attribute(thr_pg, _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                        _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
        io_base += PAGE_SIZE;
        phys_addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
    local_flush_tlb_all();
    return ret_val;    
}

void iounmap(void *io_addr)
{
    // TODO: a very naive iounmap() is OK
    // maybe no one would call this function?
    // *pte = 0;
}
