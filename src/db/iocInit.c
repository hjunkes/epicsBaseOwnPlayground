/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* iocInit.c	ioc initialization */ 
/* base/src/db $Id$ */
/*
 *      Author:		Marty Kraimer
 *      Date:		06-01-91
 *
 */

#include	<vxWorks.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<string.h>
#include 	<errno.h>

#include	<sysLib.h>
#include	<symLib.h>
#include	<sysSymTbl.h>	/* for sysSymTbl*/
#include	<logLib.h>
#include	<taskLib.h>
#include	<envLib.h>
#include	<errnoLib.h>

#include	"dbDefs.h"
#include	"epicsPrint.h"
#include	"ellLib.h"
#include	"fast_lock.h"
#include	"dbDefs.h"
#include	"dbBase.h"
#include	"dbAccess.h"
#include	"dbScan.h"
#include	"taskwd.h"
#include	"callback.h"
#include	"dbCommon.h"
#include	"dbLock.h"
#include	"dbFldTypes.h"
#include	"devSup.h"
#include	"drvSup.h"
#include	"errMdef.h"
#include	"recSup.h"
#include	"envDefs.h"
#include	"dbStaticLib.h"
#include	"initHooks.h"
#include	"drvTS.h"
#include	"asLib.h"

/*This module will declare and initilize module_type variables*/
#define MODULE_TYPES_INIT 1
#include        <module_types.h>

void errlogDevInit ();

LOCAL int initialized=FALSE;

/* The following is for use by interrupt routines */
volatile int interruptAccept=FALSE;

struct dbBase *pdbbase=NULL;

/* define forward references*/
LOCAL long initDrvSup(void);
LOCAL long initRecSup(void);
LOCAL long initDevSup(void);
LOCAL long finishDevSup(void);
LOCAL long initDatabase(void);
LOCAL long initialProcess(void);
LOCAL long getResources(char  *fname);
LOCAL int getResourceToken(FILE *fp, char *pToken, unsigned maxToken);
LOCAL int getResourceTokenInternal(FILE *fp, char *pToken, unsigned maxToken);


/*
 *  Initialize EPICS on the IOC.
 */
int iocInit(char * pResourceFilename)
{
    long status;
    char name[40];
    long rtnval;
    void (*pinitHooks)() = NULL;
    SYM_TYPE type;

    if (initialized) {
	epicsPrintf("iocInit can only be called once\n");
	return(-1);
    }

    epicsPrintf("Starting iocInit\n");
    if (!pdbbase) {
	epicsPrintf("iocInit aborting because No database\n");
	return(-1);
    }

    errlogDevInit ();

   /* Setup initialization hooks, if  initHooks routine has been defined.  */
    strcpy(name, "_");
    strcat(name, "initHooks");
    rtnval = symFindByNameEPICS(sysSymTbl, name, (void *) &pinitHooks, &type);
    if (pinitHooks) (*pinitHooks)(initHookAtBeginning);

    coreRelease();
    status = getResources(pResourceFilename); 
    if (pinitHooks) (*pinitHooks)(initHookAfterGetResources);

    status = iocLogInit();
    if (status!=0) {
        epicsPrintf("iocInit Failed to Initialize Ioc Log Client \n");
    }
    if (pinitHooks) (*pinitHooks)(initHookAfterLogInit);


   /* After this point, further calls to iocInit() are disallowed.  */
    initialized = TRUE;

   /*
    *  Initialize the watchdog task.  These is different from vxWorks watchdogs,
    *    this task is assigned to watching and reporting if the vxWorks tasks
    *    (that it has been told to watch) are suspended.
    */
    taskwdInit();

    callbackInit();

   /* Wait 1/10 second for above initializations to complete*/
    (void)taskDelay(sysClkRateGet()/10);
    if (pinitHooks) (*pinitHooks)(initHookAfterCallbackInit);

   /* Initialize Channel Access Link mechanism.  */
    dbCaLinkInit();
    if (pinitHooks) (*pinitHooks)(initHookAfterCaLinkInit);

    if (initDrvSup() != 0)
         epicsPrintf("iocInit: Drivers Failed during Initialization\n");
    if (pinitHooks) (*pinitHooks)(initHookAfterInitDrvSup);

    if (initRecSup() != 0)
         epicsPrintf("iocInit: Record Support Failed during Initialization\n");
    if (pinitHooks) (*pinitHooks)(initHookAfterInitRecSup);

    if (initDevSup() != 0)
         epicsPrintf("iocInit: Device Support Failed during Initialization\n");
    if (pinitHooks) (*pinitHooks)(initHookAfterInitDevSup);

    TSinit(); /* new time stamp driver (jbk) */
    if (pinitHooks) (*pinitHooks)(initHookAfterTS_init);

   /* initialize database records */
    if (initDatabase() != 0)
         epicsPrintf("iocInit: Database Failed during Initialization\n");

    dbLockInitRecords(pdbbase);
    if (pinitHooks) (*pinitHooks)(initHookAfterInitDatabase);

    if (finishDevSup() != 0)
         epicsPrintf("iocInit: Device Support Failed during Finalization\n");
    if (pinitHooks) (*pinitHooks)(initHookAfterFinishDevSup);

    scanInit();
    if(asInit()) {
	epicsPrintf("iocInit: asInit Failed during initialization\n");
	return(-1);
    }
    (void)taskDelay(sysClkRateGet()/2);

    if (pinitHooks) (*pinitHooks)(initHookAfterScanInit);
   /*
    *  Process all records that have their "process at initialization"
    *      field set (pini).
    */
    taskDelay(sysClkRateGet());
    if (initialProcess() != 0)
          epicsPrintf("iocInit: initialProcess Failed\n");

    if (pinitHooks) (*pinitHooks)(initHookAfterInitialProcess);

   /* Enable scan tasks and some driver support functions.  */
    interruptAccept=TRUE;

    if (pinitHooks) (*pinitHooks)(initHookAfterInterruptAccept);


   /*  Start up CA server */
    rsrv_init();

    epicsPrintf("iocInit: All initialization complete\n");

    if (pinitHooks) (*pinitHooks)(initHookAtEnd);

    return(0);
}

