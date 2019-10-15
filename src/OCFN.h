#ifndef OCFNDEFFLAG
#define OCFNDEFFLAG

#include <inttypes.h>
#include <set>
#include <string>
#include "OMutex.h"
class OConfig;

/* Constraint functionoid.  
Given a collection, it returns true if it satisfies the constraints.

There are 2 ways to create instances:
	1. Factory method OCFN::GetFromType.  This only works for the 2 internal types
	2. In C++ we can construct the specific constraint class (setting its members via the constructor parms)

How to create a user constraint class:  

First, note that a user defined class can only be called via C++.  At present we don't have a generic factory and registry for user-defined types or the ability to call them from the command-line or python API.  

Derive from OCFN, and pick a "type" id that is >1  (the two intrinsic types are 0 and 1) and doesn't conflict with any other of your user-defined types.  This actually is irrelevant at this point, but good practice for future use.

Write the init(), test(), desc(), gettype(), and isvalid() fns, as well as a virtual destructor if needed. 

To use, construct an instance of the specified fns, passing whatever config info is needed via the constructor parms.  Add the pointer to the instance via OConfig::SetConstraint().  Note that once passed in, ownership is assumed by OConfig.  

All mutex-management is at the base-class level, so do not use your own mutexes in any user-defined subclass!

*/
class OCFN : public OMtxCtlBase
{
private:
	OCFN(void) {}
	OCFN(const OCFN &x) {}
protected:
	typedef OMtxCtl<OCFN> OCFNMtxCtl;
	friend class OMtxCtl<OCFN>;
	const OConfig *_src;		// We need this for various lookups.
protected:
	virtual bool init(void)=0;	// Call this after OConfig is constructed and set up but before any tests run. 
	virtual bool test(const int *) const=0;	// Test a collection.   Returns true if passes (i.e. not in violation of constraint).
	virtual bool isvalid(void) const=0;	// Is the config of this OCFN valid?
	virtual int  gettype(void) const=0;	// Unique ID for the type of constraint. 0 and 1 are reserved, all others are free for user derived classes.
	virtual std::string desc(void) const=0;	// For debugging
	virtual void reset(void)=0;	// Reset everything but _src
public:
	OCFN(const OConfig *src) : OMtxCtlBase(), _src(src) {}
	virtual ~OCFN(void) {}
	static OCFN *GetFromIntrinsicType(const OConfig *src,int t,int l,int *vl);	// This is how we create instances of constraints.  Given a type, an arg-list len, and an arglist, instantiate a class of the right type with the parms set as needed.  NULL on failure.  The meaning of vl depends on the class type.
	bool Init(void);	// Mutex-protected
	bool Test(const int *x) const { return this->test(x); }	// Read-only and too expensive to mutex protect so we don't
	bool IsValid(void) const;	// Mutex-protected
	int Type(void) const { return this->gettype(); }	// No need to mutex protect
	std::string Desc(void) const;	// Mutex-protected
	void Reset(void);	// Mutex-protected
	

};

// Base constraint for built-in group-counting constraints which require a partition
class OCFNGrpCntBase : public OCFN
{
protected:
	int _clen;		// Items in collection
	int _ni;		// Number of items
	int _fn;		// Feature number
	int _ng;		// Number of groups in feature
	int *_l;		// List of groups for items.  Length ni
public:
	OCFNGrpCntBase(const OConfig *src,int fnum);	// fnum= feature num
	virtual ~OCFNGrpCntBase(void) { this->reset(); }
	virtual bool init(void);
	virtual bool isvalid(void) const;
	virtual std::string desc(void) const;
	virtual void reset(void);
};

// At least n groups from feature m
class OCFNMinGroups : public OCFNGrpCntBase
{
protected:
	int _cnt;	// Minimum number of groups needed
public:
	OCFNMinGroups(const OConfig *src,int fnum,int mcnt);	// fnum= feature num, mcnt= min count
	~OCFNMinGroups(void) { this->reset(); }
	virtual bool init(void);
	virtual bool test(const int *) const;
	virtual bool isvalid(void) const;
	static int SType(void) { return 0; }
	virtual int gettype(void) const { return OCFNMinGroups::SType(); }
	virtual std::string desc(void) const;
	virtual void reset(void);
};

// At most n items from any group of feature m
class OCFNMaxItems : public OCFNGrpCntBase
{
protected:
	int _cnt;	// Max count of items allowed in a group
	mutable int *_x;	// Counts for groups.  Length _ng.  (tmp used in Test() only)
public:
	OCFNMaxItems(const OConfig *src,int fnum,int mcnt);	// fnum= feature num, mcnt= max count
	~OCFNMaxItems(void) { this->reset(); }
	virtual bool init(void);
	virtual bool test(const int *) const;
	virtual bool isvalid(void) const;
	static int SType(void) { return 1; }
	virtual int gettype(void) const { return OCFNMaxItems::SType(); }
	virtual std::string desc(void) const;
	virtual void reset(void);
};

#endif
