/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
#ifndef GDDI_H
#define GDDI_H

/*
 * Author: Jim Kowalkowski
 * Date: 3/97
 *
 * $Id$
 */

inline void gdd::setData(void* d)					{ data.Pointer=d; }
inline gddDestructor* gdd::destructor(void) const	{ return destruct; }

inline gdd::gdd(void)					{ init(0,aitEnumInvalid,0); }
inline gdd::gdd(int app)				{ init(app,aitEnumInvalid,0); }
inline gdd::gdd(int app,aitEnum prim)	{ init(app,prim,0); }

inline unsigned gdd::applicationType(void) const{ return appl_type; }
inline aitEnum gdd::primitiveType(void) const	{ return (aitEnum)prim_type; }
inline const gddBounds* gdd::getBounds(void) const	{ return bounds; }
inline const gddBounds* gdd::getBounds(int bn) const { return &bounds[bn]; }

inline gdd* gdd::next(void) { return nextgdd; }
inline void gdd::setNext(gdd* n) { nextgdd=n; }
inline unsigned gdd::dimension(void) const	{ return dim; }
inline aitType& gdd::getData(void) 			{ return data; }
inline aitType* gdd::dataUnion(void)		{ return &data; }
inline void gdd::setApplType(int t)			{ appl_type=(aitUint16)t; }
inline gddStatus gdd::copyInfo(gdd* dd)		{ return copyStuff(dd,0); }
inline gddStatus gdd::copy(gdd* dd)			{ return copyStuff(dd,1); }
inline gddStatus gdd::Dup(gdd* dd)			{ return copyStuff(dd,2); }
inline void* gdd::dataAddress(void)	const	{ return (void*)&data; }
inline void* gdd::dataPointer(void)	const	{ return data.Pointer; }

inline void* gdd::dataVoid(void) const
{
	return (dimension()||primitiveType()==aitEnumFixedString)?
		dataPointer():dataAddress();
}

inline aitUint32 gdd::align8(unsigned long count) const
{
	unsigned long tmp=count&(~((unsigned long)0x07));
	return (tmp!=count)?tmp+8:tmp;
}

inline void* gdd::dataPointer(aitIndex f) const
	{ return (void*)(((aitUint8*)dataPointer())+aitSize[primitiveType()]*f); }

inline int gdd::isManaged(void) const	{ return flags&GDD_MANAGED_MASK; }
inline int gdd::isFlat(void) const		{ return flags&GDD_FLAT_MASK; }
inline int gdd::isNoRef(void) const		{ return flags&GDD_NOREF_MASK; }
inline int gdd::isConstant(void) const	{ return flags&GDD_CONSTANT_MASK; }
inline int gdd::isLocalDataFormat(void) const { return !(flags&GDD_NET_MASK); }
inline int gdd::isNetworkDataFormat(void) const
	{ return !isLocalDataFormat() || aitLocalNetworkDataFormatSame; }

inline void gdd::markConstant(void)		{ flags|=GDD_CONSTANT_MASK; }
inline void gdd::markFlat(void)			{ flags|=GDD_FLAT_MASK; }
inline void gdd::markManaged(void)		{ flags|=GDD_MANAGED_MASK; }
inline void gdd::markUnmanaged(void)	{ flags&=~GDD_MANAGED_MASK; }
inline void gdd::markLocalDataFormat(void)		{ flags&=~GDD_NET_MASK; }
inline void gdd::markNotLocalDataFormat(void)	{ flags|=GDD_NET_MASK; }

inline void gdd::getTimeStamp(struct timespec* const ts) const
	{ ts->tv_sec=time_stamp.tv_sec; ts->tv_nsec=time_stamp.tv_nsec; }
inline void gdd::setTimeStamp(const struct timespec* const ts) {
	time_stamp.tv_sec=(aitUint32)ts->tv_sec;
	time_stamp.tv_nsec=(aitUint32)ts->tv_nsec; }
inline void gdd::getTimeStamp(aitTimeStamp* const ts) const { *ts=time_stamp; }
inline void gdd::setTimeStamp(const aitTimeStamp* const ts) { time_stamp=*ts; }

inline void gdd::setStatus(aitUint32 s)		{ status=s; }
inline void gdd::getStatus(aitUint32& s)	{ s=status; }
inline void gdd::setStatus(aitUint16 high, aitUint16 low)
	{ status=(((aitUint32)high)<<16)|low; }
