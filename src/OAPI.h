#ifndef OAPIDEFFLAG
#define OAPIDEFFLAG

/* Threadsafe routines.  These are the thread-safe interiors of the exact ones called from python.  They can be used in C++ in a threadsafe manner. 

To use them, just instantiate an OConfig object, and then pass it to the routines --- calling them just as from the python API. 

*/

#include "OConfig.h"

/* 

Initialize the overall structure and global spec
	nf= number of features
	pf= primary feature number [0,nf)
	pfn= array listing number of items to be chosen for each group of the primary feature (len= number of groups in primary feature).
	pfnn= length of pfn array (i.e. number of groups in primary feature)
	ni= number of items (total pool of items available for selection)
	mc= maximum cost for a collection
	nc= number of constraints

NOTE: this must be done AFTER not before kopt_init_parms_ts!!!!!
*/
void kopt_init_struct_ts(OConfig &ac,int nf,int pf,int *pfn,int pfnn,int ni,float mc,int nc);

/*

Initialize the relevant global search parameters

	ctol: Collection tolerance. We discard any collections with value < ctol% below best so far.
		0= keep best only
		1= keep all
	itol: Item tolerance. We won't cull unless there are suitable replacements itol% higher in value and with lower or equal cost
		0= cull more
		higher= cull less (can go above 1)
	ntol: Number of extra items to require in each primary feature group for individual cull.
		0= ordinary cull
		1+= cull less
	resnumb: Block allocation size.  Generally this doesn't affect the user much unless very large numbers of collections are being analyzed and discarded.  
	maxres: Maximum number of collections to allow.  If we get more, then a garbage collection is triggered. 
	smode: Search order.  There are 2 search decisions specified here.  The order of primary feature groups scanned by combinatoric number of selections possible can be low to high or high to low, and the order of selections in each group can be increasing by cost or decreasing by value.  
		1=  Fewest-to-most combinations / Decreasing Value
		2=  Most-to-fewest combinations / Decreasing Value
		3=  Fewest-to-most combinations / Increasing Cost
		4=  Most-to-fewest combinations / Increasing Cost

	The parameters may be grouped as:
		ntol, itol	govern initial individual cull
		ctol, maxres (and somewhat resnumb)	govern number of collections kept and how the list is managed
		smode		governs search order

	Generally, ntol=0, itol= low (ex. 0.1) is good unless there is a flex group.  In that case, increase to ntol=1 and/or itol slightly higher.
	 
*/
void kopt_init_parms_ts(OConfig &ac,float ctol,float itol,int ntol,int resnumb,long maxres,int smode);

/* 

Initialize a feature
	fn= feature number [0,nf) (with nf from the init_struct command)
	ng= number of groups in feature
	ni= number of items (we already know this, but useful to put here as well for verification)
	ispart= is the feature a partition?  I.e. one and only one group for every item.  We can autodetect partitions, but ask for this to allow error checking of whether what the user expects and what we've recevied are the same!  I.e. for verification.
	f= 2d unsigned int array rows= ni, cols= ng  (with ni from init_struct command)
*/
void kopt_init_feature_ts(OConfig &ac,int fn,int ng,int ni,int ispart,int **f);

/*

Initialize the item info arrays
	c= float array (length ni from init_struct command) of item costs
	v= float array (length ni from init_struct command) of mean pred item values

*/
void kopt_init_items_ts(OConfig &ac,float *c,float *v);

/*

Add a constraint functions. 
	cn= constraint number (0..nc-1) where nc was set in kopt_init_struct
	t= constraint type (presently 0 or 1)
	al= length of (int) argument array
	a= int argument array of length al

	In addition to any user-defined types, there are 2 built in ones.  They both operate using a single feature which MUST be a partition!

	1. Require items in at least n groups from feature m
		t= 0, al= 2, l= [m,n] 
	2. Prohibit more than n items from the same group of feature m
		t= 1, al= 2, l= [m,n]

	Note that the same constraint type can be added multiple times (obviously with different parameters)
*/
void kopt_set_constraint_ts(OConfig &ac,int cn,int t,int al,int *a);

/*

Set the cost tolerance.  This is a parameter which is used to deal with floating point issues when summing costs.  If the costs are integers, we don't want rounding errors to mark us as above cost, so we demand that the cost be <=maxcost+maxcosttol.  The default is 0.01.  For applications where costs truly are floats, it may be necessary to change this (it can be set to anything>=0).   */
void kopt_set_maxcosttol_ts(OConfig &ac,float x);

/*

Post-initialize preparation for calculation.  
This must be called prior to any searches.
Verifies we have everything and generates various info that will be needed for the calculation.  Returns 0 on failure.

*/
int kopt_lock_and_load_ts(OConfig &ac);

/* 

Utility function to estimate the full state space size (if no culling or pruning)

*/
double kopt_get_log_state_space_est_ts(OConfig &ac);

/* 

Execute the search.  Returns 0 on failure

debug is the level of verbosity.  It is an or'ed list of flags:

1:	Dump info to tie out with calling script (parms, etc)
2:	Dump pre/post execution stats
4:	Dump item list/feature tables and group info (pre-cull and post-cull)
8:	Dump detailed sorted combos for all primary groups
16:	Search algo per-call info
32:	Search algo per-combo info
64:	Memory Manager addition info
128:	Search duper-detailed
	
If displaystats>0, then will dump various pruning info along the way.

*/
int kopt_execute_ts(OConfig &ac,int debug);

/* 

Prep to begin obtaining results and get the number of collections the search kept.  
This should be called before querying the collections themselves.

*/
int kopt_prepres_ts(OConfig &ac);

/* 

Utility function to obtain the collection length.  Obviously, this already is known to the user (it's the sum of the pf array values
set in kopt_init_struct), but it's useful to be able to query it directly in order to allocate the right around of memory for the 
collection results.  

*/
int kopt_colllen_ts(OConfig &ac);

/* 

Read the results.  This can be done in stages if memory allocation is an issue.  

NOTE: the arrays passed in must be of the appropriate length to hold n results.  I.e., have python call kopt_prepres and then allocate arrays of the appropriate length.   It also may iterate since we allow staged retrieval.  In that case, the array must be the size of the stage.  
Sequential calls will get sequential portions of n results (from top value down), unless kopt_prepres_ts is called --- which resets to the beginning.
	n= length of arrays being passed by user
	r= Allocated 2d array of unsigned ints.  Must be of size n (rows) x cl (cols), where cl is the result of kopt_colllen().  This array will be populated with collections.  The i,j entry will correspond to the item number in slot j of collection istart+i   I.e. r contains the collection composition info.
	m= Allocated 1d array of floats.  Of length n.  This array will be populated with the mean value for each collection (i.e. the sum of item values).  This is the number we care about for most purposes.

	The return value is the number of collections populated.  In general this will be n, but if we would end up moving past the end, only the allowed number of rows are filled (the rest are untouched), and we return the actual number filled.  -1 on error.  0 if simply none left.
*/
int kopt_getres_ts(OConfig &ac,int n,unsigned int **r,float *m);

/* 

Deallocate and release all memory used in the calculation.  After this, the results will be unavailable for future use.  HOWEVER, will not affect any of the arrays passed via kopt_getres calls, only the internal storage used on the C/C++ end.

This should be used when we will perform multiple searches using the same structure (constraints/features), or when cleaning up in the end.

*/

void kopt_release_ts(OConfig &ac);

/*

Reset the entire object.  Because we have one global instance as far as the python interface is concerned, we need to reset it if performing multiple searches with different constraints/features/etc.

*/
void kopt_reset_ts(OConfig &ac);

#endif
