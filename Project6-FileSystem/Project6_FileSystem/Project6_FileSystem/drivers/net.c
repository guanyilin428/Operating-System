#include <net.h>
#include <os/string.h>
#include <screen.h>
#include <emacps/xemacps_example.h>
#include <emacps/xemacps.h>

#include <os/sched.h>
#include <os/mm.h>

EthernetFrame rx_buffers[RXBD_CNT];
EthernetFrame tx_buffer;
uint32_t rx_len[RXBD_CNT];

int net_poll_mode = 0;

volatile int rx_curr = 0, rx_tail = 0;

long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{
    // TODO: 
    // receive packet by calling network driver's function
    // wait until you receive enough packets(`num_packet`).
    // maybe you need to call drivers' receive function multiple times ?
    int recv_num;

    while(num_packet > 0){
        recv_num = num_packet > 32 ? 32 : num_packet;
        EmacPsRecv(&EmacPsInstance, kva2pa(rx_buffers), recv_num); 
        EmacPsWaitRecv(&EmacPsInstance, recv_num, rx_len);
        
        for(int i = 0; i < recv_num; i++){
            memcpy(addr, rx_buffers[i], rx_len[i]);
            // char *temp = (char*)(&rx_buffers[i]);
            // printk("%lx %lx %lx %lx\n\r", *temp, *(temp+1), *(temp+2), *(temp+3));
            addr += rx_len[i];
            *frLength = rx_len[i];
            frLength++;
        }
        num_packet -= recv_num;
    }
    
    return 1;
}

void do_net_send(uintptr_t addr, size_t length)
{
    // TODO:
    // send all packet
    memcpy(&tx_buffer, addr, length);
    EmacPsSend(&EmacPsInstance, kva2pa(&tx_buffer), length);
    EmacPsWaitSend(&EmacPsInstance);
}

void do_net_irq_mode(int mode)
{
    net_poll_mode = mode;
    u32 EmacInt_mask = (XEMACPS_IXR_TX_ERR_MASK | XEMACPS_IXR_RX_ERR_MASK |
                       (u32)XEMACPS_IXR_FRAMERX_MASK | (u32)XEMACPS_IXR_TXCOMPL_MASK);
    if (mode){
        XEmacPs_IntEnable(&EmacPsInstance, EmacInt_mask);
    }else{
        XEmacPs_IntDisable(&EmacPsInstance, EmacInt_mask);
    }
}
