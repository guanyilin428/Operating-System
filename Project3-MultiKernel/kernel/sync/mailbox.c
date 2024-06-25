#include <os/list.h>
#include <mthread.h>
#include <os/sync.h>
#include <mailbox.h>
#include <os/sched.h>
#include <string.h>

#define MAX_MBOX_SIZE 20
LIST_HEAD(mbox_queue);

kmailbox arr[MAX_MBOX_SIZE];
int mbox_first = 0;
int allocated[MAX_MBOX_SIZE];

void mobx_init_allocated(){
    for(int i = 0; i < MAX_MBOX_SIZE; i++)
        allocated[i] = 0;
}

int mbox_find_id(){
    if(!mbox_first){
        mobx_init_allocated();
        mbox_first++;
    }
    for(int i=0; i <MAX_MBOX_SIZE; i++){
       if(!allocated[i])
            return i;
    }
}

mailbox_t do_mailbox_open(char *name){
    int found = 0, i;
    kmailbox *m;
    for(i = 0; i < MAX_MBOX_SIZE; i++){
        if(!allocated[i])
            continue;
        m = &arr[i];
        if(kstrcmp(name, m->name) == 0){
            found = 1;
            break;
        }
    }

    if(found)
        return m->id;
    
    int id = mbox_find_id();
    allocated[id] = 1;
    m = &arr[id];
    m->id = id;
    strcpy(m->name, name);
    m->start = m->end = 0;
    m->empty = 1;
    __list_init(&(m->recv));
    __list_init(&(m->send));
    return m->id;
}


void do_mailbox_close(mailbox_t* mailbox){
    int id = *mailbox;
    kmailbox *m = &arr[id];
    if(!allocated[id])
        return;
    while(!list_empty(&(m->send)))
        do_unblock(&(m->send));
    while(!list_empty(&(m->recv)))
        do_unblock(&(m->recv));
    allocated[id] = 0;
}

int do_mailbox_send(mailbox_t *mailbox, char* msg, int msg_len){
    int id = *mailbox;
    kmailbox *m = &arr[id];
    int blocked = 0;
    if(!allocated[id])
        return;
    int filled = m->empty ? 0 : (m->end <= m->start) ? m->end + MAX_MBOX_LENGTH - m->start : m->end - m->start;
    int rest = MAX_MBOX_LENGTH - filled;
    while(rest < msg_len){//full
        do_block(*current_running, &(m->send));
        do_scheduler();
        blocked++;
        filled = m->empty ? 0 : (m->end <= m->start) ? m->end + MAX_MBOX_LENGTH - m->start : m->end - m->start;
        rest = MAX_MBOX_LENGTH - filled;
    }
    
    //send mail
    for(int i = 0; i < msg_len; i++){
        m->buff[(m->end++) % MAX_MBOX_LENGTH] = *(msg + i);
    }
    m->end %= MAX_MBOX_LENGTH; 
    m->empty = 0;
    
    if(!list_empty(&(m->recv)))
        do_unblock(m->recv.next);
    return blocked;
}


int do_mailbox_recv(mailbox_t* mailbox, char *msg, int msg_len){
    int id = *mailbox;
    kmailbox *m = &arr[id];
    int blocked = 0;
    if(!allocated[id])
        return;
    int filled = m->empty ? 0 : (m->end <= m->start) ? m->end + MAX_MBOX_LENGTH - m->start : m->end - m->start;
    while(filled < msg_len){
        do_block(*current_running, &(m->recv));
        blocked++;
        do_scheduler();
        filled = m->empty ? 0 : (m->end <= m->start) ? m->end + MAX_MBOX_LENGTH - m->start : m->end - m->start;
    }  

    //recv mail
    for(int i = 0; i <msg_len; i++){
        *(msg+i) = m->buff[m->start++];
        if(m->start >= MAX_MBOX_LENGTH)
            m->start -= MAX_MBOX_LENGTH;
    }
    if(m->start == m->end)
        m->empty = 1;
    
    if(!list_empty(&(m->send)))
        do_unblock(m->send.next);
    
    return blocked;
}

//由用户来维护times变量
int do_mailbox_op(mailbox_t* mailbox, msg_info* msg, op_info* op){
    char* buff = msg->buff;
    int msg_len = msg->length;
    int op_type = op->op_type;
    int times = op->times;

    int id = *mailbox;
    kmailbox *m = &arr[id];
    if(!allocated[id])
        return;
    int filled = m->empty ? 0 : (m->end <= m->start) ? m->end + MAX_MBOX_LENGTH - m->start : m->end - m->start;
    int rest = MAX_MBOX_LENGTH - filled;

    if(op_type){ // send_op
        if(rest < msg_len){
            if(!times)
                return 0; //fail, return the recv_op
            do_block(*current_running, &mbox_queue);
            do_scheduler();
            return 3;
        }

        // success, then send mail
        for(int i = 0; i < msg_len; i++){
            m->buff[(m->end++) % MAX_MBOX_LENGTH] = *(buff + i);
        }
        m->end %= MAX_MBOX_LENGTH; 
        m->empty = 0;
        if(!list_empty(&mbox_queue))
            do_unblock(mbox_queue.next);
        return 2;
    }else{// recv_op
        if(filled < msg_len){
            if(!times)
                return 1;//fail, return the send_op
            do_block(*current_running, &mbox_queue);
            do_scheduler();
            return 3;
        }
        // success, then recv mail
        for(int i = 0; i <msg_len; i++){
            *(buff+i) = m->buff[m->start++];
        if(m->start >= MAX_MBOX_LENGTH)
            m->start -= MAX_MBOX_LENGTH;
        }
        if(m->start == m->end)
            m->empty = 1;

        if(!list_empty(&mbox_queue))
            do_unblock(mbox_queue.next);
        return 2;
    }
}