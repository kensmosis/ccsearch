#include <list>
#include <set>
#include <vector>
#include <assert.h>
#include <string.h>
#include "OConfig.h"
#include "OSearch.h"
#include "OColl.h"

//// Useful calc fns

//////// OSGrpCombos

long OSGrpCombos::nchoosem(int n,int m)
{
	if (n<=0||m<=0||m>n) return 0;
	if (m==n) return 1;
	long x= 1;
	long y= 1;
	for (int j=0;j<m;++j)
	{
		x*= (long)(n-j);
		y*= (long)(j+1);
	}
	assert ((x % y)==0);
	return x/y;
}

OSGrpCombos::~OSGrpCombos(void) { Clear(); }

void OSGrpCombos::Clear(void)
{
	if (_i) delete [] _i;
	if (_v) delete [] _v;
	if (_c) delete [] _c;
	_i= NULL;
	_v= NULL;
	_c= NULL;
	_bycost= false;
	_np= 0;
	_nc= 0;
}

bool OSGrpCombos::Build(bool bycost,int np,int ni,float *v,float *c,const int *n)
{
	if (np<=0||ni<=0||!v||!c||!n) return false;
	if (_i) return false; 	// Already built, must call Clear() first

	if (!build(bycost,np,ni,v,c,n))
	{
		Clear();
		return false;
	}
	return true;
}

void OSGrpCombos::initctr(int *c,int n)
{
	for (int i=0;i<n;++i) c[i]= i;
}

bool OSGrpCombos::nextctr(int *cl,int *cn,int np,int ni)
{
	memcpy(cn,cl,sizeof(int)*np);
	for (int i=np-1;i>=0;--i)
	{
		if (cn[i]>=ni-np+i) continue;		// We're at max in this slot, so try augmenting next slot.
		cn[i]++;
		for (int j=i+1;j<np;++j)
			cn[j]= cn[i]+j-i;
		return true;
	}
	return false;
}

bool OSGrpCombos::OSGRecCmpValDesc(const OSGCRec &a,const OSGCRec &b) { return (a._v>b._v); }

bool OSGrpCombos::OSGRecCmpCostAsc(const OSGCRec &a,const OSGCRec &b) { return (a._c<b._c); }


bool OSGrpCombos::build(bool bycost,int np,int ni,float *v,float *c,const int *n)
{
	_bycost= bycost;
	_np= np;
	if (ni<np) return false;
	_nc= nchoosem(ni,np);		// Total combos for this group
	if (_nc<=0) return false;
	_n= n;
	_ni= ni;

	// Create arrays
	long sz= (_nc+1)*(long)_np;	// The 1 in _nc+1 is a special safety buffer because of the way we generate things
	_i= new int[sz];
	_v= new float[_nc];
	_c= new float[_nc];

	// Populate _i
	initctr(_i,_np);
	long i=-1;
	do
	{
		i= i+1;
	} while(nextctr(&(_i[_np*i]),&(_i[_np*(i+1)]),_np,ni));

	if (i!=_nc-1) return false;	// If =_nc+1 then too many combos, if less than _nc then too few.  Testing this is why we have the extra _nc+1 slot!

	// Populate our temp list
	typedef std::list<OSGCRec> RLIST;
	RLIST r;
	for (long i=0;i<_nc;++i)
	{
		float rv= 0.0;
		for (int j=0;j<_np;++j)
		{
			float vv= v[_n[_i[_np*i+j]]];
			if (IsBadVal(vv)) { rv= BadVal(); break; }
			rv+= vv;
		}
		float rc= 0.0;
		for (int j=0;j<_np;++j)
		{
			float cc= c[_n[_i[_np*i+j]]];
			if (IsBadCost(cc)) { rc= BadCost(); break; }
			rc+= cc;
		}
		r.push_back(OSGCRec(i,rv,rc));
	}
	
	// Sort our temp list
	if (bycost) r.sort(OSGRecCmpCostAsc);
	else r.sort(OSGRecCmpValDesc);

	// Repopulate arrays
	i=0;
	int *itmp= new int [sz];
	for (RLIST::const_iterator ii= r.begin();ii!=r.end();++ii)
	{
		_v[i]= ii->_v;
		_c[i]= ii->_c;
		long n1= ii->_n;
		memcpy(&(itmp[i*_np]),&(_i[n1*_np]),_np*sizeof(int));
		++i;
	}
	delete [] _i;
	_i= itmp;

	return true;
}

//////// OSGrpRec

