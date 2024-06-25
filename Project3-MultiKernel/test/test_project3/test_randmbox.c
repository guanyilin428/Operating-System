#include <time.h>
#include <test3.h>
#include <mthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailbox.h>
#include <sys/syscall.h>
#include <os.h>
#define MBOX_TASK 3

struct MsgHeader
{
    int length;
    uint32_t checksum;
    pid_t sender;
};

typedef struct SendRecvByte{
    int send_bytes;
    int recv_bytes;
}bytes_info;

int PackHeader(mailbox_t *mq, const char* content, int length, op_info* op)
{
    int i;
    msg_info msg;
    char msgBuffer[MAX_MBOX_LENGTH] = {0};
    struct MsgHeader* header = (struct MsgHeader*)msgBuffer;
    char* _content = msgBuffer + sizeof(struct MsgHeader);
    header->length = length;
    header->checksum = adler32(content, length);
    header->sender = sys_getpid();
        
    for (i = 0; i < length; ++i) {
        _content[i] = content[i];
    }

    msg.buff = msgBuffer;
    msg.length = length + sizeof(struct MsgHeader);
    return mbox_op(mq, &msg, op);
}

int UserMboxOp(op_info *op, mailbox_t* send, mailbox_t* recv, bytes_info* byte){
    int len = 0;
    int strBuffer[MAX_MBOX_LENGTH - sizeof(struct MsgHeader)];
    struct MsgHeader header;
    char msgBuffer[MAX_MBOX_LENGTH];
    msg_info msg;
    int send_feedback, recv_feedback;
    if(op->op_type){ // send
        len = (rand() % ((MAX_MBOX_LENGTH - sizeof(struct MsgHeader))/2)) + 1;
        generateRandomString(strBuffer, len);
        send_feedback = PackHeader(send, strBuffer, len, op);
        if(send_feedback == 2){
            byte->send_bytes = len;
        }
        return send_feedback;
    }else{ // recv
        msg.buff = &header;
        msg.length = sizeof(struct MsgHeader);
        recv_feedback = mbox_op(recv, &msg, op);
        if(recv_feedback != 2)
            return recv_feedback;
        msg.buff = msgBuffer;
        msg.length = header.length;
        mbox_op(recv, &msg, op);
        byte->recv_bytes = header.length;

        return recv_feedback;
    }
}


void mbox_A(){
    mailbox_t* send, *recv = mbox_open("mbox_a");
    int sel;
    op_info op;
    int feedback;
    bytes_info byte;
    int64_t send_bytes_to_b = 0, send_bytes_to_c = 0, recv_bytes = 0;
    byte.send_bytes = byte.recv_bytes = 0;
    op.times = 0; 
    for(;;){
        sel = sys_get_tick() % 2;
        if(sel){
            send = mbox_open("mbox_b");
        }else{
            send = mbox_open("mbox_c");
        }
        feedback = UserMboxOp(&op, send, recv, &byte);

        if(feedback == 2){//success
            op.times = 0;
            if(sel == 1 && op.op_type == 1){ // send to B
                sys_move_cursor(0, 1);
                send_bytes_to_b += byte.send_bytes;
                printf("A send bytes:%ld to B", send_bytes_to_b);
            }else if(sel == 0 && op.op_type == 1){ // send to C
                sys_move_cursor(0, 2);
                send_bytes_to_c += byte.send_bytes;
                printf("A send bytes:%ld to C", send_bytes_to_c);
            }else if(op.op_type==0){ // A recv bytes
                sys_move_cursor(0, 3);
                recv_bytes += byte.recv_bytes;
                printf("A received bytes:%ld", recv_bytes);
            }
            op.op_type = rand() % 2;
        }
        else if(feedback == 3){
            op.times = 0;
            op.op_type = rand() % 2;
        }else{
            op.times = 1;
            op.op_type = feedback;
        }
    }
}


