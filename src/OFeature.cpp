#include "OFeature.h"

OFeature::~OFeature(void)
{
	delitems();
	delete [] _ftable;
}

bool OFeature::Configure(int ng,int ni,bool ispart,bool *ft)
{
	OFeatMtxCtl mtx(this);
	if (ng<=0||ni<=0||!ft) return false;
	if (_ftable) return false;	// already set
	_ng= ng;
	_ni= ni;
	_ispart= ispart;
	_ftable= ft;	// We take ownership because synthesized on C-side of things!
	scangroups();
	return true;
}

void OFeature::delitems(void)
{
	if (_items)
	{
		for (int i=0;i<_ng;++i) delete [] _items[i];
		delete [] _items;
		delete [] _nitems;
	}
}

// Creates the list of items in each group, clearing the old list if one already exists.
void OFeature::scangroups(void)
{
	delitems();
	_nitems= new int[_ng];
	_items= new int*[_ng];
	for (int j=0;j<_ng;++j)
	{
		int n= 0;
		for (int i=0;i<_ni;++i)
			if (getitem(i,j)) ++n;
		_nitems[j]= n;
		_items[j]= (n>0)?(new int[n]):NULL;
		n= 0;
		for (int i=0;i<_ni;++i)
		{
			if (getitem(i,j))
			{
				_items[j][n]= i;
				++n;
			}
		}
	}
}

void OFeature::PrepToExclude(int i,int j) 
{ 
	OFeatMtxCtl mtx(this);
	_texc.push_back(std::pair<int,int>(i,j));
}

// After we've designated a bunch of items to exclude (via PrepToExclude()) from consideration in certain groups, we process these exclusions.
void OFeature::ProcessExclusions(void)
{
	OFeatMtxCtl mtx(this);
	for (ELIST::const_iterator ii= _texc.begin();ii!=_texc.end();++ii)
	{
		int i= ii->first;
		int j= ii->second;
		if (j>=0) _ftable[_ng*i+j]= false;
		else
		{
			for (j=0;j<_ng;++j)
				_ftable[_ng*i+j]= false;
		}
	}
	_texc.clear();
	scangroups();	// Must recalc
}

bool OFeature::IsSensible(void) const
{
	OFeatMtxCtl mtx(this);
	if (_ng<=0) return false;
	if (_ni<=0) return false;
	if (!_ftable) return false;
	if (!_ispart) return true;	// Nothing more to test

	// Test if a proper partition (each item has one and only one group)
	for (int i=0;i<_ni;++i)
	{
		int n= 0;
		for (int j=0;j<_ng;++j)
			if (getitem(i,j)) n+=1;
		if (n!=1) return false;
	}
	// NOTE: Doesn't test calc'ed objects
	return true;
}

void OFeature::Dump(FILE *f) const
{
	OFeatMtxCtl mtx(this);
	fprintf(f,"NG: %d\n",_ng);
	fprintf(f,"NI: %d\n",_ni);
	for (int i=0;i<_ni;++i)
	{
		fprintf(f,"I%5d ",i);
		for (int j=0;j<_ng;++j)
			fprintf(f,"%c",getitem(i,j)?'1':'.');
		fprintf(f,"\n");
	}
	for (int i=0;i<_ng;++i)
	{
		fprintf(f,"G%2d ",i);
		fprintf(f,"%3d ",_nitems[i]);
		for (int j=0;j<_nitems[i];++j)
			fprintf(f,"%3d ",_items[i][j]);
		fprintf(f,"\n");
	}
}
