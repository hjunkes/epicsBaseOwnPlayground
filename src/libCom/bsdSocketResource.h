/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

#include "osiSock.h"
#include "shareLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * convert IP address to ASCII in this order
 * 1) look for matching host name
 * 2) convert to traditional dotted IP address (with trailing port)
 */
epicsShareFunc void epicsShareAPI ipAddrToA
	(const struct sockaddr_in *pInetAddr, char *pBuf, unsigned bufSize);

/*
 * attempt to convert ASCII string to an IP address in this order
 * 1) look for traditional doted ip with optional port
 * 2) look for raw number form of ip address with optional port
 * 3) look for valid host name with optional port
 */
epicsShareFunc int epicsShareAPI aToIPAddr
	(const char *pAddrString, unsigned short defaultPort, struct sockaddr_in *pIP);

/*
 * attempt to convert ASCII host name string with optional port to an IP address
 */
epicsShareFunc int epicsShareAPI hostToIPAddr 
				(const char *pHostName, struct in_addr *pIPA);
/*
 * attach to BSD socket library
 */
epicsShareFunc int epicsShareAPI bsdSockAttach(); /* returns T if success, else F */

/*
 * release BSD socket library
 */
epicsShareFunc void epicsShareAPI bsdSockRelease();

#ifdef _WIN32
epicsShareFunc const char * epicsShareAPI getLastWSAErrorAsString();
epicsShareFunc unsigned epicsShareAPI wsaMajorVersion();
#endif

epicsShareFunc void epicsShareAPI setCloseOnExec ( SOCKET sock );

#ifdef __cplusplus
}
#endif

