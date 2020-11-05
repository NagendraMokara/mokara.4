#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "oss.h"

#define NUM_PROCS	        20
#define INT_SIZE		4
#define NUM_LINES_LIMIT         10000

/* queue create */
void q_create(int *queue, int len) 
{
    	int i;

    	for(i = 0; i < len; i++) 
	{
        	queue[i] = 0;
    	}
}

/* get next  */
int q_get_next() 
{
    	if (g_queue_0[1] != 0) 
        	return 0;
    	else if (g_queue_1[1] != 0) 
        	return 1;
    	else if (g_queue_2[1] != 0) 
        	return 2;
    	else if (g_queue_3[1] != 0) 
        	return 3;
    	else
		return -1;
}

/* queue add item */
int q_add_item (int *queue, int len, int item) 
{
    	int ret = -1, i;

    	for (i = 1; i < len; i++) 
	{
        	if (queue[i] == 0) 
		{ 
            		queue[i] = item;
            		return 1;
        	}
    	}

    	return ret; 
}

/* queue delete */
int q_del_item(int *queue, int len, int item) 
{
    	int j;

    	for (j = 1; j < item; j++) 
	{
        	if (queue[j] == item) 
		{ 
            		while(j+1 < len) 
			{ 
                		queue[j] = queue[j+1];
                		j++;
            		}

            		queue[NUM_PROCS - 2] = 0; 

            		return 1;
        	}
    	}

    	return -1;
}

/* get spot from bit vector */
int get_bitvec_slot() 
{
    	int j, ret = -1;

    	for (j = 1; j < NUM_PROCS - 1; j++) 
	{
        	if (g_bit_map[j] == 0) 
		{
            		ret = j;
            		break;
        	}
    	}

    	return ret;
}

/* set up ipcs */
void ipc_init() 
{   
    	if ((shmid = shmget(KEY_PCB, (NUM_PROCS - 2) * sizeof(struct process_control_block), PERMS_KEY)) < 0) {
            	printf("OSS: error while shmget\n");
            	exit(1);
        }
    	if ((pcbinfo = (struct process_control_block *)shmat(shmid, 0, 0)) == (struct process_control_block *)(-1)) {
        	printf("OSS: error while shmat\n");
        	exit(1);
    	}   
    	if ((smSec = shmget(KEY_INT1, sizeof(UINT32), PERMS_KEY)) < 0) {
            	printf("OSS: error while shmget\n");
            	exit(1);
        }
    	clockSec = (UINT32 *) shmat(smSec, 0, 0);
    	if ((smNS = shmget(KEY_INT2, sizeof(UINT32), PERMS_KEY)) < 0) {
            	printf("OSS: error while shmget\n");
            	exit(1);
        }
    	clockNS = (UINT32*) shmat(smNS, 0, 0);
    	if ((qid = msgget(KEY_INT3, PERMS_KEY)) == -1) { 
            	printf("OSS: error while shmget\n");
        	exit(0);
    	}
}

/* shmem detach */
void shmem_detach() 
{
    	shmctl(smNS, IPC_RMID, NULL);
        shmctl(smSec, IPC_RMID, NULL);

    	shmctl(shmid, IPC_RMID, NULL);
    	msgctl(qid, IPC_RMID, NULL);
}


/* display summary */
void display_summary()
{
	double wait_time_avg = ((((double) totalWaitNS / NANO_SECS_PER_SEC) + (double)totalWaitSec / 100));
	double cpu_util_avg = ((((double) totalNS / NANO_SECS_PER_SEC)      + (double)totalSec / 100));
	double blocked_time_avg = ((((double) bWholeNS / NANO_SECS_PER_SEC) + (double)bWholeSec / 100));
        
        printf("CPU idle time = %u sec:%u nanosec\n", stopSec, stopNS);
	printf("Average CPU utilization = %.9f seconds\n", cpu_util_avg);
	printf("Average user wait time = %.9f seconds\n", wait_time_avg);
	printf("Average blocked time = %.9f seconds\n", blocked_time_avg);
}

/* signal handler */
void sig_handler(int sig)
{               
        if (sig == SIGINT) {
		display_summary();
		printf("\nInterupted by ctrl-c\n");
        } else {
		display_summary();
                printf("\nInterupted by alarm signal - 3 secs\n");
        }
	
	shmem_detach();
        exit(0);
}

