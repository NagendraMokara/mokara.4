#ifndef COMMON_H
#define COMMON_H

#define UINT32			unsigned int
#define NANO_SECS_PER_SEC 	1000000000  

#define KEY_PCB			4020014
#define KEY_INT1		4020012
#define KEY_INT2		4020013
#define KEY_INT3		4020069

#define PERMS_KEY		0777 | IPC_CREAT

int g_bit_map[19];
int g_queue_0[19];
int g_queue_1[19];
int g_queue_2[19];
int g_queue_3[19];

/* Process control block information */
struct process_control_block {
    UINT32 ui_start_sec;
    UINT32 ui_start_nsec; 
    UINT32 totalSec;
    UINT32 totalNS;
    UINT32 totalWholeSec;
    UINT32 totalWholeNS;
    UINT32 burstNS;
    int blocked;
    int bTimes;
    UINT32 bSec;
    UINT32 bNS;
    UINT32 bWholeSec;
    UINT32 bWholeNS;
    int localPid;
    int realTimeC;
    int currentQueue;
};

/* Contains communication variables for oss->user */
struct msg_queue {
    long msgTyp;
    int sPid;
    pid_t userPid;
    UINT32 timeGivenNS;
    int termFlg;
    int timeFlg;
    int blockedFlg;
    UINT32 burst;
};

#endif
