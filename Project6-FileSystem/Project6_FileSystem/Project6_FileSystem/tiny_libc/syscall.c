#include <sys/syscall.h>
#include <sys/syscall_number.h>
#include <stdint.h>

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, buff, IGNORE, IGNORE, IGNORE); 
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(void)
{
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{  
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE, IGNORE); 
}

long sys_get_timebase()
{
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE);
   
}

long sys_get_tick()
{
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE);
}
    

void sys_yield()
{
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE);    
}

int sys_getchar()
{
    return invoke_syscall(SYSCALL_GETCHAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_prior(int prior)
{
    invoke_syscall(SYSCALL_PRIOR, prior, IGNORE, IGNORE, IGNORE);
}

uint32_t sys_get_wall_time(uint32_t* time_elapsed)
{
    return invoke_syscall(SYSCALL_GET_TIME, time_elapsed, IGNORE, IGNORE, IGNORE);
}

void sys_process_show()
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_kill(pid_t pid)
{
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE, IGNORE);
}

void sys_exit(void)
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid)
{
    return invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid()
{
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_putchar(char ch)
{
    invoke_syscall(SYSCALL_PUTCHAR, ch, IGNORE, IGNORE, IGNORE);
}

void* shmpageget(int key){
    return invoke_syscall(SYSCALL_SHMPAGEGET, key, IGNORE, IGNORE, IGNORE);
}

void shmpagedt(void *addr){
    invoke_syscall(SYSCALL_SHMPAGEDT, addr, IGNORE, IGNORE, IGNORE);
}

pid_t sys_exec(const char *file_name, int argc, char* argv[]){
    return invoke_syscall(SYSCALL_EXEC, file_name, argc, argv, IGNORE);
}


void sys_show_exec(){
    invoke_syscall(SYSCALL_SHOW_EXEC, IGNORE, IGNORE, IGNORE, IGNORE);
}

int binsemget(int key)
{
    return invoke_syscall(SYSCALL_BINSEMGET, key, IGNORE, IGNORE, IGNORE);
}

int binsemop(int binsem_id, int op)
{
    return invoke_syscall(SYSCALL_BINSEMOP, binsem_id, op, IGNORE, IGNORE);
}


long sys_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{   
    return invoke_syscall(SYSCALL_NET_RECV, addr, length, num_packet, frLength);
}

void sys_net_send(uintptr_t addr, size_t length){
    invoke_syscall(SYSCALL_NET_SEND, addr, length, IGNORE, IGNORE);
}

void sys_net_irq_mode(int mode){
    invoke_syscall(SYSCALL_NET_IRQ_MODE, mode, IGNORE, IGNORE, IGNORE);
}

void sys_mkfs(){
    invoke_syscall(SYSCALL_MKFS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_statfs(){
    invoke_syscall(SYSCALL_STATFS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_cd(char* file_name){
    invoke_syscall(SYSCALL_CD, file_name, IGNORE, IGNORE, IGNORE);
}

void sys_mkdir(char* file_name){
    invoke_syscall(SYSCALL_MKDIR, file_name, IGNORE, IGNORE, IGNORE);
}

void sys_rmdir(char* file_name){
    invoke_syscall(SYSCALL_RMDIR, file_name, IGNORE, IGNORE, IGNORE);
}

void sys_ls(char *file_name){
    invoke_syscall(SYSCALL_LS, file_name, IGNORE, IGNORE, IGNORE);
}

void sys_touch(char* file_name){
    invoke_syscall(SYSCALL_TOUCH, file_name, IGNORE, IGNORE, IGNORE);
}

void sys_cat(char* file_name){
    invoke_syscall(SYSCALL_CAT, file_name, IGNORE, IGNORE, IGNORE);
}

int sys_fopen(char* name, int access){
    return invoke_syscall(SYSCALL_FOPEN, name, access, IGNORE, IGNORE);
}

int sys_fwrite(int fd, char* buff, int size){
    return invoke_syscall(SYSCALL_FWRITE, fd, buff, size, IGNORE);
}

int sys_fread(int fd, char* buff, int size){
    return invoke_syscall(SYSCALL_FREAD, fd, buff, size, IGNORE);
}

void sys_close(int fd){
    invoke_syscall(SYSCALL_FCLOSE, fd, IGNORE, IGNORE, IGNORE);
}

int sys_lseek(int fd, int offset, int whence)
{
    return invoke_syscall(SYSCALL_LSEEK, fd, offset, whence, IGNORE);
}

void sys_ln(char *src, char *dst)
{
    invoke_syscall(SYSCALL_LN, src, dst, IGNORE, IGNORE);
}

void sys_rm(char *name)
{
    invoke_syscall(SYSCALL_RM, name, IGNORE, IGNORE, IGNORE);
}