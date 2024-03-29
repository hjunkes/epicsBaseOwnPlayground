/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* $Id$
 * Very efficient routines to convert numbers to strings
 *
 *      Author: Bob Dalesio wrote cvtFloatToString (called FF_TO_STR)
 *			Code is same for cvtDoubleToString
 *		Marty Kraimer wrote cvtCharToString,cvtUcharToString
 *			cvtShortToString,cvtUshortToString,
 *			cvtLongToString, and cvtUlongToString
 *		Mark Anderson wrote cvtLongToHexString, cvtLongToOctalString,
 *			adopted cvt[Float/Double]ExpString and
 *			cvt[Float/Double]CompactString from fToEStr
 *			and fixed calls to gcvt
 *      Date:   12-9-92
 *
 */
#ifndef INCcvtFasth
#define INCcvtFasth

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "shareLib.h"

#if defined(__STDC__) || defined(__cplusplus)

/*
 * each of these functions return the number of characters "transmitted"
 *	(as in ANSI-C/POSIX.1/XPG3 sprintf() functions)
 */
epicsShareFunc int epicsShareAPI 
	cvtFloatToString(float value, char *pstring, unsigned short precision);
epicsShareFunc int epicsShareAPI 
	cvtDoubleToString(double value, char *pstring, unsigned short precision);
epicsShareFunc int epicsShareAPI 
	cvtFloatToExpString(float value, char *pstring, unsigned short precision);
epicsShareFunc int epicsShareAPI 
	cvtDoubleToExpString(double value, char *pstring, unsigned short precision);
epicsShareFunc int epicsShareAPI 
	cvtFloatToCompactString(float value, char *pstring, unsigned short precision);
epicsShareFunc int epicsShareAPI 
	cvtDoubleToCompactString(double value, char *pstring, unsigned short precision);
epicsShareFunc int epicsShareAPI 
	cvtCharToString(char value, char *pstring);
epicsShareFunc int epicsShareAPI 
	cvtUcharToString(unsigned char value, char *pstring);
epicsShareFunc int epicsShareAPI 
	cvtShortToString(short value, char *pstring);
epicsShareFunc int epicsShareAPI 
	cvtUshortToString(unsigned short value, char *pstring);
epicsShareFunc int epicsShareAPI 
	cvtLongToString(long value, char *pstring);
epicsShareFunc int epicsShareAPI 
	cvtUlongToString(unsigned long value, char *pstring);
epicsShareFunc int epicsShareAPI 
	cvtLongToHexString(long value, char *pstring);
epicsShareFunc int epicsShareAPI 
	cvtLongToOctalString(long value, char *pstring);
epicsShareFunc unsigned long epicsShareAPI cvtBitsToUlong(
	unsigned long  src,
	unsigned bitFieldOffset,
	unsigned  bitFieldLength);
epicsShareFunc unsigned long epicsShareAPI cvtUlongToBits(
	unsigned long src,
	unsigned long dest,
	unsigned      bitFieldOffset,
	unsigned      bitFieldLength);

#else /*__STDC__*/

epicsShareFunc int epicsShareAPI cvtFloatToString();
epicsShareFunc int epicsShareAPI cvtDoubleToString();
epicsShareFunc int epicsShareAPI cvtFloatToExpString();
epicsShareFunc int epicsShareAPI cvtDoubleToExpString();
epicsShareFunc int epicsShareAPI cvtFloatToCompactString();
epicsShareFunc int epicsShareAPI cvtDoubleToCompactString();
epicsShareFunc int epicsShareAPI cvtCharToString();
epicsShareFunc int epicsShareAPI cvtUcharToString();
epicsShareFunc int epicsShareAPI cvtShortToString();
epicsShareFunc int epicsShareAPI cvtUshortToString();
epicsShareFunc int epicsShareAPI cvtLongToString();
epicsShareFunc int epicsShareAPI cvtUlongToString();
epicsShareFunc int epicsShareAPI cvtLongToHexString();
epicsShareFunc int epicsShareAPI cvtLongToOctalString();
epicsShareFunc unsigned long epicsShareAPI cvtBitsToUlong();
epicsShareFunc unsigned long epicsShareAPI cvtUlongToBits();

#endif /*__STDC__*/

#ifdef __cplusplus
}
#endif

#endif /*INCcvtFasth*/
