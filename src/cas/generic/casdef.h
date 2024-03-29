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
 *	$Id$
 *
 *      Author: Jeffrey O. Hill
 *              johill@lanl.gov
 *              (505) 665 1831
 *      Date:   1-95
 *
 * TODO:
 * .03  document new event types for limits change etc
 * .04  certain things like native type cant be changed during
 *	pv id's life time or we will be required to have locking
 *	(doc this)
 * .08	Need a mechanism by which an error detail string can be returned
 *	to the server from a server app (in addition to the normal
 *	error constant)
 * .12 	Should the server have an interface so that all PV names
 *	can be obtained (even ones created after init)? This
 *	would be used to implement update of directory services and 
 * 	wild cards? Problems here with knowing PV name structure and 
 *	maximum permutations of name components.
 * .16 	go through this file and make sure that we are consistent about
 *	the use of const - use a pointer only in spots where NULL is
 *	allowed?
 * NOTES:
 * .01  When this code is used in a multi threaded situation we
 * 	must be certain that the derived class's virtual function 
 * 	are not called between derived class destruction and base
 * 	class destruction (or prevent problems if they are).
 *	Possible solutions
 *	1) in the derived classes destructor set a variable which
 * 	inhibits use of the derived classes virtual function.
 *	Each virtual function must check this inhibit bit and return 
 *	an error if it is set
 *	2) call a method on the base which prevents further use
 * 	of it by the server from the derived class destructor.
 *	3) some form of locking (similar to (1) above)
 */

#ifndef includecasdefh
#define includecasdefh

//
// EPICS
//
#include "alarm.h"	// EPICS alarm severity/condition 
#include "errMdef.h"	// EPICS error codes 
#include "gdd.h" 		// EPICS data descriptors 
#include "shareLib.h"	// EPICS compiler specific sharable lib keywords

//
// This eliminates a warning resulting from passing *this
// to a base class during derived class construction.
//
#if defined(_MSC_VER)
#	pragma warning (disable:4355)
#endif

typedef aitUint32 caStatus;


/*
 * ===========================================================
 * for internal use by the server library 
 * (and potentially returned to the server tool)
 * ===========================================================
 */
#define S_cas_success 0
#define S_cas_internal (M_cas| 1) /*Internal failure*/
#define S_cas_noMemory (M_cas| 2) /*Memory allocation failed*/
#define S_cas_bindFail (M_cas| 3) /*Attempt to set server's IP address/port failed*/
#define S_cas_hugeRequest (M_cas | 4) /*Requested op does not fit*/
#define S_cas_sendBlocked (M_cas | 5) /*Blocked for send q space*/
#define S_cas_badElementCount (M_cas | 6) /*Bad element count*/
#define S_cas_noConvert (M_cas | 7) /*No conversion between src & dest types*/
#define S_cas_badWriteType (M_cas | 8) /*Src type inappropriate for write*/
#define S_cas_noContext (M_cas | 11) /*Context parameter is required*/
#define S_cas_disconnect (M_cas | 12) /*Lost connection to server*/
#define S_cas_recvBlocked (M_cas | 13) /*Recv blocked*/
#define S_cas_badType (M_cas | 14) /*Bad data type*/
#define S_cas_timerDoesNotExist (M_cas | 15) /*Timer does not exist*/
#define S_cas_badEventType (M_cas | 16) /*Bad event type*/
#define S_cas_badResourceId (M_cas | 17) /*Bad resource identifier*/
#define S_cas_chanCreateFailed (M_cas | 18) /*Unable to create channel*/
#define S_cas_noRead (M_cas | 19) /*read access denied*/
#define S_cas_noWrite (M_cas | 20) /*write access denied*/
#define S_cas_noEventsSelected (M_cas | 21) /*no events selected*/
#define S_cas_noFD (M_cas | 22) /*no file descriptors available*/
#define S_cas_badProtocol (M_cas | 23) /*protocol from client was invalid*/
#define S_cas_redundantPost (M_cas | 24) /*redundundant io completion post*/
#define S_cas_badPVName (M_cas | 25) /*bad PV name from server tool*/
#define S_cas_badParameter (M_cas | 26) /*bad parameter from server tool*/
#define S_cas_validRequest (M_cas | 27) /*valid request*/
#define S_cas_tooManyEvents (M_cas | 28) /*maximum simult event types exceeded*/
#define S_cas_noInterface (M_cas | 29) /*server isnt attached to a network*/
#define S_cas_badBounds (M_cas | 30) /*server tool changed bounds on request*/
#define S_cas_pvAlreadyAttached (M_cas | 31) /*PV attached to another server*/
#define S_cas_badRequest (M_cas | 32) /*client's request was invalid*/
/*
 * ===========================================================
 * returned by the application (to the server library)
 * ===========================================================
 */
