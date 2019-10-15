#ifndef OSEARCHDEFFLAG
#define OSEARCHDEFFLAG
#include <string>
#include <algorithm>
#include "OConfig.h"
#include "OMutex.h"
#include "OGlobal.h"

// Because costs often arrive as integers, but we use floats, we need to make sure we don't disallow values which exceed the maximum only due to rounding issues.  For that reason, we require a tiny threshold above the maxcost overall.  For more general applications this may need to be changed.
#define RCOSTEPSILON (0.01)

// Utility record
struct OSGCRec
{
	OSGCRec(long n,float v,float c) : _n(n), _v(v), _c(c) {}
	long _n;		// The combo number (index)
	float _v;		// The value
	float _c;		// The cost
};

// Utility class for creating n-tuple arrays ordered as requested by user
class OSGrpCombos : public OGlobal
{
private:
	OSGrpCombos(const OSGrpCombos()) {}

protected:
	bool _bycost;		// Are we sorted ascending by cost or descending by value.
	int _np;		// Number of items per combo
	long _nc;		// Number of combos
	int *_i;		// Array of combos ordered by increasing cost or decreasing value as requested.  Length is _combos*_np
	float *_v;		// Array of values
	float *_c;		// Array of costs
	const int *_n;		// Item numbers (for group).  We do not own.
	int _ni;		// Length of _n array

	bool build(bool bycost,int np,int ni,float *v,float *c,const int *n);	// Build from number of items, list of item values, costs
	static long nchoosem(int n,int m);	// Returns n choose m or 0 if error. 
	static void initctr(int *c,int n);	// Initialize to [0,1,...n-1]
	static bool nextctr(int *cl,int *cn,int np,int ni);	// Copy cl to cn and augments cn to the next ntuple (increases last value, then previous, etc, while maintaining strictly increasing sequence.  Returns false if no more combos possible
	static bool OSGRecCmpValDesc(const OSGCRec &a,const OSGCRec &b);	// Comparator for sorting by descending value
	static bool OSGRecCmpCostAsc(const OSGCRec &a,const OSGCRec &b);	// Comparator for sorting by ascending cost
public:
	// Managament
	OSGrpCombos(void) : _bycost(false), _np(0), _nc(0), _i(NULL), _v(NULL), _c(NULL), _n(NULL), _ni(0) {}
	~OSGrpCombos(void);
	bool Build(bool bycost,int np,int ni,float *v,float *c,const int *n);	// Build from number of items, list of item values, costs
	void Clear(void);

	// Info
	long Combos(void) const { return _nc; }	// Return total number of combos
	int RawItem(long i,int j) const { return (_np>0&&_i&&i>=0&&i<_nc&&j>=0&&j<_np)?_i[_np*i+j]:-1; }	// Return item j in combo i.  Indexed 0..ni-1.  -1 if out of range
	int Item(long i,int j) const { int k= RawItem(i,j); return (_n&&k>=0&&k<_ni)?_n[k]:-1; }	// Return item j in combo i, as actual item # overall.  -1 if out of range
	float Val(long i) const { return (_v&&i>=0&&i<_nc)?_v[i]:(BadVal()); }		// Return value of combo i
	float Cost(long i) const { return (_c&&i>=0&&i<_nc)?_c[i]:(BadCost()); }		// Return cost of combo i
};



// Useful info for a single primary feature group
class OSGrpRec : public OGlobal
{
private:
	OSGrpRec(const OSGrpRec &x) {}
public:
	int _g;			// Which primary group it represents
	int _np;		// Number of items we need to pick
	float _bval;		// The sum of the top _np values (the best we can do)
	float _lcost;		// The sum of the bottom _np costs (the cheapest we can do)
	float _rbval;		// The sum of _bval for all following groups (excluding current)
	float _rlcost;		// The sum of _rlcost for all following groups (excluding current)
	long _rcombos;		// Total combos involving all remaining groups (excluding current). 1 if last group.
	int _ni;		// The number of items in this group
	const int *_i;		// Items in the group (length _ni) [not owned by us]
	OSGrpCombos _gc;	// Appropriately sorted list of combos along with their values and costs
	long _c;		// Current combo
	bool Init(const OConfig &x,int i,bool bycost);	// Fill with info for ith group

	OSGrpRec(void);
	~OSGrpRec(void) {}
	long Combos(void) const { return _gc.Combos(); }
	int Item(long i,int j) const { return _gc.Item(i,j); }
	void DumpCombos(FILE *f) const;	// List all combos and total value and cost for each
};

class OSearch : public OMtxCtlBase, public OGlobal
{
private:
	OSearch(const OSearch &x) {}
protected:
	typedef OMtxCtl<OSearch> OSrchMtxCtl;
	friend class OMtxCtl<OSearch>;
	OSGrpRec *_r;		// Length _ng.  We own this.
	OSGrpRec **_rp;		// Pointers to the _r (for sorting)
	const OConfig *_oc;
	OCollMM *_m;		// Memory manager for result records.  NOT managed here.
	int _ng;		// Number of groups
	int _nc;		// Number of constraints
	bool _bycost;		// We're ordered by cost instead of value
	int _cs;		// Collection Size

	// Used for diagnostics and tracking
	long *_pcnt;		// Pruning/etc counters
	int *_icnt;		// Count of items of each val (length ni) to test for dups!
	int *_tcol;		// Dummy collection values
	int *_tloc;		// Starting loc in tcol for each group
public:
	OSearch(void);
	~OSearch(void);
	bool Search(const OConfig &x,bool bycost,bool grouplowtohigh,int debug);	// If bycost, scans each group from lowest to highest cost, otherwise (default) is from highest to lowest value.  grouplowtohigh says whether to move through groups from fewest combos to most (default is most combos to fewest).  NOTE that grouplowtohigh is completely independent of bycost and involves the group ordering, NOT the ordering of search within a group!  
	void search(float ctol,float rcost,float val,int g,int debug);
	int NumCounters(void) const { return NumIntCnts()+_nc; }
	long ReadCounter(int n) const { return (n>=0&&n<NumCounters())?_pcnt[n]:-1; }

	// Diagnostic counter indices
	static int NumIntCnts(void) { return 8; }	// Number of intrinsic counters
	static int CntAdded(void) { return 0; } 	// Total added to memory manager
	static int CntAnal(void) { return 1; }		// Total analyzed (i.e. survived pruning)
	static int CntPruned(void) { return 2; }	// Total pruned during search
	static int CntStrict(void) { return 3; }	// Pruned node and all beyond it (cost or val)
	static int CntWeak(void) { return 4; }		// Pruned just node (cost or val)
	static int CntCantAdd(void) { return 5; }	// Pruned because failed addition test relative to existing records
	static int CntDup(void) { return 6; }		// Pruned due to dup item
	static int CntConstrain(void) { return 7; }	// Pruned due to any constraint
	static std::string NameOfCnt(int n);	// Return string for counter n
};


#endif