/* bit vector initialization */
void init_bitmap_vec(int size) 
{
    	int j;

    	for (j = 0; j < size; j++) 
	{
        	g_bit_map[j] = 0;
    	}
}

/* set time for next process */
void set_next_proc_time()
{
	UINT32 i_sec = *clockSec;
	UINT32 tmp;
	UINT32 i_ns = *clockNS;

	nextProcSec = rand_r(&begin) % (maxSec + 1);
    	nextProcNS = rand_r(&begin) % (maxNS + 1);
    	createSec = i_sec + nextProcSec;
    	createNS = i_ns + nextProcNS;

	if (createNS >= NANO_SECS_PER_SEC)
	{
		createSec++;
		tmp = createNS - NANO_SECS_PER_SEC;
		createNS = tmp;
	}
}

/* PCB Init */
void init_process_control_block(int pnum, int rt) 
{
    	pcbinfo[pnum].ui_start_sec = 0;
    	pcbinfo[pnum].ui_start_nsec = 0;
    	pcbinfo[pnum].totalSec = 0;
    	pcbinfo[pnum].totalNS = 0;
    	pcbinfo[pnum].totalWholeSec = 0;
    	pcbinfo[pnum].totalWholeNS = 0;
    	pcbinfo[pnum].burstNS = 0;
    	pcbinfo[pnum].blocked = 0;
    	pcbinfo[pnum].bTimes = 0;
    	pcbinfo[pnum].bSec = 0;
    	pcbinfo[pnum].bNS = 0;
    	pcbinfo[pnum].bWholeSec = 0;
    	pcbinfo[pnum].bWholeNS = 0;
    	pcbinfo[pnum].localPid = pnum; 
    	pcbinfo[pnum].realTimeC = rt;

    	if (rt == 1) 
	{
        	pcbinfo[pnum].currentQueue = 0;
    	}
    	else 
	{
        	pcbinfo[pnum].currentQueue = 1;
    	}
    	g_bit_map[pnum] = 1;
}

/* blocked queue checking */
int check_blocked_queue() 
{
    	if (blocked[1] != 0) 
        	return 0;

    	return 1;
}

/* retrive from blocked state */
void retrive_child_from_blocked(int pid) 
{
    	UINT32 i_sec, i_ns, tmp;

    	q_del_item(blocked, array, pid);

    	pcbinfo[pid].blocked = 0;
    	pcbinfo[pid].bNS = 0;
    	pcbinfo[pid].bSec = 0; 

    	if (pcbinfo[pid].realTimeC == 1) 
	{
        	q_add_item(g_queue_0, array, pid);
        	pcbinfo[pid].currentQueue = 0;
    	}
    	else 
	{
        	q_add_item(g_queue_1, array, pid);
        	pcbinfo[pid].currentQueue = 1;
    	}

    	i_sec = *clockSec;
    	i_ns = *clockNS;

    	tmp = random();

    	if (tmp < 100) 
		tmp = NUM_LINES_LIMIT;
    	else 
		tmp = tmp * 100;

    	i_ns = i_ns + tmp; 
}

/* retrieve child from queue */
int q_retrive_child(int *queue) 
{
    	if (queue[1] == 0) 
        	return -1;
    	else 
		return queue[1];
}


/* Checks the blocked queue to see if it is time to spawn new member */
int blockq_check() 
{
    	int k, retval = 0;
    	UINT32 i_sec = *clockSec;
    	UINT32 i_ns = *clockNS;

    	for (k = 1; k < array; k++) { 
        	if (blocked[k] != 0) { 
            		if ((i_sec > pcbinfo[blocked[k]].bSec) || ( (i_sec >= pcbinfo[blocked[k]].bSec) && (i_ns >= pcbinfo[blocked[k]].bNS))) { 
                		retrive_child_from_blocked(blocked[k]);
                		retval++;
            		}
        	}
    	}

    	return retval;
}