OSGrpRec::OSGrpRec(void) : _g(-1), _np(0), _bval(BadVal()), _lcost(BadCost()), _rbval(BadVal()), _rlcost(BadCost()), _rcombos(0), _ni(0), _i(NULL), _gc() {}

bool OSGrpRec::Init(const OConfig &x,int g,bool bycost)
{
	if (_g>=0) return false;	// Already set
	if (g<0||g>=x.NumPrimaryGroups()) return false;
	_g= g;
	_np= x.NumPicks(g);
	_ni= x.AccessPrimaryFeature()->NumItemsInGroup(g);
	_i= x.AccessPrimaryFeature()->ItemsInGroup(g);
	if (_ni<=0||_np<=0) return false;
	if (_ni<_np) return false;	// Too few items!!!!

	// Obtain cost of lowest np items
	std::vector<float> fl;
	for (int i=0;i<_ni;++i) fl.push_back(x.Costs()[_i[i]]);
	std::sort(fl.begin(),fl.end());
	_lcost= 0;
	for (int i=0;i<_np;++i)
	{
		if (IsBadCost(fl[i])) return false;
		_lcost+= fl[i];
	}

	// Obtain cost of highest np items
	fl.clear();
	for (int i=0;i<_ni;++i) fl.push_back(x.Vals()[_i[i]]);
	std::sort(fl.begin(),fl.end());
	_bval= 0;
	for (int i=0;i<_np;++i)
	{
		if (IsBadVal(fl[_ni-1-i])) return false;
		_bval+= fl[_ni-i-1];
	}

	// Combo generations and sorting
	if (!_gc.Build(bycost,_np,_ni,x.Vals(),x.Costs(),_i)) return false;

	return true;
}

void OSGrpRec::DumpCombos(FILE *f) const
{
	if (!f) return;
	fprintf(f,"------Combos for group %d with ni=%d, np= %d, nc= %ld-----\n",_g,_ni,_np,_gc.Combos());
	for (int i=0;i<_gc.Combos();++i)
	{
		fprintf(f,"%ld %f %f: [",i,_gc.Val(i),_gc.Cost(i));
		for (int j=0;j<_np;++j)
		{
			int n= _gc.RawItem(i,j);
			fprintf(f,"%d ",n);
		}
		fprintf(f,"] [");
		for (int j=0;j<_np;++j)
		{
			int n= Item(i,j);
			fprintf(f,"%d ",n);
		}
		fprintf(f,"]\n");
	}
}


//////// OSearch

OSearch::OSearch(void) : OMtxCtlBase(), _r(NULL), _rp(NULL), _oc(NULL), _m(NULL), _ng(0), _nc(0), _bycost(false), _cs(0), _pcnt(NULL), _icnt(NULL), _tcol(NULL), _tloc(NULL) {}
OSearch::~OSearch(void) 
{ 
	delete [] _r; 
	delete [] _rp;
	delete [] _pcnt;
	delete [] _tcol;
	delete [] _tloc;
}

bool OSearch::Search(const OConfig &x,bool bycost,bool grouplowtohigh,int debug)
{
	OSrchMtxCtl mtx(this);
	int ng= x.NumPrimaryGroups();
	if (ng<=0) return false;
	_nc= x.NumConstraints();
	if (_nc<0) return false;
	if (_r) return false;	// Already set
	_bycost= bycost;
	_cs= x.CollectionSize();

	// Create ordered list of groups decreasing by number of picks, then number of items
	_r= new OSGrpRec [ng];
	_rp= new OSGrpRec* [ng];
	_ng= ng;
	_oc= &x;
	_m= _oc->AccessMM();
	if (_tloc) delete [] _tloc;
	_tloc= new int [ng];

	// Populate group info
	for (int i=0;i<ng;++i)		
		if (!_r[i].Init(x,i,bycost)) return false;

	// Create sorted list of groups by combos
	typedef std::vector<std::pair<long,OSGrpRec *> > AVEC;
	AVEC av;
	for (int i=0;i<ng;++i) av.push_back(std::pair<long,OSGrpRec *>(_r[i].Combos(),&(_r[i])));
	std::sort(av.begin(),av.end());	// Sort on pairs sorts by 1st element first, so perfect.
	if (!grouplowtohigh) std::reverse(av.begin(),av.end());
	for (int i=0;i<ng;++i) _rp[i]= av[i].second;

	if (debug & 8)
		for (int i=0;i<ng;++i) 
			_rp[i]->DumpCombos(stdout);

	// Accumulate sum info for group records
	long cc= 1;
	float rv= 0;
	int rc= 0;
	for (int i=ng-1;i>0;--i)
	{
		rv+= _rp[i]->_bval;
		rc+= _rp[i]->_lcost;
		cc*= _rp[i]->_gc.Combos();
		_rp[i-1]->_rbval= rv;
		_rp[i-1]->_rlcost= rc;
		_rp[i-1]->_rcombos= cc;
	}
	_rp[ng-1]->_rbval= 0;
	_rp[ng-1]->_rlcost= 0;
	_rp[ng-1]->_rcombos= 1;

	int tl= 0;
	for (int i=0;i<ng;++i)
	{
		_tloc[i]= tl;
		tl+= _rp[i]->_np;
	}

	// Init diagnostic counters
	if (_pcnt) delete [] _pcnt;
	_pcnt= new long [NumCounters()];
	memset(_pcnt,0,sizeof(long)*NumCounters());

	// Init dup tester
	if (_icnt) delete [] _icnt;
	_icnt= new int [_oc->NumItems()];

	// Setup tcol
	if (_tcol) delete [] _tcol;
	_tcol= new int [_cs];

	// Do the work
	search(_oc->CTol(),_oc->MaxCost(),0.0,0,debug);

	// Done
	return true;
}