/*
 *  Initialize Driver Support
 *      Locate all driver support entry tables.
 *      Call the initialization routine (init) for each
 *        driver type.
 */
LOCAL long initDrvSup(void) /* Locate all driver support entry tables */
{
    char	name[40];
    SYM_TYPE	type;
    long	status=0;
    long	rtnval;
    STATUS	vxstatus;
    drvSup	*pdrvSup;
    struct drvet *pdrvet;

   /*
    *  For every driver support module, look up the name
    *    for that function in the vxWorks symbol table.  If
    *    a driver entry table doesn't exist, report that
    *    fact.
    */
    for(pdrvSup = (drvSup *)ellFirst(&pdbbase->drvList); pdrvSup;
    pdrvSup = (drvSup *)ellNext(&pdrvSup->node)) {
	strcpy(name,"_");
	strcat(name,pdrvSup->name);
	vxstatus = symFindByNameEPICS(sysSymTbl, name,
		(void *) &pdrvet, &type);
	if (vxstatus != OK) {
	    status = S_drv_noDrvet;
	    errPrintf(status,__FILE__,__LINE__,"%s",pdrvSup->name);
	    continue;
	}
	pdrvSup->pdrvet = pdrvet;
       /*
        *   If an initialization routine is defined (not NULL),
        *      for the driver support call it.
        */
	if (!pdrvet->init) continue;
	rtnval = (*(pdrvet->init))();
	if (status == 0) status = rtnval;
    }
    return(status);
}

/*
 *  Initialize Record Support
 *      Allocate a record support structure for every record type
 *        plus space for an array of pointers to RSETs.
 *      Locate all record support entry tables.
 *      Call the initialization routine (init) for each
 *        record type.
 */