/* terminate member */
void proc_terminate(int pid) 
{
    	int ret;
    	UINT32 tmp;

    	waitpid(msgstruct.userPid, &ret, 0);
    
    	totalSec += pcbinfo[pid].totalWholeSec / 100;
    	totalNS += pcbinfo[pid].totalWholeNS / 100;

    	if (totalNS >= NANO_SECS_PER_SEC) 
	{
        	totalSec++;
        	tmp = totalNS - NANO_SECS_PER_SEC;
        	totalNS = tmp;
    	}
 	totalWaitSec += (pcbinfo[pid].totalWholeSec - pcbinfo[pid].totalSec) / 100;
    	totalWaitNS +=(pcbinfo[pid].totalWholeNS - pcbinfo[pid].totalNS) / 100;

    	if (totalWaitNS >= NANO_SECS_PER_SEC) 
	{
        	totalWaitSec++;
        	tmp = totalWaitNS - NANO_SECS_PER_SEC;
        	totalWaitNS = tmp;
    	}
    
    	if (pcbinfo[pid].currentQueue == 0) 
        	q_del_item(g_queue_0, array, pid);
    	else if (pcbinfo[pid].currentQueue == 1) 
        	q_del_item(g_queue_1, array, pid);
    	else if (pcbinfo[pid].currentQueue == 2) 
        	q_del_item(g_queue_2, array, pid);
    	else if (pcbinfo[pid].currentQueue == 3) 
        	q_del_item(g_queue_3, array, pid);

    	g_bit_map[pid] = 0;
    	numChild--;

    	pcbinfo[pid].totalSec = 0;
    	pcbinfo[pid].totalNS = 0;
    	pcbinfo[pid].totalWholeSec = 0;
    	pcbinfo[pid].totalWholeNS = 0;

	if(lines < NUM_LINES_LIMIT)
        {
		fprintf(fp,"OSS: User %d has terminated\n",msgstruct.sPid);
		lines++;
	}
}


/* create child */
void spawn_child() 
{
    	int rand, s_pid;
    	char id[NUM_PROCS]; 
    	char str_pid_child[INT_SIZE]; 
    	pid_t pid_child;

    	set_next_proc_time(); 
    	if ((s_pid = get_bitvec_slot()) < 0) {
        	shmem_detach();
        	exit(0);
    	}
   	numProc++;

    	if (numProc >= 100) 
        	newChild = 0;
    	
	msgstruct.msgTyp = s_pid;
    	msgstruct.timeFlg = 0;
    	msgstruct.termFlg = 0;
    	msgstruct.sPid = s_pid;

    	rand = random();
    	if (rand < 45) { 
        	msgstruct.timeGivenNS = cost0;
        	init_process_control_block(s_pid, 1);
        	q_add_item(g_queue_0, array, s_pid);
        	pcbinfo[s_pid].currentQueue = 0;
    	} else {
        	msgstruct.timeGivenNS = cost1;
        	init_process_control_block(s_pid, 0);
        	q_add_item(g_queue_1, array, s_pid);
        	pcbinfo[s_pid].currentQueue = 1;
    	}

	if(lines < NUM_LINES_LIMIT)
        {
    		fprintf(fp, "OSS: Generating process PID %d and putting it in queue %d at time %u:%09u\n", pcbinfo[s_pid].localPid, pcbinfo[s_pid].currentQueue, *clockSec, *clockNS);
		lines++;
	}

	fflush(fp);
    	if ((pid_child = fork()) < 0) {
        	printf("OSS: Error forking user\n");
        	fprintf(fp, "Fork error\n");
        	shmem_detach();
        	exit(0);
    	}
    	if (pid_child == 0) { 
        	sprintf(id, "%d", shmid); 
        	sprintf(str_pid_child, "%d", s_pid);
       		execlp("./user", "./user", id, str_pid_child, (char *)NULL);
        	printf("OSS: execl() failure\n"); 
        	exit(0);
    	}
    	pids[s_pid] = pid_child;
}

/* increment idle time */
int idle_time_increment()
{
        UINT32 tmp, i_sec, i_ns, sim_local_s, sim_local_ns;

        i_sec = 0;
        sim_local_s = *clockSec;
        sim_local_ns = *clockNS;

        if (sim_local_s == createSec)
                i_ns = (createNS - sim_local_ns);
        else
        {
                i_sec = (createSec - sim_local_s);
                i_ns = createNS + (NANO_SECS_PER_SEC - sim_local_ns);
                i_sec--;
        }
        if (i_ns >= NANO_SECS_PER_SEC)
        {
                i_sec++;
                tmp = i_ns - NANO_SECS_PER_SEC;
                i_ns = tmp;
        }
        stopSec = stopSec + i_sec;
        stopNS = stopNS + i_ns;
        if (stopNS >= NANO_SECS_PER_SEC)
        {
                stopSec++;
                tmp = stopNS - NANO_SECS_PER_SEC;
                stopNS = tmp;
        }
        return 1;
}

