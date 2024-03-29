/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* dbBkpt.h			*/
/* base/include $Id$ */
/*
 *      Author:          Matthew Needes
 *      Date:            8-30-93
 */

#ifndef INCdbBkptsh
#define INCdbBkptsh 1

#include <lstLib.h>
/* Needs to be put into dbTest.h ! */
long dbpr(char *name, int level);

/*
 *  Structure containing a list of set breakpoints
 *    in a lockset
 */

struct BP_LIST {
   NODE *next_list;
   NODE *prev_list; 
   struct dbCommon *precord;
};

/*
 *  Structure containing queue of entrypoints
 *    detected for a lockset.
 */
struct EP_LIST {
   NODE *next_list; 
   NODE *prev_list;
   struct dbCommon *entrypoint;  /* pointer to entry point in lockset */
   unsigned long count;          /* number of times record processed */
   unsigned long time;           /* time record first logged */
   char sched;                   /* schedule record for next dbContTask() pass */
};

/*
 *  Structure for stack of lock sets that
 *  currently contain breakpoints. (uses lstLib)
 */
struct LS_LIST {
   NODE *next_list;
   NODE *prev_list;
   struct dbCommon *precord;/* points to where execution is currently stopped */
   struct dbCommon *current_ep; /* current entrypoint */
   LIST bp_list;  /* list of records containing breakpoints in a lockset */
   LIST ep_queue; /* queue of entrypoints found so far */
   SEM_ID ex_sem; /* semaphore for execution queue */
   int taskid;    /* saved taskid for the task in stepping mode */
   int step;      /* one if currently "stepping," else zero */
   unsigned long l_num;   /* lockset number */
};

/* Values for BKPT (breakpoint) field in record    */

/*  1st bit = 0 if breakpoint is not set,          */
/*            1 if breakpoint set                  */
/*  2nd bit = 0 if no printing after processing    */
/*            1 if print after processing set      */

/*  Breakpoint Masks */
#define BKPT_ON_MASK         0x001
#define BKPT_OFF_MASK        0x0FE
#define BKPT_PRINT_MASK      0x002
#define BKPT_PRINT_OFF_MASK  0x0FD

#define MAX_EP_COUNT         99999

long dbb();
long dbd();
long dbc();
long dbs();
long dbstat();
long dbtap();
int  dbBkpt();
void dbPrint();

extern long lset_stack_not_empty;

#endif
