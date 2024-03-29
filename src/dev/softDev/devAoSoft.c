/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* devAoSoft.c */
/* base/src/dev $Id$ */

/* Device Support Routines for soft Analog Output Records*/
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
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
#include	<link.h>
#include	<special.h>
#include	<aoRecord.h>

/* added for Channel Access Links */
static long init_record();

/* Create the dset for devAoSoft */
static long write_ao();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_ao;
	DEVSUPFUN	special_linconv;
}devAoSoft={
	6,
	NULL,
	NULL,
	init_record,
	NULL,
	write_ao,
	NULL};


static long init_record(pao)
struct aoRecord *pao;
{

    long status=0;
    status = 2;
    return status;

} /* end init_record() */

static long write_ao(pao)
    struct aoRecord	*pao;
{
    long status;

    status = dbPutLink(&pao->out,DBR_DOUBLE, &pao->oval,1);

    return(status);
}
