#include "OPython.h"
#include "OConfig.h"
#include "OAPI.h"

static OConfig &AC(void)
{
	static OConfig foo;
	return foo;
}

void kopt_init_struct(int nf,int pf,int *pfn,int pfnn,int ni,float mc,int nc)
{
	kopt_init_struct_ts(AC(),nf,pf,pfn,pfnn,ni,mc,nc);
}

void kopt_init_parms(float ctol,float itol,int ntol,int resnumb,long maxres,int smode)
{
	kopt_init_parms_ts(AC(),ctol,itol,ntol,resnumb,maxres,smode);
}

void kopt_init_feature(int fn,int ng,int ni,int ispart,int **f)
{
	kopt_init_feature_ts(AC(),fn,ng,ni,ispart,f);
}

void kopt_init_items(float *c,float *v)
{
	kopt_init_items_ts(AC(),c,v);
}

void kopt_set_constraint(int cn,int t,int al,int *a)
{
	kopt_set_constraint_ts(AC(),cn,t,al,a);
}

void kopt_set_maxcosttol(float x)
{
	kopt_set_maxcosttol_ts(AC(),x);
}

int kopt_lock_and_load(void)
{
	return kopt_lock_and_load_ts(AC());
}

double kopt_get_log_state_space_est(void)
{
	return kopt_get_log_state_space_est_ts(AC());
}

int kopt_execute(int debug)
{
	return kopt_execute_ts(AC(),debug);
}

int kopt_colllen(void)
{
	return kopt_colllen_ts(AC());
}

int kopt_prepres(void)
{
	return kopt_prepres_ts(AC());
}

int kopt_getres(int n,unsigned int **r,float *m)
{
	return kopt_getres_ts(AC(),n,r,m);
}

void kopt_release(void)
{
	kopt_release_ts(AC());
}

void kopt_reset(void)
{
	kopt_reset_ts(AC());
}