LOCAL long initRecSup(void)
{
    char	name[60];
    SYM_TYPE	type;
    long	status=0;
    long	rtnval;
    STATUS	vxstatus;
    dbRecordType	*pdbRecordType;
    struct rset *prset;
    
    for(pdbRecordType = (dbRecordType *)ellFirst(&pdbbase->recordTypeList); pdbRecordType;
    pdbRecordType = (dbRecordType *)ellNext(&pdbRecordType->node)) {
	strcpy(name,"_");
	strcat(name,pdbRecordType->name);
	strcat(name,"RSET");
	vxstatus = symFindByNameEPICS(sysSymTbl, name,
            (void *)&pdbRecordType->prset, &type);
	if (vxstatus != OK) {
	    status = S_rec_noRSET;
	    errPrintf(status,__FILE__,__LINE__,"%s",pdbRecordType->name);
	    continue;
	}
	prset = pdbRecordType->prset;
       /* If an initialization routine exists for a record type, execute it */
	if(!prset->init) {
            continue;
	} else {
	    rtnval = (*prset->init)();
	    if (status==0)
                status = rtnval;
	}
    }
    return(status);
}

/*  Initialize Device Support
 *      Locate all device support entry tables.
 *      Call the initialization routine (init) for each
 *        device type (First Pass).
 */
LOCAL long initDevSup(void)
{
    char	*pname;
    char	name[40];
    SYM_TYPE	type;
    long	status=0;
    long	rtnval = 0;
    STATUS	vxstatus;
    dbRecordType	*pdbRecordType;
    devSup	*pdevSup;
    struct dset *pdset;
    
    for(pdbRecordType = (dbRecordType *)ellFirst(&pdbbase->recordTypeList); pdbRecordType;
    pdbRecordType = (dbRecordType *)ellNext(&pdbRecordType->node)) {
	for(pdevSup = (devSup *)ellFirst(&pdbRecordType->devList); pdevSup;
	pdevSup = (devSup *)ellNext(&pdevSup->node)) {
	    if(!(pname = pdevSup->name)) continue;
	    strcpy(name, "_");
	    strcat(name, pname);
	    vxstatus = (long) symFindByNameEPICS(sysSymTbl, name,
		(void *) &pdset, &type);
	    if (vxstatus != OK) {
		status = S_dev_noDSET;
		errPrintf(status,__FILE__,__LINE__,"%s",pdevSup->name);
		continue;
	    }
	    pdevSup->pdset = pdset;
	    if(!(pdset->init)) continue;
	    rtnval = (*pdset->init)(0);
	    if (status == 0) status = rtnval;
	}
    }
    return(status);
}

/*  Calls the second pass for each device support
 *    initialization routine.  The second pass is made
 *    after the database records have been initialized and
 *    placed into lock sets.
 */
LOCAL long finishDevSup(void) 
{
    dbRecordType	*pdbRecordType;
    devSup	*pdevSup;
    struct dset *pdset;

    for(pdbRecordType = (dbRecordType *)ellFirst(&pdbbase->recordTypeList); pdbRecordType;
    pdbRecordType = (dbRecordType *)ellNext(&pdbRecordType->node)) {
	for(pdevSup = (devSup *)ellFirst(&pdbRecordType->devList); pdevSup;
	pdevSup = (devSup *)ellNext(&pdevSup->node)) {
	    if(!(pdset = pdevSup->pdset)) continue;
	    if(pdset->init) (*pdset->init)(1);
	}
    
    }
    return(0);
}