/* clock incrementing */
void clock_add(int pid_child) 
{
    	UINT32 i_sec = *clockSec;
    	UINT32 i_ns = *clockNS;
    	UINT32 tmp;

    	tmp = random();
    	if (tmp < 100) 
		tmp = 100;
    	else 	
		tmp = tmp * 10;
    	i_ns = i_ns + tmp; 

	if(lines<NUM_LINES_LIMIT)
        {
		fprintf(fp, "OSS: Time spent this dispatch: %u nanoseconds\n", tmp);
    		lines++;
	}
	i_ns = i_ns + pcbinfo[pid_child].burstNS;
    
    	if (i_ns >= NANO_SECS_PER_SEC) 
	{
        	i_sec++;
        	tmp = i_ns - NANO_SECS_PER_SEC;
        	i_ns = tmp;
    	}
    
    	*clockSec = i_sec;
    	*clockNS = i_ns;
}

/* right time to launch a new proc? */
int is_good_time() 
{
    	UINT32 i_sec = *clockSec;
    	int rvalue = 0;
    	UINT32 i_ns = *clockNS;

    	if ((i_sec > createSec) ||  ((i_sec >= createSec) && (i_ns >= createNS))) 
        	rvalue = 1;
 
    	return rvalue;
}

/* internal checking of block time and queues */
void block(int pid) 
{
    	int temp;
    	UINT32 i_sec, i_ns;
    
    	bWholeSec += pcbinfo[pid].bWholeSec * 100;
	bWholeNS += pcbinfo[pid].bWholeNS * 100;
    
	if (bWholeNS >= NANO_SECS_PER_SEC) { 
        	bWholeSec++;
        	temp = bWholeNS - NANO_SECS_PER_SEC;
        	bWholeNS = temp;
    	}
   
    	pcbinfo[pid].blocked = 1;

    	if (pcbinfo[pid].currentQueue == 0)
        	q_del_item(g_queue_0, array, pid);
    	else if (pcbinfo[pid].currentQueue == 1)
	        q_del_item(g_queue_1, array, pid);
    	else if (pcbinfo[pid].currentQueue == 2)
       		q_del_item(g_queue_2, array, pid);
    	else if (pcbinfo[pid].currentQueue == 3)
	        q_del_item(g_queue_3, array, pid);

    	q_add_item(blocked, array, pid);

    	i_sec = *clockSec;
    	i_ns = *clockNS;
    	temp = random();

    	if (temp < 100) 
		temp = NUM_LINES_LIMIT;
    	else 	
		temp = temp * 100;

    	i_ns = i_ns + temp; 
    
	if(lines < NUM_LINES_LIMIT)
        {
		fprintf(fp, "OSS: Time used to move user to blocked queue: %u nanoseconds\n", temp);
		lines++;
	}	
	if (i_ns >= NANO_SECS_PER_SEC) 
	{
        	i_sec++;
        	temp = i_ns - NANO_SECS_PER_SEC;
        	i_ns = temp;
    	}
   
    	*clockSec = i_sec;
    	*clockNS = i_ns;
}

