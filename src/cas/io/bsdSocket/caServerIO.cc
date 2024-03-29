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
// $Id$
//
// verify connection state prior to doing anything in this file
//
//
// $Log$
// Revision 1.9.8.1  2002/07/12 22:16:49  jba
// Updated license comments.
//
// Revision 1.9  1998/06/16 02:35:50  jhill
// use aToIPAddr and auto attach to winsock if its a static build
//
// Revision 1.8  1998/05/29 20:08:20  jhill
// use new sock ioctl() typedef
//
// Revision 1.7  1997/08/05 00:47:22  jhill
// fixed warnings
//
// Revision 1.6  1997/06/30 23:40:48  jhill
// use %p for pointers
//
// Revision 1.5  1997/06/13 09:16:12  jhill
// connect proto changes
//
// Revision 1.4  1997/04/10 19:40:31  jhill
// API changes
//
// Revision 1.3  1996/11/02 00:54:42  jhill
// many improvements
//
// Revision 1.2  1996/06/21 02:12:40  jhill
// SOLARIS port
//
// Revision 1.1.1.1  1996/06/20 00:28:19  jhill
// ca server installation
//
//

#include <ctype.h>

#include "server.h"
#include "sigPipeIgnore.h"
#include "addrList.h"
#include "bsdSocketResource.h"

static char *getToken(const char **ppString, char *pBuf, unsigned bufSIze);

int caServerIO::staticInitialized;

//
// caServerIO::~caServerIO()
//
caServerIO::~caServerIO()
{
	bsdSockRelease();
}

//
// caServerIO::staticInit()
//
inline void caServerIO::staticInit()
{
	if (caServerIO::staticInitialized) {
		return;
	}

	installSigPipeIgnore();

	caServerIO::staticInitialized = TRUE;
}


//
// caServerIO::init()
//
caStatus caServerIO::init(caServerI &cas)
{
	char buf[64u]; 
	const char *pStr; 
	char *pToken;
	caStatus stat;
	unsigned short port;
	struct sockaddr_in saddr;
	int autoBeaconAddr;

	if (!bsdSockAttach()) {
		return S_cas_internal;
	}

	caServerIO::staticInit();

	//
	// first try for the server's private port number env var.
	// If this available use the CA server port number (used by
	// clients to find the server). If this also isnt available
	// then use a hard coded default - CA_SERVER_PORT.
	//
	if (envGetConfigParamPtr(&EPICS_CAS_SERVER_PORT)) {
		port = caFetchPortConfig(&EPICS_CAS_SERVER_PORT, CA_SERVER_PORT);
	}
	else {
		port = caFetchPortConfig(&EPICS_CA_SERVER_PORT, CA_SERVER_PORT);
	}

	memset((char *)&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port =  ntohs (port);

	pStr = envGetConfigParam(&EPICS_CA_AUTO_ADDR_LIST, sizeof(buf), buf);
	if (pStr) {
		if (strstr(pStr,"no")||strstr(pStr,"NO")) {
			autoBeaconAddr = FALSE;
		}
		else if (strstr(pStr,"yes")||strstr(pStr,"YES")) {
			autoBeaconAddr = TRUE;
		}
		else {
			fprintf(stderr, 
		"CAS: EPICS_CA_AUTO_ADDR_LIST = \"%s\"? Assuming \"YES\"\n", pStr);
			autoBeaconAddr = TRUE;
		}
	}
	else {
		autoBeaconAddr = TRUE;
	}

	//
	// bind to the the interfaces specified - otherwise wildcard
	// with INADDR_ANY and allow clients to attach from any interface
	//
	pStr = envGetConfigParamPtr(&EPICS_CAS_INTF_ADDR_LIST);
	if (pStr) {
		int configAddrOnceFlag = TRUE;
		stat = S_cas_noInterface; 
		while ( (pToken = getToken(&pStr, buf, sizeof(buf))) ) {
			int status;

			status = aToIPAddr (pToken, port, &saddr);
			if (status) {
				ca_printf(
					"%s: Parsing '%s'\n",
					__FILE__,
					EPICS_CAS_INTF_ADDR_LIST.name);
				ca_printf(
					"\tBad internet address or host name: '%s'\n",
					pToken);
				continue;
			}
			stat = cas.addAddr(caNetAddr(saddr), autoBeaconAddr, configAddrOnceFlag);
			if (stat) {
				errMessage(stat, NULL);
				break;
			}
			configAddrOnceFlag = FALSE;
		}
	}
	else {
		saddr.sin_addr.s_addr = htonl(INADDR_ANY);
		stat = cas.addAddr(caNetAddr(saddr), autoBeaconAddr, TRUE);
		if (stat) {
			errMessage(stat, NULL);
		}
	}

	return stat;
}

//
// caServerIO::show()
//
void caServerIO::show (unsigned /* level */) const
{
	printf ("caServerIO at %p\n", this);
}

//
// getToken()
//
static char *getToken(const char **ppString, char *pBuf, unsigned bufSIze)
{
        const char *pToken;
        unsigned i;
 
        pToken = *ppString;
        while(isspace(*pToken)&&*pToken){
                pToken++;
        }
 
        for (i=0u; i<bufSIze; i++) {
                if (isspace(pToken[i]) || pToken[i]=='\0') {
                        pBuf[i] = '\0';
                        break;
                }
                pBuf[i] = pToken[i];
        }
 
        *ppString = &pToken[i];
 
        if(*pToken){
                return pBuf;
        }
        else{
                return NULL;
        }
}

