/*************************************************************************\
* Copyright (c) 2002 Lawrence Berkeley Laboratory, Advanced Light Source,
*     Control System Group
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* devK486Gpib.c */
/* @(#)devK486Gpib.c	1.2	3/18/92 */

/*
 *      Author: Bill Brown
 *      Date:   03-18-93
 */

#define	DSET_AI		devAiK486Gpib
#define	DSET_AO		devAoK486Gpib
#define	DSET_LI		devLiK486Gpib
#define	DSET_LO		devLoK486Gpib
#define	DSET_BI		devBiK486Gpib
#define	DSET_BO		devBoK486Gpib
#define	DSET_MBBO	devMbboK486Gpib
#define	DSET_MBBI	devMbbiK486Gpib
#define	DSET_SI		devSiK486Gpib
#define	DSET_SO		devSoK486Gpib

#include	<vxWorks.h>
#include        <stdlib.h>
#include        <stdio.h>
#include        <string.h>
#include	<taskLib.h>
#include	<rngLib.h>

#include	<alarm.h>
#include	<cvtTable.h>
#include	<dbDefs.h>
#include	<dbAccess.h>
#include	<devSup.h>
#include	<recSup.h>
#include	<drvSup.h>
#include	<link.h>
#include	<module_types.h>
#include	<dbCommon.h>
#include	<aiRecord.h>
#include	<aoRecord.h>
#include	<biRecord.h>
#include	<boRecord.h>
#include	<mbbiRecord.h>
#include	<mbboRecord.h>
#include	<stringinRecord.h>
#include	<stringoutRecord.h>
#include	<longinRecord.h>
#include	<longoutRecord.h>

#include	<drvGpibInterface.h>
#include	<devCommonGpib.h>

static long	init_dev_sup(), report();
static	struct  devGpibParmBlock devSupParms;

/******************************************************************************
 *
 * Define all the dset's.
 *
 * Note that the dset names are provided via the #define lines at the top of
 * this file.
 *
 * Other than for the debugging flag(s), these DSETs are the only items that
 * will appear in the global name space within the IOC.
 *
 * The last 3 items in the DSET structure are used to point to the parm 
 * structure, the  work functions used for each record type, and the srq 
 * handler for each record type.
 *
 ******************************************************************************/
gDset DSET_AI   = {6, {report, init_dev_sup, devGpibLib_initAi, NULL, 
	devGpibLib_readAi, NULL, (DRVSUPFUN)&devSupParms,
	(DRVSUPFUN)devGpibLib_aiGpibWork, (DRVSUPFUN)devGpibLib_aiGpibSrq}};

gDset DSET_AO   = {6, {NULL, NULL, devGpibLib_initAo, NULL, 
	devGpibLib_writeAo, NULL, (DRVSUPFUN)&devSupParms,
	(DRVSUPFUN)devGpibLib_aoGpibWork, NULL}};

gDset DSET_BI   = {5, {NULL, NULL, devGpibLib_initBi, NULL, 
	devGpibLib_readBi, (DRVSUPFUN)&devSupParms,
	(DRVSUPFUN)devGpibLib_biGpibWork, (DRVSUPFUN)devGpibLib_biGpibSrq}};

gDset DSET_BO   = {5, {NULL, NULL, devGpibLib_initBo, NULL, 
	devGpibLib_writeBo, (DRVSUPFUN)&devSupParms,
	(DRVSUPFUN)devGpibLib_boGpibWork, NULL}};

gDset DSET_MBBI = {5, {NULL, NULL, devGpibLib_initMbbi, NULL, 
	devGpibLib_readMbbi, (DRVSUPFUN)&devSupParms,
	(DRVSUPFUN)devGpibLib_mbbiGpibWork, (DRVSUPFUN)devGpibLib_mbbiGpibSrq}};

gDset DSET_MBBO = {5, {NULL, NULL, devGpibLib_initMbbo, NULL, 
	devGpibLib_writeMbbo, (DRVSUPFUN)&devSupParms,
	(DRVSUPFUN)devGpibLib_mbboGpibWork, NULL}};

