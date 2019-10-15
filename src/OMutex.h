#ifndef OMUTEXDEFFLAG
#define OMUTEXDEFFLAG

#include <pthread.h>

//! Simple Mutex wrapper
class OMutex
{
private:
	OMutex(const OMutex &x) {}	// Non-copyable
protected:
	pthread_mutex_t _m;
public:
	OMutex(void) : _m() { pthread_mutex_init(&_m,NULL); }
	~OMutex(void) { pthread_mutex_destroy(&_m); }
	int Lock(void) { return pthread_mutex_lock(&_m); }
	int TryLock(void) { return pthread_mutex_trylock(&_m); }
	int UnLock(void) { return pthread_mutex_unlock(&_m); }
	int Lock(bool blocking) { return blocking?Lock():TryLock(); }
	pthread_mutex_t *AccessMtx(void) { return &_m; }	//!< For use with condition variable blocking, etc
};

//! Derive from this to use our mutex control RIIA paradigm.  This works with OMtxCtl<> (not the non-template form)
class OMtxCtlBase
{
protected:
	mutable OMutex _mtx;
public:
	OMtxCtlBase(void) : _mtx() {}
	OMtxCtlBase(const OMtxCtlBase &x) : _mtx() {}	// We don't copy the mutex
	OMutex &DirectAccessMtx(void) { return _mtx; }	
};

//! If lots of our member fns of our class FOO would need an internal version due to mtx wrapping, then add "friend class OMtxCtl<FOO>;" to the class and just add a local member "OMTxCtl<FOO> at the start of each mutex-protected function.  Then return or generate exceptions as normal.  This is RIIA.  Note that the mutex must be mutable.   Raning:  when declaring the friend, you have to use the full templatized name -- NOT any typedef'ed nickname -- for some reason the compliler complains otherwise.
template<class FOO>
class OMtxCtl
{
private:
	OMtxCtl(OMtxCtl &x) {}
protected:
	const FOO *_foo;
public:
	OMtxCtl(const FOO *z)
	{
		_foo= z;
		if (_foo) _foo->_mtx.Lock();
	}
	void UnLock(void)	// Explicitly unlock -- which we only can do once.  This is for manual control when scope-control is inconvenient.
	{
		if (_foo) _foo->_mtx.UnLock();
		_foo= NULL;
	}
	~OMtxCtl(void) { UnLock(); }
	pthread_mutex_t *AccessMtx(void) { return _foo?(_foo->_mtx.AccessMtx()):NULL; }
};

#endif