#define S_casApp_success 0 
#define S_casApp_noMemory (M_casApp | 1) /*Memory allocation failed*/
#define S_casApp_pvNotFound (M_casApp | 2) /*PV not found*/
#define S_casApp_badPVId (M_casApp | 3) /*Unknown PV identifier*/
#define S_casApp_noSupport (M_casApp | 4) /*No application support for op*/
#define S_casApp_asyncCompletion (M_casApp | 5) /*will complete asynchronously*/
#define S_casApp_badDimension (M_casApp | 6) /*bad matrix size in request*/
#define S_casApp_canceledAsyncIO (M_casApp | 7) /*asynchronous io canceled*/
#define S_casApp_outOfBounds (M_casApp | 8) /*operation was out of bounds*/
#define S_casApp_undefined (M_casApp | 9) /*undefined value*/
#define S_casApp_postponeAsyncIO (M_casApp | 10) /*postpone asynchronous IO*/

#include "caNetAddr.h"

//
// pv exist test return
//
// If the server tool does not wish to start another simultaneous
// asynchronous IO operation or if there is not enough memory
// to do so return pverDoesNotExistHere (and the client will
// retry the request later).
//
enum pvExistReturnEnum {pverExistsHere, pverDoesNotExistHere, 
	pverAsyncCompletion};

class epicsShareClass pvExistReturn {
public:
	//
	// most server tools will use this
	//
	pvExistReturn (pvExistReturnEnum s=pverDoesNotExistHere) :
		status(s) {}
	//
	// directory service server tools 
	// will use this
	//
	// (see caNetAddr.h)
	//
	pvExistReturn (const caNetAddr &addressIn) :
		status(pverExistsHere), address(addressIn) {}

	const pvExistReturn &operator = (pvExistReturnEnum rhs)
	{
		this->status = rhs;
		this->address.clear();
		return *this;
	}
	const pvExistReturn &operator = (const caNetAddr &rhs)
	{
		this->status = pverExistsHere;
		this->address = rhs;
		return *this;
	}
	pvExistReturnEnum getStatus() const {return this->status;}
	int addrIsValid() const {return this->address.isSock();}
	caNetAddr getAddr() const {return this->address;}
private:
	pvExistReturnEnum	status;
	caNetAddr		address;
};

class casPV;

class epicsShareClass pvCreateReturn {
public:
	pvCreateReturn(caStatus statIn)
		{ this->pPV = NULL; this->stat = statIn; }
	pvCreateReturn(casPV &pv) 
		{ this->pPV = &pv; this->stat = S_casApp_success; }

	const pvCreateReturn &operator = (caStatus rhs)
	{
		this->pPV = NULL;
		if (rhs == S_casApp_success) {
			this->stat = S_cas_badParameter;
		}
		else {
			this->stat = rhs;
		}
		return *this;
	}
	
