#ifndef OCOLLDEFFLAG
#define OCOLLDEFFLAG

#include <set>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "OMutex.h"
#include "OGlobal.h"

// Puts into descending order (so true if a>b)
struct OCollGreaterThan
{
	bool operator() (const char *a,const char *b) const
	{
		if ( (*((float *)(a))) > (*((float *)(b))) ) return true;
		else if ( (*((float *)(a))) < (*((float *)(b))) ) return false;
		else return (a<b);		// Use pointers when vals are same to establish strict ordering
	}
};


/* Collection memory manager */
class OCollMM : public OMtxCtlBase, public OGlobal
{
private:
	OCollMM(void) {}
	OCollMM(const OCollMM &x) {}
protected:
	// Mutex control for thread-safety
	typedef OMtxCtl<OCollMM> OCMMMtxCtl;
	friend class OMtxCtl<OCollMM>;

	// A set of blocks (purely for memory allocation purposes in case max size not specified up front)
	typedef std::set<char *> BSET;
	BSET _s;
	
	// The ordered set of collections (by value).  Sorted by descending value
	typedef std::set<char *,OCollGreaterThan> CSET;
	CSET _c;
	mutable CSET::const_iterator _cii;	// For returning the results

	// Configuration
	int _rsize;		// Record size in bytes (collection + value storage size)
	int _bsize;		// Number of records per block
	int _clen;		// Number of items in collection
	long _maxrec;		// Maximum number of records we retain.  0 if no limit
	float _ctol;		// Max allowed value is maxval*(1.0-ctol)

	// Stats
	long _nreqs;	// Total requests
	long _ncurr;	// Current active entries requests
	float _maxval;	// Max collection value encountered (NOTE: there may be no existing coll with this if GC removes it later!!!)
	float _minval;	// Min collection value currently present

	void addblock(void);		// Add a new block
	char *droplowest(bool isbad);		// Drop the lowest entry (returning pointer) and update info
	std::string getstatstr(void) const;		// Return a string of stats
	void gc(void);		// Unset all entries below minallowed
public:
	// Manage
	OCollMM(int clen,int bsize,long maxrec,float ctol);
	~OCollMM(void);
	bool Add(bool verbose,int *c,float v);	// Get a coll
	
	// Inspect (some of these may be slightly costly
	long GetNumRec(void) const { return _ncurr; }	// Total number of active records
	long GetNumRecAlloc(void) const { return _s.size()*_bsize; }	// How many records have been allocated so far
	long GetNumReqs(void) const { return _nreqs; }	// Total additions so far (some may have been removed later, though)
	float GetMaxVal(void) const { return _maxval; }	// True maxval so far
	float GetMinVal(void) const { return _minval; }	// Present minval
	float GetMinAllowed(void) const { return (!IsBadVal(_maxval))?(_maxval*(1.0-_ctol)):BadVal(); }
	bool IsFull(void) const { return (_maxrec>0&&_ncurr==_maxrec); }	// Are we full
	bool CanAdd(float v) const;	// If we try to add a record with value v will we succeed?

	// Access results.  NOTE: only can be called after Finalize()!!!!!!
	void InitResIter(void) { gc(); _cii= _c.begin(); }	// Prepare to read results from start.  MUST be called after all items have been added and before any call to GetRes()!
	int GetRes(int n,unsigned int **r,float *m) const;	 // Populate the necessary arrays with the next (up to) n results.  n is the size of r,m (which must be >=iend-istart or we reduce iend to istart+n).  We return the results from where we left off (or the start if first call after InitResIter(), and in descending order of value. For a fixed value, result order is not stable.  
	std::string GetStatStr(void) const;		// Return a string of stats

	// Access an individual record
	float GetVal(const char *o) const { return o?(*((float *)(o))):BadVal(); }	// Return the value, or BadVal() if error
	void SetVal(char *o,float v) const { if (o) *((float *)(o))= v; }
	int GetItem(const char *o,int i) const { return o?int(*((int16_t *)(o+sizeof(float)+sizeof(int16_t)*i))):-1; }	// Return item i or -1 if error. 
	void SetItem(char *o,int i,int n) const { if (o&&i>=0&&i<_clen&&n>=0&&n<32767) *((int16_t *)(o+sizeof(float)+sizeof(int16_t)*i))= (int16_t)n; }	// Set item i to n
	std::string GetStr(const char *o,bool justints) const;	// Display string.  If justints, only displays list of items
	bool IsUnset(const char *o) const { return (!o||IsBadVal(GetVal(o))); }
	void Unset(char *o) const { SetVal(o,BadVal()); }
};

#endif
