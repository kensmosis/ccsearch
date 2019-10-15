#include <set>
#include <vector>
#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include "OConfig.h"
#include "OFeature.h"
#include "OCFN.h"
#include "OColl.h"

OConfig::OConfig(void) : OMtxCtlBase(), _nf(0), _f(NULL), _pfnum(0), _pf(NULL), _pfn(NULL), _maxcost(0), _cfn(NULL), _numcfn(0), _ni(0), _ic(NULL), _iv(NULL), _ctol(-1), _itol(-1), _ntol(0), _resnumb(0), _maxres(0), _smode(1), _maxcosttol(0.01), _res(NULL) {}

OConfig::~OConfig(void)
{
	Clear();
}

bool OConfig::Init(int nf,int pf,int *pfn,int pfnn,int ni,float mc,int nc)
{
	OConfigMtxCtl mtx(this);
	if (IsInited()) return false;	// Non-mtx so ok to call
	_nf= nf;
	if (nf>0) _f= new OFeature [nf];
	_pfnum= pf;
	if (_f&&_pfnum>=0&&_pfnum<_nf) _pf= &(_f[pf]);
	_cs= 0;
	if (pfn&&pfnn>0)
	{
		_pfn= new int [pfnn];
		for (int i=0;i<pfnn;++i) 
		{
			_pfn[i]= pfn[i];
			_cs+= _pfn[i];
		}
	}
	_ni= ni;
	_maxcost= mc;
	if (_numcfn<0) return false;
	_numcfn= nc;
	_cfn= (_numcfn>0)?(new OCFN* [_numcfn]):NULL;
	for (int i=0;i<_numcfn;++i) _cfn[i]= NULL;

	// Create res
	if (_res) delete _res;
	_res= new OCollMM(_cs,_resnumb,_maxres,_ctol);

	return true;
}

bool OConfig::IsSensible(bool ctoo) const
{
	OConfigMtxCtl mtx(this);
	if (_nf<=0) return false;
	if (!_f) return false;
	if (_pfnum<0||_pfnum>=_nf) return false;
	if (!_pf) return false;
	if (!_pfn) return false;
	for (int i=0;i<_pf->NumGroups();++i)
		if (_pfn[i]<0) return false;
	if (_maxcost<0) return false;
	if (_ni<=0) return false;
	if (_ni>32767) return false;	// Since we need to be able to wedge into int16_t at some point
	if (!_ic) return false;
	if (!_iv) return false;
	if (_resnumb<=0) return false;
	if (_maxres<0) return false;
	if (_smode<1||_smode>4) return false;
	if (_ctol<0) return false;
	if (_itol<0) return false;
	if (_ntol<0) return false;
	for (int i=0;i<_nf;++i)
		if (!_f[i].IsSensible()) return false;
	if (_numcfn<0||(_numcfn>0&&!_cfn)) return false;
	if (ctoo)
	{
		for (int i=0;i<_numcfn;++i)
			if (!_cfn[i]||!_cfn[i]->IsValid()) return false;
		if (!_res) return false;
	}
	// Add some tests for reasonableness of tolerances, etc
	return true;
}

bool OConfig::SetFeature(int fn,int ng,bool ispart,bool *f)
{
	OConfigMtxCtl mtx(this);
	if (!_f) return false;
	if (fn<0||fn>=_nf) return false;
	if (!f) return false;
	if (ng<=0) return false;
	if (!_f[fn].Configure(ng,_ni,ispart,f)) return false;
	return true;
}

bool OConfig::InitItems(float *c,float *v)
{
	OConfigMtxCtl mtx(this);
	if (!c||!v) return false;
	if (_ni<=0) return false;
	if (_ic||_iv) return false;
	_ic= new float [_ni];
	_iv= new float [_ni];
	for (int i=0;i<_ni;++i)
	{
		_ic[i]= c[i];
		_iv[i]= v[i];
	}
	return true;
}

bool OConfig::InitConstraints(void)
{
	OConfigMtxCtl mtx(this);
	bool rc= true;
	for (int i=0;i<_numcfn;++i)
	{
		if (!_cfn[i]) rc= false;
		else
		{
			_cfn[i]->Reset();
			if (!_cfn[i]->Init()) rc= false;
		}
	}
	return rc;
}

