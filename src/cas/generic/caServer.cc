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
 * Revision 1.8.6.2  2001/09/20 22:43:14  jhill
 * dont use bool - early compilers dont support it
 *
 * Revision 1.8.6.1  2000/07/13 23:37:24  jhill
 * fixed missing type in static init
 *
 * Revision 1.8  1997/08/05 00:46:56  jhill
 * fixed warnings
 *
 * Revision 1.7  1997/06/25 05:09:00  jhill
 * removed templInst.cc
 *
 * Revision 1.6  1997/06/13 09:15:50  jhill
 * connect proto changes
 *
 * Revision 1.5  1997/04/10 19:33:52  jhill
 * API changes
 *
 * Revision 1.4  1996/11/02 00:53:53  jhill
 * many improvements
 *
 * Revision 1.3  1996/09/16 18:23:56  jhill
 * vxWorks port changes
 *
 * Revision 1.2  1996/06/21 02:30:52  jhill
 * solaris port
 *
 * Revision 1.1.1.1  1996/06/20 00:28:14  jhill
 * ca server installation
 *
 *
 */

#include "dbMapper.h"		// ait to dbr types 
#include "gddAppTable.h"	// EPICS application type table

#include "server.h"
#include "caServerIIL.h"	// caServerI in line func

//
// if the compiler supports explicit instantiation of
// template member functions
//
#if defined(EXPL_TEMPL)
	//
	// From Stroustrups's "The C++ Programming Language"
	// Appendix A: r.14.9 
	//
	// This explicitly instantiates the template class's member
	// functions into "templInst.o"
	//
	template class resTable <casEventMaskEntry, stringId>;
	template class resTable <casRes, uintId>;
#endif


//
// caServer::caServer()
//
epicsShareFunc caServer::caServer(unsigned pvCountEstimateIn) :
        	pCAS (new caServerI(*this, pvCountEstimateIn)),
		valueEventMask(this->registerEvent("value")),
		logEventMask(this->registerEvent("log")),
		alarmEventMask(this->registerEvent("alarm"))
{
	static int init = 0;

	if (!init) {
		gddMakeMapDBR(gddApplicationTypeTable::app_table);
		init = 1;
	}

	if (!this->pCAS) {
		errMessage(S_cas_noMemory, NULL);
		return;
	}

	if (!this->pCAS->ready()) {
		delete this->pCAS;
		this->pCAS = NULL;
		return;
	}
}

//
// caServer::~caServer()
//
epicsShareFunc caServer::~caServer()
{
	if (this->pCAS) {
		delete this->pCAS;
	}
}

//
// caServer::pvExistTest()
//
epicsShareFunc pvExistReturn caServer::pvExistTest (const casCtx &, const char *)
{
	return pverDoesNotExistHere;
}

//
// caServer::createPV()
//
epicsShareFunc pvCreateReturn caServer::createPV (const casCtx &, const char *)
{
	return S_casApp_pvNotFound;
}

//
// caServer::registerEvent()
//
epicsShareFunc casEventMask caServer::registerEvent (const char *pName)
{
	if (this->pCAS) {
		return this->pCAS->registerEvent(pName);
	}
	else {
		casEventMask emptyMask;
		printf("caServer:: no server internals attached\n");
		return emptyMask;
	}
}

//
// caServer::show()
//
epicsShareFunc void caServer::show(unsigned level) const
{
	if (this->pCAS) {
		this->pCAS->show(level);
	}
	else {
		printf("caServer:: no server internals attached\n");
	}
}

//
// caServer::setDebugLevel()
//
epicsShareFunc void caServer::setDebugLevel (unsigned level)
{
	if (pCAS) {
		this->pCAS->setDebugLevel(level);
	}
	else {
		printf("caServer:: no server internals attached\n");
	}
}

//
// caServer::getDebugLevel()
//
epicsShareFunc unsigned caServer::getDebugLevel ()
{
        if (pCAS) {
                return this->pCAS->getDebugLevel();
        }
        else {
		printf("caServer:: no server internals attached\n");
		return 0u;
        }
}

//
// casRes::~casRes()
//
// This must be virtual so that derived destructor will
// be run indirectly. Therefore it cannot be inline.
//
casRes::~casRes()
{
}

//
// This must be virtual so that derived destructor will
// be run indirectly. Therefore it cannot be inline.
//
casEvent::~casEvent() 
{
}

