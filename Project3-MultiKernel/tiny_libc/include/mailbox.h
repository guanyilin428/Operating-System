#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#include <stddef.h>
#define MAX_MBOX_LENGTH (64)

// TODO: please define mailbox_t;
// mailbox_t is just an id of kernel's mail box.
typedef int mailbox_t;

typedef struct Msg_info{
    char* buff;
    int length;
}msg_info;

typedef struct op_info{
    int op_type;  // 1 is send, 0 is recv
    int times;
}op_info;

extern int clientSendMsg(mailbox_t *mq, const char* content, int length);
extern uint32_t adler32(unsigned char *data, size_t len);
extern void generateRandomString(char* buf, int len);



mailbox_t* mbox_open(char *name);
void mbox_close(mailbox_t*);
int mbox_send(mailbox_t*, void *, int);
int mbox_recv(mailbox_t*, void *, int);
int do_mailbox_op(mailbox_t* mailbox, msg_info* msg, op_info* op);
#endif
