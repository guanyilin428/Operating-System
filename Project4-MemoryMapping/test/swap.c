#include <stdio.h>
#include <stdint.h>
#include <sys/syscall.h>

int main(int argc, char* argv[]){
    sys_move_cursor(1,1);
    int data = 0;
	uintptr_t mem[5];
	int i;
    for(i = 1; i < argc; i++){
        mem[i] = atol(argv[i]);
        *(int*)mem[i] = ++data;
        printf("%d> write %d into the address at 0x%lx\n", data, data, mem[i]);
        sys_sleep(1);
    }

    //validify
    for(i = 1; i < argc; i++){
        if(*(int*)mem[i] == i){
            printf("0x%lx stores the right data.\n", mem[i]);
        }
    }
    printf("test success\n");
    return 0;
}