LOCAL long initDatabase(void)
{
    long		status=0;
    long		rtnval=0;
    dbRecordType		*pdbRecordType;
    dbFldDes		*pdbFldDes;
    dbRecordNode 	*pdbRecordNode;
    devSup		*pdevSup;
    struct rset		*prset;
    struct dset		*pdset;
    dbCommon		*precord;
    DBADDR		dbaddr;
    DBLINK		*plink;
    int			j;
   
    for(pdbRecordType = (dbRecordType *)ellFirst(&pdbbase->recordTypeList); pdbRecordType;
    pdbRecordType = (dbRecordType *)ellNext(&pdbRecordType->node)) {
	prset = pdbRecordType->prset;
	for (pdbRecordNode=(dbRecordNode *)ellFirst(&pdbRecordType->recList);
	pdbRecordNode;
	pdbRecordNode = (dbRecordNode *)ellNext(&pdbRecordNode->node)) {
	    if(!prset) break;
           /* Find pointer to record instance */
	    precord = pdbRecordNode->precord;
	    if(!(precord->name[0])) continue;
	    precord->rset = prset;
	    precord->rdes = pdbRecordType;
	    FASTLOCKINIT(&precord->mlok);
	    ellInit(&(precord->mlis));

           /* Reset the process active field */
	    precord->pact=FALSE;

	    /* Init DSET NOTE that result may be NULL */
	    pdevSup = (devSup *)ellNth(&pdbRecordType->devList,precord->dtyp+1);
	    pdset = (pdevSup ? pdevSup->pdset : 0);
	    precord->dset = pdset;
	    if(!prset->init_record) continue;
	    rtnval = (*prset->init_record)(precord,0);
	    if (status==0) status = rtnval;
	}
    }

   /*
    *  Second pass to resolve links
    */
    for(pdbRecordType = (dbRecordType *)ellFirst(&pdbbase->recordTypeList); pdbRecordType;
    pdbRecordType = (dbRecordType *)ellNext(&pdbRecordType->node)) {
	prset = pdbRecordType->prset;
	for (pdbRecordNode=(dbRecordNode *)ellFirst(&pdbRecordType->recList);
	pdbRecordNode;
	pdbRecordNode = (dbRecordNode *)ellNext(&pdbRecordNode->node)) {
	    precord = pdbRecordNode->precord;
	    if(!(precord->name[0])) continue;
            /* Convert all PV_LINKs to DB_LINKs or CA_LINKs */
            /* For all the links in the record type... */
	    for(j=0; j<pdbRecordType->no_links; j++) {
		pdbFldDes = pdbRecordType->papFldDes[pdbRecordType->link_ind[j]];
		plink = (DBLINK *)((char *)precord + pdbFldDes->offset);
		if (plink->type == PV_LINK) {
		    if(!(plink->value.pv_link.pvlMask&(pvlOptCA|pvlOptCP|pvlOptCPP))
		    && (dbNameToAddr(plink->value.pv_link.pvname,&dbaddr)==0)) {
			DBADDR	*pdbAddr;

			plink->type = DB_LINK;
			pdbAddr = dbCalloc(1,sizeof(struct dbAddr));
			*pdbAddr = dbaddr; /*structure copy*/;
			plink->value.pv_link.pvt = pdbAddr;
		    } else {/*It is a CA link*/
			char	*pperiod;

			if(pdbFldDes->field_type==DBF_INLINK) {
			    plink->value.pv_link.pvlMask |= pvlOptInpNative;
			}
			dbCaAddLink(plink);
			if(pdbFldDes->field_type==DBF_FWDLINK) {
			    pperiod = strrchr(plink->value.pv_link.pvname,'.');
			    if(pperiod && strstr(pperiod,"PROC"))
				plink->value.pv_link.pvlMask |= pvlOptFWD;
			}
		    }
		}
	    }
	}
    }

    /* Call record support init_record routine - Second pass */
    for(pdbRecordType = (dbRecordType *)ellFirst(&pdbbase->recordTypeList); pdbRecordType;
    pdbRecordType = (dbRecordType *)ellNext(&pdbRecordType->node)) {
	prset = pdbRecordType->prset;
	for (pdbRecordNode=(dbRecordNode *)ellFirst(&pdbRecordType->recList);
	pdbRecordNode;
	pdbRecordNode = (dbRecordNode *)ellNext(&pdbRecordNode->node)) {
	    if(!prset) break;
           /* Find pointer to record instance */
	    precord = pdbRecordNode->precord;
	    if(!(precord->name[0])) continue;
	    precord->rset = prset;
	    if(!prset->init_record) continue;
	    rtnval = (*prset->init_record)(precord,1);
	    if (status==0) status = rtnval;
	}
    }
    return(status);
}

/*
 *  Process database records at initialization if
 *     their pini (process at init) field is set.
 */
LOCAL long initialProcess(void)
{
    dbRecordType		*pdbRecordType;
    dbRecordNode 	*pdbRecordNode;
    dbCommon		*precord;
    
    for(pdbRecordType = (dbRecordType *)ellFirst(&pdbbase->recordTypeList);
    pdbRecordType;
    pdbRecordType = (dbRecordType *)ellNext(&pdbRecordType->node)) {
	for (pdbRecordNode=(dbRecordNode *)ellFirst(&pdbRecordType->recList);
	pdbRecordNode;
	pdbRecordNode = (dbRecordNode *)ellNext(&pdbRecordNode->node)) {
	    precord = pdbRecordNode->precord;
	    if(!(precord->name[0])) continue;
	    if(!precord->pini) continue;
	    (void)dbProcess(precord);
	}
    }
    return(0);
}