// Utility function for dumping 
static std::string getstatestr(long i,int g,int ng,OSGrpRec **rp)
{
	char buf[128];
	std::string x= "";
	for (int j=0;j<ng;++j)
	{
		if (j<g)
		{
			if (j>0) x+= ":";
			x+= "(";
			long cnum= rp[j]->_c;
			for (int k=0;k<rp[j]->_np;++k)
			{
				if (k>0) x+= ",";
				int inum= rp[j]->Item(cnum,k);
				sprintf(buf,"%03d",inum);
				x+= buf;
			}
			x+= ")";
		}
		else if (j==g)
		{
			x+= "*[";
			long cnum= i;
			for (int k=0;k<rp[j]->_np;++k)
			{
				if (k>0) x+= ",";
				int inum= rp[j]->Item(cnum,k);
				sprintf(buf,"%03d",inum);
				x+= buf;
			}
			x+= "]*";
		}
		else
		{
			if (j>g+1) x+= ":";
			x+= "(";
			for (int k=0;k<rp[j]->_np;++k)
			{
				if (k>0) x+= ",";
				x+= "---";
			}
			x+= ")";
		}
	}
	return x;
}

#define PRINTSTATE(c,n)		if (debug & 32) printf("%2s [%20ld] %s C:%.1f*%.1f*%.1f V:%.1f*%.1f*%.1f\n",c,(long)(n), getstatestr(i,g,_ng,_rp).c_str(), _oc->MaxCost()-rcost,cc,mrc,val,cv,mrv);



