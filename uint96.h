/* uint96.h
 * R.Kubo 2010-2012
 * 96bit符号無し整数uint96クラス
 */

#ifndef _UINT96_
#define _UINT96_

#ifdef _MSC_VER
#	ifndef WIN32
#		define WIN32			1
#	endif
#	ifndef VC6
#		define VC6				1
#	endif
#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) ) && !defined(__MINGW32__)
#	ifndef POSIX
#		define POSIX			1
#	endif
#else
#	define UNKNOWN_API			1
#endif

#ifdef WIN32
#	include <windows.h>
#endif

#include <iostream>

// 64bit整数
#if defined(WIN32) && !defined(__MINGW32__)
typedef unsigned __int64 uint64;
typedef __int64 int64;
#define ULLONG_MAX		_UI64_MAX
#define LLONG_MAX		_I64_MAX
#else
typedef unsigned long long uint64;
typedef long long int64;
#endif

#if defined(UNKNOWN_API) || defined(VC6)
extern const unsigned int _bfirst[];
extern const unsigned int _blast[];
#endif // UNKNOWN_API

#ifdef _MSC_VER
#	define U64(a)					( (uint64)a )
#else
#	define U64(a)					( a ## ULL )
#endif

// bit counter
inline int countBits( uint64 x ){
	x = x - ( ( x >> 1 ) & U64(0x5555555555555555) );
	x = ( x & U64(0x3333333333333333) ) + ( ( x >> 2) & U64(0x3333333333333333) );
	x = ( x + ( x >> 4 ) ) & U64(0x0F0F0F0F0F0F0F0F);
	x = x + ( x >> 8 );
	x = x + ( x >> 16 );
	x = x + ( x >> 32 );
	return (int)( x & U64(0x000000000000007F) );
}

inline int countBits( unsigned x ){
	x = x - ( ( x >> 1 ) & 0x55555555U );
	x = ( x & 0x33333333U ) + ( ( x >> 2) & 0x33333333U );
	x = ( x + ( x >> 4 ) ) & 0x0F0F0F0FU;
	x = x + ( x >> 8 );
	x = x + ( x >> 16 );
	return x & 0x0000003FU;
}

inline int getFirstBit( unsigned x ){
#if defined(WIN32) && !defined(VC6) && !defined(__MINGW32__)
	int b;
	if( _BitScanForward( (DWORD*)&b, x ) ){ return b + 1; }
	return 0;
#elif defined(POSIX)
	if( x == 0U ){ return 0; }
	return __builtin_ffs( x );
#else
	int b;
	if( x == 0U ){ return 0; }
	if( ( b = _bfirst[ x     &0xff] ) != 0 ){ return b; }
	if( ( b = _bfirst[(x>> 8)&0xff] ) != 0 ){ return b + 8; }
	if( ( b = _bfirst[(x>>16)&0xff] ) != 0 ){ return b + 16; }
	return    _bfirst[(x>>24)     ] + 24;
#endif
}

inline int getLastBit( unsigned x ){
#if defined(WIN32) && !defined(VC6) && !defined(__MINGW32__)
	int b;
	if( _BitScanReverse( (DWORD*)&b, x ) ){ return 32 - b; }
	return 0;
#elif defined(POSIX)
	if( x == 0U ){ return 0; }
	return 32 - __builtin_clz( x );
#else
	int b;
	if( x == 0U ){ return 0; }
	if( ( b = _blast[(x>>24)     ] ) != 0 ){ return b + 24; }
	if( ( b = _blast[(x>>16)&0xff] ) != 0 ){ return b + 16; }
	if( ( b = _blast[(x>> 8)&0xff] ) != 0 ){ return b + 8; }
	return    _blast[ x     &0xff];
#endif
}

inline int getFirstBit( uint64 x ){
	int b = getFirstBit( (unsigned)x );
	if( b != 0 ){ return b; }
	else{
		return getFirstBit( (unsigned)( x >> 32 ) ) + 32;
	}
}

