#include "randomdata.h"

float float_rand( float min, float max )
{
    
    float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
    return min + scale * ( max - min );      /* [min, max] */
}


uint32_t getTime()
{
	time_t curtime;
	time(&curtime);
	return curtime;
}