#include <mailbox.h>
#include <string.h>
#include <sys/syscall.h>
#include <os/syscall_number.h>
#define MAX_MBOX_SIZE 20
mailbox_t user_mb_arr[MAX_MBOX_SIZE];
mailbox_t* mbox_open(char *name)
{   
    int id = invoke_syscall(SYSCALL_MBOX_OPEN, name, IGNORE, IGNORE);
    user_mb_arr[id] = id;
    return &user_mb_arr[id];
}

void mbox_close(mailbox_t* mailbox)
{
    invoke_syscall(SYSCALL_MBOX_CLOSE, mailbox, IGNORE, IGNORE);
}

int mbox_send(mailbox_t* mailbox, void *msg, int msg_length)
{
    return invoke_syscall(SYSCALL_MBOX_SEND, mailbox, msg, msg_length);
}

int mbox_recv(mailbox_t* mailbox, void *msg, int msg_length)
{
    return invoke_syscall(SYSCALL_MBOX_RECV, mailbox, msg, msg_length);
}

int mbox_op(mailbox_t* mailbox, void* msg, void* op)
{
    return invoke_syscall(SYSCALL_MBOX_OP, mailbox, msg, op);
}