	// 
	// const pvCreateReturn &operator = (casPV &pvIn)
	//
	// The above assignement operator is not included 
	// because it does not match the strict definition of an 
	// assignement operator unless "const casPV &"
	// is passed in, and we cant use a const PV
	// pointer here because the server library _will_ make
	// controlled modification of the PV in the future.
	//
	const pvCreateReturn &operator = (casPV *pPVIn)
	{
		if (pPVIn!=NULL) {
			this->stat = S_casApp_success;
		}
		else {
			this->stat = S_casApp_pvNotFound;
		}
		this->pPV = pPVIn;
		return *this;
	}	
	const caStatus getStatus() const { return this->stat; }
	casPV *getPV() const { return this->pPV; }
	
private:
	casPV *pPV;
	caStatus stat;
};

#include "casEventMask.h"	// EPICS event select class 
#include "casInternal.h"	// CA server private 

class caServerI;

//
// caServer - Channel Access Server API Class
//
class caServer {
private:
	//
	// this private data member appears first so that
	// initialization of the constant event masks below
	// uses this member only after it has been initialized
	//
	// We do not use private inheritance here in order
	// to avoid os/io dependent -I during server tool compile
	//
	caServerI *pCAS;
public:
	epicsShareFunc caServer (unsigned pvCountEstimate=1024u);
	epicsShareFunc virtual ~caServer() = 0;

	//caStatus enableClients ();
	//caStatus disableClients ();

	epicsShareFunc void setDebugLevel (unsigned level);
	epicsShareFunc unsigned getDebugLevel ();

	epicsShareFunc casEventMask registerEvent (const char *pName);

	//
	// show()
	//
	epicsShareFunc virtual void show (unsigned level) const;

	//
	// pvExistTest()
	//
	// The request is allowed to complete asynchronously
	// (see Asynchronous IO Classes below).
	//
	// The server tool is encouraged to accept multiple PV name
	// aliases for the same PV here. 
	//
	// example return from this procedure:
	// return pverExistsHere;	// server has PV
	// return pverDoesNotExistHere;	// server does know of this PV
	// return pverAsynchCompletion;	// deferred result 
	//
	// Return pverDoesNotExistHere if too many simultaneous
	// asynchronous IO operations are pending against the server. 
	// The client library will retry the request at some time
	// in the future.
	//
	epicsShareFunc virtual pvExistReturn pvExistTest (const casCtx &ctx, 
		const char *pPVAliasName);

	//
	// createPV() is called _every_ time that a PV is attached to
	// by a client. The name supplied here may be a PV canonical
	// (base) name or it may instead be a PV alias name.
	//	
	// The request is allowed to complete asynchronously
	// (see Asynchronous IO Classes below).
	//
	// IMPORTANT: 
	// It is a responsability of the server tool 
	// to detect attempts by the server lib to create a 2nd PV with 
	// the same name as an existing PV. It is also the responsability 
	// of the server tool to detect attempts by the server lib to 
	// create a 2nd PV with a name that is an alias of an existing PV. 
	// In these situations the server tool should avoid PV duplication 
	// by returning a pointer to an existing PV (and not create a new 
	// PV).
	//
	// example return from this procedure:
	// return pPV;	// success (pass by pointer)
	// return PV;	// success (pass by ref)
	// return S_casApp_pvNotFound; // no PV by that name here
	// return S_casApp_noMemory; // no resource to create pv
	// return S_casApp_asyncCompletion; // deferred completion
	//
	// Return S_casApp_postponeAsyncIO if too many simultaneous
	// asynchronous IO operations are pending against the server. 
	// The server library will retry the request whenever an
	// asynchronous IO operation (create or exist) completes
	// against the server.
	//
	epicsShareFunc virtual pvCreateReturn createPV (const casCtx &ctx,
		const char *pPVAliasName);

	//
	// common event masks 
	// (what is currently used by the CA clients)
	//
	const casEventMask	valueEventMask; // DBE_VALUE
	const casEventMask	logEventMask; 	// DBE_LOG
	const casEventMask	alarmEventMask; // DBE_ALARM

	//
	// this is only used by casPVI::casPVI() too convert from
	// caServer to a caServerI 
	//
	friend class casPVI;
};

