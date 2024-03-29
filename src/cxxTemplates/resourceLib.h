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
 */

#ifndef INCresourceLibh
#define INCresourceLibh 

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#ifndef assert // allow use of epicsAssert.h
#include <assert.h>
#endif

#include "tsSLList.h"
#include "shareLib.h"

typedef int resLibStatus;
typedef unsigned resTableIndex;

#define resTableIndexBitWidth (sizeof(resTableIndex)*CHAR_BIT)

//
// class T must derive from class ID and also from class tsSLNode<T>
//
// NOTE: Classes installed into this table should have
// a virtual destructor so that the delete in ~resTable() will
// work correctly.
//
template <class T, class ID>
class resTable {
public:
	resTable() :
		pTable(0), hashIdMask(0), hashIdNBits(0), nInUse(0) {}

	int init(unsigned nHashTableEntries);

	virtual ~resTable();

	//
	// destroy all res in the table
	//
	void destroyAllEntries();

	//
	// call T::show(level) for each res in the table
	//
	void show (unsigned level) const;

	//
	// add a res to the table
	//
	int add (T &res);

	//
	// remove a res from the table
	//
	T *remove (const ID &idIn)
	{
		tsSLList<T> &list = this->pTable[this->hash(idIn)];
		return this->findDelete(list, idIn);
	}


	//
	// find an res in the table
	//
	T *lookup (const ID &idIn)
	{
		tsSLList<T> &list = this->pTable[this->hash(idIn)];
		return this->find(list, idIn);
	}

#ifdef _MSC_VER
	//
	// required by MS vis c++ 5.0 (but not by 4.0)
	//
	typedef void (T::*pResTableMFArg_t)();
#	define pResTableMFArg(ARG) pResTableMFArg_t ARG
#else
	//
	// required by gnu g++ 2.7.2
	//
#	define pResTableMFArg(ARG) void (T:: * ARG)()
#endif

	//
	// Call (pT->*pCB) () for each item in the table
	//
	// where pT is a pointer to type T and pCB is
	// a pointer to a memmber function of T with 
	// no parameters that returns void
	//
	void traverse(pResTableMFArg(pCB));

private:
	tsSLList<T>	*pTable;
	unsigned	hashIdMask;
	unsigned	hashIdNBits;
	unsigned	nInUse;

	resTableIndex hash(const ID & idIn)
	{
		return idIn.resourceHash(this->hashIdNBits) 
				& this->hashIdMask;
	}

	//
	// find
	// searches from where the iterator points to the
	// end of the list for idIn
	//
	// iterator points to the item found upon return
	// (or NULL if nothing matching was found)
	//
	T *find (tsSLList<T> &list, const ID &idIn);

	//
	// findDelete
	// searches from where the iterator points to the
	// end of the list for idIn
	//
	// iterator points to the item found upon return
	// (or NULL if nothing matching was found)
	//
	// removes the item if it finds it
	//
	T *findDelete (tsSLList<T> &list, const ID &idIn);
};

//
// Some ID classes that work with the above template
//


//
// unsigned identifier
//
class epicsShareClass uintId {
public:
	uintId (unsigned idIn=UINT_MAX) : id(idIn) {}
	virtual ~uintId();

	int operator == (const uintId &idIn)
	{
	    return this->id == idIn.id;
	}

	//
	// uintId::resourceHash()
	//
	resTableIndex resourceHash(unsigned nBitsId) const;

	const unsigned getId() const
	{
		return id;
	}
protected:
	unsigned id;
};


//
// special resource table which uses 
// unsigned integer keys allocated in chronological sequence
// 
// NOTE: ITEM must public inherit from uintRes<ITEM>
//
template <class ITEM>
class uintResTable : public resTable<ITEM, uintId> {
public:
	uintResTable() : allocId(1u) {} // hashing is faster close to zero
	virtual ~uintResTable();

	inline void installItem(ITEM &item);
private:
	unsigned 	allocId;
};

//
// resource with unsigned chronological identifier
//
template <class ITEM>
class uintRes : public uintId, public tsSLNode<ITEM> {
friend class uintResTable<ITEM>;
public:
	uintRes(unsigned idIn=UINT_MAX) : uintId(idIn) {}
	virtual ~uintRes();
private:
	//
	// workaround for bug in DEC compiler
	//
	void setId(unsigned newId) {this->id = newId;}
};


