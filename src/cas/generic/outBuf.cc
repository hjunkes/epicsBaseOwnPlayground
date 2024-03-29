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
 * Revision 1.7.8.1  2000/06/28 22:52:15  jhill
 * m_type => m_dataType
 *
 * Revision 1.7  1998/04/10 23:11:10  jhill
 * cosmetic
 *
 * Revision 1.6  1996/12/06 22:36:29  jhill
 * use destroyInProgress flag now functional nativeCount()
 *
 * Revision 1.5  1996/11/02 00:54:30  jhill
 * many improvements
 *
 * Revision 1.4  1996/09/16 18:24:07  jhill
 * vxWorks port changes
 *
 * Revision 1.3  1996/09/04 20:27:01  jhill
 * doccasdef.h
 *
 * Revision 1.2  1996/08/13 22:53:59  jhill
 * fixed little endian problem
 *
 * Revision 1.1.1.1  1996/06/20 00:28:15  jhill
 * ca server installation
 *
 *
 */

#include "server.h"
#include "outBufIL.h" // outBuf in line func

//
// outBuf::outBuf()
//
outBuf::outBuf(osiMutex &mutexIn, unsigned bufSizeIn) : 
	mutex(mutexIn),
	pBuf(NULL),
	bufSize(bufSizeIn),
	stack(0u)
{
}

//
// outBuf::init()
//
caStatus outBuf::init() 
{
	this->pBuf = new char [this->bufSize];
	if (!this->pBuf) {
		return S_cas_noMemory;
	}
	memset (this->pBuf, '\0', this->bufSize);
	return S_cas_success;
}

//
// outBuf::~outBuf()
//
outBuf::~outBuf()
{
	if (this->pBuf) {
		delete [] this->pBuf;
	}
}


//
//	outBuf::allocMsg() 
//
//	allocates space in the outgoing message buffer
//
//	(if space is avilable this leaves the send lock applied)
//			
caStatus outBuf::allocMsg (
bufSizeT	extsize,	// extension size 
caHdr		**ppMsg,
int		lockRequired
)
{
	bufSizeT msgsize;
	bufSizeT stackNeeded;
	
	extsize = CA_MESSAGE_ALIGN(extsize);

	msgsize = extsize + sizeof(caHdr);
	if (msgsize>this->bufSize || this->pBuf==NULL) {
		return S_cas_hugeRequest;
	}

	stackNeeded = this->bufSize - msgsize;

	//
	// In some special situations we preallocate space for
	// two messages and prefer not to lock twice 
	// (so that lock/unlock pairs will match)
	//
	if (lockRequired) {
		this->mutex.osiLock();
	}

	if (this->stack > stackNeeded) {
		
		/*
		 * Try to flush the output queue
		 */
		this->flush(casFlushSpecified, msgsize);

		/*
		 * If this failed then the fd is nonblocking 
		 * and we will let select() take care of it
		 */
		if (this->stack > stackNeeded) {
			this->mutex.osiUnlock();
			this->sendBlockSignal();
			return S_cas_sendBlocked;
		}
	}

	/*
	 * it fits so commitMsg() will move the stack pointer forward
	 */
	*ppMsg = (caHdr *) &this->pBuf[this->stack];

	return S_cas_success;
}



/*
 * outBuf::commitMsg()
 */
void outBuf::commitMsg ()
{
	caHdr 		*mp;
	bufSizeT	extSize;
	bufSizeT	diff;

	mp = (caHdr *) &this->pBuf[this->stack];

	extSize = CA_MESSAGE_ALIGN(mp->m_postsize);

	//
	// Guarantee that all portions of outgoing messages 
	// (including alignment pad) are initialized 
	//
	diff = extSize - mp->m_postsize;
	if (diff>0u) {
		char 		*pExt = (char *) (mp+1);
		memset(pExt + mp->m_postsize, '\0', diff);
	}

  	mp->m_postsize = extSize;

  	if (this->getDebugLevel()) {
		ca_printf (
"CAS Response => cmd=%d id=%x typ=%d cnt=%d psz=%d avail=%x outBuf ptr=%lx\n",
			mp->m_cmmd,
			mp->m_cid,
			mp->m_dataType,
			mp->m_count,
			mp->m_postsize,
			mp->m_available,
			(long)mp);
	}

	/*
	 * convert to network byte order
	 * (data following header is handled elsewhere)
	 */
	mp->m_cmmd = htons (mp->m_cmmd);
	mp->m_postsize = htons (mp->m_postsize);
	mp->m_dataType = htons (mp->m_dataType);
	mp->m_count = htons (mp->m_count);
	mp->m_cid = htonl (mp->m_cid);
	mp->m_available = htonl (mp->m_available);

	this->stack += sizeof(caHdr) + extSize;
	assert (this->stack <= this->bufSize);

	this->mutex.osiUnlock();
}

//
// outBuf::flush()
//
casFlushCondition outBuf::flush(casFlushRequest req, bufSizeT spaceRequired)
{
	bufSizeT 		nBytes;
	bufSizeT 		stackNeeded;
	bufSizeT 		nBytesNeeded;
	xSendStatus		stat;
	casFlushCondition	cond;

	if (!this->pBuf) {
		return casFlushDisconnect;
	}

	this->mutex.osiLock();

	if (req==casFlushAll) {
		nBytesNeeded = this->stack;
	}
	else {
		stackNeeded = this->bufSize - spaceRequired;
		if (this->stack>stackNeeded) {
			nBytesNeeded = this->stack - stackNeeded;
		}
		else {
			nBytesNeeded = 0u;
		}
	}

	if (nBytesNeeded==0u) {
		this->mutex.osiUnlock();
		return casFlushCompleted;
	}

	nBytes = 0u;
	stat = this->xSend(this->pBuf, this->stack, 
			nBytesNeeded, nBytes);
	if (stat == xSendOK) {
		if (nBytes) {
			bufSizeT len;
			char buf[64];

			if (nBytes >= this->stack) {
				this->stack=0u;	
				cond = casFlushCompleted;
			}
			else {
				len = this->stack-nBytes;
				//
				// memmove() is ok with overlapping buffers
				//
				memmove (this->pBuf, &this->pBuf[nBytes], len);
				this->stack = len;
				cond = casFlushPartial;
			}

			if (this->getDebugLevel()>2u) {
				this->clientHostName(buf,sizeof(buf));
				ca_printf(
					"CAS: Sent a %d byte reply to %s\n",
					nBytes, buf);
			}
		}
		else {
			cond = casFlushNone;
		}
	}
	else {
		cond = casFlushDisconnect;
	}
	this->mutex.osiUnlock();
	return cond;
}

//
// outBuf::show(unsigned level)
//
void outBuf::show(unsigned level) const
{
	if (level>1u) {
                printf(
			"\tUndelivered response bytes =%d\n",
                        this->bytesPresent());
	}
}

//
// outBuf::sendBlockSignal()
//
void outBuf::sendBlockSignal()
{
	fprintf(stderr, "In virtula base sendBlockSignal() ?\n");
}