//
// casPV() - Channel Access Server Process Variable API Class
//
// Deletion Responsibility
// -------- --------------
// o the server lib will not call "delete" directly for any
// casPV because we dont know that "new" was called to create 
// the object
// o The server tool is responsible for reclaiming storage for any
// casPV it creates. The destroy() virtual function will
// assist the server tool with this responsibility. The
// virtual function casPV::destroy() does a "delete this".
// o The virtual function "destroy()" is called by the server lib 
// each time that the last client attachment to the PV object is removed. 
// o The destructor for this object will cancel any
// client attachment to this PV (and reclaim any resources
// allocated by the server library on its behalf)
//
// HINT: if the server tool precreates the PV during initialization
// then it may decide to provide a "destroy()" implementation in the
// derived class which is a noop.
//
class casPV : private casPVI {
public:
	epicsShareFunc casPV ();

	epicsShareFunc virtual ~casPV () = 0;

	//
	// This is called for each PV in the server if
	// caServer::show() is called and the level is high 
	// enough
	//
	epicsShareFunc virtual void show (unsigned level) const;

	//
	// Called by the server libary each time that it wishes to
	// subscribe for PV change notification from the server 
	// tool via postEvent() below.
	//
	epicsShareFunc virtual caStatus interestRegister ();

	//
	// called by the server library each time that it wishes to
	// remove its subscription for PV value change events
	// from the server tool via caServerPostEvents()
	//
	epicsShareFunc virtual void interestDelete ();

	//
	// called by the server library immediately before initiating
	// a transaction (PV state must not be modified during a
	// transaction)
	//
	// HINT: their may be many read/write operations performed within
	// a single transaction if a large array is being transferred
	//
	epicsShareFunc virtual caStatus beginTransaction ();

	//
	// called by the server library immediately after completing
	// a tranaction (PV state modification may resume after the
	// transaction completes)
	//
	epicsShareFunc virtual void endTransaction ();

	//
	// read
	//
	// The request is allowed to complete asynchronously
	// (see Asynchronous IO Classes below).
	//
	// RULE: if this completes asynchronously and the server tool references
	// its data into the prototype descriptor passed in the args to read() 
	// then this data must _not_ be modified while the reference count 
	// on the prototype is greater than zero.
	//
	// Return S_casApp_postponeAsyncIO if too many simultaneous
	// asynchronous IO operations are pending aginst the PV. 
	// The server library will retry the request whenever an
	// asynchronous IO operation (read or write) completes
	// against the PV.
	//
	epicsShareFunc virtual caStatus read (const casCtx &ctx, gdd &prototype);

	//
	// write 
	//
	// The request is allowed to complete asynchronously
	// (see Asynchronous IO Classes below).
	//
	// Return S_casApp_postponeAsyncIO if too many simultaneous
	// asynchronous IO operations are pending aginst the PV. 
	// The server library will retry the request whenever an
	// asynchronous IO operation (read or write) completes
	// against the PV.
	//
	epicsShareFunc virtual caStatus write (const casCtx &ctx, gdd &value);

	//
	// chCreate() is called each time that a PV is attached to
	// by a client. The server tool may choose not to
	// implement this routine (in which case the channel
	// will be created by the server). If the server tool
	// implements this function then it must create a casChannel object
	// (or a derived class) each time that this routine is called
	//
	epicsShareFunc virtual casChannel *createChannel (const casCtx &ctx,
		const char * const pUserName, const char * const pHostName);

	//
	// destroy() is called 
	// 1) each time that a PV transitions from
	// a situation where clients are attached to a situation
	// where no clients are attached.
	// 2) once for all PVs that exist when the server is deleted
	//
	// the default (base) "destroy()" executes "delete this"
	//
	epicsShareFunc virtual void destroy ();

	//
	// tbe best type for clients to use when accessing the
	// value of the PV
	//
	epicsShareFunc virtual aitEnum bestExternalType () const;

