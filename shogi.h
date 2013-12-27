/* shogi.h
 * R.Kubo 2008-2012
 * 将棋の局面と棋譜を管理するShogiクラス
 */

#ifndef _SHOGI_
#define _SHOGI_

#ifdef WIN32
#	include <windows.h>
#endif

#include <ctime>
#include <cstdlib>
#include <climits>
#include <cstring>
#include <iostream>
#include <string>
#include <list>
#include <functional>
#include <vector>
using namespace std;

#include <cassert>

// 乱数
#include "rand.h"

// 96ビット演算
#include "uint96.h"

// Debug
#define DEBUG_PRINT			cout << "Debug : " << __FILE__ << '(' << __LINE__ << ')' << '\n'; cout.flush();
#define DEBUG_VALUE(a)		cout << "Debug : " << __FILE__ << '(' << __LINE__ << "):" << #a << '=' << (a) << '\n'; cout.flush();

#define MAX_KIFU			1024   // 棋譜の最大サイズ
#define MAX_MOVES			1024   // 着手可能手の最大数
#define DOUBLE_CHECK		(1<<7) // 両王手
#define NOJUDGE_CHECK		(1<<8) // 王手未判定

#define BanAddr(x,y)		( ((y)<<4)+(x) )
#define PrintAddr(addr)		cout<<(addr)%16<<(addr)/16
#define Addr2X(addr)		( (addr) % 16 )
#define Addr2Y(addr)		( (addr) / 16 )

#define VMAX				( 10000000) // 評価値の最大値
#define VMIN				(-10000000) // 評価値の最小値

#define DNUM				12 // 駒の進める方向
#define _DNUM				8  // 駒の進める方向(桂馬を除く)

class Evaluate;
class Shogi;
class ShogiEval;

/************************************************
Debug
************************************************/
class Debug{
public:
	static void Print( const char* str, ... );
};

/************************************************
文字列処理
************************************************/
class String{
public:
	static int WildCard( const char* str, const char* wcard );
	static const char* GetSecondToken( const char* str, char c ){
		const char* p;
		if( NULL != ( p = strchr( str, c ) ) ){
			return p + 1;
		}
		return str;
	}
};

/************************************************
Timer 時間計測
************************************************/
class Timer{
private:

#ifdef WIN32
	LARGE_INTEGER time_b;
#else
	struct timespec time_b;
#endif

public:
	void SetTime(){
#ifdef WIN32
		QueryPerformanceCounter( &time_b );
#else
		clock_gettime( CLOCK_REALTIME, &time_b );
#endif
	}
	double GetTime(){
#ifdef WIN32
		LARGE_INTEGER time_n, freq;
		QueryPerformanceCounter( &time_n );
		QueryPerformanceFrequency( &freq );
		return ( time_n.QuadPart - time_b.QuadPart + 1 ) / (double)freq.QuadPart;
#else
		struct timespec time_n;
		clock_gettime( CLOCK_REALTIME, &time_n );
		return ( time_n.tv_sec - time_b.tv_sec )
			+ ( time_n.tv_nsec - time_b.tv_nsec + 1 ) * 1.0e-9;
#endif
	}
};

/************************************************
FileList ファイルの列挙
非常によくない作りになっている->あとで考える。
************************************************/
class FileList{
private:
	list<string> flist;
	list<string>::iterator it;
public:
	FileList(){
		it = flist.begin();
	}
	virtual ~FileList(){
	}
	int Enumerate( const char* directory, const char* extension );
	void Clear(){
		flist.clear();
		it = flist.begin();
	}
	void Begin(){
		it = flist.begin();
	}
	void Push( const string& fname ){
		flist.push_back( fname );
	}
	int Pop( string& fname ){
		if( it != flist.end() ){
			fname = (*it);
			it++;
			return 1;
		}
		else
			return 0;
	}
	int Size(){
		return flist.size();
	}
};

/************************************************
TEAI 手合
************************************************/
enum TEAI{
	HIRATE = 0, // 平手
	KY_OCHI,    // 香落ち
	KA_OCHI,    // 角落ち
	HI_OCHI,    // 飛車落ち
	HIKY_OCHI,  // 飛香落ち
	M2_OCHI,    // 2枚落ち
	M4_OCHI,    // 4枚落ち
	M6_OCHI,    // 6枚落ち
	M8_OCHI,    // 8枚落ち
	M10_OCHI,   // 10枚落ち
};

/************************************************
KOMA マス目(駒)の表現
************************************************/
typedef unsigned int KOMA;

enum{
	// 空
	EMPTY = 0,
	EMP = 0,

	// 駒の種別
	FU = 1,           // 0x0001 1
	KY = 2,           // 0x0002 2
	KE = 3,           // 0x0003 3
	GI = 4,           // 0x0004 4
	KI = 5,           // 0x0005 5
	KA = 6,           // 0x0006 6
	HI = 7,           // 0x0007 7
	OU = 8,           // 0x0008 8

	// 成り駒
	NARI = (1<<3),    // 0x0008 8

	TO = FU + NARI,   // 0x0009 9
	NY = KY + NARI,   // 0x000A 10
	NK = KE + NARI,   // 0x000B 11
	NG = GI + NARI,   // 0x000C 12
	UM = KA + NARI,   // 0x000E 14
	RY = HI + NARI,   // 0x000F 15

	// 先後
	SENTE = (1<<4),   // 0x0010 16
	GOTE = (1<<5),    // 0x0020 32
	SENGO = SENTE | GOTE,

	// 先手の駒
	SFU = SENTE + FU, // 0x0011 17
	SKY = SENTE + KY, // 0x0012 18
	SKE = SENTE + KE, // 0x0013 19
	SGI = SENTE + GI, // 0x0014 20
	SKI = SENTE + KI, // 0x0015 21
	SKA = SENTE + KA, // 0x0016 22
	SHI = SENTE + HI, // 0x0017 23
	SOU = SENTE + OU, // 0x0018 24
	STO = SENTE + TO, // 0x0019 25
	SNY = SENTE + NY, // 0x001A 26
	SNK = SENTE + NK, // 0x001B 27
	SNG = SENTE + NG, // 0x001C 28
	SUM = SENTE + UM, // 0x001E 30
	SRY = SENTE + RY, // 0x001F 31

	// 0x00 後手の駒
	GFU = GOTE + FU,  // 0x0021 33
	GKY = GOTE + KY,  // 0x0022 34
	GKE = GOTE + KE,  // 0x0023 35
	GGI = GOTE + GI,  // 0x0024 36
	GKI = GOTE + KI,  // 0x0025 37
	GKA = GOTE + KA,  // 0x0026 38
	GHI = GOTE + HI,  // 0x0027 39
	GOU = GOTE + OU,  // 0x0028 40
	GTO = GOTE + TO,  // 0x0029 41
	GNY = GOTE + NY,  // 0x002A 42
	GNK = GOTE + NK,  // 0x002B 43
	GNG = GOTE + NG,  // 0x002C 44
	GUM = GOTE + UM,  // 0x002E 46
	GRY = GOTE + RY,  // 0x002F 47

	// 番兵
	WALL = (1<<6),    // 0x0040 64

	// 持ち駒
	DAI = (1<<8),     // 0x0100 256

	// パス
	PASS = (1<<9),    // 0x0200 512
};

/************************************************
ConstData
************************************************/
class ConstData{
	// data.cpp
private:
	static const int __idr[];
	static const int __diff2dir[];
	static const unsigned __diff2bitF[];
	static const int __diff2dis[];
	static const int __diff2num[17][17];
	static const unsigned int __dir2bit[];
	static const unsigned int __dir2bitF[];
	static const unsigned __diff2attack[][17][17];

public:
	static const int def_piece[];
	static const int _sym[];
	static const int _addr[];
	static const int _iaddr0[];
	static const int* _iaddr;
	static const int sente_num[];
	static const int gote_num[];

	static const KOMA _hirate[];
	static const KOMA _empty[];

	static const int isFly[];
	static const int koma_flag[];
	static const int _dir[DNUM];
	static const int* _idr;
	static const int _mov[][DNUM];
	static const int* _diff2dir;
	static const unsigned* _diff2bitF;
	static const int* _diff2dis;
	static const int (*_diff2num)[17];
	static const int _aa2num[81][81];
	static const uint96 _addr2bit[];

	static const unsigned int* _dir2bit;
	static const unsigned int* _dir2bitF;
	static const unsigned int _mov4bit[];

	static const unsigned int _brev9[];
	static const unsigned int _bnum[];

	static const unsigned (*_diff2attack[GRY+1])[17];

	// 評価項目「玉の周囲の金銀の数」を廃止したので不要
	//static const uint96 nei8;
	//static const uint96 nei24;

	static const uint96 nf1;
	static const uint96 nf9;

	static const uint96 _dbits_lu[81];
	static const uint96 _dbits_up[81];
	static const uint96 _dbits_ru[81];
	static const uint96 _dbits_lt[81];
	static const uint96 _dbits_rt[81];
	static const uint96 _dbits_ld[81];
	static const uint96 _dbits_dn[81];
	static const uint96 _dbits_rd[81];

	static const uint96 _nbits8[81];
	static const uint96 _nbits24[81];

	static const int wdiss_lu[81];
	static const int wdiss_up[81];
	static const int wdiss_ru[81];
	static const int wdiss_lt[81];
	static const int wdiss_rt[81];
	static const int wdiss_ld[81];
	static const int wdiss_dn[81];
	static const int wdiss_rd[81];

	static const int inaniwaFU[81];
	static const int inaniwaKY[81];
	static const int inaniwaKE[81];
	static const int inaniwaGI[81];
	static const int inaniwaKI[81];
	static const int inaniwaKA[81];
	static const int inaniwaHI[81];
	static const int inaniwaOU[81];
};

/************************************************
ハッシュ
************************************************/
class HashHandler{
public:
	virtual void Resize() = 0;
};

class Hash{
private:
	static uint64 hash_size;
	static uint64 hash_mask;
	static list<HashHandler*> handlers;

public:
	static void SetHandler( HashHandler* p ){
		handlers.push_back( p );
	}
	static void UnsetHandler( HashHandler* p ){
		handlers.remove( p );
	}
	static void SetSize( unsigned hash_pow2 ){
		list<HashHandler*>::iterator ite;
		hash_size = ( U64(1) << hash_pow2 );
		hash_mask = hash_size - 1;
		for( ite = handlers.begin() ; ite != handlers.end() ; ++ite ){
			(*ite)->Resize();
		}
	}
	static uint64 Size(){
		return hash_size;
	}
	static uint64 Mask(){
		return hash_mask;
	}

private:
	uint64 hash_ban[GRY-SFU+1][0x100];
	uint64 hash_dai[GHI-SFU+1][19];
	uint64 hash_sengo;

private:
	int InitHash();
	int ReadHash();
	int WriteHash();
public:
	Hash(){
		InitHash();
	}
	virtual ~Hash(){
	}
	uint64 Ban( KOMA koma, int addr ){
		return hash_ban[koma-SFU][addr];
	}
	uint64 Dai( KOMA koma, int num ){
		return hash_dai[koma-SFU][num];
	}
	uint64 Sengo(){
		return hash_sengo;
	}
};

/************************************************
局面評価
************************************************/
#define EV_SCALE			32.0

enum{
	// 駒割り以外の要素数
	PARAM_NUM =
		+ 19                // 持駒の歩
		+ 5                 // 持駒の香
		+ 5                 // 持駒の桂
		+ 5                 // 持駒の銀
		+ 5                 // 持駒の金
		+ 3                 // 持駒の角
		+ 3                 // 持駒の飛
 
		+ 81 * 19           // 持駒の歩
		+ 81 * 5            // 持駒の香
		+ 81 * 5            // 持駒の桂
		+ 81 * 5            // 持駒の銀
		+ 81 * 5            // 持駒の金
		+ 81 * 3            // 持駒の角
		+ 81 * 3            // 持駒の飛
 
		+ 81 * 19           // 持駒の歩
		+ 81 * 5            // 持駒の香
		+ 81 * 5            // 持駒の桂
		+ 81 * 5            // 持駒の銀
		+ 81 * 5            // 持駒の金
		+ 81 * 3            // 持駒の角
		+ 81 * 3            // 持駒の飛
 
		+ 289 * 18          // 王-他の駒
		+ 9 * 289 * 18      // 王-他の駒
		+ 9 * 289 * 18      // 王-他の駒

		+ 289 * 18 * 18     // 王-隣接2枚
		+ 289 * 18 * 18     // 王-隣接2枚
		+ 289 * 18 * 18     // 王-隣接2枚
		+ 289 * 18 * 18     // 王-隣接2枚

