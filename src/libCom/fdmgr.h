/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* fdmgr.h
 *
 *      share/epicsH/$Id$
 *
 *      Header file associated with a file descriptor manager 
 *	for use with the UNIX system call select
 *
 *      Author  Jeffrey O. Hill
 *              hill@atdiv.lanl.gov
 *              505 665 1831
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 */

#ifndef includeFdmgrH
#define includeFdmgrH

#ifdef __cplusplus
extern "C" {
#endif

#include "ellLib.h"
#include "bucketLib.h"
#include "osiSock.h"
#include "shareLib.h"

enum fdi_type {fdi_read, fdi_write, fdi_excp};
enum alarm_list_type {alt_invalid, alt_alarm, alt_expired, alt_free};

typedef struct{
    ELLLIST		fdentry_list;
    ELLLIST		fdentry_in_use_list;
    ELLLIST		fdentry_free_list;
    ELLLIST		alarm_list;
    ELLLIST		expired_alarm_list;
    ELLLIST   	free_alarm_list;
    fd_set		readch;
    fd_set		writech;
    fd_set		excpch;
	BUCKET		*pAlarmBucket;
	unsigned	nextAlarmId;
    SOCKET		maxfd;
	int		select_tmo;
#	if defined (vxWorks)
	SEM_ID		lock;
    SEM_ID		fdmgr_pend_event_lock;
	SEM_ID		expired_alarm_lock;
	SEM_ID		fd_handler_lock;
	unsigned long   clk_rate;       /* ticks per sec */
	unsigned long	last_tick_count;
	unsigned long	sec_offset;
	WIND_TCB	*fdmgr_pend_event_tid;
#	else 
	unsigned	fdmgr_pend_event_in_use;
#	endif
}fdctx;

/*
 * C "typedef" name "alarm" was changed to "fdmgrAlarm" to avoid collisions
 * with other libraries. Next the identifier was changed again to
 * an unsigned integer type "fdmgrAlarmId".
 *
 * This "#define" is for codes that used to use a pointer to the old typedef
 * "alarm" or "fdmgrAlarm" types to identify an alarm.
 *
 * ie the following code will allow compilation against
 * all versions:
 *
 * #if defined (NEW_FDMGR_ALARMID)
 * fdmgrAlarmId	XXXX
 * #elif defined (NEW_FDMGR_ALARM)
 * fdmgrAlarm   *XXXX;
 * #else
 * alarm        *XXXX;
 * #endif
 *
 * XXXX = fdmgrAlarmId fdmgr_add_timeout()
 */
typedef unsigned fdmgrAlarmId;
#define NEW_FDMGR_ALARMID

#ifdef __STDC__

/*
 *
 * Initialize a file descriptor manager session
 *
 */
epicsShareFunc fdctx * epicsShareAPI fdmgr_init(void);

/*
 * Specify a function to be called with a specified parameter
 * after a specified delay relative to the current time
 *
 * Returns fdmgrNoAlarm (zero) if alarm cant be created
 */
#define fdmgrNoAlarm 0
epicsShareFunc fdmgrAlarmId epicsShareAPI fdmgr_add_timeout(
fdctx           *pfdctx,	/* fd mgr ctx from fdmgr_init()		*/
struct timeval  *ptimeout,	/* relative delay from current time	*/
void            (*func)(void *pParam),	
				/* function (handler) to call 		*/
void            *param		/* first parameter passed to the func	*/
);

/*
 * Clear a timeout which has not executed its function (handler)
 * yet.
 */
epicsShareFunc int epicsShareAPI fdmgr_clear_timeout(
fdctx           *pfdctx,	/* fd mgr ctx from fdmgr_init() 	*/
fdmgrAlarmId	id		/* alarm to delete                      */
);

/*
 *
 * Specify a function (handler) to be called with a specified parameter
 * when a file descriptor becomes active. The parameter fdi (file
 * descriptor interest) specifies the type of activity (IO) we wish
 * to be informed of: read, write, or exception. For more
 * info on this see the man pages for the UNIX system call select().
 *
 * read and exception callbacks are permanent( ie the application's
 * function (handler) continues to be called each time the
 * file descriptor becomes active until fdmgr_add_callback()
 * is called).
 *
 * write callbacks are called only once after each call to
 * fdmgr_add_callback()
 * 
 */
epicsShareFunc int epicsShareAPI fdmgr_add_callback(
fdctx *pfdctx,			/* fd mgr ctx from fdmgr_init() 	*/
SOCKET fd,				/* file descriptor			*/
enum fdi_type fdi,		/* file descriptor interest type	*/	
void (*pfunc)(void *pParam),	/* function (handler) to call			*/
void *param				/* first parameter passed to the func   */
);

/*
 * 
 * Clear nterest in a type of file descriptor activity (IO).
 *
 */ 
epicsShareFunc int epicsShareAPI fdmgr_clear_callback(
fdctx			*pfdctx,	/* fd mgr ctx from fdmgr_init() 	*/
SOCKET		fd,		/* file descriptor                      */
enum fdi_type	fdi		/* file descriptor interest type        */
);

/*
 *
 * Wait a specified delay relative from the current time for file 
 * descriptor activity (IO) or timeouts (timer expiration). Application 
 * specified functions (handlers) will not be called unless the 
 * application waits in this function or polls it frequently
 * enough. 
 *
 */
epicsShareFunc int epicsShareAPI fdmgr_pend_event(
fdctx		*pfdctx,	/* fd mgr ctx from fdmgr_init() */
struct timeval	*ptimeout
);


/*
 * obsolete interface
 */
epicsShareFunc int epicsShareAPI fdmgr_clear_fd(
fdctx		*pfdctx,		/* fd mgr ctx from fdmgr_init() */
SOCKET	fd
);

/*
 * obsolete interface
 */
epicsShareFunc int epicsShareAPI fdmgr_add_fd(
fdctx   *pfdctx,		/* fd mgr ctx from fdmgr_init() */
SOCKET  fd,
void    (*pfunc)(void *pParam),
void    *param
);

epicsShareFunc int epicsShareAPI fdmgr_delete(fdctx *pfdctx);

#else

epicsShareFunc fdctx * epicsShareAPI fdmgr_init();
epicsShareFunc fdmgrAlarmId epicsShareAPI fdmgr_add_timeout();
epicsShareFunc int epicsShareAPI fdmgr_clear_timeout();
epicsShareFunc int epicsShareAPI fdmgr_add_callback();
epicsShareFunc int epicsShareAPI fdmgr_clear_callback();
epicsShareFunc int epicsShareAPI fdmgr_pend_event();
epicsShareFunc int epicsShareAPI fdmgr_delete();

/*
 * obsolete interface
 */
epicsShareFunc int epicsShareAPI fdmgr_clear_fd();
epicsShareFunc int epicsShareAPI fdmgr_add_fd();

#endif

#ifdef __cplusplus
}
#endif

#endif /* ifndef includeFdmgrH (last line in this file) */

