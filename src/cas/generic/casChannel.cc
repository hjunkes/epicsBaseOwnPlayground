/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 *      $Id$
 *
 *      Author  Jeffrey O. Hill
 *              johill@lanl.gov
 *              505 665 1831
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *      This software was produced under  U.S. Government contracts:
 *      (W-7405-ENG-36) at the Los Alamos National Laboratory,
 *      and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *      Initial development by:
 *              The Controls and Automation Group (AT-8)
 *              Ground Test Accelerator
 *              Accelerator Technology Division
 *              Los Alamos National Laboratory
 *
 *      Co-developed with
 *              The Controls and Computing Group
 *              Accelerator Systems Division
 *              Advanced Photon Source
 *              Argonne National Laboratory
 *
 *
 * History
 * $Log$
 * Revision 1.6  1997/08/05 00:47:03  jhill
 * fixed warnings
 *
 * Revision 1.5  1997/04/10 19:33:59  jhill
 * API changes
 *
 * Revision 1.4  1996/09/04 20:17:34  jhill
 * use ptr not ref to satisfy MSVISC++
 *
 * Revision 1.3  1996/08/13 22:52:31  jhill
 * changes for MVC++
 *
 * Revision 1.2  1996/07/01 19:56:09  jhill
 * one last update prior to first release
 *
 * Revision 1.1.1.1  1996/06/20 00:28:14  jhill
 * ca server installation
 *
 *
 */

#include "server.h"
#include "casChannelIIL.h" // casChannelI inline func
#include "casPVListChanIL.h" // casPVListChan inline func

//
// casChannel::casChannel()
//
epicsShareFunc casChannel::casChannel(const casCtx &ctx) : 
	casPVListChan (ctx, *this) 
{
}

//
// casChannel::~casChannel()
//
epicsShareFunc casChannel::~casChannel() 
{
}

epicsShareFunc casPV *casChannel::getPV()
{
	casPVI *pPVI = &this->casChannelI::getPVI();

	if (pPVI!=NULL) {
		return pPVI->interfaceObjectPointer();
	}
	else {
		return NULL;
	}
}

//
// casChannel::setOwner()
//
epicsShareFunc void casChannel::setOwner(const char * const /* pUserName */, 
	const char * const /* pHostName */)
{
	//
	// NOOP
	//
}

//
// casChannel::readAccess()
//
epicsShareFunc aitBool casChannel::readAccess () const 
{
	return aitTrue;
}

//
// casChannel::writeAccess()
//
epicsShareFunc aitBool casChannel::writeAccess() const 
{
	return aitTrue;
}


//
// casChannel::confirmationRequested()
//
epicsShareFunc aitBool casChannel::confirmationRequested() const 
{
	return aitFalse;
}

//
// casChannel::show()
//
epicsShareFunc void casChannel::show(unsigned level) const
{
	if (level>2u) {
		printf("casChannel: read access = %d\n",
			this->readAccess());
		printf("casChannel: write access = %d\n",
			this->writeAccess());
		printf("casChannel: confirmation requested = %d\n",
			this->confirmationRequested());
	}
}

//
// casChannel::destroy()
//
epicsShareFunc void casChannel::destroy()
{
	delete this;
}

//
// casChannel::postAccessRightsEvent()
//
epicsShareFunc void casChannel::postAccessRightsEvent()
{
	this->casChannelI::postAccessRightsEvent();
}