		+ 2 * 289 * 9       // 自由度(香)
		+ 2 * 289 * 9       // 自由度(角-左前)
		+ 2 * 289 * 9       // 自由度(角-右前)
		+ 2 * 289 * 9       // 自由度(角-左後)
		+ 2 * 289 * 9       // 自由度(角-右後)
		+ 2 * 289 * 9       // 自由度(飛-前)
		+ 2 * 289 * 9       // 自由度(飛-左)
		+ 2 * 289 * 9       // 自由度(飛-右)
		+ 2 * 289 * 9       // 自由度(飛-後)
 
		+ 2 * 289 * 33      // 自由度(角-合計)
		+ 2 * 289 * 33      // 自由度(飛-合計)

		+ 2 * 289 * 20      // 利き(香)
		+ 2 * 289 * 20      // 利き(角-左前)
		+ 2 * 289 * 20      // 利き(角-右前)
		+ 2 * 289 * 20      // 利き(角-左後)
		+ 2 * 289 * 20      // 利き(角-右後)
		+ 2 * 289 * 20      // 利き(飛-前)
		+ 2 * 289 * 20      // 利き(飛-左)
		+ 2 * 289 * 20      // 利き(飛-右)
		+ 2 * 289 * 20      // 利き(飛-後)
 
#if 1
		+ 289 * 20          // 自分の歩の2マス前の駒
		+ 289 * 20          // 相手の歩の2マス前の駒
#endif
 
#if 1
		+ 9                 // 歩を打てる筋
		+ 81 * 9            // 自分が歩を打てる筋
		+ 81 * 9            // 相手が歩を打てる筋
#endif

		+ 8 * 18 * 289 * 20 // 王-8近傍-その他

#if 1
		+ 81                // 歩打ちで王手できるか
#endif

		+ 18                // 玉-桂馬の利き先
		+ 289 * 18          // 玉-桂馬の利き先(左)
		+ 289 * 18          // 玉-桂馬の利き先(右)
		+ 289 * 18          // 玉-桂馬の利き先(左)
		+ 289 * 18          // 玉-桂馬の利き先(右)

		+ 10                // 玉の周囲9マスで移動可能な箇所の数
		+ 9 * 10            // 玉の周囲9マスで移動可能な箇所の数
		+ 9 * 10            // 玉の周囲9マスで移動可能な箇所の数

		+ 8                 // 玉の周囲8マスでの利きの勝ち負け

		+ 81                // 自玉の位置
		,
};

#define PARAM				hand_fu

/************************************************
TempParam 評価パラメータテンプレート
************************************************/
template <class T, class T2>
class TempParam : public ConstData{
public:
	enum{
		X = 0,
		Y,
	};

	enum{
		S = 0,
		G,
	};

	T2 piece[RY];                        // 駒割り

	// 最初の要素が変更されたらPARAMマクロを書き換える。
	T hand_fu[19];                       // 持駒の歩
	T hand_ky[5];                        // 持駒の香
	T hand_ke[5];                        // 持駒の桂
	T hand_gi[5];                        // 持駒の銀
	T hand_ki[5];                        // 持駒の金
	T hand_ka[3];                        // 持駒の角
	T hand_hi[3];                        // 持駒の飛

	T king_hand_sfu[81][19];             // 持駒の歩
	T king_hand_sky[81][5];              // 持駒の香
	T king_hand_ske[81][5];              // 持駒の桂
	T king_hand_sgi[81][5];              // 持駒の銀
	T king_hand_ski[81][5];              // 持駒の金
	T king_hand_ska[81][3];              // 持駒の角
	T king_hand_shi[81][3];              // 持駒の飛

	T king_hand_gfu[81][19];             // 持駒の歩
	T king_hand_gky[81][5];              // 持駒の香
	T king_hand_gke[81][5];              // 持駒の桂
	T king_hand_ggi[81][5];              // 持駒の銀
	T king_hand_gki[81][5];              // 持駒の金
	T king_hand_gka[81][3];              // 持駒の角
	T king_hand_ghi[81][3];              // 持駒の飛

	T king_piece[289][18];               // 王-他の駒
	T kingX_piece[9][289][18];           // 王X-他の駒
	T kingY_piece[9][289][18];           // 王Y-他の駒

	T king_adjoin_r [289][18][18];       // 王-隣接2枚
	T king_adjoin_ld[289][18][18];       // 王-隣接2枚
	T king_adjoin_d [289][18][18];       // 王-隣接2枚
	T king_adjoin_rd[289][18][18];       // 王-隣接2枚

	T free_ky  [2][289][9];              // 自由度(香)
	T free_ka_l[2][289][9];              // 自由度(角-左前)
	T free_ka_r[2][289][9];              // 自由度(角-右前)
	T free_ka_L[2][289][9];              // 自由度(角-左後)
	T free_ka_R[2][289][9];              // 自由度(角-右後)
	T free_hi_f[2][289][9];              // 自由度(飛-前)
	T free_hi_l[2][289][9];              // 自由度(飛-左)
	T free_hi_r[2][289][9];              // 自由度(飛-右)
	T free_hi_b[2][289][9];              // 自由度(飛-後)

	T free_ka_a[2][289][33];             // 自由度(角-合計)
	T free_hi_a[2][289][33];             // 自由度(飛-合計)

	T effect_ky  [2][289][20];           // 利き(香)
	T effect_ka_l[2][289][20];           // 利き(角-左前)
	T effect_ka_r[2][289][20];           // 利き(角-右前)
	T effect_ka_L[2][289][20];           // 利き(角-左後)
	T effect_ka_R[2][289][20];           // 利き(角-右後)
	T effect_hi_f[2][289][20];           // 利き(飛-前)
	T effect_hi_l[2][289][20];           // 利き(飛-左)
	T effect_hi_r[2][289][20];           // 利き(飛-右)
	T effect_hi_b[2][289][20];           // 利き(飛-後)

#if 1
	T front2_sfu[289][20];               // 自分の歩の2マス前の駒
	T front2_gfu[289][20];               // 相手の歩の2マス前の駒
#endif

#if 1
	T fu_file[9];                        // 歩を打てる筋
	T sfu_file[81][9];                   // 自分が歩を打てる筋
	T gfu_file[81][9];                   // 相手が歩を打てる筋
#endif

	T king8_piece[8][18][289][20];       // 王-8近傍-その他

#if 1
	T king_check_fu[81];                 // 歩打ちで王手できるか
#endif

	T ke[18];                            // 玉-桂馬の利き先(左)
	T kingS_ke_l[289][18];               // 玉-桂馬の利き先(左)
	T kingS_ke_r[289][18];               // 玉-桂馬の利き先(右)
	T kingG_ke_l[289][18];               // 玉-桂馬の利き先(左)
	T kingG_ke_r[289][18];               // 玉-桂馬の利き先(右)

	T king_effnum9[10];                  // 玉の周囲9マスで移動可能な箇所の数
	T kingX_effnum9[9][10];              // 玉の周囲9マスで移動可能な箇所の数
	T kingY_effnum9[9][10];              // 玉の周囲9マスで移動可能な箇所の数

	T king_effwin[8];                    // 玉の周囲8マスでの利きの勝ち負け

	T king_addr[81];                    // 自玉の位置

	// Cumulation
	T c_hand_fu[19];                     // 持駒の歩
	T c_hand_ky[5];                      // 持駒の香
	T c_hand_ke[5];                      // 持駒の桂
	T c_hand_gi[5];                      // 持駒の銀
	T c_hand_ki[5];                      // 持駒の金
	T c_hand_ka[3];                      // 持駒の角
	T c_hand_hi[3];                      // 持駒の飛

	T u_king_hand_sfu[81][19];           // 持駒の歩
	T u_king_hand_sky[81][5];            // 持駒の香
	T u_king_hand_ske[81][5];            // 持駒の桂
	T u_king_hand_sgi[81][5];            // 持駒の銀
	T u_king_hand_ski[81][5];            // 持駒の金
	T u_king_hand_ska[81][3];            // 持駒の角
	T u_king_hand_shi[81][3];            // 持駒の飛

	T c_king_hand_gfu[81][19];           // 持駒の歩
	T c_king_hand_gky[81][5];            // 持駒の香
	T c_king_hand_gke[81][5];            // 持駒の桂
	T c_king_hand_ggi[81][5];            // 持駒の銀
	T c_king_hand_gki[81][5];            // 持駒の金
	T c_king_hand_gka[81][3];            // 持駒の角
	T c_king_hand_ghi[81][3];            // 持駒の飛

	T u_king_piece[81][81][18];          // 王-他の駒

	T c_free_ky  [2][289][9];            // 自由度(香)
	T c_free_ka_l[2][289][9];            // 自由度(角-左前)
	T c_free_ka_r[2][289][9];            // 自由度(角-右前)
	T c_free_ka_L[2][289][9];            // 自由度(角-左後)
	T c_free_ka_R[2][289][9];            // 自由度(角-右後)
	T c_free_hi_f[2][289][9];            // 自由度(飛-前)
	T c_free_hi_l[2][289][9];            // 自由度(飛-左)
	T c_free_hi_r[2][289][9];            // 自由度(飛-右)
	T c_free_hi_b[2][289][9];            // 自由度(飛-後)

	T c_free_ka_a[2][289][33];           // 自由度(角-合計)
	T c_free_hi_a[2][289][33];           // 自由度(飛-合計)

	T c_sfu_file[81][512];               // 自分が歩を打てる筋
	T c_gfu_file[81][512];               // 相手が歩を打てる筋

	T u_kingS_ke_l[289][18];             // 玉-桂馬の利き先(左)
	T u_kingS_ke_r[289][18];             // 玉-桂馬の利き先(右)

	T c_king_effnum9[10];                // 玉の周囲9マスで移動可能な箇所の数
	T c_kingX_effnum9[9][10];            // 玉の周囲9マスで移動可能な箇所の数
	T c_kingY_effnum9[9][10];            // 玉の周囲9マスで移動可能な箇所の数

