#include "OAPI.h"
#include "OConfig.h"
#include "OFeature.h"
#include "OCFN.h"
#include "OSearch.h"
#include "OColl.h"

void kopt_init_struct_ts(OConfig &ac,int nf,int pf,int *pfn,int pfnn,int ni,float mc,int nc)
{
	ac.Init(nf,pf,pfn,pfnn,ni,mc,nc);
}

void kopt_init_parms_ts(OConfig &ac,float ctol,float itol,int ntol,int resnumb,long maxres,int smode)
{
	ac.InitParms(ctol,itol,ntol,resnumb,maxres,smode);
}

void kopt_init_feature_ts(OConfig &ac,int fn,int ng,int ni,int ispart,int **f)
{
	if (ng<=0||ni<=0) return;
	bool *fb= new bool [ng*ni];
	for (int i=0;i<ni;++i)
		for (int j=0;j<ng;++j)
			fb[ng*i+j]= (f[i][j]>0);
	ac.SetFeature(fn,ng,(ispart>0),fb);
}

void kopt_init_items_ts(OConfig &ac,float *c,float *v)
{
	ac.InitItems(c,v);
}

void kopt_set_constraint_ts(OConfig &ac,int cn,int t,int al,int *a)
{
	OCFN *f= OCFN::GetFromIntrinsicType(&ac,t,al,a);
	if (!f) return;
	ac.SetConstraint(cn,f);
}

void kopt_set_maxcosttol_ts(OConfig &ac,float x)
{
	ac.SetMaxCostTol(x);
}

int kopt_lock_and_load_ts(OConfig &ac)
{
	if (!ac.IsSensible(false)) return 0;
	ac.InitConstraints();
	if (!ac.IsSensible(true)) return 0;
	return 1;
}

double kopt_get_log_state_space_est_ts(OConfig &ac)
{
	return ac.EstFullStateSpace();
}

int kopt_execute_ts(OConfig &ac,int debug)
{
	if (debug & 1) ac.DumpConfig(stdout);
	if (debug & 2) printf("Pre-cull state space est log_10(size): %lf\n",ac.EstFullStateSpace());
	if (debug & 4) 
	{
		printf("-------Pre-Cull Feature Tables------------\n");
		ac.DumpFeatures(stdout);
		ac.DumpItems(stdout);
	}
	ac.CullByTol();
	if (debug & 4) 
	{
		printf("-------Post-Cull Feature Tables------------\n");
		ac.DumpFeatures(stdout);
	}
	ac.InitConstraints();	// Pull in cull'ed features
	if (debug & 2) printf("Post-cull state space est log_10(size): %lf\n",ac.EstFullStateSpace());
	OSearch s;
	if (!s.Search(ac,ac.IsSearchByCost(),ac.IsGroupLowToHigh(),debug))
	{
		printf("ERROR: OSearch Search failed\n");
		return 0;
	}
	if (debug & 2)
	{
		for (int i=0;i<s.NumCounters();++i)
			printf("%s : %ld\n",s.NameOfCnt(i).c_str(),s.ReadCounter(i));
	}
	return 1;
}

int kopt_colllen_ts(OConfig &ac)
{
	return ac.CollectionSize();
}

int kopt_prepres_ts(OConfig &ac)
{
	ac.AccessMM()->InitResIter();
	return ac.AccessMM()->GetNumRec();
}

int kopt_getres_ts(OConfig &ac,int n,unsigned int **r,float *m)
{
	return ac.AccessMM()->GetRes(n,r,m);
}

void kopt_release_ts(OConfig &ac)
{
	ac.Clear();
}

void kopt_reset_ts(OConfig &ac)
{
	ac.ResetResults();
}