/* main */
int main(int argc, char* argv[]) 
{
	int opt;
	
	/* parse command line options */
	while((opt = getopt(argc, argv, "i:h")) != EOF)
        {
                switch(opt)
                {
        		case 'h':
                		printf("Usage: ./oss [-h] [-i x]\n");
				printf("       -i             specify logfile name (default: osslog.txt)\n");
	       	        	printf("       -h             Display this help \n");
				return 0;
       		 	case 'i':
       			        strcpy(outputFileName, optarg);
               		break;
	       
		 	default:
       		         	return -1;
        	}
	}
	
	fp = fopen(outputFileName, "w");
    	maxNS = NANO_SECS_PER_SEC;
    	maxSec = 5;
	begin = (UINT32) getpid();   
    	int firstblocked;	

	/* install signal handlers */
	signal(SIGINT, sig_handler);
        signal(SIGALRM, sig_handler);

	/* shared mem init */
	ipc_init();
	
	/* init bit vector */
	init_bitmap_vec(array);
	
	/* setup queues */
    	q_create(g_queue_0, array);
    	q_create(g_queue_1, array);
    	q_create(g_queue_2, array);
    	q_create(g_queue_3, array);
    	q_create(blocked, array);
    
	/* first process scheduling */
   	set_next_proc_time();

	/* 3 secs set */	
	alarm(3);
	
	while (1) 
	{

		/* exit loop for 100 count  */
		if (newChild == 0 && numChild == 0) 
       		 	break;

		/* checking blocked queue to generate users */
       		numUnblocked = 0;
       		if (check_blocked_queue() == 0) 
        		numUnblocked = blockq_check();

                /* spawn new member by retrieving highest priority one */
                nextQ = q_get_next();
                if ((nextQ == -1) && (newChild == 0)) 
                {
                        set_next_proc_time();
                        idle_time_increment();
                        *clockSec = createSec;
                        *clockNS = createNS;
                
                        if (numUnblocked == 0) 
                                continue;
                    
                }
                /* if queues are empty, schedule new member */
                else if (nextQ == -1 && newChild == 1) 
                {
                        idle_time_increment();

                        *clockSec = createSec;
                        *clockNS = createNS;

                        if ((newChild == 1) && (get_bitvec_slot() != -1)) 
                        {
                                spawn_child();         
                                numChild++;
                                if(lines < NUM_LINES_LIMIT)
                                {
                                    fprintf(fp, "OSS: Dispatching process PID %d from queue %d at time %u:%09u\n", msgstruct.sPid                      ,pcbinfo[msgstruct.sPid].currentQueue,*clockSec, *clockNS);
                                    lines++;
                                }
                                if ( msgsnd(qid, &msgstruct, sizeof(msgstruct), 0) == -1 ) 
                                {
                                        printf("OSS: init msg send error");
                                        shmem_detach();
                                        exit(0);
                                }
                        }
                }

                /* check msg queue */
                firstblocked = blocked[1]; 
                if ( msgrcv(qid, &msgstruct, sizeof(msgstruct), 99, 0) == -1 ) 
                {
                        printf("oss: error msgrcv\n");
                        shmem_detach();
                        exit(0);
                }
                blocked[1] = firstblocked; 
                /* record burst time */
                pcbinfo[msgstruct.sPid].burstNS = msgstruct.burst;
                /* advance clock */
                clock_add(msgstruct.sPid);

                /* check for termination */
                if (msgstruct.termFlg == 1) 
                {
                        if(lines < NUM_LINES_LIMIT)
                        {
                            fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds and then terminated\n", msgstruct.sPid, msgstruct.burst);
                            lines++;
                        }
                        proc_terminate(msgstruct.sPid);
                }
            
                /* blocked indication */
                else if (msgstruct.blockedFlg == 1) 
                {	
                        if(lines < NUM_LINES_LIMIT)
                        {

                                fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds and then was blocked by an event. Moving to blocked queue\n", msgstruct.sPid, msgstruct.burst);
                                lines++;
                        }
                        block(msgstruct.sPid);
                }
                /* not using entire quantum of its time */
                else if (msgstruct.timeFlg == 0) 
                {
                        if(lines < NUM_LINES_LIMIT)
                        {
                                fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds\n", msgstruct.sPid, msgstruct.burst);
                                fprintf(fp, "OSS: not using its entire quantum");
                                lines++;
                        }
                        if (pcbinfo[msgstruct.sPid].currentQueue == 2) 
                        {
                                if(lines < NUM_LINES_LIMIT)
                                {
                                        fprintf(fp, ", moving to queue 1\n");
                                        lines++;
                                }
                                q_del_item(g_queue_2, array, msgstruct.sPid);
                                q_add_item(g_queue_1, array, msgstruct.sPid);
                                pcbinfo[msgstruct.sPid].currentQueue = 1;
                        }
                        else if (pcbinfo[msgstruct.sPid].currentQueue == 3) 
                        {
                                if(lines < NUM_LINES_LIMIT)
                                {
                                fprintf(fp, ", moving to queue 1\n");
                                lines++;
                                }
                                q_del_item(g_queue_3, array, msgstruct.sPid);
                                q_add_item(g_queue_1, array, msgstruct.sPid);
                                pcbinfo[msgstruct.sPid].currentQueue = 1;
                        }
                        else 
                        {
                                if(lines < NUM_LINES_LIMIT)
                                {
                                        fprintf(fp, "\n");
                                        lines++;
                                }
                        }
                }
                
                /* change queue appropriately if it uses its full time slice */
                else if (msgstruct.timeFlg == 1) 
                {
                        if(lines < NUM_LINES_LIMIT)
                        {
                                fprintf(fp, "OSS: Receiving that process PID %d ran for %u nanoseconds", msgstruct.sPid,msgstruct.burst);
                                lines++;
                        }
                        if (pcbinfo[msgstruct.sPid].currentQueue == 1) 
                        {
                                if(lines < NUM_LINES_LIMIT)
                                {
                                        fprintf(fp, ", moving to queue 2\n");
                                        lines++;
                                }
                                q_del_item(g_queue_1, array, msgstruct.sPid);
                                q_add_item(g_queue_2, array, msgstruct.sPid);
                                pcbinfo[msgstruct.sPid].currentQueue = 2;
                        }
                
                        else if (pcbinfo[msgstruct.sPid].currentQueue == 2) 
                        {
                                if(lines < NUM_LINES_LIMIT)
                                {
                                        fprintf(fp, ", moving to queue 3\n");
                                        lines++;
                                }
                                q_del_item(g_queue_2, array, msgstruct.sPid);
                                q_add_item(g_queue_3, array, msgstruct.sPid);
                                pcbinfo[msgstruct.sPid].currentQueue = 3;
                        }
                        else 
                        {
                                if(lines < NUM_LINES_LIMIT)
                                {
                                        fprintf(fp, "\n");
                                        lines++;
                                }
                        }
                }

                /* fork a new proc */
                if (newChild == 1) 
                {
                        if (is_good_time()) 
                        {
                                if (get_bitvec_slot() != -1) 
                                {
                                        spawn_child();
                                        numChild++;
                                }
                        else 
                        {
                                set_next_proc_time();
                        }
                        }
                }
                
                /* look for another queue  */
                nextQ = q_get_next();
                if (nextQ == 0) 
                {
                        nextP = q_retrive_child(g_queue_0);
                        msgstruct.timeGivenNS = cost0;
                        q_del_item(g_queue_0, array, nextP);
                        q_add_item(g_queue_0, array, nextP);
                }
                else if (nextQ == 1) 
                {
                        nextP = q_retrive_child(g_queue_1);
                        msgstruct.timeGivenNS = cost1;
                        q_del_item(g_queue_1, array, nextP);
                        q_add_item(g_queue_1, array, nextP);
                }
                else if (nextQ == 2) 
                {
                        nextP = q_retrive_child(g_queue_2);
                        msgstruct.timeGivenNS = cost2;
                        q_del_item(g_queue_2, array, nextP);
                        q_add_item(g_queue_2, array, nextP);
                }
                else if (nextQ == 3) 
                {
                        nextP = q_retrive_child(g_queue_3);
                        msgstruct.timeGivenNS = cost3;
                        q_del_item(g_queue_3, array, nextP);
                        q_add_item(g_queue_3, array, nextP);
                }    
                else 
                {
                        continue;
                }

                /* now, schedule it */
                if (nextP != -1) 
                {
                        msgstruct.msgTyp = (long) nextP;
                        msgstruct.termFlg = 0;
                        msgstruct.timeFlg = 0;
                        msgstruct.blockedFlg = 0;
                        msgstruct.sPid = nextP;
                        
                        if(lines < NUM_LINES_LIMIT)
                        {
                                fprintf(fp, "OSS: Dispatching process PID %d from queue %d at time %u:%09u\n", nextP, pcbinfo[nextP].currentQueue, *clockSec, *clockNS);
                                lines++;
                        }
                        if (msgsnd(qid, &msgstruct, sizeof(msgstruct), 0) == -1) 
                        {
                                printf("OSS: init msg send failure\n");
                                shmem_detach();
                                exit(0);
                        }
                }
    	}

	display_summary();
	shmem_detach();

	return 0;
}
