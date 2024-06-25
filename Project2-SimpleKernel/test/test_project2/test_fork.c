#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <test2.h>
#include <sys/syscall.h>


void fork_task(){
    char child_prior;
    int pid;
    int pr_print_location = 3;
    int father_num = 0;
    int child = 0;
    //int child_num;
    //int ch_print_location;
    sys_move_cursor(1, pr_print_location - 1);
    printf("> [TASK] This task is to test fork.");
    sys_prior(0);
    while(1){
        while((child_prior = sys_getchar()) == 255){
            sys_move_cursor(1, pr_print_location);
            printf("This is father process(%d)", father_num++);
        }

        sys_move_cursor(1,11);
        pid = sys_fork();
        child++;
        if(!pid){
            sys_prior((int)(child_prior - '0'));
            while(1){
                sys_move_cursor(1, pr_print_location + child);
                printf("This is child process(%d)", father_num++);
            }
        }
    }
}