bool OConfig::SetConstraint(int cn,OCFN *c)
{
	OConfigMtxCtl mtx(this);
	if (cn<0||cn>=_numcfn) return false;
	if (!c) return false;
	if (_cfn[cn]!=NULL) return false;
	_cfn[cn]= c;
	return true;
}

const OFeature *OConfig::AccessFeature(int n) const { return (_f&&n>=0&&n<_nf)?&(_f[n]):NULL; }

void OConfig::PrepToExclude(int i,int j) 
{ 
	// Deliberately don't mutex protect at OConfig-level
	if (_pf) _pf->PrepToExclude(i,j); 
}

void OConfig::ProcessExclusions(void)
{
	// Deliberately don't mutex protect at OConfig-level
	if (_pf) _pf->ProcessExclusions();
}


double OConfig::EstFullStateSpace(void) const
{
	if (!_pf||!_pfn) return 0.0;
	double x= 0.0;
	for (int i=0;i<_pf->NumGroups();++i)
	{
		int cnt= _pfn[i];
		if (cnt<=0) continue;
		long n= _pf->NumItemsInGroup(i);
		if (n<cnt) return -1;

		// The following is just log_2 of (n choose cnt)
		for (long j=0;j<cnt;++j)
		{
			x+= log10((double)(n-j));
			x-= log10((double)(j+1));
		}
	}
	return x;
}

int OConfig::TestConstraints(const int *x) const
{
	// NO mutex protection here!!
	if (!x) return -1;
	for (int i=0;i<_numcfn;++i)
		if (_cfn[i]&&!_cfn[i]->Test(x)) 
			return i;
	return -1;
}

void OConfig::InitParms(float ctol,float itol,int ntol,int resnumb,long maxres,int smode)
{
	OConfigMtxCtl mtx(this);
	_ctol= ctol;
	_itol= itol;
	_ntol= ntol;
	_resnumb= resnumb;
	_maxres= maxres;
	_smode= smode;
}

/*

This function culls (via removal from primary feature) from each primary group all items which are strictly inferior in the following sense:  if we are to choose n items in that primary group then if there are n or more items of cost <= item's cost and value > (1+itol)*item's value.  We use > so that if itol=0 we won't exclude items randomly.  The basic idea is that if we can pick n items which are no more expensive and perform at least as well then we don't need the item in question.  Note that this isn't strictly true, though.  If an item can appear in multiple primary groups then it may be the case that one of those superior items will be chosen in another group, leaving a slot open for the item we culled.  There is no elegant way to avoid this.  However, we are not aiming for absolute precision.  That scenario presumably will be rare --- especially if tol is any reasonable value (as opposed to 0).  In our old system, we compensated by using n' instead of n, where n' uses knowledge of which groups are unions of other groups and adjusting accordingly.  We don't do that for 2 reasons:  (1) we have no concept of groups being unions of other groups.  They are distinct and can overlap.  (2) if a group IS a union of lots of other groups, then all groups have n' much bigger than n and therefore the cull effectively does nothing.  Note that the return value may count the same item twice if removed from two groups!

To allow for some buffer, we leave ntol+np items.  ntol is a user parm which says how many extra items we need to keep in the group.

*/
int OConfig::CullByTol(void)
{
	OConfigMtxCtl mtx(this);
	typedef std::vector<std::pair<float,int> > VVEC;
	typedef std::set<int> ISET;
	if (!_pf) return -1;
	int ng= _pf->NumGroups();
	int ntot= 0;
	// Do for each primary group
	for (int g=0;g<ng;++g)
	{
		int n= _pf->NumItemsInGroup(g);
		VVEC byc;
		VVEC byv;
		for (int i=0;i<n;++i)
		{
			int inum= _pf->ItemsInGroup(g)[i];
			float v= _iv[inum];
			float c= _ic[inum];
			byc.push_back(std::pair<float,int>(c,inum));
			byv.push_back(std::pair<float,int>(v,inum));
		}
		std::sort(byc.begin(),byc.end());	// Now byc is by value in asc order
		std::sort(byv.begin(),byv.end());
		std::reverse(byv.begin(),byv.end());	// Now byv is by value in desc order
		ISET s;

		// Scan over items in group in descending order of value
		for (int i=0;i<n;++i)
		{
			int ii= byv[i].second;
			if (ii<0) continue;	// Really an error
			float c= _ic[ii];
			float v= _iv[ii];
			float tv= v*(1.0+_itol);
			int cnt= 0;
			// Scan over items of performance >v(1+itol).
			// Note that we don't need to worry about double counting items which already have been marked for removal (i.e. using them as part of cnt)
			// If they are reached (as ii) afterward, then clearly lower perf, so would be eliminated anyway for same reason as jj if higher salary and 
			// wouldn't be affected by jj if not.  
			for (int j=0;j<i;++j)
			{
				int jj= byv[j].second;
				if (jj<0) continue;	// Really an error
				float c2= _ic[jj];
				float v2= _iv[jj];
				if (c2>c) continue;	// Ignore if higher cost
				if (v2<=tv) break;	// We're done (since descending on value)
				++cnt;
				if (cnt>=_pfn[g]+_ntol)
				{
					s.insert(ii);
					break;
				} 
			}
		}
		for (ISET::const_iterator kk= s.begin();kk!=s.end();++kk)
			PrepToExclude(*kk,g);
		ntot+= s.size();
	}

	ProcessExclusions();
	return ntot;
}