	//
	// Returns the maximum bounding box for all present and
	// future data stored within the PV. 
	//
	// The routine "dimension()" returns the maximum
	// number of dimensions in the hypercube (0=scaler, 
	// 1=array, 2=plane, 3=cube ...}.
	//
	// The routine "maxBound(dimension)" returns the 
	// maximum length of a particular dimension of the
	// hypercube as follows:
	// 
	//	dim equal to	0	1	3	...	
	//	-------------------------------------------
	// hypercube 
	// type	
	// ---------
	//	
	// array		array
	//			length
	//	
	// plane		x	y	
	//
	// cube			x	y	z 
	//
	// .
	// .
	// .
	//
	// The default (base) "dimension()" returns zero (scaler).
	// The default (base) "maxBound()" returns scaler bounds
	// for all dimensions.
	//
	// Clients will see that the PV's data is scaler if
	// these routines are not supplied in the derived class.
	//
	// If the "dimension" argument to maxBounds() is set to
	// zero then the bound on the first dimension is being
	// fetched. If the "dimension" argument to maxBound() is
	// set to one then the bound on the second dimension
	// are being fetched...
	//
	epicsShareFunc virtual unsigned maxDimension() const; // return zero if scaler
	epicsShareFunc virtual aitIndex maxBound (unsigned dimension) const;

	//
	// Server tool calls this function to post a PV event.
	//
	epicsShareFunc void postEvent (const casEventMask &select, gdd &event);

	//
	// peek at the pv name
	//
	// NOTE if there are several aliases for the same PV
	// this routine should return the canonical (base)
	// name for the PV
	//
	epicsShareFunc virtual const char *getName() const = 0;

	//
	// Find the server associated with this PV
	// ****WARNING****
	// this returns NULL if the PV isnt currently installed 
	// into a server (this situation will exist if
	// the pv isnt deleted in a derived classes replacement
	// for virtual casPV::destroy() or if the PV is created
	// before the server
	// ***************
	//
	epicsShareFunc caServer *getCAS() const;

	//
	// only used when caStrmClient converts between
	// casPV * and casPVI *
	//
	friend class casStrmClient;

	//
	// This constructor is preserved for backwards compatibility only.
	// Please do _not_ use this constructor.
	//
	epicsShareFunc casPV (caServer &);
};

//
// casChannel - Channel Access Server - Channel API Class
//
// Deletion Responsibility
// -------- --------------
// o the server lib will not call "delete" directly for any
// casChannel created by the server tool because we dont know 
// that "new" was called to create the object
// o The server tool is responsible for reclaiming storage for any
// casChannel it creates. The destroy() virtual function will
// assist the server tool with this responsibility. The
// virtual function casChannel::destroy() does a "delete this".
// o The destructor for this object will cancel any
// client attachment to this channel (and reclaim any resources
// allocated by the server library on its behalf)
//
class casChannel : private casPVListChan {
public:
	epicsShareFunc casChannel(const casCtx &ctx);
	epicsShareFunc virtual ~casChannel();

	//
	// Called when the user name and the host name are changed
	// for a live connection.
	//
	epicsShareFunc virtual void setOwner(const char * const pUserName, 
		const char * const pHostName);

	//
	// the following are encouraged to change during an channel's
	// lifetime
	//
	epicsShareFunc virtual aitBool readAccess () const;
	epicsShareFunc virtual aitBool writeAccess () const;
	// return true to hint that the opi should ask the operator
	// for confirmation prior writing to this PV
	epicsShareFunc virtual aitBool confirmationRequested () const;

	//
	// This is called for each channel in the server if
	// caServer::show() is called and the level is high 
	// enough
	//
	epicsShareFunc virtual void show(unsigned level) const;

	//
	// destroy() is called when 
	// 1) there is a client initiated channel delete 
	// 2) there is a server tool initiaed PV delete
	// 3) there is a server tool initiated server delete
	//
	// the casChannel::destroy() executes a "delete this"
	//
	epicsShareFunc virtual void destroy();

