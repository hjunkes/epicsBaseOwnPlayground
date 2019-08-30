/*************************************************************************\
* Copyright (c) 2002 The University of Saskatchewan
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 * RTEMS configuration for EPICS
 *      Author: W. Eric Norum
 *              norume@aps.anl.gov
 *              (630) 252-4793
 */

#include <rtems.h>

/*
 ***********************************************************************
 *                         RTEMS CONFIGURATION                         *
 ***********************************************************************
 */

extern void *POSIX_Init(void *argument);

#define CONFIGURE_POSIX_INIT_THREAD_TABLE
#define CONFIGURE_POSIX_INIT_THREAD_ENTRY_POINT POSIX_Init
/*
 * nfs is using rtems tasks
#define CONFIGURE_MAXIMUM_TASKS 		5
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES    	5
#define CONFIGURE_MAXIMUM_SEMAPHORES  		5
#define CONFIGURE_MAXIMUM_EVENTS    		5
*/

/* test CONFIGURE_UNLIMITED_OBJECTS
// #define CONFIGURE_MAXIMUM_POSIX_MUTEXES     	500
#define CONFIGURE_MAXIMUM_POSIX_THREADS     	200
#define CONFIGURE_MAXIMUM_POSIX_KEYS            200
#define CONFIGURE_MAXIMUM_POSIX_KEY_VALUE_PAIRS 200
// #define CONFIGURE_MAXIMUM_POSIX_SPINLOCKS   	100
#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES  	300
#define CONFIGURE_MAXIMUM_POSIX_TIMERS      	100
#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES  200
// #define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES 400
*/

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_MAXIMUM_PERIODS 	5
#define CONFIGURE_MICROSECONDS_PER_TICK 10000
#define CONFIGURE_MALLOC_STATISTICS     1
/* MINIMUM_STACK_SIZE == 8K */
#define CONFIGURE_EXTRA_TASK_STACKS         (4000 * RTEMS_MINIMUM_STACK_SIZE)

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define CONFIGURE_FILESYSTEM_DEVFS
#define CONFIGURE_FILESYSTEM_TFTPFS
#define CONFIGURE_FILESYSTEM_NFS
#define CONFIGURE_FILESYSTEM_IMFS
#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM
#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 150
#define CONFIGURE_MAXIMUM_PIPES		2

#define CONFIGURE_MAXIMUM_NFS_MOUNTS 		3
#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 	5

#define CONFIGURE_POSIX_INIT_THREAD_STACK_SIZE  (64*1024)

#define CONFIGURE_MAXIMUM_DRIVERS 8

//#define CONFIGURE_INITIAL_EXTENSIONS { .fatal = fatal_extension }

#define CONFIGURE_INIT

/*
 * This should be made BSP dependent, not CPU dependent but I know of no
 * appropriate conditionals to use.
 * The new general time support makes including the RTC driver less important.
 */
#if !defined(mpc604) && !defined(__mc68040__) && !defined(__mcf5200__) && \
    !defined(mpc7455) && !defined(__arm__)  && !defined(__nios2__)
    /* don't have RTC code */
#define CONFIGURE_APPLICATION_NEEDS_RTC_DRIVER
#endif

/* Enables runtime check checker
 * Adds extra overhead to task creation/destruction.
 * Detects stack overflow on task exit.
 * Enables the "stackuse" monitor command to show actual usage.
 */
#define STACK_CHECKER_ON

#include <bsp.h>
#include <rtems/confdefs.h>
