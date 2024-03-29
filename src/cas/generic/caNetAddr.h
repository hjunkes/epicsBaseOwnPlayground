/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
//
// caNetAddr
//
// special cas specific network address class so:
// 1) we dont drag BSD socket headers into 
//	the server tool
// 2) we are able to use other networking services
//	besides sockets in the future
//
// get() will assert fail if the init flag has not been
//	set
//

#ifndef caNetAddrH
#define caNetAddrH

#ifdef caNetAddrSock
#include "osiSock.h"
#endif

#include "epicsAssert.h"
#include "shareLib.h"

class verifyCANetAddr {
public:
	inline void checkSize(); 
};

enum caNetAddrType {casnaUDF, casnaSock}; // only IP addresses (and undefined) supported at this time
class epicsShareClass caNetAddr {
	friend class verifyCANetAddr;
public:

	// 
	// clear()
	//
	inline void clear ()
	{
		this->type = casnaUDF;
	}

	//
	// caNetAddr()
	//
	inline caNetAddr ()
	{	
		this->clear();
	}

	inline int isSock () const
	{
		return this->type == casnaSock;
	}

	//
	// This is specified so that compilers will not use one of 
	// the following assignment operators after converting to a 
	// sockaddr_in or a sockaddr first.
	//
	// caNetAddr caNetAddr::operator =(const struct sockaddr_in&)
	// caNetAddr caNetAddr::operator =(const struct sockaddr&)
	//
	inline caNetAddr operator = (const caNetAddr &naIn)
	{
		this->addr = naIn.addr;
		this->type = naIn.type;
		return *this;
	}	

	//
	// conditionally drag BSD socket headers into the server
	// and server tool
	//
	// to use this #define caNetAddrSock
	//
#	ifdef caNetAddrSock
		inline void setSockIP(unsigned long inaIn, unsigned short portIn)
		{
			this->type = casnaSock;
			this->addr.sock.sin_family = AF_INET;
			this->addr.sock.sin_addr.s_addr = inaIn;
			this->addr.sock.sin_port = portIn;
		}	

		inline void setSockIP(const struct sockaddr_in &sockIPIn)
		{
			this->type = casnaSock;
			assert(sockIPIn.sin_family == AF_INET);
			this->addr.sock = sockIPIn;
		}	

		inline void setSock(const struct sockaddr &sock)
		{
			this->type = casnaSock;
			const struct sockaddr_in *psip = 
				(const struct sockaddr_in *) &sock;
			assert(sizeof(sock)==sizeof(this->addr.sock));
			this->addr.sock = *psip;
		}	

		inline caNetAddr(const struct sockaddr_in &sockIPIn)
		{
			this->setSockIP(sockIPIn);
		}

		inline caNetAddr operator = (const struct sockaddr_in &sockIPIn)
		{
			this->setSockIP(sockIPIn);
			return *this;
		}			

		inline caNetAddr operator = (const struct sockaddr &sockIn)
		{
			this->setSock(sockIn);
			return *this;
		}		

		inline struct sockaddr_in getSockIP() const
		{
			assert (this->type==casnaSock);
			return this->addr.sock;
		}

		inline struct sockaddr getSock() const
		{
			struct sockaddr sa;
			assert (this->type==casnaSock);
			assert (sizeof(sa)==sizeof(this->addr.sock));
			struct sockaddr_in *psain = (struct sockaddr_in *) &sa;
			*psain = this->addr.sock;

			return sa;
		}

		inline operator sockaddr_in () const
		{
			return this->getSockIP();
		}

		inline operator sockaddr () const
		{
			return this->getSock();
		}

	#	endif // caNetAddrSock

private:
	union {	
#		ifdef caNetAddrSock		
			struct sockaddr_in sock;
#		endif
		//
		// this must be as big or bigger 
		// than the largest field
		//
		// see checkDummySize() above
		//
		unsigned char dummy[16];	
	} addr;

	caNetAddrType type;
};

inline void verifyCANetAddr::checkSize()
{
	//
	// temp used because brain dead
	// ms compiler does not allow
	// use of scope res operator
	//
	caNetAddr t;
	size_t ds = sizeof (t.addr.dummy);
	size_t as = sizeof (t.addr);
	assert (ds==as);
}


#endif // caNetAddrH
