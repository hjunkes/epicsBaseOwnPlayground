/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

// Author: Jim Kowalkowski
// Date: 2/96
// 
// $Id$
//

#define AIT_CONVERT_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#define epicsExportSharedSymbols
#include "aitConvert.h"

extern "C" {
int aitNoConvert(void* /*dest*/,const void* /*src*/,aitIndex /*count*/) {return -1;}
}

#ifdef AIT_CONVERT
#undef AIT_CONVERT
#endif
#ifdef AIT_TO_NET_CONVERT
#undef AIT_TO_NET_CONVERT
#endif
#ifdef AIT_FROM_NET_CONVERT
#undef AIT_FROM_NET_CONVERT
#endif

#ifndef min
#define min(A,B) ((A)<(B)?(A):(B))
#endif

/* put the fixed conversion functions here (ones not generated) */

/* ------- extra string conversion functions --------- */
static int aitConvertStringString(void* d,const void* s,aitIndex c)
{
	// does not work - need to be fixed
	aitIndex i;
	aitString *in=(aitString*)s, *out=(aitString*)d;

	for(i=0;i<c;i++) out[i]=in[i];
	return 0;
}
static int aitConvertToNetStringString(void* d,const void* s,aitIndex c)
{ return aitConvertStringString(d,s,c);}
static int aitConvertFromNetStringString(void* d,const void* s,aitIndex c)
{ return aitConvertStringString(d,s,c);}

/* ------ all the fixed string conversion functions ------ */
static int aitConvertFixedStringFixedString(void* d,const void* s,aitIndex c)
{
	aitUint32 len = c*AIT_FIXED_STRING_SIZE;
	memcpy(d,s,len);
	return 0;
}
static int aitConvertToNetFixedStringFixedString(void* d,const void* s,aitIndex c)
{ return aitConvertFixedStringFixedString(d,s,c);}
static int aitConvertFromNetFixedStringFixedString(void* d,const void* s,aitIndex c)
{ return aitConvertFixedStringFixedString(d,s,c);}

static int aitConvertStringFixedString(void* d,const void* s,aitIndex c)
{
	aitIndex i;
	aitString* out = (aitString*)d;
	aitFixedString* in = (aitFixedString*)s;

	for(i=0;i<c;i++) out[i].copy(in[i].fixed_string);
	return 0;
}

static int aitConvertFixedStringString(void* d,const void* s,aitIndex c)
{
	aitIndex i;
	aitString* in = (aitString*)s;
	aitFixedString* out = (aitFixedString*)d;

	//
	// joh - changed this from strcpy() to stncpy() in order to:
	// 1) shut up purify 
	// 2) guarantee that all fixed strings will be terminated
	// 3) guarantee that we will not overflow a fixed string
	//
	for(i=0;i<c;i++){
		strncpy(out[i].fixed_string,in[i].string(),AIT_FIXED_STRING_SIZE);
		out[i].fixed_string[AIT_FIXED_STRING_SIZE-1u] = '\0';
	}
	return 0;
}

static int aitConvertToNetStringFixedString(void* d,const void* s,aitIndex c)
{ return aitConvertStringFixedString(d,s,c); }
static int aitConvertFromNetFixedStringString(void* d,const void* s,aitIndex c)
{ return aitConvertFixedStringString(d,s,c); }
static int aitConvertToNetFixedStringString(void* d,const void* s,aitIndex c)
{ return aitConvertStringFixedString(d,s,c); }
static int aitConvertFromNetStringFixedString(void* d,const void* s,aitIndex c)
{ return aitConvertFixedStringString(d,s,c); }

#define AIT_CONVERT 1
#include "aitConvertGenerated.cc"
#undef AIT_CONVERT

/* include the network byte order functions if needed */
#ifdef AIT_NEED_BYTE_SWAP

#define AIT_TO_NET_CONVERT 1
#include "aitConvertGenerated.cc"
#undef AIT_TO_NET_CONVERT

#define AIT_FROM_NET_CONVERT 1
#include "aitConvertGenerated.cc"
#undef AIT_FROM_NET_CONVERT

#endif