void OConfig::Clear(void)
{
	OConfigMtxCtl mtx(this);
	delete [] _f;
	_f= NULL;
	_nf= 0;
	delete [] _pfn;
	_pfn= NULL;
	_pfnum= 0;
	_pf= NULL;
	_cs= 0;
	_maxcost= 0;
	for (int i=0;i<_numcfn;++i) delete _cfn[i];
	delete [] _cfn;
	_cfn= NULL;
	_numcfn= 0;
	_ni= 0;
	delete [] _ic;
	_ic= NULL;
	delete [] _iv;
	_iv= NULL;
	_ctol= -1;
	_itol= -1;
	_ntol= 0;
	_resnumb= 0;
	_maxres= 0;
	_smode= 1;
	if (_res) delete _res;
	_res= NULL;
}

void OConfig::ResetResults(void)
{
	OConfigMtxCtl mtx(this);
	if (_res) delete _res;
	_res= new OCollMM(_cs,_resnumb,_maxres,_ctol);
}

void OConfig::DumpItems(FILE *f) const
{
	if (!f) return;
	fprintf(f,"--------Items-----------\n");
	for (int i=0;i<_ni;++i)
		fprintf(f,"%d %f %f\n",i,_ic[i],_iv[i]);
}

void OConfig::DumpFeatures(FILE *f) const
{
	if (!f) return;
	for (int i=0;i<_nf;++i)
	{
		fprintf(f,"------Feature %d-------\n",i);
		_f[i].Dump(f);
	}
}

void OConfig::DumpConfig(FILE *f) const
{
	fprintf(f,"---------Config-----------\n");
	fprintf(f,"%20s : %10d\n","NumFeatures",_nf);

	fprintf(f,"%20s : ","GroupsPerFeature");
	for (int i=0;i<_nf;++i)
	{
		if (i>0) fprintf(f,":");
		fprintf(f,"%d",_f[i].NumGroups());
	}
	fprintf(f,"\n");

	fprintf(f,"%20s : %10d\n","PrimaryFeature",_pfnum);
	fprintf(f,"%20s : ","PrimaryGroupPicks");
	for (int i=0;i<_pf->NumGroups();++i)
	{
		if (i>0) fprintf(f,":");
		fprintf(f,"%d",_pfn[i]);
	}
	fprintf(f,"\n");

	fprintf(f,"%20s : %10d\n","CollectionSize",_cs);
	fprintf(f,"%20s : %10d\n","NumItems",_ni);
	fprintf(f,"%20s : %10d\n","NumConstraints",_numcfn);
	for (int i=0;i<_numcfn;++i)
		fprintf(f,"Constraint%d : %s\n",i+1,_cfn[i]->Desc().c_str());

	fprintf(f,"%20s : %f\n","MaxCost",_maxcost);
	fprintf(f,"%20s : %f\n","MaxCostTol",_maxcosttol);
	fprintf(f,"%20s : %f\n","ctol",_ctol);
	fprintf(f,"%20s : %f\n","itol",_itol);
	fprintf(f,"%20s : %d\n","ntol",_ntol);
	fprintf(f,"%20s : %d\n","resnumb",_resnumb);
	fprintf(f,"%20s : %ld\n","maxres",_maxres);
	fprintf(f,"%20s : %d\n","smode",_smode);
}


