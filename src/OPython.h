#ifndef OAPIPYTHONDEFFLAG
#define OAPIPYTHONDEFFLAG

/* The optimizer API for python 

These routines just wrap OAPI_ts.h, but with a static (global) instance of OConfig so python can call them. As such they are NOT threadsafe.  But they don't need to be because they are only meant to be called from python.  

See OAPI.h for API details.

*/

extern "C" void kopt_init_struct(int nf,int pf,int *pfn,int pfnn,int ni,float mc,int nc);
extern "C" void kopt_init_parms(float ctol,float itol,int ntol,int resnumb,long maxres,int smode);
extern "C" void kopt_init_feature(int fn,int ng,int ni,int ispart,int **f);
extern "C" void kopt_init_items(float *c,float *v);
extern "C" void kopt_set_constraint(int cn,int t,int al,int *a);
extern "C" void kopt_set_maxcosttol(float x);
extern "C" int kopt_lock_and_load(void);
extern "C" double kopt_get_log_state_space_est(void);
extern "C" int kopt_execute(int debug);
extern "C" int kopt_prepres(void);
extern "C" int kopt_colllen(void);
extern "C" int kopt_getres(int n,unsigned int **r,float *m);
extern "C" void kopt_release(void);
extern "C" void kopt_reset(void);

#endif