//
// character string identifier
//
// NOTE: to be robust in situations where the new()
// in the constructor might fail a careful consumer 
// of this class should check to see if the 
// stringId::resourceName() below
// returns a valid (non--NULL) string pointer.
// Eventually an exception will be thrown if
// new fails (when this is portable).
//
class epicsShareClass stringId {
public:
	enum allocationType {copyString, refString};

	//
	// allocCopyString()
	//
	static inline char * allocCopyString(const char * const pStr)
	{
		char *pNewStr = new char [strlen(pStr)+1u];
		if (pNewStr) {
			strcpy (pNewStr, pStr);
		}
		return pNewStr;
	}

	//
	// stringId() constructor
	//
	// Use typeIn==refString only if the string passed in will exist
	// and remain constant during the entire lifespan of the stringId
	// object.
	//
	stringId (char const * const idIn, allocationType typeIn=copyString) :
		pStr(typeIn==copyString?allocCopyString(idIn):idIn),
		allocType(typeIn) {}

	virtual ~ stringId();

	//
	// The hash algorithm is a modification of the algorithm described in 
	// Fast Hashing of Variable Length Text Strings, Peter K. Pearson, 
	// Communications of the ACM, June 1990 
	// The modifications were designed by Marty Kraimer
	//
	resTableIndex resourceHash(unsigned nBitsId) const;
 
	int operator == (const stringId &idIn)
	{
		if (this->pStr!=NULL && idIn.pStr!=NULL) {
			return strcmp(this->pStr,idIn.pStr)==0;
		}
		else {
			return 0u; // not equal
		}
	}

	//
	// return the pointer to the string
	// (also used to test to see if "new()"
	// failed in the constructor
	//
	const char * resourceName()
	{
		return this->pStr;
	}

	void show (unsigned level) const
	{
		if (level>2u) {
			printf ("resource id = %s\n", this->pStr);
		}
	}
private:
	const char * const pStr;
	allocationType const allocType;
	static const unsigned char stringIdFastHash[256];
};

//
// resTable<T,ID>::init()
//
template <class T, class ID>
int resTable<T,ID>::init(unsigned nHashTableEntries) 
{
	unsigned	nbits;

	if (nHashTableEntries<1u) {
		return -1;
	}

	//
	// count the number of bits in the hash index
	//
	for (nbits=0; nbits<resTableIndexBitWidth; nbits++) {
		this->hashIdMask = (1<<nbits) - 1;
		if ( ((nHashTableEntries-1) & ~this->hashIdMask) == 0){
			break;
		}
	}
	this->hashIdNBits = nbits;
	this->nInUse = 0u;
	this->pTable = new tsSLList<T> [this->hashIdMask+1u];
	if (!pTable) {
		return -1;
	}
	return 0;
}

//
// resTable<T,ID>::destroyAllEntries()
//
template <class T, class ID>
void resTable<T,ID>::destroyAllEntries()
{
	tsSLList<T> *pList = this->pTable;

	while (pList<&this->pTable[this->hashIdMask+1]) {
		T *pItem;
		T *pNextItem;

		{
			tsSLIter<T> iter(*pList);
			pItem = iter();
			while (pItem) {
				pNextItem = iter();
				pItem->destroy();
				pItem = pNextItem;
			}
		}

		//
		// Check to see if a defective class is
		// installed that does not remove itself
		// from the table when it is destroyed.
		//
		{
			tsSLIterRm<T> iter(*pList);
			while ( (pItem=iter()) ) {
				fprintf(stderr, 
"Warning: Defective class still in resTable<T,ID> after it was destroyed\n");
				//
				// remove defective class
				//
				iter.remove();
				this->nInUse--;
			}
		}

		pList++;
	}
}

