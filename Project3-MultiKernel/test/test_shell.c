/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <test.h>
#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#define MAX_SIZE 100
struct task_info task_test_waitpid = {
    (uintptr_t)&wait_exit_task, USER_PROCESS};
struct task_info task_test_semaphore = {
    (uintptr_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task_test_barrier = {
    (uintptr_t)&test_barrier, USER_PROCESS};
    
struct task_info strserver_task = {(uintptr_t)&strServer, USER_PROCESS};
struct task_info strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

struct task_info task_test_multicore = {(uintptr_t)&test_multicore, USER_PROCESS};
struct task_info task_test_affinity = {(uintptr_t)&test_affinity, USER_PROCESS};
struct task_info task_test_randmbox = {(uintptr_t)&randmbox_task, USER_PROCESS};
static struct task_info *test_tasks[16] = {&task_test_waitpid,
                                           &task_test_semaphore,
                                           &task_test_barrier,
                                           &task_test_multicore,
                                           &strserver_task, &strgenerator_task,
                                           &task_test_randmbox,
                                           &task_test_affinity};
static int num_test_tasks = 8;

void null_task(){
    while(1) ;
}
#define SHELL_BEGIN 25

#define CMD_MAX_LENGTH 20
#define ARG_MAX_NUM 5
#define ARG_MAX_LENGTH 20

void shell_clear(){
    sys_screen_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
}

void shell_mask_exec(int id, int mask){
    pid_t pid = sys_spawn(test_tasks[id], NULL, AUTO_CLEANUP_ON_EXIT);
    sys_pid_mask_set(pid, mask);
    printf("successfully exec task%d, the process id is %d, the mask is %d\n", id, pid, mask);
}

void shell_exec(int id){
    pid_t pid = sys_spawn(test_tasks[id], NULL, AUTO_CLEANUP_ON_EXIT);
    printf("successfully exec task%d, the process id is %d\n", id, pid);
}

int myatoi(char buff[]){
    int i = 0;
    int sum = 0;
    for(i = 0; buff[i] != '\n' && buff[i]!='\r' && buff[i] != '\0'; i++){
        if(buff[i] > '0' && buff[i] < '9')
            sum = 10*sum + buff[i] - '0';
    }
    return sum;
}


void test_shell()
{
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    printf("> root@UCAS_OS: ");
    char input_buff[MAX_SIZE];
    char cmd[CMD_MAX_LENGTH] = {0};
    char arg[ARG_MAX_NUM][ARG_MAX_LENGTH] = {0};
    memset(arg, 0, ARG_MAX_NUM*ARG_MAX_LENGTH);

    while (1)
    {
        // TODO: call syscall to read UART port
        char ch;
        int i = 0;
        while(1){
            while((ch = sys_getchar()) == 255)
                ;
            if(ch == '\r' || i >= MAX_SIZE){
                sys_putchar('\n');
                break;
            }
            else if(ch == 8 || ch == 127){
                if(i){
                    i--;
                    sys_putchar(ch);
                }
            }
            else{
                input_buff[i++] = ch;
                sys_putchar(ch);
            }
        }
        input_buff[i] = '\0';

        // TODO: parse input
        char* parse = input_buff;
        parse = strtok(cmd, parse, ' ', CMD_MAX_LENGTH);
        int arg_idx = 0;
        while(parse = strtok(arg[arg_idx++], parse, ' ', ARG_MAX_LENGTH))
            ;
        int id = 0;
        pid_t pid;
        int mask;
        // TODO: ps, exec, kill, clear
        if(!strcmp(cmd, "ps")){
            sys_process_show();
        }else if(!strcmp(cmd, "exec")){
            id = myatoi(arg[0]);
            shell_exec(id);
        }else if(!strcmp(cmd, "kill")){
            id = myatoi(arg[0]);
            sys_kill(id);
        }else if(!strcmp(cmd, "clear")){
            shell_clear();
        }else if(!strcmp(cmd, "taskset")){
            if(!strcmp(arg[0], "-p")){
                pid = myatoi(arg[2]);
                mask = myatoi(arg[1]);
                sys_pid_mask_set(pid, mask);
            }else{
                id = myatoi(arg[1]);
                mask = myatoi(arg[0]);
                shell_mask_exec(id,mask);
            }
        }else{
              printf("UNKNOWN COMMAND.\n");
        }
        printf("> root@UCAS_OS: ");
    }
}
