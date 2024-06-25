#include <stdio.h>
#include <sys/syscall.h>



void fork_task(){
    char child_prior;
    int pid;
    int pr_print_location = 3;
    int ch_print_location = 4;
    int father_num = 0;
    int child_num;
    sys_move_cursor(1, pr_print_location - 1);
    printf("> [TASK] This task is to test fork.");
    sys_prior(0);

    while((child_prior = (unsigned char)sys_getchar()) < '0' || child_prior > '9')
    {
        sys_move_cursor(1, pr_print_location);
        printf("This is father process(%d)", father_num++);
    }  

    pid = sys_fork();
    if(!pid){
        child_num = father_num;
        sys_prior(child_prior - '0');
        while(1){
            sys_move_cursor(1, ch_print_location);
            printf("This is child process(%d)", child_num++);
        }
    }
    else{
        while(1){
            sys_move_cursor(1, pr_print_location);
            printf("This is father process(%d)", father_num++);
        }
    }       
}