inline int getLastBit( uint64 x ){
	int b = getLastBit( (unsigned)( x >> 32 ) );
	if( b != 0 ){ return b + 32; }
	else{
		return getLastBit( (unsigned)x );
	}
}

inline unsigned removeFirstBit( unsigned& x ){
	return ( x &= ( x - 1 ) );
}

inline uint64 removeFirstBit( uint64& x ){
	return ( x &= ( x - 1 ) );
}

class uint96{
public:
	unsigned high;
	uint64   low;

	uint96(){
	}
	virtual ~uint96(){
	}
	uint96( const uint96& x ){
		high = x.high;
		low  = x.low;
	}
	uint96( unsigned h, uint64 l ){
		high = h;
		low  = l;
	}
	uint96( unsigned x ){
		high = 0U;
		low  = (uint64)x;
	}
	uint96( uint64 x ){
		high = 0U;
		low  = x;
	}

	uint96& operator=( const uint96& x ){
		high = x.high;
		low  = x.low;
		return *this;
	}
	uint96& operator=( unsigned x ){
		high = 0U;
		low  = (uint64)x;
		return *this;
	}
	uint96& operator=( uint64 x ){
		high = 0U;
		low  = x;
		return *this;
	}

	uint96& operator&=( const uint96& x ){
		high &= x.high;
		low  &= x.low;
		return *this;
	}
	uint96& operator|=( const uint96& x ){
		high |= x.high;
		low  |= x.low;
		return *this;
	}
	uint96& operator^=( const uint96& x ){
		high ^= x.high;
		low  ^= x.low;
		return *this;
	}
	uint96& operator<<=( int x ){
		if( x > 0 ){
			if( x < 64 ){
				high <<= x;
				high |= (unsigned)( low >> ( 64 - x ) );
				low <<= x;
			}
			else{
				high = (unsigned)( low << ( x - 64 ) );
				low = U64(0);
			}
		}
		return *this;
	}
	uint96& operator>>=( int x ){
		if( x > 0 ){
			if( x < 64 ){
				low >>= x;
				low |= (uint64)high << ( 64 - x );
				high >>= x;
			}
			else{
				low = (uint64)high >> ( x - 64 );
				high = 0U;
			}
		}
		return *this;
	}

	uint96 operator&( const uint96& x ) const{
		return uint96( *this ) &= x;
	}
	uint96 operator|( const uint96& x ) const{
		return uint96( *this ) |= x;
	}
	uint96 operator^( const uint96& x ) const{
		return uint96( *this ) ^= x;
	}
	uint96 operator<<( int x ) const{
		return uint96( *this ) <<= x;
	}
	uint96 operator>>( int x ) const{
		return uint96( *this ) >>= x;
	}
	uint96 operator~() const{
		return uint96( ~high, ~low );
	}

	bool operator==( const uint96& x ) const{
		return ( high == x.high ) && ( low == x.low );
	}
	bool operator==( unsigned x ) const{
		return ( high == 0U ) && ( low == (uint64)x );
	}
	bool operator==( uint64 x ) const{
		return ( high == 0U ) && ( low == x );
	}

	bool operator!=( const uint96& x ) const{
		return ( high != x.high ) || ( low != x.low );
	}
	bool operator!=( unsigned x ) const{
		return ( high != 0U ) || ( low != (uint64)x );
	}
	bool operator!=( uint64 x ) const{
		return ( high != 0U ) || ( low != x );
	}

	uint96& setBit( int bit ){
		if( bit < 64 ){
			low |= ( ((uint64)1U) << bit );
		}
		else{
			high |= ( 1U << (bit-64) );
		}
		return (*this);
	}
	uint96& unsetBit( int bit ){
		if( bit < 64 ){
			low &= ~( ((uint64)1U) << bit );
		}
		else{
			high &= ~( 1U << (bit-64) );
		}
		return (*this);
	}

