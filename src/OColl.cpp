#include <string.h>
#include <assert.h>
#include "OColl.h"

/////////// OCollMM

OCollMM::OCollMM(int clen,int bsize,long maxrec,float ctol) : OMtxCtlBase(), _s(), _c(), _cii(_c.end()), _rsize(0), _bsize(bsize), _clen(clen), _maxrec(maxrec), _ctol(ctol), _nreqs(0), _ncurr(0), _maxval(BadVal()), _minval(BadVal())
{
	_rsize= _clen*sizeof(int16_t) + sizeof(float);
}

OCollMM::~OCollMM(void)
{
	for (BSET::iterator ii= _s.begin();ii!=_s.end();++ii)
		delete [] *ii;
}

void OCollMM::addblock(void)
{
	// Allocate new block
	char *b= new char [_bsize*_rsize];

	// 0 out the new block except for the val which we set to bad (so at end of sort)
	memset(b,0,_bsize*_rsize);
	for (int i=0;i<_bsize;++i)
		Unset(b+i*_rsize);

	// Add to Set of blocks
	_s.insert(b);

	// Add all empty records to Set of recs
	for (int i=0;i<_bsize;++i) _c.insert(b+i*_rsize);
}

bool OCollMM::CanAdd(float v) const
{
	if (IsBadVal(v)) return false;			// Bad value can't be added
	if (!IsBadVal(_maxval)&&v<_maxval*(1.0-_ctol)) return false;		// Falls below the threshold relative to max value so far
	if (IsFull()&&!IsBadVal(_minval)&&v<=_minval) return false;		// Already full and can't displace a lower entry
	return true;
}

char *OCollMM::droplowest(bool isbad)
{
	// Drop the entry
	CSET::iterator ii= _c.end();
	--ii;
	char *o= *ii;
	_c.erase(ii);
	if (isbad) return o;	// We know the entry is a blank

	--_ncurr;
	// Update extrema if needed
	if (_c.empty()) _minval= _maxval= BadVal();
	else
	{
		ii= _c.end();
		--ii;
		_minval= GetVal(*ii);
	}
	return o;
}

// This does a very quick test whether necessary. 
void OCollMM::gc(void)
{
	float mv= GetMinAllowed();
	if (IsBadVal(_minval)||_minval>=mv) return;
	bool cut= false;
	float newmin=BadVal();
	for (CSET::reverse_iterator rii= _c.rbegin(); rii!= _c.rend(); ++rii)
	{
		float v= GetVal(*rii);
		if (IsBadVal(v)) continue;
		if (v>=mv)
		{
			newmin= v;
			break;
		}
		Unset(*rii);
		--_ncurr;
		cut= true;
	}
	if (cut) _minval= newmin;
}

bool OCollMM::Add(bool verbose,int *c,float v)
{
	OCMMMtxCtl mtx(this);
	++_nreqs;

	if (!c) return false;
	if (!CanAdd(v)) return false;

	// Obtain a record
	char *o= NULL;
	if (IsFull()) 
	{
		if (verbose) printf("CollMM: Performing GC\n");
		gc();
	}
	if (IsFull())	// If still is full after gc
	{
		o= droplowest(false);  // If at legal limit, drop the lowest entry and use that.  
	}
	else 
	{
		if (GetNumRec()==GetNumRecAlloc()) addblock();	// If current blocks full, add one
		o= droplowest(true); // No reason not to just pull the last (now known to be unset) record since it will be sorted back in
	}
	assert(o);
	if (!o) return false;

	// Populate the record
	for (int j=0;j<_clen;++j)
		SetItem(o,j,c[j]);
	SetVal(o,v);

	// Reinsert back into set (so it's in the correct order)
	_c.insert(o);

	// Update parms
	++_ncurr;
	if (IsBadVal(_maxval)||v>_maxval) _maxval= v;
	if (IsBadVal(_minval)||v<_minval) _minval= v;

	// Done
	if (verbose) 
		printf("CollMM: Added Rec.  New State: %s\n",getstatstr().c_str());
	return true;
}


int OCollMM::GetRes(int n,unsigned int **r,float *m) const
{
	OCMMMtxCtl mtx(this);
	if (n<=0) return -1;
	if (!r||!m) return -1;
	int i=0;
	for (;i<n&&_cii!=_c.end();++i)
	{
		char *c= *_cii;
		if (IsUnset(c)) break;
		for (int j=0;j<_clen;++j) 
			r[i][j]= GetItem(c,j);
		m[i]= GetVal(c);
		++_cii;
	}
	return i;
}

std::string OCollMM::GetStatStr(void) const
{
	OCMMMtxCtl mtx(this);
	return getstatstr();
}

std::string OCollMM::getstatstr(void) const
{
	char buf[256];
	sprintf(buf,"Nreqs:%ld NCurr:%ld Alloc:%ld MinVal:%f MaxVal:%f",GetNumReqs(),GetNumRec(),GetNumRecAlloc(),GetMinVal(),GetMaxVal());
	return buf;
}

std::string OCollMM::GetStr(const char *o,bool justints) const
{
	char buf[50];
	std::string x= "";
	if (!justints)
	{
		sprintf(buf,"%f ",GetVal(o));
		x+= buf;
	}
	for (int i=0;i<_clen;++i)
	{
		if (i>0) x+= " ";
		sprintf(buf,"%d",GetItem(o,i));
		x+= buf;
	}
	return x;
}

