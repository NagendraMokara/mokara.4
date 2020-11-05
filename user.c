#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "user.h"

/* Increments total CPU time */
void add_time() 
{
    	UINT32 tmp = 0;
        
    	ptr_pcb_info[pid].totalNS += mq_msg.burst;
    	if (ptr_pcb_info[pid].totalNS >= NANO_SECS_PER_SEC) 
	{
        	ptr_pcb_info[pid].totalSec++;
        	tmp = ptr_pcb_info[pid].totalNS - NANO_SECS_PER_SEC;
        	ptr_pcb_info[pid].totalNS = tmp;
    	}
}

int main(int argc, char *argv[]) 
{
	UINT32 i_sec, i_ns;
    	int rand;
	const int full_time_opt = 75;
        
    	pcb_shmid = atoi(argv[1]);
    	pid = atoi(argv[2]);
        mq_msg.userPid = getpid();

        ptr_pcb_info = (struct process_control_block *)shmat(pcb_shmid, 0, 0);
        if (ptr_pcb_info == (struct process_control_block *)(-1) )
        {
                printf("user: error shmat\n");
                exit(1);
        }

        secs_shmid = shmget(KEY_INT1, sizeof (UINT32), 0444);
        if (secs_shmid == -1)
        {
                printf("user: error while shmget");
                exit(1);
        }
        ptr_shm_clk_secs = (UINT32*) shmat(secs_shmid, 0, 0);

        ns_shmid = shmget(KEY_INT2, sizeof (UINT32), 0444);
        if (ns_shmid == -1)
        {
                printf("user: error while shmget\n");
                exit(1);
        }
        ptr_shm_clk_nsecs = (UINT32*) shmat(ns_shmid, 0, 0);

        if ((queue_msgid = msgget(KEY_INT3, 0777)) == -1)
        {
                printf("user: error while msgget");
                exit(0);
        }
	
	ptr_pcb_info[pid].ui_start_sec = *ptr_shm_clk_secs; 
    	ptr_pcb_info[pid].ui_start_nsec = *ptr_shm_clk_nsecs;

	while(1) 
	{
        	if (msgrcv(queue_msgid, &mq_msg, sizeof(mq_msg), pid, 0) == -1 ) 
		{
	            	exit(0);
        	}
        	mq_msg.userPid = getpid(); 
        	int rand = rand_r(&seed) % 1000 + 1;;
        	
		/* while the random is less than the expected value */
		if (rand < range_exp) 
		{
			UINT32 randomSlice = (rand_r(&seed) % (mq_msg.timeGivenNS) + 1);
            		mq_msg.burst = randomSlice;
            		add_time();
			UINT32 myEndTimeSecs = *ptr_shm_clk_secs; 
    			UINT32 myEndTimeNS = *ptr_shm_clk_nsecs;
    			UINT32 temp;

    			if (myEndTimeSecs == ptr_pcb_info[pid].ui_start_sec) 
			{
        			ptr_pcb_info[pid].totalWholeNS = (myEndTimeNS - ptr_pcb_info[pid].ui_start_nsec);
        			ptr_pcb_info[pid].totalWholeSec = 0;
    			}
    			else 
			{
        			ptr_pcb_info[pid].totalWholeSec = myEndTimeSecs - ptr_pcb_info[pid].ui_start_sec;
        			ptr_pcb_info[pid].totalWholeNS = myEndTimeNS + ( NANO_SECS_PER_SEC - ptr_pcb_info[pid].ui_start_nsec);
        			ptr_pcb_info[pid].totalWholeSec--;
    			}
    			if (ptr_pcb_info[pid].totalWholeNS >=  NANO_SECS_PER_SEC) 
			{
        			ptr_pcb_info[pid].totalWholeSec++;
        			temp = ptr_pcb_info[pid].totalWholeNS -  NANO_SECS_PER_SEC;
        			ptr_pcb_info[pid].totalWholeNS = temp;
    			}
            		mq_msg.blockedFlg = 0;
    			mq_msg.termFlg = 1;
    			mq_msg.timeFlg = 0;
    			mq_msg.sPid = pid;
    			mq_msg.msgTyp = 99;
    
    			if ( msgsnd(queue_msgid, &mq_msg, sizeof(mq_msg), 0) == -1 ) 
			{
        			printf("User: msg sending error");
        			exit(0);
    			}
            		return 1;
        	}
        
		/* block it if it is less than range_blocked */
        	else if (rand < range_blocked) 
		{
            		i_sec = *ptr_shm_clk_secs; 
           	 	i_ns = *ptr_shm_clk_nsecs;
            		blocked_ns = rand_r(&seed) % 1000 + 1;
            		blocked_sec = rand_r(&seed) % 3 + 1;
            		ptr_pcb_info[pid].bSec = i_sec + blocked_sec;
            		ptr_pcb_info[pid].bNS = i_ns + blocked_ns;
            		UINT32 now_secs = *ptr_shm_clk_secs;
    			UINT32 now_ns = *ptr_shm_clk_nsecs;
    			UINT32 temp = 0;
    
    			ptr_pcb_info[pid].bWholeSec += blocked_sec; 
   			ptr_pcb_info[pid].bWholeNS += blocked_ns;
    			if (ptr_pcb_info[pid].bWholeNS >= NANO_SECS_PER_SEC) 
			{
        			ptr_pcb_info[pid].bWholeSec++;
        			temp = ptr_pcb_info[pid].bWholeNS - NANO_SECS_PER_SEC;
        			ptr_pcb_info[pid].bWholeNS = temp;
    			}       
            		ptr_pcb_info[pid].bTimes++;
            		mq_msg.msgTyp = 99;
    			mq_msg.timeFlg = 0;
    			mq_msg.burst = rand_r(&seed) % 99 + 1;
    			add_time();
    			mq_msg.blockedFlg = 1;
    			if ( msgsnd(queue_msgid, &mq_msg, sizeof(mq_msg), 0) == -1 ) 
			{
        			printf("user: msg sending error");
        			exit(0);
    			}
        	}
       
		/* preemption condition */ 
        	else if (rand < range_preempt) 
		{
            		mq_msg.blockedFlg = 0;
    			mq_msg.termFlg = 0;
    			mq_msg.timeFlg = 0;
			UINT32 randomSlice = (rand_r(&seed) % (mq_msg.timeGivenNS) + 1);
    			mq_msg.burst = randomSlice;
    			add_time();
    			mq_msg.sPid = pid;
    			mq_msg.msgTyp = 99;
    			if ( msgsnd(queue_msgid, &mq_msg, sizeof(mq_msg), 0) == -1 ) 
			{
        			printf("User: message sending error");
        			exit(0);
    			}
        	}
		/* send stats back */
        	else 
		{
            		add_time();
    			mq_msg.blockedFlg = 0;
    			mq_msg.termFlg = 0;
    			mq_msg.timeFlg = 1;
    			mq_msg.burst = mq_msg.timeGivenNS;
    			mq_msg.sPid = pid;
    			mq_msg.msgTyp = 99;
    			if ( msgsnd(queue_msgid, &mq_msg, sizeof(mq_msg), 0) == -1 ) 
			{
        			printf("User: message sending error");
        			exit(0);
    			}
        	}

    	}

	return 0;
}