inline void gdd::getStatus(aitUint16& high, aitUint16& low)
	{ high=(aitUint16)(status>>16); low=(aitUint16)(status&0x0000ffff); }

inline void gdd::setStat(aitUint16 s)
	{ aitUint16* x = (aitUint16*)&status; x[0]=s; }
inline void gdd::setSevr(aitUint16 s)
	{ aitUint16* x = (aitUint16*)&status; x[1]=s; }
inline aitUint16 gdd::getStat(void) const
	{ aitUint16* x = (aitUint16*)&status; return x[0]; }
inline aitUint16 gdd::getSevr(void) const
	{ aitUint16* x = (aitUint16*)&status; return x[1]; }
inline void gdd::getStatSevr(aitInt16& st, aitInt16& se)
	{ st=getStat(); se=getSevr(); }
inline void gdd::setStatSevr(aitInt16 st, aitInt16 se)
	{ setStat(st); setSevr(se); }

inline gdd& gdd::operator=(const gdd& v)
	{ memcpy(this,&v,sizeof(gdd)); return *this; }

inline int gdd::isScalar(void) const { return dimension()==0?1:0; }
inline int gdd::isContainer(void) const
	{ return (primitiveType()==aitEnumContainer)?1:0; }
inline int gdd::isAtomic(void) const
	{ return (dimension()>0&&primitiveType()!=aitEnumContainer)?1:0; }
inline gddStatus gdd::noReferencing(void)
{
	int rc=0;
	if(ref_cnt>1)
	{
		gddAutoPrint("gdd::noReferencing()",gddErrorNotAllowed);
		rc=gddErrorNotAllowed;
	}
	else			flags|=GDD_NOREF_MASK;
	return rc;
}
inline gddStatus gdd::reference(void)
{
	int rc=0;

	if(isNoRef())
	{
		gddAutoPrint("gdd::reference()",gddErrorNotAllowed);
		rc=gddErrorNotAllowed;
	}
	else			ref_cnt++;

	if(ref_cnt>((1u<<(sizeof(ref_cnt)*CHAR_BIT))-2u))
	{
		fprintf(stderr,"gdd reference count overflow!!\n");
		gddAutoPrint("gdd::reference()",gddErrorOverflow);
		rc=gddErrorOverflow;
	}
	return rc;
}

inline gddStatus gdd::unreference(void)
{
	int rc=0;

	if(ref_cnt==0u)
	{
		fprintf(stderr,"gdd reference count underflow!!\n");
		gddAutoPrint("gdd::unreference()",gddErrorUnderflow);
		rc=gddErrorUnderflow;
	}
	else if(--ref_cnt<=0u)
	{
		if(isManaged())
		{
			// managed dd always destroys the entire thing
			ref_cnt=1;
			if(destruct) destruct->destroy(this);
			destruct=NULL;
		}
		else if(!isFlat())
			delete this;
	}
	return rc;
}

inline void gdd::destroyData(void)
{
	if(destruct)
	{
		if(isContainer())
			destruct->destroy(this);
		else
			destruct->destroy(dataPointer());

		freeBounds(); // must be done here
		destruct=NULL;
		setData(NULL);
	}
	else if (isScalar())
	{
		//
		// this destroys the string types
		//
		this->setPrimType (aitEnumInvalid);
	}
}

inline void gdd::adjust(gddDestructor* d, void* v, aitEnum type,aitDataFormat)
{
	if(destruct) destruct->destroy(dataPointer());
	destruct=d;
	if(destruct) destruct->reference();
	setPrimType(type);
	setData(v);
}

// These function do NOT work well for aitFixedString because
// fixed strings are always stored by reference.  You WILL get
// into trouble is the gdd does not contain a fixed string
// before you putConvert() something into it.

#if aitLocalNetworkDataFormatSame == AIT_FALSE
inline void gdd::get(aitEnum t,void* v,aitDataFormat f)
#else
inline void gdd::get(aitEnum t,void* v,aitDataFormat)
#endif
{
	if(primitiveType()==aitEnumFixedString)
	{
		if(dataPointer()) aitConvert(t,v,primitiveType(),dataPointer(),1);
	}
	else
	{
#if aitLocalNetworkDataFormatSame == AIT_FALSE
		if(f!=aitLocalDataFormat)
			aitConvertToNet(t,v,primitiveType(),dataAddress(),1);
		else
#endif
			aitConvert(t,v,primitiveType(),dataAddress(),1);
	}
}

