/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
#ifndef GDD_NEWDEL_H
#define GDD_NEWDEL_H

/*
 * Author: Jim Kowalkowski
 * Date: 2/96
 *
 * $Id$
 */

// this file if formatted with tab stop = 4

#include <stdlib.h>
#include "gddSemaphore.h"

// Avoid using templates at the cost of very poor readability.
// This forces the user to have a static data member named "gddNewDel_freelist"

// To use this stuff:
//
// ** In class description header file:
//	class myClass
//	{
//	public:
//		gdd_NEWDEL_FUNC(address_to_be_used_for_freelist_next_pointer)
//	private:
//		gdd_NEWDEL_DATA(myClass)
//	};
//
// ** In source file where functions for class are written:
//	gdd_NEWDEL_STAT(myClass)
//	gdd_NEWDEL_DEL(myClass)
//	gdd_NEWDEL_NEW(myClass)

#define gdd_CHUNK_NUM 20
#define gdd_CHUNK(mine) (gdd_CHUNK_NUM*sizeof(mine))

// private data to add to a class
#define gdd_NEWDEL_DATA \
	static char* newdel_freelist; \
	static gddSemaphore newdel_lock;

// public interface for the new/delete stuff
// user gives this macro the address they want to use for the next pointer
#define gdd_NEWDEL_FUNC(fld) \
	void* operator new(size_t); \
	void operator delete(void*); \
	char* newdel_next(void) { char** x=(char**)&(fld); return *x; } \
	void newdel_setNext(char* n) { char** x=(char**)&(fld); *x=n; }

// declaration of the static variable for the free list
#define gdd_NEWDEL_STAT(clas) \
	char* clas::newdel_freelist=NULL; \
	gddSemaphore clas::newdel_lock;

// code for the delete function
#define gdd_NEWDEL_DEL(clas) \
 void clas::operator delete(void* v) { \
	clas* dn = (clas*)v; \
	if(dn->newdel_next()==(char*)(-1)) free((char*)v); \
	else { \
		clas::newdel_lock.take(); \
		dn->newdel_setNext(clas::newdel_freelist); \
		clas::newdel_freelist=(char*)dn; \
		clas::newdel_lock.give(); \
	} \
 }

// following function assumes that reading/writing address is atomic

// code for the new function
#define gdd_NEWDEL_NEW(clas) \
 void* clas::operator new(size_t size) { \
	int tot; \
	clas *nn,*dn; \
	if(!clas::newdel_freelist) { \
		tot=gdd_CHUNK_NUM; \
		nn=(clas*)malloc(gdd_CHUNK(clas)); \
		gddCleanUp::Add(nn); \
		for(dn=nn;--tot;dn++) dn->newdel_setNext((char*)(dn+1)); \
		clas::newdel_lock.take(); \
		(dn)->newdel_setNext(clas::newdel_freelist); \
		clas::newdel_freelist=(char*)nn; \
		clas::newdel_lock.give(); \
	} \
	if(size==sizeof(clas)) { \
		clas::newdel_lock.take(); \
		dn=(clas*)clas::newdel_freelist; \
		clas::newdel_freelist=((clas*)clas::newdel_freelist)->newdel_next(); \
		clas::newdel_lock.give(); \
		dn->newdel_setNext(NULL); \
	} else { \
		dn=(clas*)malloc(size); \
		dn->newdel_setNext((char*)(-1)); \
	} \
	return (void*)dn; \
 }

class gddCleanUpNode
{
public:
	void* buffer;
	gddCleanUpNode* next;
};

class gddCleanUp
{
public:
	gddCleanUp(void);
	~gddCleanUp(void);

	static void Add(void*);
	static void CleanUp(void);
private:
	static gddCleanUpNode* bufs;
	static gddSemaphore lock;
};

#endif