void mbox_B(){
   mailbox_t* send, *recv = mbox_open("mbox_b");
    int sel;
    op_info op;
    int feedback;
    bytes_info byte;
    int64_t send_bytes_to_a = 0, send_bytes_to_c = 0, recv_bytes = 0;
    byte.send_bytes = byte.recv_bytes = 0;
    op.times = 0; 
    for(;;){
        sel = sys_get_tick() % 2;
        if(sel){
            send = mbox_open("mbox_a");
        }else{
            send = mbox_open("mbox_c");
        }
        feedback = UserMboxOp(&op, send, recv, &byte);

        if(feedback == 2){//success
            op.times = 0;
            if(sel == 1 && op.op_type == 1){ // send to B
                sys_move_cursor(0, 4);
                send_bytes_to_a += byte.send_bytes;
                printf("B send bytes:%ld to A", send_bytes_to_a);
            }else if(sel == 0 && op.op_type == 1){ // send to C
                sys_move_cursor(0, 5);
                send_bytes_to_c += byte.send_bytes;
                printf("B send bytes:%ld to C", send_bytes_to_c);
            }else if(op.op_type==0){ // A recv bytes
                sys_move_cursor(0, 6);
                recv_bytes += byte.recv_bytes;
                printf("B received bytes:%ld", recv_bytes);
            }
            op.op_type = rand() % 2;
        }
        else if(feedback == 3){
            op.times = 0;
            op.op_type = rand() % 2;
        }else{
            op.times = 1;
            op.op_type = feedback;
        }
    }
}
void mbox_C(){
    mailbox_t* send, *recv = mbox_open("mbox_c");
    int sel;
    op_info op;
    int feedback;
    bytes_info byte;
    int64_t send_bytes_to_b = 0, send_bytes_to_a = 0, recv_bytes = 0;
    byte.send_bytes = byte.recv_bytes = 0;
    op.times = 0; 
    for(;;){
        sel = sys_get_tick() % 2;
        if(sel){
            send = mbox_open("mbox_a");
        }else{
            send = mbox_open("mbox_b");
        }
        feedback = UserMboxOp(&op, send, recv, &byte);

        if(feedback == 2){//success
            op.times = 0;
            if(sel == 1 && op.op_type == 1){ // send to B
                sys_move_cursor(0, 7);
                send_bytes_to_a += byte.send_bytes;
                printf("C send bytes:%ld to A", send_bytes_to_a);
            }else if(sel == 0 && op.op_type == 1){ // send to C
                sys_move_cursor(0, 8);
                send_bytes_to_b += byte.send_bytes;
                printf("C send bytes:%ld to B", send_bytes_to_b);
            }else if(op.op_type==0){ // A recv bytes
                sys_move_cursor(0, 9);
                recv_bytes += byte.recv_bytes;
                printf("C received bytes:%ld", recv_bytes);
            }
            op.op_type = rand() % 2;
        }
        else if(feedback == 3){
            op.times = 0;
            op.op_type = rand() % 2;
        }else{
            op.times = 1;
            op.op_type = feedback;
        }
    }
}

void randmbox_task(){
    mailbox_t* mbox_a = mbox_open("mbox_a");
    mailbox_t* mbox_b = mbox_open("mbox_b");
    mailbox_t* mbox_c = mbox_open("mbox_c");
    task_info_t task_a = {(uintptr_t)&mbox_A, USER_THREAD};
    task_info_t task_b = {(uintptr_t)&mbox_B, USER_THREAD};
    task_info_t task_c = {(uintptr_t)&mbox_C, USER_THREAD};

    pid_t pid_a = sys_spawn(&task_a, NULL, ENTER_ZOMBIE_ON_EXIT);
    pid_t pid_b = sys_spawn(&task_b, NULL, ENTER_ZOMBIE_ON_EXIT);
    pid_t pid_c = sys_spawn(&task_c, NULL, ENTER_ZOMBIE_ON_EXIT);

    sys_waitpid(pid_a);
    sys_waitpid(pid_b);
    sys_waitpid(pid_c);
    sys_exit();
}