#if aitLocalNetworkDataFormatSame == AIT_FALSE
inline void gdd::set(aitEnum t,const void* v,aitDataFormat f)
#else
inline void gdd::set(aitEnum t,const void* v,aitDataFormat)
#endif
{
	if (primitiveType()==aitEnumInvalid) {
		this->setPrimType (t);
	}

#if aitLocalNetworkDataFormatSame == AIT_FALSE
	if(f!=aitLocalDataFormat)
		aitConvertFromNet(primitiveType(),dataVoid(),t,v,1);
	else
#endif
		aitConvert(primitiveType(),dataVoid(),t,v,1);

	markLocalDataFormat();
}

// -------------------getRef(data pointer) functions----------------
inline void gdd::getRef(aitFloat64*& d) { d=(aitFloat64*)dataVoid(); }
inline void gdd::getRef(aitFloat32*& d) { d=(aitFloat32*)dataVoid(); }
inline void gdd::getRef(aitUint32*& d)	{ d=(aitUint32*)dataVoid(); }
inline void gdd::getRef(aitInt32*& d)	{ d=(aitInt32*)dataVoid(); }
inline void gdd::getRef(aitUint16*& d)	{ d=(aitUint16*)dataVoid(); }
inline void gdd::getRef(aitInt16*& d)	{ d=(aitInt16*)dataVoid(); }
inline void gdd::getRef(aitUint8*& d)	{ d=(aitUint8*)dataVoid(); }
inline void gdd::getRef(aitInt8*& d)	{ d=(aitInt8*)dataVoid(); }
inline void gdd::getRef(void*& d)		{ d=dataVoid(); }
inline void gdd::getRef(aitFixedString*& d)	{ d=(aitFixedString*)dataVoid(); }
inline void gdd::getRef(aitString*& d)		{ d=(aitString*)dataVoid(); }

// -------------------putRef(data pointer) functions----------------
inline void gdd::putRef(void* v,aitEnum code, gddDestructor* d)
	{ adjust(d, v, code); }
inline void gdd::putRef(aitFloat64* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumFloat64); }
inline void gdd::putRef(aitFloat32* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumFloat32); }
inline void gdd::putRef(aitUint8* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumUint8); }
inline void gdd::putRef(aitInt8* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumInt8); }
inline void gdd::putRef(aitUint16* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumUint16); }
inline void gdd::putRef(aitInt16* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumInt16); }
inline void gdd::putRef(aitUint32* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumUint32); }
inline void gdd::putRef(aitInt32* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumInt32); }
inline void gdd::putRef(aitString* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumString); }
inline void gdd::putRef(aitFixedString* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumFixedString); }

// -------------------putRef(const data pointer) functions----------------
inline void gdd::putRef(const aitFloat64* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumFloat64); markConstant(); }
inline void gdd::putRef(const aitFloat32* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumFloat32); markConstant(); }
inline void gdd::putRef(const aitUint8* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumUint8); markConstant(); }
inline void gdd::putRef(const aitInt8* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumInt8); markConstant(); }
inline void gdd::putRef(const aitUint16* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumUint16); markConstant(); }
inline void gdd::putRef(const aitInt16* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumInt16); markConstant(); }
inline void gdd::putRef(const aitUint32* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumUint32); markConstant(); }
inline void gdd::putRef(const aitInt32* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumInt32); markConstant(); }
inline void gdd::putRef(const aitString* v, gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumString); markConstant(); }
inline void gdd::putRef(const aitFixedString* v,gddDestructor* d)
	{ adjust(d, (void*)v, aitEnumFixedString); markConstant(); }

// -------------------getConvert(scalar) functions ----------------------
inline void gdd::getConvert(aitFloat64& d)	{ get(aitEnumFloat64,&d); }
inline void gdd::getConvert(aitFloat32& d)	{ get(aitEnumFloat32,&d); }
inline void gdd::getConvert(aitUint32& d)	{ get(aitEnumUint32,&d); }
inline void gdd::getConvert(aitInt32& d)	{ get(aitEnumInt32,&d); }
inline void gdd::getConvert(aitUint16& d)	{ get(aitEnumUint16,&d); }
inline void gdd::getConvert(aitInt16& d)	{ get(aitEnumInt16,&d); }
inline void gdd::getConvert(aitUint8& d)	{ get(aitEnumUint8,&d); }
inline void gdd::getConvert(aitInt8& d)		{ get(aitEnumInt8,&d); }

