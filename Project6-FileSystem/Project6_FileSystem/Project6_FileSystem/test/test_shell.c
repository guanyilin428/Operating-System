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


#define SHELL_BEGIN 25

#define CMD_MAX_LENGTH 20
#define ARG_MAX_NUM 5
#define ARG_MAX_LENGTH 20

void shell_clear(){
    sys_screen_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
}

void shell_exec(const char* file_name, int argc, char* argv[]){
    pid_t pid = sys_exec(file_name, argc, argv);
    printf("successfully exec task %s, the process id is %d\n", file_name, pid);
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

char *strtok(char *substr, char *str, const char delim, int length)
{
    int len = strlen(str);
    int i;
    if (len == 0)
        return NULL;
    for (i = 0; i < len; i++){
        if (str[i] != delim){
            if(i < length){
                substr[i] = str[i];
            }
        }
        else{
            substr[i] = 0;
            return &str[i + 1];
        }
    }
    return &str[i + 1];
}


void main()
{
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    printf("> root@UCAS_OS: ");
    char input_buff[MAX_SIZE];
    char cmd[CMD_MAX_LENGTH] = {0};
    char arg[ARG_MAX_NUM][ARG_MAX_LENGTH] = {0};

    int arg_idx = 0;
    while (1)
    {
        memset(input_buff, 0, MAX_SIZE);
        memset(cmd, 0, CMD_MAX_LENGTH);
        char ch;
        int i = 0;
        while(1){
            while((ch = sys_getchar()) == 255)
                ;
            if(ch == '\r' || ch == '\n' || i >= MAX_SIZE){
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

        arg_idx = 0;
        memset(arg, 0, ARG_MAX_NUM*ARG_MAX_LENGTH);
        char* parse = input_buff;
        parse = strtok(cmd, parse, ' ', CMD_MAX_LENGTH);

        int id = 0;
        char name[ARG_MAX_LENGTH];
        // ps, exec, kill, clear, ls
        if(!strcmp(cmd, "ps")){
            sys_process_show();
        }else if(!strcmp(cmd, "show")){
            sys_show_exec();  // modify ls!!!!!!!!!!!
        }else if(!strcmp(cmd, "ls")){
            sys_ls(parse);
        }else if(!strcmp(cmd, "exec")){
            while(parse = strtok(arg[arg_idx++], parse, ' ', ARG_MAX_LENGTH))
                ;
            strcpy(name, arg[0]);
            char* argv[]={&arg[0], &arg[1], &arg[2], &arg[3], &arg[4]};
            shell_exec(name, arg_idx-1, argv);
        }else if(!strcmp(cmd, "kill")){
            id = myatoi(arg[0]);
            sys_kill(id);
        }else if(!strcmp(cmd, "clear")){
            shell_clear();
        }else if(!strcmp(cmd, "mkfs")){
            sys_mkfs();
        }else if(!strcmp(cmd, "statfs")){
            sys_statfs();
        }else if(!strcmp(cmd, "cd")){
            sys_cd(parse);
        }else if(!strcmp(cmd, "mkdir")){
            sys_mkdir(parse);
        }else if(!strcmp(cmd, "rmdir")){
            sys_rmdir(parse);
        }else if(!strcmp(cmd, "touch")){
            sys_touch(parse);
        }else if(!strcmp(cmd, "cat")){
            sys_cat(parse);
        }else if(!strcmp(cmd, "ln")){
            while(parse = strtok(arg[arg_idx++], parse, ' ', ARG_MAX_LENGTH))
                ;
            sys_ln(&arg[0], &arg[1]);
        }else if(!strcmp(cmd, "rm")){
            sys_rm(parse);
        }else{
            printf("UNKNOWN COMMAND.\n");
        }
        printf("> root@UCAS_OS: ");
    }
}
