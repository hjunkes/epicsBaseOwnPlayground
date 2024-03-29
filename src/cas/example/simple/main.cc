/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

#include "exServer.h"
#include "fdManager.h"

//
// main()
// (example single threaded ca server tool main loop)
//
extern int main (int argc, const char **argv)
{
	osiTime		begin(osiTime::getCurrent());
	exServer	*pCAS;
	unsigned 	debugLevel = 0u;
	float		executionTime;
	char		pvPrefix[128] = "";
	unsigned	aliasCount = 1u;
	unsigned	scanOn = 1u;
	aitBool	forever = aitTrue;
	int		i;

	for (i=1; i<argc; i++) {
		if (sscanf(argv[i], "-d\t%u", &debugLevel)==1) {
			continue;
		}
		if (sscanf(argv[i],"-t %f", &executionTime)==1) {
			forever = aitFalse;
			continue;
		}
		if (sscanf(argv[i],"-p %127s", pvPrefix)==1) {
			continue;
		}
		if (sscanf(argv[i],"-c %u", &aliasCount)==1) {
			continue;
		}
		if (sscanf(argv[i],"-s %u", &scanOn)==1) {
			continue;
		}
		printf ("\"%s\"?\n", argv[i]);
		printf (
"usage: %s [-d<debug level> -t<execution time> -p<PV name prefix> -c<numbered alias count>] -s<1=scan on (default), 0=scan off]>\n", 
			argv[0]);

		return (1);
	}

	pCAS = new exServer(pvPrefix, aliasCount, (aitBool) scanOn);
	if (!pCAS) {
		return (-1);
	}

	pCAS->setDebugLevel(debugLevel);

	if (forever) {
		osiTime	delay(1000u,0u);
		//
		// loop here forever
		//
		while (aitTrue) {
			fileDescriptorManager.process(delay);
		}
	}
	else {
		osiTime total(executionTime);
		osiTime delay(osiTime::getCurrent() - begin);
		//
		// loop here untime the specified execution time
		// expires
		//
		while (delay < total) {
			fileDescriptorManager.process(delay);
			delay = osiTime::getCurrent() - begin;
		}
	}
	pCAS->show(2u);
	delete pCAS;
	return (0);
}