int dbLoadDatabase(char *filename,char *path,char *substitutions)
{
    return(dbReadDatabase(&pdbbase,filename,path,substitutions));
}

/*Remaining code supplied by Bob Zieman*/
#define MAX 256 
#define SAME 0

enum resType {
	resDBF_STRING,
	resDBF_SHORT,
	resDBF_LONG,
	resDBF_FLOAT,
	resDBF_DOUBLE,
	resInvalid}; 
LOCAL char    *cvt_str[] = {
    "DBF_STRING",
    "DBF_SHORT",
    "DBF_LONG",
    "DBF_FLOAT",
    "DBF_DOUBLE",
    "Invalid",
};

#define EPICS_ENV_PREFIX "EPICS_"

long getResources(char  *fname)
{
    char            s1[MAX];
    char            s2[MAX];
    char            s3[MAX];
    char            name[40];
    FILE            *fp;
    enum resType    cvType = resInvalid;
    int		    epicsFlag;
    SYM_TYPE	    type;
    char            *pSymAddr;
    short           n_short;
    long            n_long;
    float           n_float;
    double          n_double;
    int		    status;

    if (!fname) return (0);

    if ((fp = fopen(fname, "r")) == 0) {
	errPrintf(
		-1L,
		__FILE__, 
		__LINE__,
		"No such Resource file - %s",
		fname);
	return (-1);
    }

    while (TRUE) {
	status = getResourceToken (fp, s1, sizeof(s1));
	if (status<0) {
		/*
		 * EOF
		 */
		break; 
	}
	status = getResourceToken (fp, s2, sizeof(s2));
	if (status<0) {
		errPrintf (
			-1L,
			__FILE__,
			__LINE__,
	"Missing resource data type field for resource=%s in file=%s",
			s1,
			fname);
		break; 
	}
	status = getResourceToken (fp, s3, sizeof(s3));
	if (status<0) {
		errPrintf (
			-1L,
			__FILE__,
			__LINE__,
	"Missing resource value field for resource=%s data type=%s file=%s",
			s1,
			s2,
			fname);
		break; /* EOF */
	}

	for (cvType = 0; cvType < resInvalid; cvType++) {
	    if (strcmp(s2, cvt_str[cvType]) == SAME) {
		break;
	    }
	}


	strcpy(name, "_");
	strcat(name, s1);
	status = symFindByNameEPICS(sysSymTbl, name, &pSymAddr, &type);
	if (status!= OK) {
	    errPrintf (
		-1L,
		__FILE__,
		__LINE__,
	"Matching Symbol name not found for resource=%s",
		s1);
	    continue;
	}

	status = strncmp (
			s1,
			EPICS_ENV_PREFIX,
			strlen (EPICS_ENV_PREFIX));
	if (status == SAME) {
            	epicsFlag = 1;
		if (cvType != resDBF_STRING) {
			errPrintf (
				-1L,
				__FILE__,
				__LINE__,
"%s should be set with type DBF_STRING not type %s",
				s1,
				s2);
			continue;
		}
	}
	else {
		epicsFlag = 0;
	}

	switch (cvType) {
	case resDBF_STRING:
            if ( epicsFlag ) {
		char		*pEnv;

		/*
		 * space for two strings, an '=' character,
		 * and a null termination
		 */
		pEnv = malloc (strlen (s3) + strlen (s1) + 2);	
		if (!pEnv) {
			errPrintf(
				-1L,
				__FILE__,
				__LINE__,
"Failed to set environment parameter \"%s\" to \"%s\" because \"%s\"\n",
				s1,
				s3,
				strerror (errnoGet()));
			break;
		}
		strcpy (pEnv, s1);
		strcat (pEnv, "=");
		strcat (pEnv, s3);
		status = putenv (pEnv);
		if (status<0) {
			errPrintf(
				-1L,
				__FILE__,
				__LINE__,
"Failed to set environment parameter \"%s\" to \"%s\" because \"%s\"\n",
				s1,
				s3,
				strerror (errnoGet()));
		}
		/*
		 * vxWorks copies into a private buffer
		 * (this does not match UNIX behavior)
		 */
		free (pEnv);
	    }
            else{
                strcpy(pSymAddr, s3);
	    }
	    break;

	case resDBF_SHORT:	
	    if (sscanf(s3, "%hd", &n_short) != 1) {
	    	errPrintf (
			-1L,
			__FILE__,
			__LINE__,
		      "Resource=%s value=%s conversion to %s failed", 
			s1,
			s3,
			cvt_str[cvType]);
	        continue;
	    }
            *(short *) pSymAddr = n_short;
	    break;

	case resDBF_LONG:
	    if (sscanf(s3, "%ld", &n_long) != 1) {
	    	errPrintf (
			-1L,
			__FILE__,
			__LINE__,
		      "Resource=%s value=%s conversion to %s failed", 
			s1,
			s3,
			cvt_str[cvType]);
	        continue;
	    }
            *(long *) pSymAddr = n_long;
	    break;

	case resDBF_FLOAT:
	    if (sscanf(s3, "%e", &n_float) != 1) {
	    	errPrintf (
			-1L,
			__FILE__,
			__LINE__,
		      "Resource=%s value=%s conversion to %s failed", 
			s1,
			s3,
			cvt_str[cvType]);
	        continue;
	    }
            *(float *) pSymAddr = n_float;
	    break;

	case resDBF_DOUBLE:
	    if (sscanf(s3, "%le", &n_double) != 1) {
	    	errPrintf (
			-1L,
			__FILE__,
			__LINE__,
		      "Resource=%s value=%s conversion to %s failed", 
			s1,
			s3,
			cvt_str[cvType]);
	        continue;
	    }
            *(double *) pSymAddr = n_double;
	    break;

	default:
		errPrintf (
			-1L,
			__FILE__,
			__LINE__,
	"Invalid data type field=%s for resource=%s", 
			s2,
			s1);
	    continue;
	}
    }
    fclose(fp);
    return (0);
}



