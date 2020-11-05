#ifndef _USER_H_
#define _USER_H_

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

int secs_shmid, ns_shmid; 
int pcb_shmid; 
int queue_msgid; 
UINT32 *ptr_shm_clk_secs; 
UINT32 *ptr_shm_clk_nsecs;  
int pid; 
UINT32 seed;
UINT32 blocked_sec, blocked_ns;
int range_exp = 15;
int range_blocked = 17;
int range_preempt = 260;

struct process_control_block *ptr_pcb_info; 
struct msg_queue mq_msg;

#endif
