/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* devMbboDirectSoft.c */
/* base/src/dev $Id$ */
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Matthew Needes
 *      Date:            10-08-93
 */
#include	<vxWorks.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	<alarm.h>
#include	<dbDefs.h>
#include	<dbAccess.h>
#include        <recSup.h>
#include	<devSup.h>
#include	<module_types.h>
#include	<mbboDirectRecord.h>
/* Create the dset for devMbboSoft */
static long init_record();
static long write_mbbo();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_mbbo;
}devMbboDirectSoft={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	write_mbbo
};

static long init_record(pmbbo)
struct mbboDirectRecord *pmbbo;
{
    long status = 0;
 
    /* dont convert */
    status = 2;
    return status;
 
} /* end init_record() */

static long write_mbbo(pmbbo)
    struct mbboDirectRecord	*pmbbo;
{
    long status;

    status = dbPutLink(&pmbbo->out,DBR_USHORT,&pmbbo->val,1);
    return(0);
}
