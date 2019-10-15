#include <string.h>
#include <stdlib.h>
#include "OCFN.h"
#include "OConfig.h"
#include "OFeature.h"

///////// OCFN

OCFN *OCFN::GetFromIntrinsicType(const OConfig *src,int t, int l, int *vl)
{
	OCFN *f= NULL;
	if (!src) return f;
	if (t==OCFNMinGroups::SType())
	{
		if (!vl||l!=2) return NULL;
		f= new OCFNMinGroups(src,vl[0],vl[1]);
	}
	else if (t==OCFNMaxItems::SType())
	{
		if (!vl||l!=2) return NULL;
		f= new OCFNMaxItems(src,vl[0],vl[1]);
	}
	return f;
}

bool OCFN::Init(void)
{
	OCFNMtxCtl mtx(this);
	return this->init();
}

bool OCFN::IsValid(void) const
{
	OCFNMtxCtl mtx(this);
	return this->isvalid();
}

std::string OCFN::Desc(void) const
{
	OCFNMtxCtl mtx(this);
	return this->desc();
}

void OCFN::Reset(void)
{
	OCFNMtxCtl mtx(this);
	return this->reset();
}

///////// OCFNGrpCntBase

OCFNGrpCntBase::OCFNGrpCntBase(const OConfig *src,int fnum) : OCFN(src), _clen(0), _ni(0), _fn(fnum), _ng(0), _l(NULL) {}

bool OCFNGrpCntBase::init(void)
{
	if (!_src) return false;
	if (_l) return false;	// Already init'ed!
	_clen= _src->CollectionSize();
	if (_clen<=0) return false;
	_ni= _src->NumItems();
	if (_ni<=0) return false;
	const OFeature *f= _src->AccessFeature(_fn);
	if (!f) return false;
	_ng= f->NumGroups();
	if (!f->IsPartition()) return false;	// We require a partition for this type 
	_l= new int [_ni];
	for (int i=0;i<_ni;++i) _l[i]= -1;
	for (int i=0;i<f->NumGroups();++i)
	{
		int n= f->NumItemsInGroup(i);
		const int *ip= f->ItemsInGroup(i);
		if (n>0&&!ip) return false;
		for (int j=0;j<n;++j)
		{
			int z= ip[j];
			if (z<0||z>=_ni) return false;
			_l[z]= i;
		}
	}
	for (int i=0;i<_ni;++i)
		if (_l[i]<0||_l[i]>=_ng)
			return false;
	
	return true;
}

bool OCFNGrpCntBase::isvalid(void) const
{
	if (!_src) return false;
	if (_clen<=0) return false;
	if (_ni<=0) return false;
	if (_ng<=0) return false;
	if (_fn<0||_fn>=_src->NumFeatures()) return false;
	if (!_l) return false;
	for (int i=0;i<_ni;++i)
		if (_l[i]<0||_l[i]>=_ng)
			return false;
	return true;	
}

std::string OCFNGrpCntBase::desc(void) const
{
	char buf[128];
	sprintf(buf,"type=%d clen=%d ni=%d fn=%d ng=%d",this->Type(),_clen,_ni,_fn,_ng);
	return std::string(buf);
}

void OCFNGrpCntBase::reset(void)
{
	if (_l) delete [] _l;
	_l= NULL;
	_clen= 0;
	_ni= 0;
	_ng= 0;
}

/////////  OFCNMinGroups

OCFNMinGroups::OCFNMinGroups(const OConfig *src,int fnum,int mcnt) : OCFNGrpCntBase(src,fnum), _cnt(mcnt) {}

bool OCFNMinGroups::init(void)
{
	if (!this->OCFNGrpCntBase::init()) return false;
	if (_cnt<=0) return false;	// If 0 not a constraint!
	return true;
}

bool OCFNMinGroups::isvalid(void) const
{
	if (!this->OCFNGrpCntBase::isvalid()) return false;
	if (_cnt<=0) return false;	// If 0 not a constraint!
	return true;
}

bool OCFNMinGroups::test(const int *c) const
{
	typedef std::set<int> GSET;
	GSET _g;
	if (!c) return false;
	if (!_l) return false;
	for (int i=0;i<_clen;++i)
	{
		if (c[i]<0||c[i]>=_ni) return false;
		_g.insert(_l[c[i]]);
//		printf("XXX %d %d %d %d %d\n",i,c[i],_l[c[i]],_g.size(),_cnt);
		if (_g.size()>=_cnt) return true;
	}
	return false;
}

std::string OCFNMinGroups::desc(void) const
{
	char buf[128];
	sprintf(buf," cnt=%d",_cnt);
	return this->OCFNGrpCntBase::desc()+std::string(buf);

}

void OCFNMinGroups::reset(void)
{
	this->OCFNGrpCntBase::reset();
}

//////////  OFCNMaxItems

OCFNMaxItems::OCFNMaxItems(const OConfig *src,int fnum,int mcnt) : OCFNGrpCntBase(src,fnum), _cnt(mcnt), _x(NULL) {}

bool OCFNMaxItems::init(void)
{
	if (!this->OCFNGrpCntBase::init()) return false;
	if (_cnt<=0) return false;	// If 0 never will pass!
	if (_ng<=0) return false;
	if (_x) return false;
	_x= new int [_ng];
	return true;
}

bool OCFNMaxItems::isvalid(void) const
{
	if (!this->OCFNGrpCntBase::isvalid()) return false;
	if (_cnt<=0) return false;	// If 0 never will pass!
	if (!_x) return false;
	return true;
}

bool OCFNMaxItems::test(const int *c) const
{
	if (!c) return false;
	if (!_l) return false;
	memset(_x,0,_ng*sizeof(int));
	for (int i=0;i<_clen;++i)
	{
		if (c[i]<0||c[i]>=_ni) return false;
		_x[_l[c[i]]]++;
		if (_x[_l[c[i]]]>_cnt) return false;
	}
	return true;
}

std::string OCFNMaxItems::desc(void) const
{
	char buf[128];
	sprintf(buf," cnt=%d",_cnt);
	return this->OCFNGrpCntBase::desc()+std::string(buf);
}

void OCFNMaxItems::reset(void)
{
	this->OCFNGrpCntBase::reset();
	if (_x) delete [] _x;
	_x= NULL;
}