// -------------------putConvert(scalar) functions ----------------------
inline void gdd::putConvert(aitFloat64 d){ set(aitEnumFloat64,&d); }
inline void gdd::putConvert(aitFloat32 d){ set(aitEnumFloat32,&d); }
inline void gdd::putConvert(aitUint32 d) { set(aitEnumUint32,&d); }
inline void gdd::putConvert(aitInt32 d)  { set(aitEnumInt32,&d); }
inline void gdd::putConvert(aitUint16 d) { set(aitEnumUint16,&d); }
inline void gdd::putConvert(aitInt16 d)  { set(aitEnumInt16,&d); }
inline void gdd::putConvert(aitUint8 d)  { set(aitEnumUint8,&d); }
inline void gdd::putConvert(aitInt8 d)	 { set(aitEnumInt8,&d); }

// ------------------------put(pointer) functions----------------------
inline gddStatus gdd::put(const aitFloat64* const d)
	{ return genCopy(aitEnumFloat64,d); }
inline gddStatus gdd::put(const aitFloat32* const d)
	{ return genCopy(aitEnumFloat32,d); }
inline gddStatus gdd::put(const aitUint32* const d)
	{ return genCopy(aitEnumUint32,d); }
inline gddStatus gdd::put(const aitInt32* const d)
	{ return genCopy(aitEnumInt32,d); }
inline gddStatus gdd::put(const aitUint16* const d)
	{ return genCopy(aitEnumUint16,d); }
inline gddStatus gdd::put(const aitInt16* const d)
	{ return genCopy(aitEnumInt16,d); }
inline gddStatus gdd::put(const aitUint8* const d)
	{ return genCopy(aitEnumUint8,d); }

// special case for aitInt8 array to aitString scalar
inline gddStatus gdd::put(const aitInt8* const d)
{
	gddStatus rc=0;

	if(primitiveType()==aitEnumString && dim==0)
	{
		aitString* p = (aitString*)dataAddress();
		p->copy((char*)d);
	}
	else if(primitiveType()==aitEnumFixedString && dim==0) {
		strncpy(data.FString->fixed_string,(char*)d, 
			sizeof(aitFixedString));
		data.FString->fixed_string[sizeof(aitFixedString)-1u]='\0';
	}
	else
		rc=genCopy(aitEnumInt8,d);

	return rc;
}

// ----------------put(scalar) functions----------------
inline gddStatus gdd::put(aitFloat64 d) {
	gddStatus rc=0;
	if(isScalar()) { setPrimType(aitEnumFloat64); data.Float64=d; }
	else { rc=gddErrorNotAllowed; gddAutoPrint("gdd:put()",rc); }
	return rc;
}
inline gddStatus gdd::put(aitFloat32 d) {
	gddStatus rc=0;
	if(isScalar()) { setPrimType(aitEnumFloat32); data.Float32=d;}
	else { rc=gddErrorNotAllowed; gddAutoPrint("gdd:put()",rc); }
	return rc;
}
inline gddStatus gdd::put(aitUint32 d) {
	gddStatus rc=0;
	if(isScalar()) { setPrimType(aitEnumUint32); data.Uint32=d; }
	else { rc=gddErrorNotAllowed; gddAutoPrint("gdd:put()",rc); }
	return rc;
}
inline gddStatus gdd::put(aitInt32 d) {
	gddStatus rc=0;
	if(isScalar()) { setPrimType(aitEnumInt32); data.Int32=d; }
	else { rc=gddErrorNotAllowed; gddAutoPrint("gdd:put()",rc); }
	return rc;
}
inline gddStatus gdd::put(aitUint16 d) {
	gddStatus rc=0;
	if(isScalar()) { setPrimType(aitEnumUint16); data.Uint16=d; }
	else { rc=gddErrorNotAllowed; gddAutoPrint("gdd:put()",rc); }
	return rc;
}
inline gddStatus gdd::put(aitInt16 d) {
	gddStatus rc=0;
	if(isScalar()) { setPrimType(aitEnumInt16); data.Int16=d; }
	else { rc=gddErrorNotAllowed; gddAutoPrint("gdd:put()",rc); }
	return rc;
}
inline gddStatus gdd::put(aitUint8 d) {
	gddStatus rc=0;
	if(isScalar()) { setPrimType(aitEnumUint8); data.Uint8=d; }
	else { rc=gddErrorNotAllowed; gddAutoPrint("gdd:put()",rc); }
	return rc;
}
inline gddStatus gdd::put(aitInt8 d) {
	gddStatus rc=0;
	if(isScalar()) { setPrimType(aitEnumInt8); data.Int8=d; }
	else { rc=gddErrorNotAllowed; gddAutoPrint("gdd:put()",rc); }
	return rc;
}
inline gddStatus gdd::put(aitType* d) {
	gddStatus rc=0;
	if(isScalar()) { data=*d; }
	else { rc=gddErrorNotAllowed; gddAutoPrint("gdd:put()",rc); }
	return rc;
}