gDset DSET_SI   = {5, {NULL, NULL, devGpibLib_initSi, NULL, 
	devGpibLib_readSi, (DRVSUPFUN)&devSupParms,
	(DRVSUPFUN)&devGpibLib_stringinGpibWork, (DRVSUPFUN)devGpibLib_stringinGpibSrq}};

gDset DSET_SO   = {5, {NULL, NULL, devGpibLib_initSo, NULL, 
	devGpibLib_writeSo, (DRVSUPFUN)&devSupParms, 
	(DRVSUPFUN)devGpibLib_stringoutGpibWork, NULL}};

gDset DSET_LI   = {5, {NULL, NULL, devGpibLib_initLi, NULL, 
	devGpibLib_readLi, (DRVSUPFUN)&devSupParms, 
	(DRVSUPFUN)devGpibLib_liGpibWork, (DRVSUPFUN)devGpibLib_liGpibSrq}};

gDset DSET_LO   = {5, {NULL, NULL, devGpibLib_initLo, NULL, 
	devGpibLib_writeLo, (DRVSUPFUN)&devSupParms, 
	(DRVSUPFUN)devGpibLib_loGpibWork, NULL}};

int K486Debug = 0;		/* debugging flags */

/*
 * Use the TIME_WINDOW defn to indicate how long commands should be ignored
 * for a given device after it times out.  The ignored commands will be
 * returned as errors to device support.
 *
 * Use the DMA_TIME to define how long you wish to wait for an I/O operation
 * to complete once started.
 */
#define TIME_WINDOW	300		/* 10 seconds on a getTick call */
#define	DMA_TIME	60		/* 1/2 second on a watchdog time */

/*
 * Strings used by the init routines to fill in the znam, onam, ...
 * fields in BI, BO, MBBI, and MBBO record types.
 */

static	char		*offOnList[] = { "Off", "On" };
static	struct	devGpibNames	offOn = { 2, offOnList, NULL, 1 };

static	char	*disableEnableList[] = { "Disable", "Enable" };
static	struct	devGpibNames	disableEnable = { 2, disableEnableList, NULL, 1 };

static	char	*resetList[] = { "Reset", "Reset" };
static	struct	devGpibNames	reset = { 2, resetList, NULL, 1 };

static	char	*lozHizList[] = { "50 OHM", "IB_Q_HIGH Z" };
static	struct	devGpibNames	lozHiz = {2, lozHizList, NULL, 1};

static	char	*invertNormList[] = { "INVERT", "NORM" };
static	struct	devGpibNames	invertNorm = { 2, invertNormList, NULL, 1 };

static	char	*fallingRisingList[] = { "FALLING", "RISING" };
static	struct	devGpibNames fallingRising = { 2, fallingRisingList, NULL, 1 };

static	char	*singleShotList[] = { "SINGLE", "SHOT" };
static	struct	devGpibNames	singleShot = { 2, singleShotList, NULL, 1 };

static	char	*clearList[] = { "CLEAR", "CLEAR" };
static	struct	devGpibNames	clear = { 2, clearList, NULL, 1 };

static	char		*tABCDList[] = { "T", "A", "B", "C", "D" };
static	unsigned long	tABCDVal[] = { 1, 2, 3, 5, 6 };
static	struct	devGpibNames	tABCD = { 5, tABCDList, tABCDVal, 3 };

static	char		*ttlNimEclVarList[] = { "TTL", "NIM", "ECL", "VAR" };
static	unsigned long	ttlNimEclVarVal[] = { 0, 1, 2, 3 };
static	struct	devGpibNames	ttlNimEclVar = { 4, ttlNimEclVarList, 
					ttlNimEclVarVal, 2 };

static	char		*intExtSsBmStopList[] = { "INTERNAL", "EXTERNAL", 
					"SINGLE SHOT", "BURST MODE", "STOP" };