/*
 * getResourceToken
 */
LOCAL int getResourceToken(FILE *fp, char *pToken, unsigned maxToken)
{
	int status;

	/*
	 * keep reading until we get a token
	 * (and comments have been stripped)
	 */
	while (TRUE) {
		status = getResourceTokenInternal (fp, pToken, maxToken);
		if (status < 0) {
			return status;
		}

		if (pToken[0] != '\0') {
			return status;
		}
	}
}


/*
 * getResourceTokenInternal
 */
LOCAL int getResourceTokenInternal(FILE *fp, char *pToken, unsigned maxToken)
{
        char    formatString[32];
        char    quoteCharString[2];
        int     status;

        quoteCharString[0] = '\0';
        status = fscanf (fp, " %1[\"`'!]", quoteCharString);
	if (status<0) {
		return status;
	}

	switch (quoteCharString[0]) {
	/*
	 * its a comment 
	 * (consume everything up to the next new line) 
	 */
	case '!':
	{
		char tmp[MAX];

                sprintf(formatString, "%%%d[^\n\r\v\f]", (int)(sizeof(tmp)-1));
                status = fscanf (fp, "%[^\n\r\v\f]",tmp);
		pToken[0] = '\0';
		if (status<0) {
			return status;
		}
		break;
	}

	/*
	 * its a plain token
	 */
	case '\0':
                sprintf(formatString, " %%%ds", maxToken-1);

                status = fscanf (fp, formatString, pToken);
                if (status!=1) {
			if (status < 0){
				pToken[0] = '\0';
				return status;
			}
                }
		break;

	/*
	 * it was a quoted string
	 */
	default:
                sprintf(
			formatString, 
			"%%%d[^%c]", 
			maxToken-1, 
			quoteCharString[0]);
                status = fscanf (fp, formatString, pToken);
		if (status!=1) {
			if (status < 0){
				pToken[0] = '\0';
				return status;
			}
		}
                sprintf(formatString, "%%1[%c]", quoteCharString[0]);
                status = fscanf (fp, formatString, quoteCharString);
		if (status!=1) {
			errPrintf (
				-1L,
				__FILE__,
				__LINE__,
	"Resource file syntax error: unterminated string \"%s\"",
				pToken);
			pToken[0] = '\0';
			if (status < 0){
				return status;
			}
		}
		break;
        }
	return 0;
}