	bool getBit( int bit ) const{
		if( bit < 64 ){
			return ( ( low & ( ((uint64)1U) << bit ) ) != (uint64)0U );
		}
		else{
			return ( ( high & ( 1U << (bit-64) ) ) != 0U );
		}
	}

	int getFirstBit() const{
		if( low != U64(0) ){
			int b = ::getFirstBit( (unsigned)low );
			if( b != 0 ){ return b; }
			else{
				return ::getFirstBit( (unsigned)( low >> 32 ) ) + 32;
			}
		}
		else if( high != 0U ){
			return ::getFirstBit( high ) + 64;
		}
		return 0;
	}

	int getLastBit() const{
		if( high != 0U ){
			return ::getLastBit( high ) + 64;
		}
		else if( low != U64(0) ){
			int b = ::getLastBit( (unsigned)( low >> 32 ) );
			if( b != 0 ){ return b + 32; }
			else{
				return ::getLastBit( (unsigned)low );
			}
		}
		return 0;
	}

	int getFirstBit( const uint96& mask ) const{
		uint64 ltemp;
		unsigned htemp;
		if( ( ltemp = low & mask.low ) != U64(0) ){
			int b = ::getFirstBit( (unsigned)ltemp );
			if( b != 0 ){ return b; }
			else{
				return ::getFirstBit( (unsigned)( ltemp >> 32 ) ) + 32;
			}
		}
		else if( ( htemp = high & mask.high ) != 0U ){
			return ::getFirstBit( htemp ) + 64;
		}
		return 0;
	}

	int getLastBit( const uint96& mask ) const{
		uint64 ltemp;
		unsigned htemp;
		if( ( htemp = high & mask.high ) != 0U ){
			return ::getLastBit( htemp ) + 64;
		}
		else if( ( ltemp = low & mask.low ) != U64(0) ){
			int b = ::getLastBit( (unsigned)( ltemp >> 32 ) );
			if( b != 0 ){ return b + 32; }
			else{
				return ::getLastBit( (unsigned)ltemp );
			}
		}
		return 0;
	}

	int getNextBit( int bit ) const{
		int b;
		if( bit < 64 ){
			// lowから探す。
			uint64 x = low >> bit;
			if( bit < 32 ){
				if( ( b = ::getFirstBit( (unsigned)x ) ) ){
					return bit + b;
				}
				x >>= 32;
				if( ( b = ::getFirstBit( (unsigned)x ) ) ){
					return bit + b + 32;
				}
			}
			else if( ( b = ::getFirstBit( (unsigned)x ) ) ){
				return bit + b;
			}

			// highから探す。
			if( ( b = ::getFirstBit( high ) ) ){
				return 64 + b;
			}
		}
		else if( bit < 96 ){
			// highから探す。
			unsigned x = high >> ( bit - 64 );
			if( ( b = ::getFirstBit( x ) ) ){
				return bit + b;
			}
		}
		return 0;
	}

	int countBits() const{
		return ::countBits( high ) + ::countBits( low );
	}

	int pickoutFirstBit(){
		int bit;
		if( low != U64(0) ){
			bit = ::getFirstBit( (unsigned)low );
			if( bit == 0 ){
				bit = ::getFirstBit( (unsigned)( low >> 32 ) ) + 32;
			}
			::removeFirstBit( low );
		}
		else if( high != 0U ){
			bit = ::getFirstBit( high ) + 64;
			::removeFirstBit( high );
		}
		else{
			bit = 0;
		}
		return bit;
	}

	void printBits(){
		int i;
		for( i = 31 ; i >= 0 ; i-- ){
			if( high & (1U<<i) ){ std::cout << '1'; }
			else                { std::cout << '0'; }
		}
		for( i = 61 ; i >= 0 ; i-- ){
			if( low & (U64(1)<<i) ){ std::cout << '1'; }
			else                   { std::cout << '0'; }
		}
	}
};

#endif