	T u_king_effnum9[81][10];            // 玉の周囲9マスで移動可能な箇所の数

protected:
	void Cum( T a[], T b[], int num ){
		int i;
		a[0] = b[0];
		for( i = 1 ; i < num ; i++ ){
			a[i] = a[i-1] + b[i];
		}
	}
	void CumR( T a[], T b[], int num ){
		int i;
		b[num-1] = a[num-1];
		for( i = num-2 ; i >= 0 ; i-- ){
			b[i] = b[i+1] + a[i];
		}
	}
	void Cum2( T a[], T b[], T c[], int num ){
		int i;
		a[0] = b[0] + c[0];
		for( i = 1 ; i < num ; i++ ){
			a[i] = a[i-1] + b[i] + c[i];
		}
	}
	void Cum2R( T a[], T b[], T c[], int num ){
		int i;
		b[num-1] = c[num-1] = a[num-1];
		for( i = num-2 ; i >= 0 ; i-- ){
			b[i] = b[i+1] + a[i];
			c[i] = c[i+1] + a[i];
		}
	}
	virtual void SymFunc( T& a, T& b )=0;

public:
	TempParam(){
	}
	virtual ~TempParam(){
	}
	void Cumulate(){
		// 足し込み計算
		int i, j, k;

		Cum( c_hand_fu, hand_fu, 19 );
		Cum( c_hand_ky, hand_ky, 5 );
		Cum( c_hand_ke, hand_ke, 5 );
		Cum( c_hand_gi, hand_gi, 5 );
		Cum( c_hand_ki, hand_ki, 5 );
		Cum( c_hand_ka, hand_ka, 3 );
		Cum( c_hand_hi, hand_hi, 3 );

		for( i = 0 ; i < 81 ; i++ ){
			Cum( u_king_hand_sfu[i], king_hand_sfu[i], 19 );
			Cum( u_king_hand_sky[i], king_hand_sky[i], 5 );
			Cum( u_king_hand_ske[i], king_hand_ske[i], 5 );
			Cum( u_king_hand_sgi[i], king_hand_sgi[i], 5 );
			Cum( u_king_hand_ski[i], king_hand_ski[i], 5 );
			Cum( u_king_hand_ska[i], king_hand_ska[i], 3 );
			Cum( u_king_hand_shi[i], king_hand_shi[i], 3 );
			Cum( c_king_hand_gfu[i], king_hand_gfu[i], 19 );
			Cum( c_king_hand_gky[i], king_hand_gky[i], 5 );
			Cum( c_king_hand_gke[i], king_hand_gke[i], 5 );
			Cum( c_king_hand_ggi[i], king_hand_ggi[i], 5 );
			Cum( c_king_hand_gki[i], king_hand_gki[i], 5 );
			Cum( c_king_hand_gka[i], king_hand_gka[i], 3 );
			Cum( c_king_hand_ghi[i], king_hand_ghi[i], 3 );

			for( j = 0 ; j < 19 ; j++ ){
				u_king_hand_sfu[i][j] += c_hand_fu[j];
			}
			for( j = 0 ; j < 5 ; j++ ){
				u_king_hand_sky[i][j] += c_hand_ky[j];
				u_king_hand_ske[i][j] += c_hand_ke[j];
				u_king_hand_sgi[i][j] += c_hand_gi[j];
				u_king_hand_ski[i][j] += c_hand_ki[j];
			}
			for( j = 0 ; j < 3 ; j++ ){
				u_king_hand_ska[i][j] += c_hand_ka[j];
				u_king_hand_shi[i][j] += c_hand_hi[j];
			}
		}

		for( i = 0 ; i < 81 ; i++ ){
			int x = i % 9;
			int y = i / 9;
			for( j = 0 ; j < 81 ; j++ ){
				int diff = _diff2num[j/9-y][j%9-x];
				for( k = 0 ; k < 18 ; k++ ){
					u_king_piece[i][j][k] = king_piece[diff][k]
						+ kingX_piece[x][diff][k] + kingY_piece[y][diff][k];
				}
			}
		}

		for( i = 0 ; i < 289 ; i++ ){
			Cum( c_free_ky  [0][i], free_ky  [0][i], 9 );
			Cum( c_free_ka_l[0][i], free_ka_l[0][i], 9 );
			Cum( c_free_ka_r[0][i], free_ka_r[0][i], 9 );
			Cum( c_free_ka_L[0][i], free_ka_L[0][i], 9 );
			Cum( c_free_ka_R[0][i], free_ka_R[0][i], 9 );
			Cum( c_free_hi_f[0][i], free_hi_f[0][i], 9 );
			Cum( c_free_hi_l[0][i], free_hi_l[0][i], 9 );
			Cum( c_free_hi_r[0][i], free_hi_r[0][i], 9 );
			Cum( c_free_hi_b[0][i], free_hi_b[0][i], 9 );
			Cum( c_free_ky  [1][i], free_ky  [1][i], 9 );
			Cum( c_free_ka_l[1][i], free_ka_l[1][i], 9 );
			Cum( c_free_ka_r[1][i], free_ka_r[1][i], 9 );
			Cum( c_free_ka_L[1][i], free_ka_L[1][i], 9 );
			Cum( c_free_ka_R[1][i], free_ka_R[1][i], 9 );
			Cum( c_free_hi_f[1][i], free_hi_f[1][i], 9 );
			Cum( c_free_hi_l[1][i], free_hi_l[1][i], 9 );
			Cum( c_free_hi_r[1][i], free_hi_r[1][i], 9 );
			Cum( c_free_hi_b[1][i], free_hi_b[1][i], 9 );

			Cum( c_free_ka_a[0][i], free_ka_a[0][i], 33 );
			Cum( c_free_hi_a[0][i], free_hi_a[0][i], 33 );
			Cum( c_free_ka_a[1][i], free_ka_a[1][i], 33 );
			Cum( c_free_hi_a[1][i], free_hi_a[1][i], 33 );
		}

		for( i = 0 ; i < 81 ; i++ ){
			for( j = 0 ; j < 512 ; j++ ){
				c_sfu_file[i][j] = 0;
				c_gfu_file[i][j] = 0;
				if( !( j & ( 1 << 1 ) ) ){
					c_sfu_file[i][j] += fu_file[0] + sfu_file[i][0];
					c_gfu_file[i][j] +=              gfu_file[i][0];
				}
				if( !( j & ( 1 << 2 ) ) ){
					c_sfu_file[i][j] += fu_file[1] + sfu_file[i][1];
					c_gfu_file[i][j] +=              gfu_file[i][1];
				}
				if( !( j & ( 1 << 3 ) ) ){
					c_sfu_file[i][j] += fu_file[2] + sfu_file[i][2];
					c_gfu_file[i][j] +=              gfu_file[i][2];
				}
				if( !( j & ( 1 << 4 ) ) ){
					c_sfu_file[i][j] += fu_file[3] + sfu_file[i][3];
					c_gfu_file[i][j] +=              gfu_file[i][3];
				}
				if( !( j & ( 1 << 5 ) ) ){
					c_sfu_file[i][j] += fu_file[4] + sfu_file[i][4];
					c_gfu_file[i][j] +=              gfu_file[i][4];
				}
				if( !( j & ( 1 << 6 ) ) ){
					c_sfu_file[i][j] += fu_file[5] + sfu_file[i][5];
					c_gfu_file[i][j] +=              gfu_file[i][5];
				}
				if( !( j & ( 1 << 7 ) ) ){
					c_sfu_file[i][j] += fu_file[6] + sfu_file[i][6];
					c_gfu_file[i][j] +=              gfu_file[i][6];
				}
				if( !( j & ( 1 << 8 ) ) ){
					c_sfu_file[i][j] += fu_file[7] + sfu_file[i][7];
					c_gfu_file[i][j] +=              gfu_file[i][7];
				}
				if( !( j & ( 1 << 9 ) ) ){
					c_sfu_file[i][j] += fu_file[8] + sfu_file[i][8];
					c_gfu_file[i][j] +=              gfu_file[i][8];
				}
			}
		}

		for( i = 0 ; i < 289 ; i++ ){
			for( j = 0 ; j < 18 ; j++ ){
				u_kingS_ke_l[i][j] = kingS_ke_l[i][j] + ke[j];
				u_kingS_ke_r[i][j] = kingS_ke_r[i][j] + ke[j];
			}
		}

		Cum( c_king_effnum9, king_effnum9, 10 );

		for( i = 0 ; i < 9 ; i++ ){
			Cum( c_kingX_effnum9[i], kingX_effnum9[i], 10 );
			Cum( c_kingY_effnum9[i], kingY_effnum9[i], 10 );
		}

		for( i = 0 ; i < 81 ; i++ ){
			int x = i % 9;
			int y = i / 9;
			for( j = 0 ; j <= 9 ; j++ ){
				u_king_effnum9[i][j] = c_king_effnum9[j]
					+ c_kingX_effnum9[x][j] + c_kingY_effnum9[y][j];
			}
		}
	}
	void CumulateR(){
		// 足し込み計算
		int i, j, k;

		memset( c_hand_fu, 0, sizeof(c_hand_fu) );
		memset( c_hand_ky, 0, sizeof(c_hand_ky) );
		memset( c_hand_ke, 0, sizeof(c_hand_ke) );
		memset( c_hand_gi, 0, sizeof(c_hand_gi) );
		memset( c_hand_ki, 0, sizeof(c_hand_ki) );
		memset( c_hand_ka, 0, sizeof(c_hand_ka) );
		memset( c_hand_hi, 0, sizeof(c_hand_hi) );

		for( i = 0 ; i < 81 ; i++ ){
			for( j = 0 ; j < 19 ; j++ ){
				c_hand_fu[j] += u_king_hand_sfu[i][j];
			}
			for( j = 0 ; j < 5 ; j++ ){
				c_hand_ky[j] += u_king_hand_sky[i][j];
				c_hand_ke[j] += u_king_hand_ske[i][j];
				c_hand_gi[j] += u_king_hand_sgi[i][j];
				c_hand_ki[j] += u_king_hand_ski[i][j];
			}
			for( j = 0 ; j < 3 ; j++ ){
				c_hand_ka[j] += u_king_hand_ska[i][j];
				c_hand_hi[j] += u_king_hand_shi[i][j];
			}

			CumR( u_king_hand_sfu[i], king_hand_sfu[i], 19 );
			CumR( u_king_hand_sky[i], king_hand_sky[i], 5 );
			CumR( u_king_hand_ske[i], king_hand_ske[i], 5 );
			CumR( u_king_hand_sgi[i], king_hand_sgi[i], 5 );
			CumR( u_king_hand_ski[i], king_hand_ski[i], 5 );
			CumR( u_king_hand_ska[i], king_hand_ska[i], 3 );
			CumR( u_king_hand_shi[i], king_hand_shi[i], 3 );
			CumR( c_king_hand_gfu[i], king_hand_gfu[i], 19 );
			CumR( c_king_hand_gky[i], king_hand_gky[i], 5 );
			CumR( c_king_hand_gke[i], king_hand_gke[i], 5 );
			CumR( c_king_hand_ggi[i], king_hand_ggi[i], 5 );
			CumR( c_king_hand_gki[i], king_hand_gki[i], 5 );
			CumR( c_king_hand_gka[i], king_hand_gka[i], 3 );
			CumR( c_king_hand_ghi[i], king_hand_ghi[i], 3 );
		}

		CumR( c_hand_fu, hand_fu, 19 );
		CumR( c_hand_ky, hand_ky, 5 );
		CumR( c_hand_ke, hand_ke, 5 );
		CumR( c_hand_gi, hand_gi, 5 );
		CumR( c_hand_ki, hand_ki, 5 );
		CumR( c_hand_ka, hand_ka, 3 );
		CumR( c_hand_hi, hand_hi, 3 );

		memset( king_piece , 0, sizeof(king_piece)  );
		memset( kingX_piece, 0, sizeof(kingX_piece) );
		memset( kingY_piece, 0, sizeof(kingY_piece) );
		for( i = 0 ; i < 81 ; i++ ){
			int x = i % 9;
			int y = i / 9;
			for( j = 0 ; j < 81 ; j++ ){
				int diff = _diff2num[j/9-y][j%9-x];
				for( k = 0 ; k < 18 ; k++ ){
					king_piece    [diff][k] += u_king_piece[i][j][k];
					kingX_piece[x][diff][k] += u_king_piece[i][j][k];
					kingY_piece[y][diff][k] += u_king_piece[i][j][k];
				}
			}
		}

		for( i = 0 ; i < 289 ; i++ ){
			CumR( c_free_ky  [0][i], free_ky  [0][i], 9 );
			CumR( c_free_ka_l[0][i], free_ka_l[0][i], 9 );
			CumR( c_free_ka_r[0][i], free_ka_r[0][i], 9 );
			CumR( c_free_ka_L[0][i], free_ka_L[0][i], 9 );
			CumR( c_free_ka_R[0][i], free_ka_R[0][i], 9 );
			CumR( c_free_hi_f[0][i], free_hi_f[0][i], 9 );
			CumR( c_free_hi_l[0][i], free_hi_l[0][i], 9 );
			CumR( c_free_hi_r[0][i], free_hi_r[0][i], 9 );
			CumR( c_free_hi_b[0][i], free_hi_b[0][i], 9 );
			CumR( c_free_ky  [1][i], free_ky  [1][i], 9 );
			CumR( c_free_ka_l[1][i], free_ka_l[1][i], 9 );
			CumR( c_free_ka_r[1][i], free_ka_r[1][i], 9 );
			CumR( c_free_ka_L[1][i], free_ka_L[1][i], 9 );
			CumR( c_free_ka_R[1][i], free_ka_R[1][i], 9 );
			CumR( c_free_hi_f[1][i], free_hi_f[1][i], 9 );
			CumR( c_free_hi_l[1][i], free_hi_l[1][i], 9 );
			CumR( c_free_hi_r[1][i], free_hi_r[1][i], 9 );
			CumR( c_free_hi_b[1][i], free_hi_b[1][i], 9 );

			CumR( c_free_ka_a[0][i], free_ka_a[0][i], 33 );
			CumR( c_free_hi_a[0][i], free_hi_a[0][i], 33 );
			CumR( c_free_ka_a[1][i], free_ka_a[1][i], 33 );
			CumR( c_free_hi_a[1][i], free_hi_a[1][i], 33 );
		}

		memset( fu_file, 0, sizeof(fu_file) );
		memset( sfu_file, 0, sizeof(sfu_file) );
		memset( gfu_file, 0, sizeof(gfu_file) );
		for( i = 0 ; i < 81 ; i++ ){
			for( j = 0 ; j < 512 ; j++ ){
				if( !( j & ( 1 << 1 ) ) ){
					fu_file[0]     += c_sfu_file[i][j];
					sfu_file[i][0] += c_sfu_file[i][j];
					gfu_file[i][0] += c_gfu_file[i][j];
				}
				if( !( j & ( 1 << 2 ) ) ){
					fu_file[1]     += c_sfu_file[i][j];
					sfu_file[i][1] += c_sfu_file[i][j];
					gfu_file[i][1] += c_gfu_file[i][j];
				}
				if( !( j & ( 1 << 3 ) ) ){
					fu_file[3]     += c_sfu_file[i][j];
					sfu_file[i][2] += c_sfu_file[i][j];
					gfu_file[i][2] += c_gfu_file[i][j];
				}
				if( !( j & ( 1 << 4 ) ) ){
					fu_file[3]     += c_sfu_file[i][j];
					sfu_file[i][3] += c_sfu_file[i][j];
					gfu_file[i][3] += c_gfu_file[i][j];
				}
				if( !( j & ( 1 << 5 ) ) ){
					fu_file[4]     += c_sfu_file[i][j];
					sfu_file[i][4] += c_sfu_file[i][j];
					gfu_file[i][4] += c_gfu_file[i][j];
				}
				if( !( j & ( 1 << 6 ) ) ){
					fu_file[5]     += c_sfu_file[i][j];
					sfu_file[i][5] += c_sfu_file[i][j];
					gfu_file[i][5] += c_gfu_file[i][j];
				}
				if( !( j & ( 1 << 7 ) ) ){
					fu_file[6]     += c_sfu_file[i][j];
					sfu_file[i][6] += c_sfu_file[i][j];
					gfu_file[i][6] += c_gfu_file[i][j];
				}
				if( !( j & ( 1 << 8 ) ) ){
					fu_file[7]     += c_sfu_file[i][j];
					sfu_file[i][7] += c_sfu_file[i][j];
					gfu_file[i][7] += c_gfu_file[i][j];
				}
				if( !( j & ( 1 << 9 ) ) ){
					fu_file[8]     += c_sfu_file[i][j];
					sfu_file[i][8] += c_sfu_file[i][j];
					gfu_file[i][8] += c_gfu_file[i][j];
				}
			}
		}

		memset( ke, 0, sizeof(ke) );
		for( i = 0 ; i < 289 ; i++ ){
			for( j = 0 ; j < 18 ; j++ ){
				kingS_ke_l[i][j] = u_kingS_ke_l[i][j];
				kingS_ke_r[i][j] = u_kingS_ke_r[i][j];
				ke[j] += u_kingS_ke_l[i][j] + u_kingS_ke_r[i][j];
			}
		}

		memset( c_king_effnum9 , 0, sizeof(c_king_effnum9) );
		memset( c_kingX_effnum9 , 0, sizeof(c_kingX_effnum9) );
		memset( c_kingY_effnum9 , 0, sizeof(c_kingY_effnum9) );
		for( i = 0 ; i < 81 ; i++ ){
			int x = i % 9;
			int y = i / 9;
			for( j = 0 ; j <= 9 ; j++ ){
				c_king_effnum9[i] += u_king_effnum9[i][j];
				c_kingX_effnum9[x][j] += u_king_effnum9[i][j];
				c_kingY_effnum9[y][j] += u_king_effnum9[i][j];
			}
		}

		CumR( c_king_effnum9, king_effnum9, 10 );

		for( i = 0 ; i < 9 ; i++ ){
			CumR( c_kingX_effnum9[i], kingX_effnum9[i], 10 );
			CumR( c_kingY_effnum9[i], kingY_effnum9[i], 10 );
		}
	}
	void Symmetry(){
		// 左右対称
		// 継承したクラスでSymFuncを定義すること
		// Cumlateはこの後に呼ぶこと
		// CumlateRはこの前に呼ぶこと
		int x0/*, y0*/, d0;
		int x1,     d1;
		int x, y;
		int i, j;
		int a, b/*, c*/;

#define NEIGHBOR_SYM( s, a, a2, term )			\
SymFunc( s[0]a, s[2]a2 ); \
if(term){SymFunc( s[1]a, s[1]a2 );} \
SymFunc( s[2]a, s[0]a2 ); \
SymFunc( s[3]a, s[5]a2 ); \
if(term){SymFunc( s[4]a, s[4]a2 );} \
SymFunc( s[5]a, s[3]a2 ); \
SymFunc( s[6]a, s[8]a2 ); \
if(term){SymFunc( s[7]a, s[7]a2 );} \
SymFunc( s[8]a, s[6]a2 );

#define DIRECTION_SYM( s, term )					\
SymFunc( s[0], s[2] ); \
if(term){SymFunc( s[1], s[1] );} \
SymFunc( s[2], s[0] ); \
SymFunc( s[3], s[4] ); \
SymFunc( s[4], s[3] ); \
SymFunc( s[5], s[7] ); \
if(term){SymFunc( s[6], s[6] );} \
SymFunc( s[7], s[5] );

#define DIRECTION_SYM2( s, a, term )				\
SymFunc( s[0]a, s[2]a ); \
if(term){SymFunc( s[1]a, s[1]a );} \
SymFunc( s[2]a, s[0]a ); \
SymFunc( s[3]a, s[4]a ); \
SymFunc( s[4]a, s[3]a ); \
SymFunc( s[5]a, s[7]a ); \
if(term){SymFunc( s[6]a, s[6]a );} \
SymFunc( s[7]a, s[5]a );

#define DIRECTION_SYM3( s, s2, a, term )			\
SymFunc( s[0]a, s2[2]a ); \
if(term){SymFunc( s[1]a, s2[1]a );} \
SymFunc( s[2]a, s2[0]a ); \
SymFunc( s[3]a, s2[4]a ); \
SymFunc( s[4]a, s2[3]a ); \
SymFunc( s[5]a, s2[7]a ); \
if(term){SymFunc( s[6]a, s2[6]a );} \
SymFunc( s[7]a, s2[5]a );

		for( i = -8 ; i <= 8 ; i++ ){
			for( j = -8 ; j <= 8 ; j++ ){
				d0 = _diff2num[j][i];
				d1 = _diff2num[j][-i];
	
				if( d0 > d1 ){ continue; }

				for( a = 0 ; a < 18 ; a++ ){
					if( d0 != d1 ){
						// 玉-他の駒
						SymFunc( king_piece[d0][a], king_piece[d1][a] );
					}

					for( b = 0 ; b < 18 ; b++ ){
						// 王-隣接2枚
						if( i != 8 ){
							SymFunc( king_adjoin_r [d0][b][a], king_adjoin_r [d1-1][a][b] );
						}
						SymFunc( king_adjoin_rd[d0][b][a], king_adjoin_ld[d1][a][b] );
						if( d0 != d1 || a != b ){
							SymFunc( king_adjoin_d [d0][b][a], king_adjoin_d [d1][a][b] );
						}
						SymFunc( king_adjoin_ld[d0][b][a], king_adjoin_rd[d1][a][b] );
					}

					// 玉-桂馬の利き先
					SymFunc( kingS_ke_l[d0][a], kingS_ke_r[d1][a] );
					SymFunc( kingS_ke_r[d0][a], kingS_ke_l[d1][a] );
					SymFunc( kingG_ke_l[d0][a], kingG_ke_r[d1][a] );
					SymFunc( kingG_ke_r[d0][a], kingG_ke_l[d1][a] );
				}

				// X方向
				for( x0 = 0 ; x0 < 9 ; x0++ ){
					// 左右反転
					x1 = 8 - x0;

					if( d0 != d1 || x0 < x1 ){
						for( a = 0 ; a < 18 ; a++ ){
							// 玉X-他の駒
							SymFunc( kingX_piece[x0][d0][a], kingX_piece[x1][d1][a] );
						}
					}
				}

				// Y方向
				if( d0 != d1 ){
					for( y = 0 ; y < 9 ; y++ ){
						for( a = 0 ; a < 18 ; a++ ){
							// 玉Y-他の駒
							SymFunc( kingY_piece[y][d0][a], kingY_piece[y][d1][a] );
						}
					}
				}
	
				if( d0 != d1 ){
					for( a = 0 ; a < 33 ; a++ ){
						// 自由度
						SymFunc( free_ka_a[S][d0][a], free_ka_a[S][d1][a] );
						SymFunc( free_hi_a[S][d0][a], free_hi_a[S][d1][a] );
					}
				}
	
				// 自由度
				for( a = 0 ; a < 9 ; a++ ){
					if( d0 != d1 ){
						SymFunc( free_ky  [S][d0][a], free_ky  [S][d1][a] );
						SymFunc( free_hi_f[S][d0][a], free_hi_f[S][d1][a] );
						SymFunc( free_ky  [G][d0][a], free_ky  [G][d1][a] );
						SymFunc( free_hi_f[G][d0][a], free_hi_f[G][d1][a] );
					}

					SymFunc( free_ka_l[S][d0][a], free_ka_r[S][d1][a] );
					SymFunc( free_ka_r[S][d0][a], free_ka_l[S][d1][a] );
					SymFunc( free_ka_L[S][d0][a], free_ka_R[S][d1][a] );
					SymFunc( free_ka_R[S][d0][a], free_ka_L[S][d1][a] );
					SymFunc( free_hi_l[S][d0][a], free_hi_r[S][d1][a] );
					SymFunc( free_hi_r[S][d0][a], free_hi_l[S][d1][a] );
					SymFunc( free_hi_b[S][d0][a], free_hi_b[S][d1][a] );
	
					SymFunc( free_ka_l[G][d0][a], free_ka_r[G][d1][a] );
					SymFunc( free_ka_r[G][d0][a], free_ka_l[G][d1][a] );
					SymFunc( free_ka_L[G][d0][a], free_ka_R[G][d1][a] );
					SymFunc( free_ka_R[G][d0][a], free_ka_L[G][d1][a] );
					SymFunc( free_hi_l[G][d0][a], free_hi_r[G][d1][a] );
					SymFunc( free_hi_r[G][d0][a], free_hi_l[G][d1][a] );
					SymFunc( free_hi_b[G][d0][a], free_hi_b[G][d1][a] );
				}

				// 利き
				for( a = 0 ; a < 20 ; a++ ){
					if( d0 != d1 ){
						SymFunc( effect_ky  [S][d0][a], effect_ky  [S][d1][a] );
						SymFunc( effect_hi_f[S][d0][a], effect_hi_f[S][d1][a] );
						SymFunc( effect_ky  [G][d0][a], effect_ky  [G][d1][a] );
						SymFunc( effect_hi_f[G][d0][a], effect_hi_f[G][d1][a] );
					}

					SymFunc( effect_ka_l[S][d0][a], effect_ka_r[S][d1][a] );
					SymFunc( effect_ka_r[S][d0][a], effect_ka_l[S][d1][a] );
					SymFunc( effect_ka_L[S][d0][a], effect_ka_R[S][d1][a] );
					SymFunc( effect_ka_R[S][d0][a], effect_ka_L[S][d1][a] );
					SymFunc( effect_hi_l[S][d0][a], effect_hi_r[S][d1][a] );
					SymFunc( effect_hi_r[S][d0][a], effect_hi_l[S][d1][a] );
					SymFunc( effect_hi_b[S][d0][a], effect_hi_b[S][d1][a] );

					SymFunc( effect_ka_l[G][d0][a], effect_ka_r[G][d1][a] );
					SymFunc( effect_ka_r[G][d0][a], effect_ka_l[G][d1][a] );
					SymFunc( effect_ka_L[G][d0][a], effect_ka_R[G][d1][a] );
					SymFunc( effect_ka_R[G][d0][a], effect_ka_L[G][d1][a] );
					SymFunc( effect_hi_l[G][d0][a], effect_hi_r[G][d1][a] );
					SymFunc( effect_hi_r[G][d0][a], effect_hi_l[G][d1][a] );
					SymFunc( effect_hi_b[G][d0][a], effect_hi_b[G][d1][a] );
				}
	
#if 1
				// 歩の2マス前の駒
				if( d0 != d1 ){
					for( a = 0 ; a < 20 ; a++ ){
						SymFunc( front2_sfu[d0][a], front2_sfu[d1][a] );
						SymFunc( front2_gfu[d0][a], front2_gfu[d1][a] );
					}
				}
#endif

				for( a = 0 ; a < 18 ; a++ ){
					for( b = 0 ; b < 20 ; b++ ){
						// 王-8近傍-その他
						NEIGHBOR_SYM( king8_piece, [a][d0][b], [a][d1][b], d0 != d1 );
					}
				}
			}
		}

		// 玉の周囲8マスでの利きの勝ち負け
		DIRECTION_SYM( king_effwin, false );

		// 歩を打てる筋
		for( x = 0 ; x < 4 ; x++ ){
			SymFunc( fu_file[x], fu_file[8-x] );
		} 

		for( x0 = 0 ; x0 < 81 ; x0++ ){
			// 左右反転
			x1 = _sym[x0];
			if( x0 > x1 ){ continue; }

			if( x0 != x1 ){
				// 持ち駒
				for( a = 0 ; a < 19 ; a++ ){
					SymFunc( king_hand_sfu[x0][a], king_hand_sfu[x1][a] );
					SymFunc( king_hand_gfu[x0][a], king_hand_gfu[x1][a] );
				}
				for( a = 0 ; a < 5 ; a++ ){
					SymFunc( king_hand_sky[x0][a], king_hand_sky[x1][a] );
					SymFunc( king_hand_ske[x0][a], king_hand_ske[x1][a] );
					SymFunc( king_hand_sgi[x0][a], king_hand_sgi[x1][a] );
					SymFunc( king_hand_ski[x0][a], king_hand_ski[x1][a] );
					SymFunc( king_hand_gky[x0][a], king_hand_gky[x1][a] );
					SymFunc( king_hand_gke[x0][a], king_hand_gke[x1][a] );
					SymFunc( king_hand_ggi[x0][a], king_hand_ggi[x1][a] );
					SymFunc( king_hand_gki[x0][a], king_hand_gki[x1][a] );
				}
				for( a = 0 ; a < 3 ; a++ ){
					SymFunc( king_hand_ska[x0][a], king_hand_ska[x1][a] );
					SymFunc( king_hand_shi[x0][a], king_hand_shi[x1][a] );
					SymFunc( king_hand_gka[x0][a], king_hand_gka[x1][a] );
					SymFunc( king_hand_ghi[x0][a], king_hand_ghi[x1][a] );
				}
			}

#if 1
			// 歩を打てる筋
			for( x = 0 ; x < 9 ; x++ ){
				if( x0 != x1 || x != 4 ){
					SymFunc( sfu_file[x0][x], sfu_file[x1][8-x] );
					SymFunc( gfu_file[x0][x], gfu_file[x1][8-x] );
				}
			}
#endif

			if( x0 != x1 ){
#if 1
				// 歩打ちで王手できるか
				SymFunc( king_check_fu[x0], king_check_fu[x1] );
#endif

				SymFunc( king_addr[x0], king_addr[x1] );
			}
		}
	}
};

