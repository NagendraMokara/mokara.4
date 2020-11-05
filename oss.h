#ifndef _OSS_H_
#define _OSS_H_

#include <stdio.h>
#include "common.h"

UINT32 totalSec;
UINT32 totalNS;
UINT32 totalWaitSec; 
UINT32 totalWaitNS;
UINT32 bWholeSec;
UINT32 bWholeNS;
int smSec, smNS;
UINT32 stopSec = 0;
UINT32 stopNS = 0;
int shmid;
int qid;
int array = 19;
UINT32 *clockSec;
UINT32 *clockNS; 


UINT32 maxNS, maxSec;
UINT32 nextProcNS, nextProcSec, begin;
UINT32 createNS, createSec;
int blocked[19];
int numProc = 0;
int newChild = 1;
int numChild = 0;
int numUnblocked;
pid_t pids[20];
int lines = 0;
int nextP, nextQ;

const static UINT32 cost0 = 10  * 1000000;
const static UINT32 cost1 = 10 * 2 * 1000000;
const static UINT32 cost2 = 10 * 3 * 1000000;
const static UINT32 cost3 = 10 * 4 * 1000000;

/* output file */
char outputFileName[] = "osslog.txt";
FILE* fp;

struct msg_queue msgstruct;
struct process_control_block *pcbinfo;

#endif
