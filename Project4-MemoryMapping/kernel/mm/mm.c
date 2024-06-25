#include <os/mm.h>
#include <pgtable.h>
#include <os/string.h>
#include <os/list.h>
#include <os/sched.h>

ptr_t memCurr = FREEMEM;
LIST_HEAD(RecyclePages);
LIST_HEAD(ShmPage);

void add_to_pg_list(uintptr_t va, pcb_t* p){
    pg_node_t* pg = (pg_node_t*)kmalloc(sizeof(pg_node_t));
    pg->page_vaddr = va;
    list_add_tail(&(pg->list), &(p->pg_list));
    p->pg_num++;
}

void freePage(uintptr_t kva){
    pg_node_t* node = (pg_node_t*)kmalloc(sizeof(pg_node_t));
    node->page_vaddr = kva;
    list_add_tail(&(node->list), &RecyclePages);
}
//return vaddr
ptr_t allocPage(pcb_t *pcb)
{
    // align PAGE_SIZE
    /*
    if(!list_empty(&RecyclePages)){
        pg_node_t* p = container_of(RecyclePages.next, pg_node_t, list);
        __list_del_entry(&(p->list));
        __list_init(&(p->list));
        list_add_tail(&(p->list), &(pcb->pg_list));
        pcb->pg_num++;
        return p->page_vaddr;
    }
    */
    ptr_t ret = ROUND(memCurr, PAGE_SIZE); // return the low addr ROUND(a, b): clear the lowest log(PAGE_SIZE) bit
    memCurr = ret + PAGE_SIZE;

    return ret;
}

void* kmalloc(size_t size)
{
    ptr_t ret = ROUND(memCurr, 4);
    memCurr = ret + size;
    return (void*)ret;
}


/* if this key does not existed 
   in ShmPage yet, return 0
   else, return the paddr */
shmpg_node_t* key_search_shmpg_list(int key){
    if(list_empty(&ShmPage))
        return NULL;
    list_node_t* tmp = ShmPage.next;
    shmpg_node_t* pg;
    for(; tmp != &ShmPage; tmp = tmp->next){
        pg = container_of(tmp, shmpg_node_t, list);
        if(pg->key == key){
            return pg;
        }
    }
    return NULL;
}

/* search the shmpg according to uva and pid */
shmpg_node_t* uva_search_shmpg_list(uintptr_t uva, pid_t pid){
    if(list_empty(&ShmPage))
        return NULL;
    list_node_t* tmp = ShmPage.next;
    shmpg_node_t* pg;
    for(; tmp != &ShmPage; tmp = tmp->next){
        pg = container_of(tmp, shmpg_node_t, list);
        
        // search the va_info list
        shmpg_info_t *info;
        list_node_t *q;
        for(q = pg->va_info.next; q != &(pg->va_info); q = q->next){
            info = container_of(q, shmpg_info_t, list);
            if(info->pid == pid && info->va == uva){
                __list_del_entry(q);
                return pg;
            }
        }
    }
    return NULL;
}

uintptr_t find_and_setmap(uintptr_t pgdir, uintptr_t pa){
    uintptr_t uva = SHM_VA_START;
    PTE* pte;
    for(; uva < USER_VA_TOP; uva += PAGE_SIZE){
        int res = search_last_pte(uva, pgdir, &pte);
        if(res == 1){ // level1 and level2 has been allocated
            if(*pte == 0){ // uva is spare
                set_pfn(pte, pa>>NORMAL_PAGE_SHIFT);
                set_attribute(pte, _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                    _PAGE_EXEC | _PAGE_USER);
                return uva;
            }
        }
    }
}

uintptr_t shm_page_get(int key)
{
    shmpg_node_t* shmpg = key_search_shmpg_list(key);
    if(shmpg == NULL){ // first invoke
        uintptr_t kva = allocPage(*current_running);
        uintptr_t pa = kva2pa(kva);

        // create a new shmpg_node
        shmpg_node_t* pg = (shmpg_node_t*)kmalloc(sizeof(shmpg_node_t));
        pg->key = key;
        pg->procnum = 0;
        pg->shmpg_paddr = pa;
        __list_init(&(pg->va_info));
        list_add_tail(&(pg->list), &ShmPage);

        shmpg = pg;
    }
    // find a spare uva
    // set up the mapping uva2paddr
    uintptr_t uva = find_and_setmap((*current_running)->pgdir, shmpg->shmpg_paddr);

    // add info_node to shmpg's va_info list
    shmpg_info_t* info = (shmpg_info_t*)kmalloc(sizeof(shmpg_info_t));
    info->pid = (*current_running)->pid;
    info->va = uva;
    list_add_tail(&(info->list), &(shmpg->va_info));
    shmpg->procnum++;

    return uva;
}