// minval= (max coll val so far)*(1-ctol)
// search takes a position starting at group g and cost c and value v so far.   It then cycles over all choices in group g and beyond. 
void OSearch::search(float ctol,float rcost,float val,int g,int debug)
{
	static long nnn= 0;
	if (debug & 16)
		printf("search: g:%d rcost:%f val:%f ctol:%f\n",g,rcost,val,ctol);

	// Cycle over all combos in group g
	long nc= _rp[g]->Combos();
	OSGrpCombos *rc= &(_rp[g]->_gc);
	float mv= _m->GetMinAllowed();
	long rcombos= _rp[g]->_rcombos;
	for (long i=0;i<nc;++i)		// Cycle over the combos in the requested order (already sorted)
	{
		++nnn;
		_rp[g]->_c= i;			// Set for future use
		float cc= rc->Cost(i);		// The cost of our current group's picks
		float cv= rc->Val(i);		// The value of our current group's picks
		assert(!IsBadCost(cc));
		assert(!IsBadVal(cv));

		float mrc= _rp[g]->_rlcost;	// Min cost of all remaining groups
		float mrv= _rp[g]->_rbval;	// Max value of all remaining groups

		// We now prune by value and cost if possible.  HOWEVER, because we are moving in different ways depending on bycost, the effect (all remaining or just this branch) of pruning is reversed.  We always perform the potentially more aggressive pruning first!
		if (!_bycost)
		{
			// If best value is too low, prune this and ALL remaining choices because we're moving in decreasing order of value so all remaining choices will be worse.  Check this first since most extensive pruning!
			if (!OGlobal::IsBadVal(mv)&&(cv+mrv+val<mv))
			{
				long pruned= (long)(nc-i)*rcombos;
				_pcnt[CntPruned()]+= pruned;
				_pcnt[CntStrict()]+= pruned;
				PRINTSTATE(">C",-pruned)
				break;
			}

			// Prune just this combo if best cost is too high
			if (cc+mrc>rcost+_oc->MaxCostTol())
			{
				long pruned= rcombos;
				_pcnt[CntPruned()]+= pruned;
				_pcnt[CntWeak()]+= pruned;
				PRINTSTATE("=C",-pruned)
				continue;
			}
		}
		else
		{
			// Prune this and all remaining combos if best cost is too high
			if (cc+mrc>rcost+_oc->MaxCostTol())
			{
				long pruned= (long)(nc-i)*rcombos;
				_pcnt[CntPruned()]+= pruned;
				_pcnt[CntStrict()]+= pruned;
				PRINTSTATE("<V",-pruned)
				break;
			}

			// If best value is too low, prune just combo.
			if (!OGlobal::IsBadVal(mv)&&(cv+mrv+val<mv))
			{
				long pruned= rcombos;
				_pcnt[CntPruned()]+= pruned;
				_pcnt[CntWeak()]+= pruned;
				PRINTSTATE("=V",-pruned)
				continue;
			}
		}

		//// It seems we don't need to prune.  Should we delegate to next level?

		// Copy current combo into tcol
		for (int k=0;k<_rp[g]->_np;++k)
		{
			int inum= _rp[g]->Item(i,k);
			_tcol[_tloc[g]+k]= inum;
		}

		if (g<_ng-1)
		{
			search(ctol,rcost-cc,val+cv,g+1,debug);
			continue;
		}

		///// Apparently we're in the last group.  Well, let's test the collection.
		_pcnt[CntAnal()]++;

		// Accumulate the collection value, cost --- and dump info if requested
		float tv= 0;
		float tc= 0;
		for (int j=0;j<_ng;++j)
		{
			long cnum= _rp[j]->_c;
			tv+= _rp[j]->_gc.Val(cnum);
			cv+= _rp[j]->_gc.Cost(cnum);
		}

		// First, let's do the easy test against the memory manager
		if (!_m->CanAdd(tv))
		{
			_pcnt[CntPruned()]++;
			_pcnt[CntCantAdd()]++;
			PRINTSTATE("NV",-1)

			continue;
		}

		// Test for duplicate items, while populating dummy collection and getting cost/value
		memset(_icnt,0,sizeof(int)*_oc->NumItems());
		bool hasdup= false;
		int ncc= 0;
		for (int j=0;j<_ng&&!hasdup;++j)
		{
			long cnum= _rp[j]->_c;
			for (int k=0;k<_rp[j]->_np;++k)
			{
				int inum= _rp[j]->Item(cnum,k);
				if (_icnt[inum]>0) { hasdup= true; break; }
				_tcol[ncc]= inum;
				_icnt[inum]++;
				++ncc;
			}
			if (hasdup) break;
		}
		if (hasdup)
		{
			_pcnt[CntPruned()]++;
			_pcnt[CntDup()]++;
			PRINTSTATE("DP",-1)
			continue;
		}
		assert(ncc==_cs);

		// Test against constraints
		int cviol= _oc->TestConstraints(_tcol);
		if (cviol>=0)
		{
			_pcnt[NumIntCnts()+cviol]++;
			_pcnt[CntPruned()]++;
			_pcnt[CntConstrain()]++;
			char buf[128];
			sprintf(buf,"C%1d",cviol);
			PRINTSTATE(buf,-1)
			continue;
		}

		// Now we have a valid collection
		if (!_m->Add(((debug & 64)!=0),_tcol,tv)) 
		{
			PRINTSTATE("ER",-1)
			printf("ERROR: FAILED TO ADD ENTRY %ld\n",nnn);
		}
		else
		{
			_pcnt[CntAdded()]++;
			mv= _m->GetMinAllowed();	// Min val for a collection allowed at this point.  Update in case needed
			PRINTSTATE("++",1)
		}
	}
}

std::string OSearch::NameOfCnt(int n)
{
	if (n==0) return "Added";
	else if (n==1) return "Analyzed";
	else if (n==2) return "PrunedTotal";
	else if (n==3) return "PrunedStrict";
	else if (n==4) return "PrunedWeak";
	else if (n==5) return "PrunedCantAdd";
	else if (n==6) return "PrunedDup";
	else if (n==7) return "PrunedTotConst";
	else
	{
		char buf[40];
		sprintf(buf,"PrunedConst%d",n-(NumIntCnts()-1));
		return buf;
	}
}