/************************************************
Param 評価パラメータ
************************************************/
class Param : public TempParam<float,double>{
public:
	Param(){
		assert( &c_hand_fu[0] - &hand_fu[0] == PARAM_NUM );
		Init0();
	}
	~Param(){
	}
	void Init0();
	int ReadData( const char* fname );
	int WriteData( const char* fname );
	Param& operator+=( Param& param );
	Param& operator-=( Param& param );
	Param& operator*=( double d );
	Param& operator=( Param& param );
	Param& operator=( Evaluate& eval );

protected:
	void SymFunc( float& a, float& b ){
		a = b = a + b;
	}
};

/************************************************
Evaluate 局面評価クラス
************************************************/
// 評価パラメータ保存ファイル
#define FNAME_EVDATA		"evdata"

#define EV_TABLE_DEBUG			0
#if EV_TABLE_DEBUG
#	include <fstream>
#endif

struct EVAL_TABLE{
	uint64 hash; // ハッシュ値
	bool entry;  // 登録
	int value;   // 評価値
};

class Evaluate : public TempParam<short,short>, public HashHandler{
private:
	short piece2[RY];       // 交換値
	short promote[RY];      // 成った時の変化
	EVAL_TABLE* eval_table; // ハッシュテーブル
	string fname;           // ファイル名
	double deviation;       // 偏差(乱数合議用)
	RandNormal rand_norm;   // 正規乱数生成器
#if EV_TABLE_DEBUG
	ofstream edebug;
#endif

private:
	int _ReadData( const char* _fname );
	int _WriteData( const char* _fname );
	int GetDegreeOfFreedomS( int& v, const KOMA* ban,
		const int diffS, const int diffG,
		int addr, const int dir,
		const short free[][289][9],
		const short effect[][289][20]
		) const
	{
		// 自由度と利きの計算
		KOMA koma;
		int cnt = 0;
		addr += dir;
		while( ( koma = ban[addr] ) != WALL ){
			if( koma & SENTE ){
				v += effect[S][diffS][sente_num[koma]];
				v -= effect[G][diffG][ gote_num[koma]];
				break;
			}
			cnt++;
			if( koma != EMPTY ){
				v += effect[S][diffS][sente_num[koma]];
				v -= effect[G][diffG][ gote_num[koma]];
				break;
			}
			addr += dir;
		}
		v += free[S][diffS][cnt];
		v -= free[G][diffG][cnt];

		return cnt;
	}
	int GetDegreeOfFreedomG( int& v, const KOMA* ban,
		const int diffS, const int diffG,
		int addr, const int dir,
		const short free[][289][9],
		const short effect[][289][20]
		) const
	{
		// 自由度と利きの計算
		KOMA koma;
		int cnt = 0;
		addr += dir;
		while( ( koma = ban[addr] ) != WALL ){
			if( koma & GOTE ){
				v += effect[G][diffS][sente_num[koma]];
				v -= effect[S][diffG][ gote_num[koma]];
				break;
			}
			cnt++;
			if( koma != EMPTY ){
				v += effect[G][diffS][sente_num[koma]];
				v -= effect[S][diffG][ gote_num[koma]];
				break;
			}
			addr += dir;
		}
		v += free[G][diffS][cnt];
		v -= free[S][diffG][cnt];

		return cnt;
	}
	int _GetValue1_dis1_ns( ShogiEval* __restrict__ pshogi ) const;

protected:
	void SymFunc( short& a, short& b ){
		a = b;
	}

public:
	Evaluate( bool load = true){
#if !defined(NDEBUG) && 0
		cout << "********** Evaluate() was called! **********\n";
		cout << (uint64)(this) << '\n';
#endif
		assert( &c_hand_fu[0] - &hand_fu[0] == PARAM_NUM );

		Hash::SetHandler( this );
		eval_table = new EVAL_TABLE[Hash::Size()];

		Init();
		if( load ){
			ReadData( FNAME_EVDATA );
		}
	}
	Evaluate( const char* _fname, bool load = true ){
#if !defined(NDEBUG) && 0
		cout << "********** Evaluate( _fname ) was called! **********\n";
		cout << (uint64)(this) << '\n';
#endif
		assert( &c_hand_fu[0] - &hand_fu[0] == PARAM_NUM );

		Hash::SetHandler( this );
		eval_table = new EVAL_TABLE[Hash::Size()];

		Init();
		if( load && _fname != NULL ){
			ReadData( _fname );
		}
	}
	virtual ~Evaluate(){
#if !defined(NDEBUG) && 0
		cout << "********** ~Evaluate() was called! **********\n";
		cout << (uint64)(this) << '\n';
#endif
		Hash::UnsetHandler( this );
		delete [] eval_table;
	}

