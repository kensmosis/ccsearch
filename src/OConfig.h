#ifndef OCONFIGDEFFLAG
#define OCONFIGDEFFLAG

#include <list>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>
#include "OFeature.h"
#include "OMutex.h"
#include "OGlobal.h"

class OCFN;
class OFeature;
class OCollMM;

// Main configuration class.
class OConfig : public OMtxCtlBase, public OGlobal
{
private:
	OConfig(const OConfig &x) {}
protected:
	typedef OMtxCtl<OConfig> OConfigMtxCtl;
	friend class OMtxCtl<OConfig>;

	// Features
	int _nf;	// Number of features
	OFeature *_f;	// features (length of array is _nf)
	int _pfnum;	// The primary feature [0.._nf-1] number
	OFeature *_pf;	// Quick pointer to primary feature

	// Main constraints
	int *_pfn;	// The number of items to be chosen for each value of the primary feature.  Length is _f[_pf]->NumGroup();
	int _cs;	// Total collection size (sum of values in _pfn)
	float _maxcost;	// Maximum allowed cost function for a collection

	// Ancillary constraints
	OCFN **_cfn;	// Contraint functions
	int _numcfn;	// Number of constraint functions

	// Item info
	int _ni;	// Number of items
	float *_ic;	// Costs of items.  Length _ni.  Not managed.
	float *_iv;	// Value of items.  Length _ni.  Not managed.

	// Global search Parameters
	float _ctol;	// Used for culling collections during tree search 
	float _itol;	// Used for CullByTol (individual culling within primary feature groups)
	int _ntol;	// Number of extra items to keep in individual item cull stage (per primary group)
	int _resnumb;	// New MM block size
	long _maxres;	// Maximum number of results to allow in MM (more triggers a special GC).  0 means ignore.  
	int _smode;	// 1= Descending Perf/small-to-large groups, 2= Desc Perf/large-to-small, 3= Asc Cost/small-to-large, 4= Asc Cost/large-to-small.  Best to worst:  1, 2, 3, 4.  
	float _maxcosttol;	// Used for integer and near-integer cost values.   Shouldn't need adjusting unless costs are floats.

	// Results
	mutable OCollMM *_res;
public:
	// Management
	OConfig(void);
	~OConfig(void);
	bool Init(int nf,int pf,int *pfn,int pfnn,int ni,float mc,int nc);	// Initialize the basics. 
	bool IsInited(void) const { return _ni>0; }	// Have we already init'ed?
	bool IsSensible(bool ctoo) const;	// Are the parms set sensible?  Some sanity checks
	bool SetFeature(int fn,int ng,bool ispart,bool *f);
	bool InitItems(float *c,float *m);
	bool SetConstraint(int cn,OCFN *c);	// We take ownership of c
	bool InitConstraints(void);	// Resets and inits constraints
	void InitParms(float ctol,float itol,int ntol,int resnumb,long maxres,int smode);
	void SetMaxCostTol(float x) { _maxcosttol= (x>0?x:0); }
	float MaxCostTol(void) const { return _maxcosttol; }
	
	// Excluding items from groups and overall [Used by individual cull function]
	void PrepToExclude(int i,int j);	// j is group of primary feature.  If -1, exclude overall
	void ProcessExclusions(void);		// Reconfigure everything so that all exclusions are applied.  This CANNOT be undone. 

	// Simple accessors
	int NumItems(void) const { return _ni; }
	const OFeature *AccessFeature(int n) const;
	const OFeature *AccessPrimaryFeature(void) const { return _pf; }
	int NumFeatures(void) const { return _nf; }
	int NumPrimaryGroups(void) const { return _pf?(_pf->NumGroups()):-1; }
	int NumPicks(int i) const { return (_pf&&_pfn&&i>=0&&i<_pf->NumGroups())?_pfn[i]:-1; }
	int PrimaryFeatureNum(void) const { return _pfnum; }
	float MaxCost(void) const { return _maxcost; }
	float CTol(void) const { return _ctol; }
	bool IsSearchByCost(void) const { return (_smode==3||_smode==4); }
	bool IsGroupLowToHigh(void) const { return (_smode==1||_smode==3); }
	OCollMM *AccessMM(void) const { return _res; }

	// Value functions.  i is the item.  Returns 
	float *Vals(void) const { return _iv; }				// List of values in item order
	float *Costs(void) const { return _ic; }				// List of costs in item order

	// Estimate the total unfiltered state space size.  Returns log_2 of the value.  We do this to avoid overflow errors (even long can't handle numbers as big as we can get!)
	double EstFullStateSpace(void) const;	// -1 on error.

	// Other useful
	int CollectionSize(void) const { return _cs; }	// Number of items in a collection. -1 if error
	int TestConstraints(const int *x) const;	// Test a collection against all constraints.  -1 if satisfies all constraints.  Otherwise returns the 1st constraint number violated (starting at 0).  NOT mutex-protected, so be careful with any late-stage modifications.  Too expensive to mutex this!  And unnecessary.
	int NumConstraints(void) const { return _numcfn; }

	// Cull Players which fail individual tol test.  Returns number culled.  Note that ni is unchanged.  The culling is done in the primary feature (and all derived arrays).  The returned value only counts those not already culled.
	int CullByTol(void);
	void Clear(void);		// Clear everything
	void ResetResults(void);	// Reset results
	void DumpItems(FILE *f) const;	// Dump items, vals, costs
	void DumpFeatures(FILE *f) const;	// Dump feature tables and items in groups
	void DumpConfig(FILE *f) const;	// Dump parms and config
};



#endif
