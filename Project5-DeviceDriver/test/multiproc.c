#include <sys/syscall.h>
#include <sys/shm.h>
#include <stdatomic.h>
#include <time.h>
#include <mthread.h>
#define ADD_MAX_TIMES 10000
#define LOCK_KEY 42
#define SHMPG_KEY 42

int main(int argc, char* argv[]){
    char *prog_name = argv[0];
    int print_location = 1;
    int is_first = 1;
    if (argc > 1) {
        is_first = atol(argv[2]);
        print_location = atol(argv[1]);
    }
    clock_t start;
    int* share = (int*)shmpageget(SHMPG_KEY);    
    int binsemid = binsemget(LOCK_KEY);

    if(is_first){
        *share = 0;
        char* sub_task_args[3];
        sub_task_args[0] = prog_name;
        sub_task_args[1] = "2";
        sub_task_args[2] = "0";
        sys_exec(prog_name, 3, sub_task_args);
        start = clock();
    }

    while(*share < ADD_MAX_TIMES){
        binsemop(binsemid, BINSEM_OP_LOCK);
        (*share)++;
        binsemop(binsemid, BINSEM_OP_UNLOCK);
    }

    if(is_first){
        clock_t end = clock();
        printf("the result is %d\n", *share);
        printf("the multi-proc uses: %ld ticks\n", end - start);
    }

    return 0;
}