	//
	// server tool calls this to indicate change in access
	// rights has occurred
	//
	epicsShareFunc void postAccessRightsEvent();

	//
	// Find the PV associated with this channel 
	// ****WARNING****
	// this returns NULL if the channel isnt currently installed 
	// into a PV (this situation will exist only if
	// the channel isnt deleted in a derived classes replacement
	// for virtual casChannel::destroy() 
	// ***************
	//
	epicsShareFunc casPV *getPV();

	//
	// only used when casStrmClient converts between
	// casChannel * and casChannelI *
	//
	friend class casStrmClient;
};

//
// Asynchronous IO Classes
//
// The following virtual functions allow for asynchronous completion:
//
//	Virtual Function		Asynchronous IO Class
//	-----------------		---------------------
// 	caServer::pvExistTest()		casAsyncPVExistIO
// 	caServer::createPV()		casAsyncCreatePVIO
// 	casPV::read()			casAsyncReadIO
// 	casPV::write()			casAsyncWriteIO
//
// To initiate asynchronous completion create a corresponding
// asynchronous IO object from within one of the virtual 
// functions shown in the table above and return the status code
// S_casApp_asyncCompletion. Use the member function 
// "postIOCompletion()" to inform the server library that the 
// requested operation has completed.
//
//
// Deletion Responsibility
// -------- --------------
// o the server lib will not call "delete" directly for any
// casAsyncIO created by the server tool because we dont know 
// that "new" was called to create the object.
// o The server tool is responsible for reclaiming storage for any
// casAsyncIO it creates. The destroy virtual function will
// assist the server tool with this responsibility. The 
// virtual function casAsyncIO::destroy() does a "delete this".
// o Avoid deleting the casAsyncIO immediately after calling
// postIOCompletion(). Instead proper operation requires that
// the server tool wait for the server lib to call destroy after 
// the response is successfully queued to the client
// o If for any reason the server tool needs to cancel an IO
// operation then it should post io completion with status
// S_casApp_canceledAsyncIO. Deleting the asynchronous io
// object prior to its being allowed to forward an IO termination 
// message to the client will result in NO IO CALL BACK TO THE
// CLIENT PROGRAM (in this situation a warning message will be printed by 
// the server lib).
//

//
// casAsyncIO
//
// this class implements a common virtual destroy for
// all of the asynchronous IO classes
//
class casAsyncIO {
public:
	//
	// force virtual destructor 
	//
	epicsShareFunc virtual ~casAsyncIO() = 0;

	//
	// called by the server lib after the response message
	// is succesfully queued to the client or when the
	// IO operation is canceled (client disconnects etc).
	//
	// default destroy executes a "delete this".
	//
	epicsShareFunc virtual void destroy();
};

//
// casAsyncReadIO 
// - for use with casPV::read()
//
// **Warning**
// The server tool must reference the gdd object
// passed in the arguments to casPV::read() if it is 
// necessary for this gdd object to continue to exist
// after the return from casPV::read(). If this
// is done then it is suggested that this gdd object
// be referenced in the constructor, and unreferenced
// in the destructor, for the class deriving from 
// casAsyncReadIO.
// **
class casAsyncReadIO : public casAsyncIO, private casAsyncRdIOI {
public:
	//
	// casAsyncReadIO()
	//
	epicsShareFunc casAsyncReadIO(const casCtx &ctx) : 
		casAsyncRdIOI(ctx, *this) {}

	//
	// force virtual destructor 
	//
	epicsShareFunc virtual ~casAsyncReadIO(); 

	//
	// place notification of IO completion on the event queue
	// (this function does not delete the casAsyncIO object). 
	// Only the first call to this function has any effect.
	//
	epicsShareFunc caStatus postIOCompletion(caStatus completionStatusIn, gdd &valueRead)
	{
		return this->casAsyncRdIOI::postIOCompletion (
			completionStatusIn, valueRead);
	}