	void Resize(){
		delete [] eval_table;
		eval_table = new EVAL_TABLE[Hash::Size()];
		InitHashTable();
	}
	void Init();
	void Update(){
		SetPiece2();
		SetPromote();
		Cumulate();
		InitHashTable();
	}
	void InitHashTable(){
		memset( eval_table, 0, sizeof(EVAL_TABLE) * Hash::Size() );
	}
	void SetDeviation( double _deviation ){
		deviation = _deviation;
	}
	double GetDeviation(){
		return deviation;
	}
	void SetPiece2(){
		int i;
		for( i = 0 ; i < OU ; i++ ){
			piece2[i] = piece[i] * 2;
		}
		for( ; i < RY ; i++ ){
			piece2[i] = piece[i] + piece[i-NARI];
		}
	}
	void SetPromote(){
		int i;
		memset( promote, 0, sizeof(promote) );
		for( i = FU - FU ; i < GI ; i++ ){
			promote[i] = piece[i+NARI] - piece[i];
		}
		for( i = KA - FU ; i < HI ; i++ ){
			promote[i] = piece[i+NARI] - piece[i];
		}
	}
	int ReadData( const char* _fname = NULL ){
		if( _fname == NULL )
			return _ReadData( fname.c_str() );
		else{
			fname = _fname;
			return _ReadData( _fname );
		}
	}
	int WriteData( const char* _fname = NULL ){
		if( _fname == NULL )
			return _WriteData( fname.c_str() );
		else{
			fname = _fname;
			return _WriteData( _fname );
		}
	}
	int MakeCopy( const char* _fname ){
		return _WriteData( _fname );
	}

	int GetValue0( ShogiEval* pshogi ) const;
	int GetValue1( ShogiEval* pshogi ){
		return GetValue1_ns( pshogi ) / (int)EV_SCALE;
	}
	int GetValue1_ns( ShogiEval* pshogi ){
		return GetValue1_dis1_ns( pshogi ) + GetValue1_dis2_ns( pshogi );
	}
	int GetValue1_dis1( ShogiEval* __restrict__ pshogi ){
		return GetValue1_dis1_ns( pshogi ) / (int)EV_SCALE;
	}
	int GetValue1_dis1_ns( ShogiEval* __restrict__ pshogi );
	int GetValue1_dis2( ShogiEval* __restrict__ pshogi ) const{
		return GetValue1_dis2_ns( pshogi ) / (int)EV_SCALE;
	}
	int GetValue1_dis2_ns( ShogiEval* __restrict__ pshogi ) const;
	int GetValue( ShogiEval* pshogi ){
		return GetValue0( pshogi ) + GetValue1( pshogi );
	}
	friend void EvaluateTest( const char* fname );

	// 差分計算用
	struct ELEMENTS{
		int material;  // 駒割り
		int pos_sou;   // 先手玉に関連する評価
		int pos_gou;   // 後手玉に関連する評価
		int pos_ou;    // 両方の玉に関連する評価
		int list_s[8]; // 先手玉周りの駒リスト
		int list_snum; // 先手玉周りの駒数
		int list_g[8]; // 後手玉周りの駒リスト
		int list_gnum; // 後手玉周りの駒数
	};
	template<KOMA ou>
	void GenerateListOU( ELEMENTS* el, ShogiEval* pshogi );
	void GetValue1_dis2_ns( ELEMENTS* __restrict__ el, ShogiEval* __restrict__ pshogi ) const;
	template <int sign>
	void GetValueDiff( ELEMENTS* __restrict__ el, ShogiEval* __restrict__ pshogi, int addr );
	void GetValueDiffSOU( ELEMENTS* __restrict__ el, ShogiEval* pshogi );
	void GetValueDiffGOU( ELEMENTS* __restrict__ el, ShogiEval* pshogi );

	int GetV_BAN( KOMA koma ) const{
		if( koma & SENTE )
			return (int)piece[koma-SFU];
		else if( koma & GOTE )
			return -(int)piece[koma-GFU];
		return 0;
	}
	int GetV_BAN2( KOMA koma ) const{
		if( koma & SENTE )
			return (int)piece2[koma-SFU];
		else if( koma & GOTE )
			return -(int)piece2[koma-GFU];
		return 0;
	}
	int GetV_Promote( KOMA koma ) const{
		if( koma & SENTE )
			return (int)promote[koma-SFU];
		else if( koma & GOTE )
			return -(int)promote[koma-GFU];
		return 0;
	}
	int GetV_PromoteN( KOMA koma ) const{
		if( koma & SENTE )
			return (int)promote[koma-SFU];
		else if( koma & GOTE )
			return (int)promote[koma-GFU];
		return 0;
	}
	int GetV_KingPiece( int diff, int kn ){
		return king_piece[diff][kn];
	}
	int GetV_KingXPiece( int x, int diff, int kn ){
		return kingX_piece[x][diff][kn];
	}
	int GetV_KingYPiece( int y, int diff, int kn ){
		return kingY_piece[y][diff][kn];
	}
	int GetV_PARAM1( int i ) const{
		return (int)piece[i];
	}
	int GetV_PARAM2( int i ) const{
		return (int)PARAM[i];
	}
	void Add1( int i, int a  ){
		piece[i] += (short)a;
	}
	void Add2( int i, int a  ){
		PARAM[i] += (short)a;
	}
	const TempParam<short,short>& GetV_PARAM() const{
		return *((TempParam<short,short>*)PARAM);
	}
	Evaluate& operator=( Param& param );
};

class EvaluateNull : public Evaluate{
public:
	EvaluateNull( bool load = true ) : Evaluate( NULL, load ){
#if !defined(NDEBUG) && 0
		cout << "********** EvaluateNull() was called! **********\n";
		cout << (uint64)(this) << '\n';
#endif
	}
	virtual ~EvaluateNull(){
#if !defined(NDEBUG) && 0
		cout << "********** ~EvaluateNull() was called! **********\n";
		cout << (uint64)(this) << '\n';
#endif
	}
};

/************************************************
持ち時間管理クラス
************************************************/
class AvailableTime{
private:
	unsigned turn;         // 手番

	double available[2];   // 持ち時間(秒)
	double readoff[2];     // 秒読み(秒)

	unsigned num;          // 手数
	double used[MAX_KIFU]; // 1手毎の消費時間(秒)
	double used_all[2];    // 累計消費時間(秒)