// ---------------------get(pointer) functions--------------------------
inline void gdd::get(void* d) {
	if(isScalar())
		aitConvert(primitiveType(),d,primitiveType(),dataAddress(),1);
	else
		aitConvert(primitiveType(),d,primitiveType(),dataPointer(),
			getDataSizeElements());
}
inline void gdd::get(void* d,aitEnum e) {
	if(isScalar())
		aitConvert(e,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(e,d,primitiveType(),dataPointer(),getDataSizeElements());
}
inline void gdd::get(aitFloat64* d)
{
	if(isScalar())
		aitConvert(aitEnumFloat64,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(aitEnumFloat64,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}
inline void gdd::get(aitFloat32* d) {
	if(isScalar())
		aitConvert(aitEnumFloat32,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(aitEnumFloat32,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}
inline void gdd::get(aitUint32* d) {
	if(isScalar())
		aitConvert(aitEnumUint32,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(aitEnumUint32,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}
inline void gdd::get(aitInt32* d) {
	if(isScalar())
		aitConvert(aitEnumInt32,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(aitEnumInt32,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}
inline void gdd::get(aitUint16* d) {
	if(isScalar())
		aitConvert(aitEnumUint16,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(aitEnumUint16,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}
inline void gdd::get(aitInt16* d) {
	if(isScalar())
		aitConvert(aitEnumInt16,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(aitEnumInt16,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}
inline void gdd::get(aitUint8* d) {
	if(isScalar())
		aitConvert(aitEnumUint8,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(aitEnumUint8,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}
inline void gdd::get(aitString* d) {
	if(isScalar())
		aitConvert(aitEnumString,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(aitEnumString,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}
inline void gdd::get(aitFixedString* d) {
	if(isScalar())
		aitConvert(aitEnumFixedString,d,primitiveType(),dataAddress(),1);
	else
		aitConvert(aitEnumFixedString,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}

// special case for string scalar to aitInt8 array!
inline void gdd::get(aitInt8* d)
{
	if(primitiveType()==aitEnumString && dim==0)
	{
		aitString* str = (aitString*)dataAddress();
		strcpy((char*)d,str->string());
	}
	else if(primitiveType()==aitEnumFixedString && dim==0)
		strcpy((char*)d,data.FString->fixed_string);
	else
		aitConvert(aitEnumInt8,d,primitiveType(),dataPointer(),
			getDataSizeElements());
}

// ------------------get(scalar) functions-----------------
inline void gdd::get(aitFloat64& d) {
	if(primitiveType()==aitEnumFloat64) d=getData().Float64;
	else get(aitEnumFloat64,&d);
}
inline void gdd::get(aitFloat32& d)	{
	if(primitiveType()==aitEnumFloat32) d=getData().Float32;
	else get(aitEnumFloat32,&d);
}
inline void gdd::get(aitUint32& d) {
	if(primitiveType()==aitEnumUint32) d=getData().Uint32;
	else get(aitEnumUint32,&d);
}
inline void gdd::get(aitInt32& d) {
	if(primitiveType()==aitEnumInt32) d=getData().Int32;
	else get(aitEnumInt32,&d);
}
inline void gdd::get(aitUint16& d) {
	if(primitiveType()==aitEnumUint16) d=getData().Uint16;
	else get(aitEnumUint16,&d);
}
inline void gdd::get(aitInt16& d) {
	if(primitiveType()==aitEnumInt16) d=getData().Int16;
	else get(aitEnumInt16,&d);
}
inline void gdd::get(aitUint8& d) {
	if(primitiveType()==aitEnumUint8) d=getData().Uint8;
	else get(aitEnumUint8,&d);
}
inline void gdd::get(aitInt8& d) {
	if(primitiveType()==aitEnumInt8) d=getData().Int8;
	else get(aitEnumInt8,&d);
}
inline void gdd::get(aitType& d)	{ d=data; }

// ---------- gdd x = primitive data type pointer functions----------
inline gdd& gdd::operator=(aitFloat64* v)	{ putRef(v); return *this;}
inline gdd& gdd::operator=(aitFloat32* v)	{ putRef(v); return *this;}
inline gdd& gdd::operator=(aitUint32* v)	{ putRef(v); return *this;}
inline gdd& gdd::operator=(aitInt32* v)		{ putRef(v); return *this;}
inline gdd& gdd::operator=(aitUint16* v)	{ putRef(v); return *this;}
inline gdd& gdd::operator=(aitInt16* v)		{ putRef(v); return *this;}
inline gdd& gdd::operator=(aitUint8* v)		{ putRef(v); return *this;}
inline gdd& gdd::operator=(aitInt8* v)		{ putRef(v); return *this;}
inline gdd& gdd::operator=(aitString* v)	{ putRef(v); return *this;}
inline gdd& gdd::operator=(aitFixedString* v){ putRef(v); return *this;}

// ----------------- gdd x = primitive data type functions----------------
inline gdd& gdd::operator=(aitFloat64 d)
	{ setPrimType(aitEnumFloat64); data.Float64=d; return *this; }
inline gdd& gdd::operator=(aitFloat32 d)
	{ setPrimType(aitEnumFloat32); data.Float32=d;return *this; }
inline gdd& gdd::operator=(aitUint32 d)
	{ setPrimType(aitEnumUint32); data.Uint32=d; return *this; }
inline gdd& gdd::operator=(aitInt32 d)	
	{ setPrimType(aitEnumInt32); data.Int32=d; return *this; }
inline gdd& gdd::operator=(aitUint16 d)
	{ setPrimType(aitEnumUint16); data.Uint16=d; return *this; }
inline gdd& gdd::operator=(aitInt16 d)
	{ setPrimType(aitEnumInt16); data.Int16=d; return *this; }
inline gdd& gdd::operator=(aitUint8 d)
	{ setPrimType(aitEnumUint8); data.Uint8=d; return *this; }
inline gdd& gdd::operator=(aitInt8 d)
	{ setPrimType(aitEnumInt8); data.Int8=d; return *this; }
inline gdd& gdd::operator=(const aitString& d)
	{ put(d); return *this; }

// ------------- primitive type pointer = gdd x functions --------------
inline gdd::operator aitFloat64*(void) const
	{ return (aitFloat64*)dataPointer(); }
inline gdd::operator aitFloat32*(void) const
	{ return (aitFloat32*)dataPointer(); }
inline gdd::operator aitUint32*(void) const
	{ return (aitUint32*)dataPointer(); }
inline gdd::operator aitInt32*(void) const
	{ return (aitInt32*)dataPointer(); }
inline gdd::operator aitUint16*(void) const
	{ return (aitUint16*)dataPointer(); }
inline gdd::operator aitInt16*(void) const
	{ return (aitInt16*)dataPointer(); }
inline gdd::operator aitUint8*(void) const
	{ return (aitUint8*)dataPointer(); }
inline gdd::operator aitInt8*(void)	 const
	{ return (aitInt8*)dataPointer(); }
inline gdd::operator aitString*(void) const
	{ return (aitString*)dataPointer(); }
inline gdd::operator aitFixedString*(void) const
	{ return (aitFixedString*)dataPointer(); }

// ------------- primitive type = gdd x functions --------------
inline gdd::operator aitFloat64(void)	{ aitFloat64 d; get(d); return d; }
inline gdd::operator aitFloat32(void)	{ aitFloat32 d; get(d); return d; }
inline gdd::operator aitUint32(void)	{ aitUint32 d; get(d); return d; }
inline gdd::operator aitInt32(void)		{ aitInt32 d; get(d); return d; }
inline gdd::operator aitUint16(void)	{ aitUint16 d; get(d); return d; }
inline gdd::operator aitInt16(void)		{ aitInt16 d; get(d); return d; }
inline gdd::operator aitUint8(void)		{ aitUint8 d; get(d); return d; }
inline gdd::operator aitInt8(void)		{ aitInt8 d; get(d); return d; }
inline gdd::operator aitString(void)	{ aitString d; get(d); return d; }

#endif