//
// resTable<T,ID>::show
//
template <class T, class ID>
void resTable<T,ID>::show (unsigned level) const
{
	tsSLList<T> *pList;
	double X;
	double XX;
	double mean;
	double stdDev;
	unsigned maxEntries;

	printf("resTable with %d resources installed\n", this->nInUse);

	if (level >=1u) {
		pList = this->pTable;
		X = 0.0;
		XX = 0.0;
		maxEntries = 0u;
		while (pList < &this->pTable[this->hashIdMask+1]) {
			unsigned count;
			tsSLIter<T> iter(*pList);
			T *pItem;

			count = 0;
			while ( (pItem = iter()) ) {
				if (level >= 3u) {
					pItem->show (level);
				}
				count++;
			}
			if (count>0u) {
				X += count;
				XX += count*count;
				if (count>maxEntries) {
					maxEntries = count;
				}
			}
			pList++;
		}
	 
		mean = X/(this->hashIdMask+1);
		stdDev = sqrt(XX/(this->hashIdMask+1) - mean*mean);
		printf( 
	"entries/occupied table entry - mean = %f std dev = %f max = %d\n",
			mean, stdDev, maxEntries);
	}
}

//
// resTable<T,ID>::traverse
//
template <class T, class ID>
void resTable<T,ID>::traverse (pResTableMFArg(pCB))
{
	tsSLList<T> 	*pList;

	pList = this->pTable;
	while (pList < &this->pTable[this->hashIdMask+1]) {
		tsSLIter<T> iter(*pList);
		T *pItem;

		while ( (pItem = iter()) ) {
			(pItem->*pCB) ();
		}
		pList++;
	}
}

//
// add a res to the table
//
template <class T, class ID>
int resTable<T,ID>::add (T &res)
{
	//
	// T must derive from ID
	//
	tsSLList<T> &list = this->pTable[this->hash(res)];

	if (this->find(list, res) != 0) {
		return -1;
	}
	list.add(res);
	this->nInUse++;
	return 0;
}

//
// find
// searches from where the iterator points to the
// end of the list for idIn
//
// iterator points to the item found upon return
// (or NULL if nothing matching was found)
//
template <class T, class ID>
T *resTable<T,ID>::find (tsSLList<T> &list, const ID &idIn)
{
	tsSLIter<T> iter(list);
	T *pItem;
	ID *pId;

	while ( (pItem = iter()) ) {
		pId = pItem;
		if (*pId == idIn) {
			break;
		}
	}
	return pItem;
}

//
// findDelete
// searches from where the iterator points to the
// end of the list for idIn
//
// iterator points to the item found upon return
// (or NULL if nothing matching was found)
//
// removes the item if it finds it
//
template <class T, class ID>
T *resTable<T,ID>::findDelete (tsSLList<T> &list, const ID &idIn)
{
	tsSLIterRm<T> iter(list);
	T *pItem;
	ID *pId;

	while ( (pItem = iter()) ) {
		pId = pItem;
		if (*pId == idIn) {
			iter.remove();
			this->nInUse--;
			break;
		}
	}
	return pItem;
}

//
// ~resTable<T,ID>::resTable()
//
template <class T, class ID>
resTable<T,ID>::~resTable() 
{
	if (this->pTable) {
		this->destroyAllEntries();
		assert (this->nInUse == 0u);
		delete [] this->pTable;
	}
}

//
// uintResTable<ITEM>::~uintResTable()
// (not inline because it is virtual)
//
template <class ITEM>
uintResTable<ITEM>::~uintResTable() {}

//
// uintRes<ITEM>::~uintRes()
// (not inline because it is virtual)
//
template <class ITEM>
uintRes<ITEM>::~uintRes() {}

//
// this needs to be instanciated only once (normally in libCom)
//
#ifdef instantiateRecourceLib
//
// uintId::resourceHash()
//
resTableIndex uintId::resourceHash(unsigned /* nBitsId */) const
{
	resTableIndex hashid = this->id;

	//
	// This assumes worst case hash table index width of 1 bit.
	// We will iterate this loop 5 times on a 32 bit architecture.
	//
	// A good optimizer will unroll this loop?
	// Experiments using the microsoft compiler show that this isnt 
	// slower than switching on the architecture size and urolling the
	// loop explicitly (that solution has resulted in portability
	// problems in the past).
	//
	for (unsigned i=(CHAR_BIT*sizeof(unsigned))/2u; i>0u; i >>= 1u) {
		hashid ^= (hashid>>i);
	}

	//
	// the result here is always masked to the
	// proper size after it is returned to the resource class
	//
	return hashid;
}

//
// uintResTable<ITEM>::~uintResTable()
// (not inline because it is virtual)
//
uintId::~uintId() {}

#endif // instantiateRecourceLib 