	double remain[2];      // 残り時間(秒)

public:
	enum{
		S = 0x0, G = 0x1,
		TURN_CHANGE = 0x1,
	};
	AvailableTime(){
		Reset( 0.0, 10.0 );
	}
	AvailableTime( double sec_a, double sec_r = 0.0 ){
		Reset( sec_a, sec_r );
	}
	virtual ~AvailableTime(){
	}
	// 初期化
	void Reset( double sec_a, double sec_r ){
		turn = S;
		num = 0;
		if( sec_a < 0.0 ){ sec_a = 0.0; }
		available[S] = available[G] = sec_a;
		remain[S] = remain[G] = sec_a;
		if( sec_r < 0.0 ){ sec_r = 0.0; }
		readoff[S] = readoff[G] = sec_r;
		used_all[S] = used_all[G] = 0;
	}
	// 手番
	unsigned GetTurn() const{
		return turn;
	}
	// 持ち時間
	double GetAvailableTime() const{
		return available[turn];
	}
	double GetAvailableTimeP() const{
		return available[turn^TURN_CHANGE];
	}
	double GetAvailableTime( unsigned t ) const{
		return available[t];
	}
	// 累計消費時間
	double GetCumUsedTime() const{
		return used_all[turn];
	}
	double GetCumUsedTimeP() const{
		return used_all[turn^TURN_CHANGE];
	}
	double GetCumUsedTime( unsigned t ) const{
		return used_all[t];
	}
	// 総手数
	int GetNumber() const{
		return num;
	}
	// 1手毎の消費時間
	double GetUsedTime( unsigned i ) const{
		if( i >= num ){ return 0; }
		return used[i];
	}
	double operator[]( unsigned i ){
		if( i >= num ){ return 0; }
		return used[i];
	}
	// 残り時間
	double GetRemainingTime() const{
		return remain[turn];
	}
	double GetRemainingTimeP() const{
		return remain[turn^TURN_CHANGE];
	}
	double GetRemainingTime( unsigned t ) const{
		return remain[t];
	}
	// 秒読み
	double GetReadingOffSeconds() const{
		return readoff[turn];
	}
	double GetReadingOffSecondsP() const{
		return readoff[turn^TURN_CHANGE];
	}
	double GetReadingOffSeconds( unsigned t ) const{
		return readoff[t];
	}
	// 次の1手における最長時間(=残り時間+秒読み)
	double GetUsableTime() const{
		return remain[turn] + readoff[turn];
	}
	double GetUsableTimeP() const{
		return remain[turn^TURN_CHANGE] + readoff[turn^TURN_CHANGE];
	}
	double GetUsableTime( unsigned t ) const{
		return remain[t] + readoff[t];
	}
	// 時間制限の有無
	bool IsValidSetting() const{
		return IsValidSetting( turn );
	}
	bool IsValidSettingP() const{
		return IsValidSetting( turn^TURN_CHANGE );
	}
	bool IsValidSetting( unsigned t ) const{
		if( available[t] == 0.0 && readoff[t] == 0.0 ){
			return false;
		}else{ return true; }
	}
	// 時間を消費
	double Reduce( double sec ){
		double ret;
		used[num++] = sec;
		used_all[turn] += sec;
		remain[turn] -= sec;
		if( remain[turn] < 0 ){ remain[turn] = 0; }
		ret = remain[turn];
		turn ^= TURN_CHANGE;
		return ret;
	}
};

/************************************************
駒と評価値のリスト
************************************************/
struct VLIST{
	KOMA koma;
	int value;
};

/************************************************
指し手の表現
************************************************/
class MOVE{
public:
	int from;      // 移動元
	int to;        // 移動先
	int nari;      // 成り・不成
	KOMA koma;     // 動かした駒
	union{
		int value; // 並べ替えのための評価値
		struct{
			int pn;    // 証明数
			int dn;        // 反証数
		};
	};

public:
	unsigned int Export() const{
	return ( to   & 0xff   )
	   + ( ( from & 0xffff ) << 8  )
	   +   ( nari            << 24 );
	}
	void Import( unsigned int m ){
		to   =   m         & 0xff;
		from = ( m >>  8 ) & 0xffff;
		nari = ( m >> 24 );
	}
	bool IsSame( const MOVE& move ) const{
		if( to   == move.to   &&
			from == move.from &&
			nari == move.nari )
		{
			return true;
		}
		return false;
	}
};

class Moves{
private:
	int num;
	MOVE* moves;
public:
	Moves(){
		moves = new MOVE[MAX_MOVES];
		num = 0;
	}
	Moves( int n ){
		moves = new MOVE[n];
		num = 0;
	}
	virtual ~Moves(){
		delete [] moves;
	}
	virtual void Init(){
		num = 0;
	}
	int GetNumber(){
		return num;
	}
	void Add( MOVE& move ){
		moves[num++] = move;
	}
	void Add( int from, int to, int nari, KOMA koma, int value = 0 ){
		moves[num].from  = from;
		moves[num].to    = to;
		moves[num].nari  = nari;
		moves[num].koma  = koma;
		moves[num].value = value;
		num++;
	}
	int Remove( int idx ){
		if( idx < num ){
			if( idx < num - 1 ){
				moves[idx] = moves[num-1];
			}
			num--;
			return 1;
		}
		return 0;
	}
	int Remove( MOVE& move ){
		int i;
		for( i = 0 ; i < num ; i++ ){
			if( move.IsSame( moves[i] ) ){
				Remove( i );
				return 1;
			}
		}
		return 0;
	}
	int RemoveAfter( int idx ){
		if( idx < num ){
			num = idx;
		}
		return 0;
	}
	static int MoveComp( const void* m1, const void* m2 ){
		return ((MOVE*)m2)->value - ((MOVE*)m1)->value;
	}
	void Sort(){
		qsort( moves, num, sizeof(MOVE), MoveComp );
	}
	void Sort( int idx ){
		if( idx >= 0 && idx < num )
			qsort( moves + idx, num - idx, sizeof(MOVE), MoveComp );
	}
	void Sort( int idx, int n ){
		if( idx >= 0 && n > 0 && idx + n <= num )
			qsort( moves + idx, n, sizeof(MOVE), MoveComp );
	}
	void StableSort( int idx = 0 ){
		StableSort( idx, num - idx );
	}
	void StableSort( int idx, int n ){
		if( idx >= 0 && n > 0 && idx + n <= num ){
			int end = idx + n;
			int i, j;
			for( i = idx + 1 ; i < end ; i++ ){
				if( moves[i-1].value < moves[i].value ){
					MOVE temp = moves[i];
					moves[i] = moves[i-1];
					for( j = i-1 ; j > idx && moves[j-1].value < temp.value ; j-- ){
						moves[j] = moves[j-1];
					}
					moves[j] = temp;
				}
			}
		}
	}
	MOVE& GetMove( int idx ){
		return moves[idx];
	}
	MOVE& operator[]( int idx ){
		return moves[idx];
	}
	void SetValue( int idx, int value ){
		moves[idx].value = value;
	}
	int CopyTo( Moves& dst ){
		memcpy( dst.moves, moves, sizeof(MOVE) * num );
		dst.num = num;
		return num;
	}
	int CopyTo( MOVE dst[] ){
		memcpy( dst, moves, sizeof(MOVE) * num );
		return num;
	}
	void CopyFrom( int n, MOVE src[] ){
		num = n;
		memcpy( moves, src, sizeof(MOVE) * num );
	}
	Moves& operator=( Moves& m ){
		num = m.num;
		memcpy( moves, m.moves, sizeof(MOVE) * num );
		return (*this);
	}
	void Shuffle(){
		int i, j;
		MOVE temp;

		for( i = num - 1 ; i > 0 ; i-- ){
			j = (int)( gen_rand32() % ( i + 1 ) );
			if( i != j ){
				temp = moves[i];
				moves[i] = moves[j];
				moves[j] = temp;
			}
		}
	}
};

/************************************************
千日手
************************************************/
enum REP_TYPE{
	NO_REP = 0, // Non Repetition
	IS_REP,     // Four Fold Repetition
	PC_WIN,     // Perpitual Check (Win)
	PC_LOS,     // Perpitual Check (Lose)
	ST_REP,     // Short Repetition
};

/************************************************
局面の変更箇所
************************************************/
enum{
	CHANGE_FROM = 0x01,
	CHANGE_TO   = 0x02,
	CHANGE_DAI  = 0x04,
	CHANGE_OU   = 0x08,
};

struct CHANGE{
	unsigned int flag;
	unsigned int* pfrom;
	unsigned int from;
	unsigned int* pto;
	unsigned int to;
	unsigned int to2;
	unsigned int* pdai;
	unsigned int dai;
	unsigned int* pou;
	unsigned int ou;
};

/************************************************
日時
************************************************/
class DateTime{
public:
	int year;
	int month;
	int day;
	int hour;
	int min;
	int sec;

public:
	DateTime(){
		init();
	}
	void init(){
		set( 1900, 1, 1, 0, 0, 0 );
	}
	void set( int y, int mt, int d, int h, int mn, int s ){
		year = y; month = mt; day = d;
		hour = h; min   = mn; sec = s;
	}
	void copy( DateTime& dt ){
		year  = dt.year;
		month = dt.month;
		day   = dt.day;
		hour  = dt.hour;
		min   = dt.min;
		sec   = dt.sec;
	}
	DateTime& operator=( DateTime& dt ){
		copy( dt );
		return (*this);
	}
	DateTime& setNow(){
		time_t t = time( NULL );
		struct tm* tmDate = localtime( &t );
		year  = 1900 + tmDate->tm_year;
		month = tmDate->tm_mon + 1;
		day   = tmDate->tm_mday;
		hour  = tmDate->tm_hour;
		min   = tmDate->tm_min;
		sec   = tmDate->tm_sec;
		return (*this);
	}
	unsigned export_date(){
		return ( year * 12 + (month-1) ) * 31 + (day-1);
	}
	unsigned export_time(){
		return ( hour * 60 + min ) * 60 + sec;
	}
	DateTime& import( unsigned uDate, unsigned uTime ){
		year  =   uDate / (12*31);
		month = ( uDate % (12*31) ) / 31 + 1;
		day   =   uDate             % 31 + 1;
		hour  =   uTime / (60*60);
		min   = ( uTime % (60*60) ) / 60;
		sec   =   uTime             % 60;
		return (*this);
	}
	bool isLegal(){
		if( year >= 1900 && month >= 1 && month <= 12 &&
			day >= 1 && day <= 31 && hour >= 0 && hour <= 23 &&
			min >= 0 && min <= 59 && sec >= 0 && sec <= 59
		){
			return true;
		}
		else{
			return false;
		}
	}
	void toString( char* str ){
		sprintf( str, "%04d/%02d/%02d %02d:%02d:%02d",
			year, month, day, hour, min, sec );
	}
};

/************************************************
対局情報
************************************************/
struct KIFU_INFO{
	string event;   // 対局名
	string sente;   // 先手の対局者名
	string gote;    // 後手の対局者名
	DateTime start; // 開始日時
	DateTime end;   // 終了日時
	KIFU_INFO(){
		init();
	}
	void init(){
		event   = "";
		sente   = "";
		gote    = "";
		start.init();
		end.init();
	}
};

/************************************************
Shogi 局面と棋譜を扱うクラス
************************************************/
enum{
	MOVE_CAPTURE   = 0x01,
	MOVE_NOCAPTURE = 0x02,
	MOVE_PROMOTE   = 0x04,
	MOVE_NOPROMOTE = 0x08,
	MOVE_KING      = 0x10,
	MOVE_CHECK     = 0x20, // 追加(Oct. 11, 2010)
	MOVE_NCHECK    = 0x40, // 追加(Mar. 17, 2011)
	MOVE_ALL       = MOVE_CAPTURE |
		MOVE_NOCAPTURE | MOVE_PROMOTE |
		MOVE_NOPROMOTE | MOVE_KING, // 追加(Mar. 6, 2011)
};

struct GEN_INFO{
	int ou;            // 自玉の位置
	int ou2;           // 適玉の位置
	int check;         // 王手の方向
	int cnt;           // 王手している駒までの距離
	uint96 pin;        // ピン
	int dir_p;         // ピンの方向
	int addr;          // 着目している駒の座標
	KOMA koma;         // 着目している駒
	unsigned int flag; // 生成する指し手の種類
};

// 駒の移動可能方向
enum{
	FLAG_FU = 1 << ( FU - FU ),
	FLAG_KY = 1 << ( KY - FU ),
	FLAG_KE	= 1 << ( KE - FU ),
	FLAG_GI = 1 << ( GI - FU ),
	FLAG_KI = 1 << ( KI - FU ),
	FLAG_KA = 1 << ( KA - FU ),
	FLAG_HI = 1 << ( HI - FU ),
	FLAG_UM = 1 << ( UM - FU ),
	FLAG_RY = 1 << ( RY - FU ),
	FLAG_OU = 1 << ( OU - FU ),