	//
	// Find the server associated with this async IO 
	// ****WARNING****
	// this returns NULL if the async io isnt currently installed 
	// into a server
	// ***************
	//
	epicsShareFunc caServer *getCAS() const
	{
		return this->casAsyncRdIOI::getCAS();
	}
};

//
// casAsyncWriteIO 
// - for use with casPV::write()
//
// **Warning**
// The server tool must reference the gdd object
// passed in the arguments to casPV::write() if it is 
// necessary for this gdd object to continue to exist
// after the return from casPV::write(). If this
// is done then it is suggested that this gdd object
// be referenced in the constructor, and unreferenced
// in the destructor, for the class deriving from 
// casAsyncWriteIO.
// **
//
class casAsyncWriteIO : public casAsyncIO, private casAsyncWtIOI {
public:
	//
	// casAsyncWriteIO()
	//
	epicsShareFunc casAsyncWriteIO(const casCtx &ctx) : 
		casAsyncWtIOI(ctx, *this) {}

	//
	// force virtual destructor 
	//
	epicsShareFunc virtual ~casAsyncWriteIO(); 

	//
	// place notification of IO completion on the event queue
	// (this function does not delete the casAsyncIO object). 
	// Only the first call to this function has any effect.
	//
	epicsShareFunc caStatus postIOCompletion(caStatus completionStatusIn)
	{
		return this->casAsyncWtIOI::postIOCompletion (completionStatusIn);
	}

	//
	// Find the server associated with this async IO 
	// ****WARNING****
	// this returns NULL if the async io isnt currently installed 
	// into a server
	// ***************
	//
	epicsShareFunc caServer *getCAS() const
	{
		return this->casAsyncWtIOI::getCAS();
	}
};

//
// casAsyncPVExistIO 
// - for use with caServer::pvExistTest()
//
class casAsyncPVExistIO : public casAsyncIO, private casAsyncExIOI {
public:
	//
	// casAsyncPVExistIO()
	//
	epicsShareFunc casAsyncPVExistIO(const casCtx &ctx) : 
		casAsyncExIOI(ctx, *this) {}

	//
	// force virtual destructor 
	//
	epicsShareFunc virtual ~casAsyncPVExistIO(); 

	//
	// place notification of IO completion on the event queue
	// (this function does not delete the casAsyncIO object). 
	// Only the first call to this function has any effect.
	//
	epicsShareFunc caStatus postIOCompletion(const pvExistReturn retValIn)
	{
		return this->casAsyncExIOI::postIOCompletion (retValIn);
	}

	//
	// Find the server associated with this async IO 
	// ****WARNING****
	// this returns NULL if the async io isnt currently installed 
	// into a server
	// ***************
	//
	epicsShareFunc caServer *getCAS() const
	{
		return this->casAsyncExIOI::getCAS();
	}
};

//
// casAsyncPVCreateIO 
// - for use with caServer::createPV()
//
class casAsyncPVCreateIO : public casAsyncIO, private casAsyncPVCIOI {
public:
	//
	// casAsyncPVCreateIO()
	//
	epicsShareFunc casAsyncPVCreateIO(const casCtx &ctx) : 
		casAsyncPVCIOI(ctx, *this) {}

	//
	// force virtual destructor 
	//
	epicsShareFunc virtual ~casAsyncPVCreateIO(); 

	//
	// place notification of IO completion on the event queue
	// (this function does not delete the casAsyncIO object). 
	// Only the first call to this function has any effect.
	//
	epicsShareFunc caStatus postIOCompletion(const pvCreateReturn &retValIn)
	{
		return this->casAsyncPVCIOI::postIOCompletion (retValIn);
	}

	//
	// Find the server associated with this async IO 
	// ****WARNING****
	// this returns NULL if the async io isnt currently installed 
	// into a server
	// ***************
	//
	epicsShareFunc caServer *getCAS() const
	{
		return this->casAsyncPVCreateIO::getCAS();
	}
};

#endif // ifdef includecasdefh (this must be the last line in this file) 

