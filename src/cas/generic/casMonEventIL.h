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
 * Revision 1.6  1998/10/27 21:34:19  jhill
 * avoid problems with early g++
 *
 * Revision 1.5  1998/10/23 00:28:20  jhill
 * fixed HP-UX warnings
 *
 * Revision 1.4  1998/06/16 02:27:53  jhill
 * use smart gdd ptr
 *
 * Revision 1.3  1997/04/10 19:34:11  jhill
 * API changes
 *
 * Revision 1.2  1996/11/02 00:54:17  jhill
 * many improvements
 *
 * Revision 1.1.1.1  1996/06/20 00:28:16  jhill
 * ca server installation
 *
 *
 */


#ifndef casMonEventIL_h
#define casMonEventIL_h

//
// casMonEvent::casMonEvent()
//
inline casMonEvent::casMonEvent () : 
		id(0u) {}

//
// casMonEvent::casMonEvent()
//
inline casMonEvent::casMonEvent (casMonitor &monitor, gdd &newValue) :
        pValue(&newValue),
        id(monitor.casRes::getId()) {}

//
// casMonEvent::casMonEvent()
//
inline casMonEvent::casMonEvent (const casMonEvent &initValue) :
        pValue(initValue.pValue),
        id(initValue.id) {}

//
// casMonEvent::operator =  ()
//
inline void casMonEvent::operator = (const class casMonEvent &monEventIn)
{
	//
	// this cast is a workaround for defects in early versions of GNU g++
	//
	this->pValue = (gdd *) monEventIn.pValue;
	this->id = monEventIn.id;
}

//
//  casMonEvent::clear()
//
inline void casMonEvent::clear()
{
	this->pValue = NULL;
	this->id = 0u;
}

//
// casMonEvent::getValue()
//
inline gdd *casMonEvent::getValue() const
{
	return this->pValue;
}

#endif // casMonEventIL_h