	FLAG_FRONT  = FLAG_FU | FLAG_KY | FLAG_GI | FLAG_KI | FLAG_HI | FLAG_UM | FLAG_RY | FLAG_OU,
	FLAG_FRONT2 = FLAG_KY | FLAG_HI | FLAG_RY,
	FLAG_BISHOP = FLAG_KE,
	FLAG_DIAG = FLAG_GI | FLAG_KI | FLAG_KA | FLAG_UM | FLAG_RY | FLAG_OU,
	FLAG_DIAG2 = FLAG_KA | FLAG_UM,
	FLAG_SIDE = FLAG_KI | FLAG_HI | FLAG_UM | FLAG_RY | FLAG_OU,
	FLAG_SIDE2 = FLAG_HI | FLAG_RY,
	FLAG_DIAGB = FLAG_GI | FLAG_KA | FLAG_UM | FLAG_RY | FLAG_OU,
	FLAG_DIAGB2 = FLAG_KA | FLAG_UM,
	FLAG_BACK = FLAG_KI | FLAG_HI | FLAG_UM | FLAG_RY | FLAG_OU,
	FLAG_BACK2 = FLAG_HI | FLAG_RY,
};

#define BAN_OFFSET			17

class Shogi : public ConstData{
protected:
	// メンバ変数を変更したらCopy()の書き換えも忘れずに!!
	KOMA _ban[16*13];             // 現在の盤面
	KOMA* ban;
	int dai[64];                  // 現在の持駒
	uint96 bb[GRY+1];             // bitboard
	uint96 bb_a;                  // bitboard(all)

	int sou;                      // 先手玉の位置
	int gou;                      // 後手玉の位置

	// 棋譜
	int      knum;                // 全手数
	int      know;                // 現在の手数
	KOMA ksengo;                  // 手番
	struct KIFU{
		MOVE     move;           // 指し手
		int      check;          // 王手かどうか(IsRepetitionで使用)
		int      chdir;          // 王手のかかっている方向
		int      cnt;            // 王手をかけている駒までの距離
		uint96   pin;            // ピン
		uint64   hashB;          // ハッシュ値(盤と手番)
		uint64   hashD;          // ハッシュ値(駒台)
		CHANGE   change;          // 局面の変更箇所
		unsigned sfu;            // 筋毎の歩の有無(先手)
		unsigned gfu;            // 筋毎の歩の有無(後手)
	};
	KIFU kifu[MAX_KIFU+1];

	Hash* phash;                  // ハッシュ
	bool halloc;                  // Hashを自分がnewしたのか

	unsigned int _effectS[16*13]; // 先手の利き情報
	unsigned int _effectG[16*13]; // 後手の利き情報
	unsigned int* effectS;
	unsigned int* effectG;

	// shogi.cpp
protected:
	virtual void UpdateBan();
	int MoveError( KOMA koma, int to );
	int IsCheckNeglect( int addr, int dir, int to, int cnt );
	int IsCheckNeglect( int addr, int to, int from );
	int IsCheckNeglect( int dir );
	int IsCheck( unsigned int effect[], int addr, int* pcnt, uint96* ppin );
	int IsCheck( int* pcnt, uint96* ppin );
	int IsCheckMove( int addr, int to, KOMA koma );
	int IsDiscCheckMove( int addr, int to, int from );
	int IsUchifu( int to );
	template<bool handler>
	int MoveD( const MOVE& move );
	template<bool handler>
	int Move( MOVE& move );
	template<bool handler>
	int NullMove();
	template<bool handler>
	KOMA GoNext( const MOVE& __restrict__ move );
	virtual void GoNextHandler(){
	}
	virtual void GoNextRemove( KOMA ){
	}
	virtual void GoNextAdd( KOMA ){
	}
	virtual void GoNextFrom( int ){
	}
	virtual void GoNextTo( int ){
	}
public:
	Shogi();
	Shogi( const KOMA* _ban0, const int* dai0, KOMA sengo );
	Shogi( TEAI teai );
	Shogi( const Shogi& org );
	virtual ~Shogi();
	static unsigned int CountEffect( unsigned int x ){
		// 20ビットだけなら分割統治より速い。
		return _bnum[x&0x3FFU] + _bnum[(x>>10)&0x3FFU];
	}
	static unsigned int CountEffectF( unsigned int x ){
		return _bnum[(x>>12)&0x3FFU];
	}
	static unsigned int CountEffectL( unsigned int x ){
		return _bnum[x&0x3FFU] + _bnum[(x>>10)&0x003U];
	}
	void SetBan( const KOMA* _ban0, const int* dai0, KOMA sengo );
	void SetBan( TEAI teai );
	void SetBanEmpty();
	void QuickCopy( const Shogi& org );
	virtual void Copy( const Shogi& org );
	KOMA GetBan( int addr ) const{
		return ban[addr];
	}
	KOMA GetBan( int x, int y ) const{
		return ban[BanAddr(x,y)];
	}
	const KOMA* GetBan() const{
		return ban;
	}
	const KOMA* _GetBan() const{
		return _ban;
	}
	const int* GetDai() const{
		return dai;
	}
	const uint96* GetBB() const{
		return bb;
	}
	uint96 GetBB_All() const{
		return bb_a;
	}
	unsigned GetSFU() const{
		return kifu[know].sfu;
	}
	unsigned GetGFU() const{
		return kifu[know].gfu;
	}
	int GetNumber() const{
		return know;
	}
	int GetTotal() const{
		return knum;
	}
	KOMA GetSengo() const{
		return ksengo;
	}
	KOMA GetSengo( int n ) const{
		if( ( know - n ) % 2 == 0 ){
			return ksengo;
		}
		else{
			return ksengo ^ SENGO;
		}
	}
	bool GetMove( MOVE& move ) const{
		if( know > 0 ){
			move = kifu[know-1].move;
			return true;
		}
		else
			return false;
	}
	bool GetNextMove( MOVE& move ) const{
		if( know < knum ){
			move = kifu[know].move;
			return true;
		}
		else
			return false;
	}
	bool GetMove( int num, MOVE& move ) const{
		if( num < knum ){
			move = kifu[num].move;
			return true;
		}
		else
			return false;
	}
	int GetSOU() const{
		return sou;
	}
	int GetGOU() const{
		return gou;
	}
	static bool Narazu( KOMA koma, int to ){
		// 不成の手を生成する意味があるか
		return koma == SKI || koma == GKI ||
			// 初期からのバグ(修正..Mar. 7, 2011)
			// koma == SGI || koma == SGI ||
			koma == SGI || koma == GGI ||
			koma == SKE || koma == GKE ||
			( koma == SKY && to >= BanAddr( 1, 3 ) ) ||
			( koma == GKY && to <= BanAddr( 9, 7 ) );
	}
	int GetCheckDir();
	bool IsCheck(){
		if( ksengo == SENTE ){
			if( effectG[sou] != 0U ){ return true; }
		}
		else{
			if( effectS[gou] != 0U ){ return true; }
		}
		return false;
	}
	bool IsReply(){
		// 不正確
		// 1手前にIsCheckが呼ばれなければNOJUDGE_CHECKになっている。
		if( know > 0 && kifu[know-1].chdir != NOJUDGE_CHECK ){
			return ( kifu[know-1].chdir != 0 );
		}
		return false;
	}
	uint96 GetPin(){
		if( kifu[know].chdir == NOJUDGE_CHECK ){
			kifu[know].chdir = IsCheck( &kifu[know].cnt, &kifu[know].pin );
		}
		return kifu[know].pin;
	}
	bool IsCheckMove( const MOVE& move ){
		// moveが王手かどうか
		KOMA sengo = ksengo;
		int ou = ( sengo == SENTE ? gou : sou );
		if( ou == 0 ){ return false; }
		if( IsCheckMove( ou, move.to, move.koma + ( move.nari ? NARI : 0 ) ) ||
			IsDiscCheckMove( ou, move.to, move.from ) )
		{
			return true;
		}
		return false;
	}
	bool IsKingMove() const{
		// 王様を動かしたか
		register int koma;
		if( know > 0 && ( ( koma = kifu[know-1].move.koma ) == SOU || koma == GOU ) ){
			return true;
		}
		return false;
	}
	bool IsCapture( MOVE& move ) const{
		// 駒を取る手か　
		if( ban[move.to] != EMPTY ){
			return true;
		}
		return false;
	}
	bool IsTacticalMove( MOVE& move ) const{
		// 駒を取る手か銀以外の駒が成る手
		if( know > 0 && ( ban[move.to] != EMPTY ||
			( move.nari && ( move.koma & ~SENGO ) != GI ) ) )
		{
			return true;
		}
		return false;
	}
	bool IsTactical() const{
		// 駒を取る手か銀以外の駒が成る手
		if( know > 0 && ( kifu[know-1].change.to != EMPTY ||
			( kifu[know-1].move.nari && ( kifu[know-1].move.koma & ~SENGO ) != GI ) ) )
		{
			return true;
		}
		return false;
	}
	bool IsCapture() const{
		if( know > 0 && kifu[know-1].change.flag & CHANGE_DAI ){
			return true;
		}
		return false;
	}
	bool IsRecapture( const MOVE& move) const{
		if( know > 0 && kifu[know-1].move.to == move.to ){
			return true;
		}
		return false;
	}
	bool IsRecapture() const{
		// 取り返し手
		if( know > 1 && kifu[know-2].move.to == kifu[know-1].move.to ){
			return true;
		}
		return false;
	}
	bool IsProtectedPiece() const{
		// 直前に動かした駒は紐がついているか
		unsigned* effect = ( ksengo == SENTE ? effectG : effectS );
		if( know >= 1 && effect[kifu[know-1].move.to] != 0U ){
			return true;
		}
		return false;
	}
	bool CapInterposition( MOVE& move );
	bool IsLegalMove( MOVE& move );
	bool Obstacle( KOMA koma, int from, int to );
	bool IsStraight( int from, int to, int middle );
	int MoveD( const MOVE& move, bool handler = true ){
		return handler ? MoveD<true>( move )
		               : MoveD<false>( move );
	}
	int Move( MOVE& move, bool handler = true ){
		return handler ? Move<true>( move )
		               : Move<false>( move );
	}
	int NullMove( bool handler = true ){
		return handler ? NullMove<true>()
		               : NullMove<false>();
	}
	int GoNext( bool handler = true ){
		if( know < knum ){
			kifu[know].move.koma = 
				handler ? GoNext<true>( kifu[know].move )
				        : GoNext<false>( kifu[know].move );
			return 1;
		}
		return 0;
	}
	int GoBack();
	int IsMate();
	int DeleteNext();
	int IsRepetition(){
		return IsRepetition( 0 );
	}
	int IsRepetition( int num );
	bool IsShortRepetition();
	uint64 GetHash() const{
		return kifu[know].hashB ^ kifu[know].hashD;
	}
	uint64 GetHashB() const{
		return kifu[know].hashB;
	}
	uint64 GetHashD() const{
		return kifu[know].hashD;
	}
	uint64 GetHash( int num ) const{
		return kifu[num].hashB ^ kifu[num].hashD;
	}
	bool GoNode( const Shogi* pshogi );

	// effect.cpp
protected:
	void InitEffect();
	void SetEffect( int addr, int dir, unsigned int* __restrict__ effect, unsigned bit );
	void UnsetEffect( int addr, int dir, unsigned int* __restrict__ effect, unsigned bit );
	void SetEffect1Step( int addr, int dir, unsigned int* __restrict__ effect, unsigned bit ){
		effect[addr+dir] |= bit;
	}
	void UnsetEffect1Step( int addr, int dir, unsigned int* __restrict__ effect, unsigned bit ){
		effect[addr+dir] -= bit;
	}
	void UpdateEffect( int addr );
	void UpdateEffectR( int addr );
public:
	const unsigned int* GetEffectS(){
		return effectS;
	}
	const unsigned int* GetEffectG(){
		return effectG;
	}

	// generate.cpp
private:
	void SetGenInfo( GEN_INFO& info, unsigned flag );
	void SetGenInfoPin( GEN_INFO& info );
	void GenerateMovesDropFu( Moves& moves );
	void GenerateMovesDropKy( Moves& moves );
	void GenerateMovesDropKe( Moves& moves );
	void GenerateMovesDrop( Moves& moves, KOMA koma );
	void GenerateMovesDrop( Moves& moves );
	void GenerateMoves1Step( Moves& moves, GEN_INFO& info, int dir );
	void GenerateMovesStraight( Moves& moves, GEN_INFO& info, int dir );
	void GenerateMovesOnBoard( Moves& moves, GEN_INFO& info );
	void GenerateMovesOu( Moves& moves, GEN_INFO& info, int dir );
	void GenerateMovesOu( Moves& moves, GEN_INFO& info );
	void GenerateEvasionBan( Moves& moves, GEN_INFO& info, bool eff = false );
	void GenerateEvasionDrop( Moves& moves, GEN_INFO& info, int to );
	void GenerateCapEvasion( Moves& moves, GEN_INFO& info );
	void GenerateNoCapEvasion( Moves& moves, GEN_INFO& info, bool no_pro = true );
public:
	void GenerateMoves( Moves& moves );
	void GenerateCaptures( Moves& moves );
	void GenerateNoCaptures( Moves& moves );
	void GenerateTacticalMoves( Moves& moves );
	void GenerateMovesOu( Moves& moves );
	void GenerateEvasion( Moves& moves );
	void GenerateCapEvasion( Moves& moves );
	void GenerateNoCapEvasion( Moves& moves, bool no_pro = true );

