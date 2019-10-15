#ifndef OGLOBALDEFFLAG
#define OGLOBALDEFFLAG

#include <math.h>

// Just useful static values
class OGlobal
{
public:
	static float BadCost(void) { return -999999; }
	static float BadVal(void) { return -999999; }
	static bool IsBadCost(float c) { return (fabs(c-BadCost())<0.01); }
	static bool IsBadVal(float v) { return (fabs(v-BadVal())<0.01); }	
};


#endif

