/* rand.h
 * 乱数生成器
 * init_gen_rand()を呼んでから使用すること
 */

#include <climits>

#ifdef _MSC_VER
#define M_PI		3.1415926535897932384626433832795
#endif
#include <cmath>

extern "C"{
#include "SFMT/SFMT.h"
}

// 正規乱数
class RandNormal{
	double x;
	double y;
	bool   b;
public:
	RandNormal(){
		b = true;
	}
	double Generate( double ave, double dev ){
		if( dev == 0.0 ){ return ave; }
		if( b ){
			x = 1.0 - gen_rand32() / (double)UINT_MAX; // rand (0,1]
			x = sqrt( -2.0 * log( x ) );
			y = 1.0 - gen_rand32() / (double)UINT_MAX; // rand (0,1]
			b = false;                                 // flag
			return x * sin( 2.0 * M_PI * y ) * dev + ave;
		}
		else{
			b = true;                                  // flag
			return x * cos( 2.0 * M_PI * y ) * dev + ave;
		}
	}
};