	// gencheck.cpp
private:
	void GenerateCheckDrop( Moves& moves, GEN_INFO& info, int dir, unsigned flag, unsigned flag2 );
	void GenerateCheckDrop( Moves& moves, GEN_INFO& info );
	void GenerateCheckDisc( Moves& moves, GEN_INFO& info, int dir, unsigned* effect );
	void GenerateCheckDisc( Moves& moves, GEN_INFO& info );
	void GenerateCheckNoDisc( Moves& moves, GEN_INFO& info );
public:
	void GenerateCheck( Moves& moves );

	// kifu.cpp
private:
	void RemoveKomaCSA( const char* str );
	void SetKomaAllCSA( KOMA sengo );
	void SetKomaCSA( const char* str, KOMA sengo );
	int InputMoveCSA( const char* str, KOMA sengo );
public:
	static string GetStrMoveSJIS( const MOVE& move, bool eol = false );
	static string GetStrMove( const MOVE& move, bool eol = false );
	string GetStrMove( bool eol = false );
	string GetStrMove( int num, bool eol = false );
	static void PrintMove( const MOVE& move, bool eol = false );
	int PrintMove( bool eol = false );
	int PrintMove( int num, bool eol = false );
	string GetStrBan(
#if POSIX
		int current = 0x00
#endif
	);
	void PrintBan(
#if POSIX
		bool useColor = false // エスケープシーケンスを使った色分け
#endif
	);
#ifndef NDEBUG
	void DebugPrintBitboard( uint96 _bb );
	void DebugPrintBitboard( KOMA koma );
#endif
	static string GetStrMoveCSA( const MOVE& move, KOMA sengo, bool eol = false );
	string GetStrMoveCSA( bool eol = false );
	string GetStrMoveCSA( int num, bool eol = false );
	static void PrintMoveCSA( const MOVE& move, KOMA sengo, bool eol = false );
	int PrintMoveCSA( bool eol = false);
	int PrintMoveCSA( int num, bool eol = false );
	string GetStrBanCSA(
#if POSIX
		int current = 0x00
#endif
	);
	void PrintBanCSA(
#if POSIX
		bool useColor = false // エスケープシーケンスを使った色分け
#endif
	);
	string GetStrKifuCSA( KIFU_INFO* kinfo = NULL, AvailableTime* atime = NULL );
	string GetStrKifuCSA( AvailableTime* atime ){
		return GetStrKifuCSA( NULL, atime );
	}
	int PrintKifuCSA( KIFU_INFO* kinfo = NULL, AvailableTime* atime = NULL );
	int PrintKifuCSA( AvailableTime* atime ){
		return PrintKifuCSA( NULL, atime );
	}
	static string GetStrMoveNum( const MOVE& move, bool eol = false, char delimiter = ',' );
	string GetStrBanNum( char delimiter = ',' );
	static KOMA GetKomaCSA( const char* str );
	int InputLineCSA( const char* line, KIFU_INFO* kinfo = NULL, AvailableTime* atime = NULL );
	int InputLineCSA( const char* line, AvailableTime* atime ){
		return InputLineCSA( line, NULL, atime );
	}
	int InputFileCSA( const char* fname, KIFU_INFO* kinfo = NULL, AvailableTime* atime = NULL );
	int InputFileCSA( const char* fname, AvailableTime* atime ){
		return InputFileCSA( fname, NULL, atime );
	}
	int OutputFileCSA( const char* fname, KIFU_INFO* kinfo = NULL, AvailableTime* atime = NULL );
	int OutputFileCSA( const char* fname, AvailableTime* atime ){
		return OutputFileCSA( fname, NULL, atime );
	}
};

/************************************************
ShogiEval Shogi+局面評価
************************************************/
class ShogiEval : public Shogi{
protected:
	// メンバ変数を変更したらCopy()の書き換えも忘れずに!!

	// 差分計算用評価値
	Evaluate::ELEMENTS evhist[MAX_KIFU+1];

	// Evaluateクラス
	Evaluate* eval;
	bool evalloc;

	// shogi2.cpp
protected:
	void UpdateBan();
	static int ValueComp( const void* v1, const void* v2 ){
		return ((VLIST*)v1)->value - ((VLIST*)v2)->value;
	}
	void ValueList( VLIST sv[], VLIST gv[], int to );
	int see( VLIST* ps, VLIST* pg, int v );
	void GoNextHandler(){
		evhist[know] = evhist[know-1];
	}
	void GoNextRemove( KOMA koma ){
		evhist[know].material -= eval->GetV_BAN( koma );
	}
	void GoNextAdd( KOMA koma ){
		evhist[know].material += eval->GetV_BAN( koma );
	}
	void GoNextFrom( int addr );
	void GoNextTo( int addr );
public:
	ShogiEval( Evaluate* _eval = NULL );
	ShogiEval( const char* evdata, Evaluate* _eval = NULL );
	ShogiEval( const KOMA* _ban0, const int* dai0, KOMA sengo, Evaluate* _eval = NULL );
	ShogiEval( const KOMA* _ban0, const int* dai0, KOMA sengo, const char* evdata, Evaluate* _eval = NULL );
	ShogiEval( TEAI teai, Evaluate* _eval = NULL );
	ShogiEval( TEAI teai, const char* evdata, Evaluate* _eval = NULL );
	ShogiEval( const ShogiEval& org );
	ShogiEval( ShogiEval& org, Evaluate* _eval );
	~ShogiEval();
	virtual void Copy( const ShogiEval& org );
	int StaticExchangeN();
	int StaticExchange(){
		if( ksengo == SENTE )
			return StaticExchangeN();
		else
			return -StaticExchangeN();
	}
	void SetDeviation( double _deviation ){
		eval->SetDeviation( _deviation );
	}
	double GetDeviation(){
		return eval->GetDeviation();
	}
	void InitHashTable(){
		eval->InitHashTable();
	}
	int GetValue(){
		return evhist[know].material + GetValue1();
	}
	int GetValueN(){
		if( ksengo == SENTE )
			return GetValue();
		else
			return -GetValue();
	}
	int GetValue0() const{
		return evhist[know].material;
	}
	int GetValue0N() const{
		if( ksengo == SENTE )
			return GetValue0();
		else
			return -GetValue0();
	}
	int GetValue1(){
		return
			( evhist[know].pos_sou
			+ evhist[know].pos_gou
			+ evhist[know].pos_ou
			+ eval->GetValue1_dis1_ns( this ) ) / (int)EV_SCALE;
	}
	int GetValue1N(){
		if( ksengo == SENTE )
			return GetValue1();
		else
			return -GetValue1();
	}
	int GetV_BAN( KOMA koma ) const{
		return eval->GetV_BAN( koma );
	}
	int GetV_BAN2( KOMA koma ) const{
		return eval->GetV_BAN2( koma );
	}
	int GetV_Promote( KOMA koma ) const{
		return eval->GetV_Promote( koma );
	}
	int GetV_PromoteN( KOMA koma ) const{
		return eval->GetV_PromoteN( koma );
	}
	int GetV_KingPieceS( int addr, KOMA koma ){
		// どっちが速いか微妙
		// (Symmetryで使うので_deff2num自体はなくさない。)
		//int diff = _diff2num[addr/16-sou/16][addr%16-sou%16];
		int diff = _aa2num[_addr[sou]][_addr[addr]];
		int kn = sente_num[koma];
		int x = Addr2X(sou) - 1;
		int y = Addr2Y(sou) - 1;
		//assert( diff != _diff2num[0][0] );
		assert( diff != _aa2num[0][0] );
		assert( x >= 0 && x < 9 && y >= 0 && y < 9 );
		assert( x == _addr[sou] % 9 && y == _addr[sou] / 9 );
		return eval->GetV_KingPiece( diff, kn )
			+ eval->GetV_KingXPiece( x, diff, kn )
			+ eval->GetV_KingYPiece( y, diff, kn );
	}
	int GetV_KingPieceG( int addr, KOMA koma ){
		//int diff = _diff2num[gou/16-addr/16][gou%16-addr%16];
		int diff = _aa2num[_addr[addr]][_addr[gou]];
		int kn = gote_num[koma];
		int x = 9 - Addr2X(gou);
		int y = 9 - Addr2Y(gou);
		//assert( diff != _diff2num[0][0] );
		assert( diff != _aa2num[0][0] );
		assert( x >= 0 && x < 9 && y >= 0 && y < 9 );
		assert( x == ( 80 - _addr[gou] ) % 9 && y == ( 80 - _addr[gou] ) / 9 );
		return eval->GetV_KingPiece( diff, kn )
			+ eval->GetV_KingXPiece( x, diff, kn )
			+ eval->GetV_KingYPiece( y, diff, kn );
	}
	int CaptureValue( const MOVE& move ){
		KOMA koma;
		if( ( koma = ban[move.to] ) )
			return -( eval->GetV_BAN2( koma ) );
		return 0;
	}
	int CaptureValueN( const MOVE& move ){
		if( ksengo == SENTE )
			return CaptureValue( move );
		else
			return -CaptureValue( move );
	}
};

/************************************************
定跡
************************************************/
#define BOOK_HASH_SIZE				( U64(1) << 16 )
#define BOOK_HASH_MASK				( BOOK_HASH_SIZE - 1 )

struct MLIST{
	unsigned int mv;   // 指し手
	int cnt;           // 指し手のカウント
	union{
		int val;       // 評価値
		unsigned date; // 日付
	};
};

struct BLIST{
	uint64 hash;       // ハッシュ値
	int cnt;           // カウントの総数
	int num;           // 指し手の数
	list<MLIST> mlist; // 指し手
};

class Book{
private:
	list<BLIST>* blist;
	string fname;      // 定跡データのファイル名

public:
	enum ADD_METHOD{
		OVER_WRITE = 0, // カウンタを使用
		ADD_NEW,        // 新規
	};
	enum TYPE{
		TYPE_FREQ = 0, // 頻度
		TYPE_EVAL,     // 評価値
	};

	// book.cpp
private:
	int _Make( const char* dir, int max = INT_MAX, ADD_METHOD method = OVER_WRITE );
public:
	Book();
	Book( const char* _fname );
	~Book();
	int Read( const char* _fname = NULL );
	int Write( const char* _fname = NULL );
	int Make( const char* dir, int max = INT_MAX, ADD_METHOD method = OVER_WRITE ){
		if( 0 == _Make( dir, max, method ) ){ return 0; }
		if( 0 == Write() ){ return 0; }
		return 1;
	}
	int Date( const char* dir, double r, int max = INT_MAX );
	int Add( uint64 hash, const MOVE& move, ADD_METHOD method, int cnt, unsigned date ){
		return Add( hash, move.Export(), method, cnt, (int)date );
	}
	int Add( uint64 hash, unsigned int mv, ADD_METHOD method, int cnt, unsigned date ){
		return Add( hash, mv, method, cnt, (int)date );
	}
	int Add( uint64 hash, const MOVE& move, ADD_METHOD method = OVER_WRITE, int cnt = 1, int val = 0 ){
		return Add( hash, move.Export(), method, cnt, val );
	}
	int Add( uint64 hash, unsigned int mv, ADD_METHOD method = OVER_WRITE, int cnt = 1, int val = 0 );
	void DelAll();
	int GetMove( Shogi* pshogi, MOVE& move, TYPE type = TYPE_FREQ );
	int GetMoveAll( Shogi* pshogi, Moves& moves, TYPE type = TYPE_FREQ );
	int EntryCheck( uint64 hash, const MOVE& move );
	Book& operator+=( Book& book );
	void PrintDistribution();
};

/************************************************
進捗表示
************************************************/
class Progress{
public:
	static void Print( int pc ){
		int i;

		cerr << "Analyzing..";
		for( i = 4 ; i < pc ; i += 5 )
			cerr << '#';
		for( ; i < 100 ; i += 5 )
			cerr << ' ';
		cerr << " [" << pc << "%]     ";
		cerr.flush();
		cerr << "\r";
	}
	static void Clear(){
		cerr << "                                      ";
		cerr.flush();
		cerr << "\r";
	}
};

#endif