static	unsigned long	intExtSsBmStopVal[] = { 0, 1, 2, 3, 2 };
static	struct	devGpibNames	intExtSsBm = { 4, intExtSsBmStopList, 
					intExtSsBmStopVal, 2 };
static	struct	devGpibNames	intExtSsBmStop = { 5, intExtSsBmStopList, 
					intExtSsBmStopVal, 3 };


/******************************************************************************
 *
 * String arrays for EFAST operations.  Note that the last entry must be 
 * NULL.
 *
 * On input operations, only as many bytes as are found in the string array
 * elements are compared.  If there are more bytes than that in the input
 * message, they are ignored.  The first matching string found (starting
 * from the 0'th element) will be used as a match.
 *
 * NOTE: For the input operations, the strings are compared literally!  This
 * can cause problems if the instrument is returning things like \r and \n
 * characters.  You must take care when defining input strings so you include
 * them as well.
 *
 ******************************************************************************/

static char	*(userOffOn[]) = {"USER OFF;", "USER ON;", NULL};

/******************************************************************************
 *
 * Array of structures that define all GPIB messages
 * supported for this type of instrument.
 *
 ******************************************************************************/

/* forward declarations of some custom convert routines */
int setDelay();
int rdDelay();

static struct gpibCmd gpibCmds[] = 
  {
  /* Param 0, (model)   */
  FILL,

  /* Param 1 initialization string */
      {
      &DSET_BO, GPIBCMD, IB_Q_HIGH, "L0 X B0 G1 R0 S1 C2 X C0 T5 X",
      NULL, 0, 32, NULL, 0, 0, NULL, NULL, -1
      },

  /* Param 2 read current value */
      {
      &DSET_AI,		/* <f1>	record type {analog in}	*/
      GPIBREAD,		/* <f2>	GPIB I/O operation type {read}	*/
      IB_Q_HIGH,	/* <f3>	processing priority	*/
      "X",		/* <f4>	command sent to device	*/
      "%*4c%lf",	/* <f5>	string descriptor for sscanf()	*/
      0,		/* <f6> buffer pointer	*/
      32,		/* <f7>	buffer length	*/
      NULL,		/* <f8>	special I/O operation	*/
      0,		/* <f9>	passed to func specified by <f9>	*/
      0,		/* <f10> ditto	*/
      NULL,		/* <f11> ditto	*/
      NULL,		/* <f12> name table pointer	*/
      -1		/* <f13> reserved	*/
      }

  };


/* The following is the number of elements in the command array above.  */
#define NUMPARAMS	sizeof(gpibCmds)/sizeof(struct gpibCmd)

/******************************************************************************
 *
 * Initialization for device support
 * This is called one time before any records are initialized with a parm
 * value of 0.  And then again AFTER all record-level init is complete
 * with a param value of 1.
 *
 * This function will no longer be required after epics 3.3 is released
 *
 ******************************************************************************/
static long 
init_dev_sup(parm)
int	parm;
{
  if(parm==0)  {
    devSupParms.debugFlag = &K486Debug;
    devSupParms.respond2Writes = -1;
    devSupParms.timeWindow = TIME_WINDOW;
    devSupParms.hwpvtHead = 0;
    devSupParms.gpibCmds = gpibCmds;
    devSupParms.numparams = NUMPARAMS;
    devSupParms.magicSrq = -1;
    devSupParms.name = "devXxK486Gpib";
    devSupParms.dmaTimeout = DMA_TIME;
    devSupParms.srqHandler = 0;
    devSupParms.wrConversion = 0;
  }
  return(devGpibLib_initDevSup(parm, &DSET_AI));
}

/******************************************************************************
 *
 * Print a report of operating statistics for all devices supported by this
 * module.
 *
 * This function will no longer be required after epics 3.3 is released
 *
 ******************************************************************************/
static long
report()
{
  return(devGpibLib_report(&DSET_AI));
}