void shm_page_dt(uintptr_t addr)
{   
    // modify the pte to cancel mapping
    PTE* pte;
    int res = search_last_pte(addr, (*current_running)->pgdir, &pte);
    *pte = 0;

    // maintain the shmpg structure
    shmpg_node_t* shmpg = uva_search_shmpg_list(addr, (*current_running)->pid);
    shmpg->procnum--;
    if(shmpg->procnum == 0){
        freePage(pa2kva(shmpg->shmpg_paddr));
    }
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    memcpy((char*)dest_pgdir, (char*)src_pgdir, NORMAL_PAGE_SIZE);
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
*/

//寻找最后一级页表项，如果前两级页表项valid为零则返回零，否则返回1
int search_last_pte(uintptr_t va, uintptr_t pgdir, PTE** pte){
    va &= VA_MASK;
    uint64_t vpn2 = 
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    uint64_t vpn0 = ((vpn1 << PPN_BITS) | (vpn2 << 2*PPN_BITS)) ^
                    (va >> (NORMAL_PAGE_SHIFT));
    
    PTE *fst_pg = (PTE *)pgdir + vpn2;
    PTE *snd_pg = NULL;
    PTE *thr_pg = NULL;

    int vpn2_valid = (*fst_pg) & _PAGE_PRESENT;
    int vpn1_valid;

    if(!vpn2_valid)
        return 0;
    snd_pg = (PTE*)pa2kva(get_pa(*fst_pg)) + vpn1;
    vpn1_valid = (*snd_pg) & _PAGE_PRESENT;
    if (!vpn1_valid)
        return 0;
    thr_pg = (PTE*)pa2kva(get_pa(*snd_pg)) + vpn0;
    *pte = thr_pg;
    return 1;
}


//check if the va has been mapped; if ture, return the kva; else return 0;
uintptr_t check_alloc(uintptr_t va, uintptr_t pgdir){
    PTE *thr_pg = NULL;
    if(!search_last_pte(va, pgdir, &thr_pg)){
        return 0;
    }
    
    int vpn0_valid = *thr_pg & _PAGE_PRESENT;
    if (!vpn0_valid){
        return 0;
    }
    
    return pa2kva(get_pa(*thr_pg));
}

uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, int is_user, pcb_t* pcb)
{
    va &= VA_MASK;
    uint64_t vpn2 = 
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
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
        ptr_t new_page_vaddr = allocPage(pcb);
        ptr_t new_page_paddr = kva2pa(new_page_vaddr);
        set_pfn(fst_pg, new_page_paddr >> NORMAL_PAGE_SHIFT);
        set_attribute(fst_pg, (is_user<<4 & _PAGE_USER)|_PAGE_PRESENT);
        clear_pgdir(pa2kva(get_pa(*fst_pg)));
    }
    snd_pg = (PTE*)pa2kva(get_pa(*fst_pg)) + vpn1;
    vpn1_valid = (*snd_pg) & _PAGE_PRESENT;
    if (!vpn1_valid){
        ptr_t new_page_vaddr = allocPage(pcb);
        ptr_t new_page_paddr = kva2pa(new_page_vaddr);
        set_pfn(snd_pg, new_page_paddr >> NORMAL_PAGE_SHIFT);
        set_attribute(snd_pg, (is_user<<4 & _PAGE_USER)|_PAGE_PRESENT);
        clear_pgdir(pa2kva(get_pa(*snd_pg)));
    }
    thr_pg = (PTE*)pa2kva(get_pa(*snd_pg)) + vpn0;
    
    vpn0_valid = *thr_pg & _PAGE_PRESENT;
    if (vpn0_valid != 0)
        return pa2kva(get_pa(thr_pg));

    ptr_t new_page_vaddr = allocPage(pcb);
    ptr_t new_page_paddr = kva2pa(new_page_vaddr);
    set_pfn(thr_pg, new_page_paddr >> NORMAL_PAGE_SHIFT);
    set_attribute(thr_pg, _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                    _PAGE_EXEC | (is_user<<4) & _PAGE_USER);
    add_to_pg_list(va, pcb);
    
    return (new_page_vaddr >> NORMAL_PAGE_SHIFT) << NORMAL_PAGE_SHIFT;
}