//
// uintRes<ITEM>::installItem()
//
// NOTE: This detects (and avoids) the case where 
// the PV id wraps around and we attempt to have two 
// resources with the same id.
//
template <class ITEM>
inline void uintResTable<ITEM>::installItem(ITEM &item)
{
	int resTblStatus;
	do {
		item.uintRes<ITEM>::setId(allocId++);
		resTblStatus = this->add(item);
	}
	while (resTblStatus);
}

//
// stringId::~stringId()
//
//
// this needs to be instanciated only once (normally in libCom)
//
#ifdef instantiateRecourceLib
stringId::~stringId()
{
	if (this->allocType==copyString) {
		if (this->pStr!=NULL) {
			//
			// the microsoft and solaris compilers will 
			// not allow a pointer to "const char"
			// to be deleted 
			//
			// the HP-UX compiler gives us a warning on
			// each cast away of const, but in this case
			// it cant be avoided. 
			//
			// The DEC compiler complains that const isnt 
			// really significant in a cast if it is present.
			//
			// I hope that deleting a pointer to "char"
			// is the same as deleting a pointer to 
			// "const char" on all compilers
			//
			delete [] (char *) this->pStr;
		}
	}
}
#endif // instantiateRecourceLib 

//
// this needs to be instanciated only once (normally in libCom)
//
#ifdef instantiateRecourceLib
//
// stringId::resourceHash()
//
// The hash algorithm is a modification of the algorithm described in 
// Fast Hashing of Variable Length Text Strings, Peter K. Pearson, 
// Communications of the ACM, June 1990 
// The modifications were designed by Marty Kraimer
//
resTableIndex stringId::resourceHash(unsigned nBitsId) const
{
	if (this->pStr==NULL) {
		return 0u;
	}

	unsigned h0 = 0u;
	unsigned h1 = 0u;
	unsigned c;
	unsigned i;
	for (i=0u; (c = this->pStr[i]); i++) {
		//
		// odd 
		//
		if (i&1u) {
			h1 = stringIdFastHash[h1 ^ c];
		}
		//
		// even 
		//
		else {
			h0 = stringIdFastHash[h0 ^ c];
		}
	}

	//
	// does not work well for more than 65k entries ?
	// (because some indexes in the table will not be produced)
	//
	if (nBitsId>=8u) {
		h1 = h1 << (nBitsId-8u);
	}
	return h1 ^ h0;
}
#endif // instantiateRecourceLib 

//
// this needs to be instanciated only once (normally in libCom)
//
#ifdef instantiateRecourceLib
//
// The hash algorithm is a modification of the algorithm described in
// Fast Hashing of Variable Length Text Strings, Peter K. Pearson,
// Communications of the ACM, June 1990
// The modifications were designed by Marty Kraimer
//
const unsigned char stringId::stringIdFastHash[256] = {
 39,159,180,252, 71,  6, 13,164,232, 35,226,155, 98,120,154, 69,
157, 24,137, 29,147, 78,121, 85,112,  8,248,130, 55,117,190,160,
176,131,228, 64,211,106, 38, 27,140, 30, 88,210,227,104, 84, 77,
 75,107,169,138,195,184, 70, 90, 61,166,  7,244,165,108,219, 51,
  9,139,209, 40, 31,202, 58,179,116, 33,207,146, 76, 60,242,124,
254,197, 80,167,153,145,129,233,132, 48,246, 86,156,177, 36,187,
 45,  1, 96, 18, 19, 62,185,234, 99, 16,218, 95,128,224,123,253,
 42,109,  4,247, 72,  5,151,136,  0,152,148,127,204,133, 17, 14,
182,217, 54,199,119,174, 82, 57,215, 41,114,208,206,110,239, 23,
189, 15,  3, 22,188, 79,113,172, 28,  2,222, 21,251,225,237,105,
102, 32, 56,181,126, 83,230, 53,158, 52, 59,213,118,100, 67,142,
220,170,144,115,205, 26,125,168,249, 66,175, 97,255, 92,229, 91,
214,236,178,243, 46, 44,201,250,135,186,150,221,163,216,162, 43,
 11,101, 34, 37,194, 25, 50, 12, 87,198,173,240,193,171,143,231,
111,141,191,103, 74,245,223, 20,161,235,122, 63, 89,149, 73,238,
134, 68, 93,183,241, 81,196, 49,192, 65,212, 94,203, 10,200, 47
};

#endif // instantiateRecourceLib 

#endif // INCresourceLibh

