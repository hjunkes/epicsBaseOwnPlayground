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
 *	O N L I N E _ N O T I F Y . C
 *
 *	tell CA clients this a server has joined the network
 *
 *	Author:	Jeffrey O. Hill
 *		hill@luke.lanl.gov
 *		(505) 665 1831
 *	Date:	103090
 *
 *	Experimental Physics and Industrial Control System (EPICS)
 *
 */

static char *sccsId = "@(#) $Id$";

/*
 * ansi includes
 */
#include <string.h>

/*
 *	system includes
 */
#include <vxWorks.h>
#include <types.h>
#include <sockLib.h>
#include <socket.h>
#include <errnoLib.h>
#include <in.h>
#include <logLib.h>
#include <sysLib.h>
#include <taskLib.h>

#define MAX_BLOCK_THRESHOLD 100000

/*
 *	EPICS includes
 */
#include "envDefs.h"
#include "server.h"
#include "task_params.h"

/*
 *	RSRV_ONLINE_NOTIFY_TASK
 *	
 *
 *
 */
int rsrv_online_notify_task()
{
	caAddrNode		*pNode;
  	unsigned long		delay;
	unsigned long		maxdelay;
	long			longStatus;
	double			maxPeriod;
	caHdr			msg;
  	struct sockaddr_in	recv_addr;
	int			status;
	int			sock;
  	int			true = TRUE;
	unsigned short		port;

	taskwdInsert(taskIdSelf(),NULL,NULL);

	longStatus = envGetDoubleConfigParam (
				&EPICS_CA_BEACON_PERIOD,
				&maxPeriod);
	if (longStatus || maxPeriod<=0.0) {
		maxPeriod = 15.0;
		epicsPrintf (
			"EPICS \"%s\" float fetch failed\n",
			EPICS_CA_BEACON_PERIOD.name);
		epicsPrintf (
			"Setting \"%s\" = %f\n",
			EPICS_CA_BEACON_PERIOD.name,
			maxPeriod);
	}

	/*
	 * 1 tick initial delay between beacons
	 */
	delay = 1ul;
	maxdelay = (unsigned long) maxPeriod*sysClkRateGet();

  	/* 
  	 *  Open the socket.
  	 *  Use ARPA Internet address format and datagram socket.
  	 *  Format described in <sys/socket.h>.
   	 */
  	if((sock = socket (AF_INET, SOCK_DGRAM, 0)) == ERROR){
    		logMsg("CAS: online socket creation error\n",
			0,
			0,
			0,
			0,
			0,
			0);
    		abort();
  	}

      	status = setsockopt(	sock,
				SOL_SOCKET,
				SO_BROADCAST,
				(char *)&true,
				sizeof(true));
      	if(status<0){
    		abort();
      	}

      	bfill((char *)&recv_addr, sizeof recv_addr, 0);
      	recv_addr.sin_family = AF_INET;
      	recv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* let slib pick lcl addr */
      	recv_addr.sin_port = htons(0);	 /* let slib pick port */
      	status = bind(sock, (struct sockaddr *)&recv_addr, sizeof recv_addr);
      	if(status<0)
		abort();

   	bfill((char *)&msg, sizeof msg, 0);
	msg.m_cmmd = htons (CA_PROTO_RSRV_IS_UP);
	msg.m_count = htons (ca_server_port);

	ellInit(&beaconAddrList);

	/*
	 * load user and auto configured
	 * broadcast address list
	 */
	port = caFetchPortConfig(&EPICS_CA_REPEATER_PORT, CA_REPEATER_PORT);
	caSetupBCastAddrList (&beaconAddrList, sock, port);

#	ifdef DEBUG
		caPrintAddrList(&beaconAddrList);
#	endif

 	while(TRUE){
		int maxBlock;

		/*
		 * check max block and disable new channels
		 * if its to small
		 */
		maxBlock = memFindMax();
		if(maxBlock<MAX_BLOCK_THRESHOLD){
			casBelowMaxBlockThresh = TRUE;
		}
		else{
			casBelowMaxBlockThresh = FALSE;
		}

		pNode = (caAddrNode *) beaconAddrList.node.next;
		while(pNode){
			msg.m_available = 
				pNode->srcAddr.in.sin_addr.s_addr;
        		status = sendto(
					sock,
        				(char *)&msg,
        				sizeof(msg),
        				0,
					&pNode->destAddr.sa,
					sizeof(pNode->destAddr.sa));
      			if(status < 0){
				logMsg( "%s: CA beacon error was \"%s\"\n",
					(int) __FILE__,
					(int) strerror(errnoGet()),
					0,
					0,
					0,
					0);
			}
			else{
				assert(status == sizeof(msg));
			}
			
			pNode = (caAddrNode *)pNode->node.next;
		}
		taskDelay(delay);
		delay = min(delay << 1, maxdelay);
	}

}


