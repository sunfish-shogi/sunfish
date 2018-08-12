/* match.h
 * R.Kubo 2008-2012
 * 将棋の対局を行なうMatchクラス
 */

#ifndef _MATCH_
#define _MATCH_

#include "shogi.h"

#ifdef WIN32
#	include <process.h>
#	include <mmsystem.h>
#	include <csignal>
#	include <ctime>
#else
#	include <unistd.h>
#	include <csignal>
#	include <dirent.h>
#	include <strings.h>
#	include <sched.h>
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <netinet/tcp.h>
#	include <netdb.h>
#	include <errno.h>
#endif

#define MAX_DEPTH			128 // 最大探索深さ
#define DEF_DEPTH			10  // 探索深さ初期値
#define DEF_LIMIT			10  // 思考時間初期値

#define DINC				16

#define TREE_NUM(t)				( t * 4 - 3 )
//#define TREE_NUM(t)				( t * 2 - 1 )
#define WORKER_NUM(t)			( t )

/************************************************
読み延長
************************************************/
#ifndef DEPTH_EXT
#	define DEPTH_EXT				1
#endif

/************************************************
読み延長と短縮の制限
************************************************/
#ifndef LIMIT_EXT
#	define LIMIT_EXT				1
#endif

/************************************************
Killer move
************************************************/
#ifndef KILLER
#	define KILLER					1
#endif

/************************************************
0.5手延長
************************************************/
#ifndef EXT05
#	define EXT05					0
#endif

/************************************************
再帰的反復深化
************************************************/
#ifndef RIDEEP
#	define RIDEEP					1
#endif

/************************************************
稲庭対策
************************************************/
#ifndef INANIWA
#	define INANIWA					1
#endif

/************************************************
futility pruningのマージン拡張版
************************************************/
#ifndef FUT_EXT
#	define FUT_EXT					1
#endif

/************************************************
razoring
************************************************/
#ifndef RAZORING
#	define RAZORING					1
#endif

/************************************************
static null move pruning
************************************************/
#ifndef STATIC_NULL_MOVE
#	define STATIC_NULL_MOVE			1
#endif

#if STATIC_NULL_MOVE != 0 && FUT_EXT == 0
#	error STATIC_NULL_MOVE is non-zero but FUT_EXT is zero.
#endif

/************************************************
move count pruning
************************************************/
#ifndef MOVE_COUNT_PRUNING
#	define MOVE_COUNT_PRUNING		0
#endif

/************************************************
singular extension
************************************************/
#ifndef SINGULAR_EXTENSION
#	define SINGULAR_EXTENSION		0
#endif

/************************************************
memcpyを使用しないShogiクラス複製
************************************************/
#ifndef USE_QUICKCOPY
#	define USE_QUICKCOPY			0
#endif

/************************************************
合議方法
************************************************/
enum CONS_TYPE{
	CONS_SINGLE   = 0, // 合議なし
	CONS_SERIAL,       // 非並列
	CONS_PARALLEL,     // 並列
};

/************************************************
HashTable ハッシュテーブル
************************************************/
enum VALUE_TYPE{
	VALUE_NULL = 0,
	VALUE_SHALL, // 浅い
	VALUE_EXACT, // 確定値
	VALUE_UPPER, // 上限値
	VALUE_LOWER, // 下限値
};

struct HASH_TABLE{
	// 要素を増やしたらXorの計算も書き換えること。
	uint64 hash;        // ハッシュ値
	int rdepth;         // 残り深さ
	int value;          // 評価値
	unsigned first;     // 前回の最善手
	unsigned second;    // 前々回の最善手
	unsigned short cnt; // カウント値
	unsigned char type; // 値の種別
};

// HASH_TABLE内ではunsinged charを使う。
enum ENTRY_TYPE{
	HASH_NOENTRY = 0,
	HASH_OVERWRITE,
	HASH_ENTRY,
	HASH_COLLISION,
};

class HashTable : public HashHandler{
private:
	HASH_TABLE* hash_table; // ハッシュテーブル
	unsigned    cnt;        // リセット用カウンタ

	// hash.cpp
private:
	void Xor( HASH_TABLE& table ){
		table.hash ^= ( (uint64)table.rdepth )
			^ ( (uint64)table.value << 32 )
			^ ( (uint64)table.first )
			^ ( (uint64)table.second << 32 )
			^ ( (uint64)table.cnt )
			^ ( (uint64)table.type << 32 );
	}
	bool GetTable( uint64 hash, HASH_TABLE& table ){
		table = hash_table[hash&Hash::Mask()];
		Xor( table );
		if( hash == table.hash && table.cnt == cnt ){
			return true;
		}
		else{
			return false;
		}
	}
	void SetTable( HASH_TABLE& table ){
		uint64 hash = table.hash;
		Xor( table );
		hash_table[hash&Hash::Mask()] = table;
	}
public:
	HashTable();
	~HashTable();
	void Resize();
	void Init(){
		memset( hash_table, 0, sizeof(HASH_TABLE) * Hash::Size() );
		cnt = 1;
	}
	void Clear(){
		cnt = ( cnt == USHRT_MAX ? 1 : cnt + 1 );
	}
	ENTRY_TYPE Entry( uint64 hash, int rdepth,
		int alpha, int beta, int value, const MOVE& move );
	VALUE_TYPE Check( uint64 hash, int rdepth, VALUE_TYPE& type, int& value ){
		HASH_TABLE table;
		if( GetTable( hash, table ) ){
			type = (VALUE_TYPE)table.type;
			value = table.value;
			if( table.rdepth >= rdepth ){
				return (VALUE_TYPE)table.type;
			}
			return VALUE_SHALL;
		}
		return VALUE_NULL;
	}
	int GetMoves( uint64 hash, ShogiEval* pshogi, Moves& moves ){
		HASH_TABLE table;
		MOVE mtemp;
		if( GetTable( hash, table ) ){
			mtemp.Import( table.first );
			if( pshogi->IsLegalMove( mtemp ) ){
				moves.Add( mtemp );
			}
			mtemp.Import( table.second );
			if( pshogi->IsLegalMove( mtemp ) )
				moves.Add( mtemp );
		}
		return moves.GetNumber();
	}
	bool GetMoves( uint64 hash, ShogiEval* pshogi, MOVE& move1, MOVE& move2 ){
		HASH_TABLE table;
		MOVE mtemp;
		if( GetTable( hash, table ) ){
			mtemp.Import( table.first );
			if( pshogi->IsLegalMove( mtemp ) ){ move1 = mtemp; }
			else{ move1.from = EMPTY; }

			mtemp.Import( table.second );
			if( pshogi->IsLegalMove( mtemp ) ){ move2 = mtemp; }
			else{ move2.from = EMPTY; }

			return true;
		}
		return false;
	}
};

/************************************************
ShekTable SHEK用ハッシュテーブル 2010/09/19
とりあえずマスク後の値の衝突は気にしていない。
************************************************/
enum{
	SHEK_FIRST = 0, // 初めての局面
	SHEK_EQUAL,     // 等しい局面
	SHEK_PROFIT,    // 得な局面
	SHEK_LOSS,      // 損な局面
};

struct SHEK_TABLE{
	uint64          hashB;  // ハッシュ値(盤+手番、持ち駒なし)
	uint64          dai;    // 手番側の持ち駒
	unsigned short  cnt;    // クリア用カウンタ
};

class ShekTable : public HashHandler{
private:
	SHEK_TABLE* shek_table; // ハッシュテーブル
	unsigned int cnt;

public:
	ShekTable();
	~ShekTable();
	void Resize();
	void Init(){
		memset( shek_table, 0, sizeof(SHEK_TABLE) * Hash::Size() );
		cnt = 1;
	}
	void Clear(){
		cnt = ( cnt == USHRT_MAX ? 1 : cnt + 1 );
	}
	uint64 GetDai( const int* _dai, KOMA sengo ){
		// 持ち駒の情報をビット列に変換
		uint64 dai = (uint64)0U;

		// (1<<n)-1で1がn個並ぶ
		dai |= ( (uint64)1 << _dai[FU|sengo] ) - (uint64)1; dai <<= 18;
		dai |= ( (uint64)1 << _dai[KY|sengo] ) - (uint64)1; dai <<= 4;
		dai |= ( (uint64)1 << _dai[KE|sengo] ) - (uint64)1; dai <<= 4;
		dai |= ( (uint64)1 << _dai[GI|sengo] ) - (uint64)1; dai <<= 4;
		dai |= ( (uint64)1 << _dai[KI|sengo] ) - (uint64)1; dai <<= 4;
		dai |= ( (uint64)1 << _dai[KA|sengo] ) - (uint64)1; dai <<= 2;
		dai |= ( (uint64)1 << _dai[HI|sengo] ) - (uint64)1;

		return dai;
	}
	int Check( uint64 hashB, const int* _dai, KOMA sengo ){
		// ハッシュテーブルのチェック
		SHEK_TABLE* ptable = &shek_table[hashB&Hash::Mask()];
		uint64 dai = GetDai( _dai, sengo );

		if( ptable->cnt == cnt && ptable->hashB == hashB ){
			if( ptable->dai == dai ){
				return SHEK_EQUAL; // 等しい局面
			}
			else if( ptable->dai & ~dai ){
				if( ~(ptable->dai) & dai ){
					return SHEK_FIRST; // 初めての局面(rare case)
				}
				else{
					return SHEK_LOSS; // 損な局面
				}
			}
			else if( ~(ptable->dai) & dai ){
				return SHEK_PROFIT; // 得な局面
			}
			else{
				return SHEK_FIRST; // 初めての局面(rare case)
			}
		}
		else{
			return SHEK_FIRST; // 初めての局面
		}
	}
	void Set( uint64 hashB, const int* _dai, KOMA sengo ){
		// 探索中フラグを立てる。
		SHEK_TABLE* ptable = &shek_table[hashB&Hash::Mask()];
		ptable->hashB = hashB;
		ptable->dai = GetDai( _dai, sengo );
		ptable->cnt = cnt;
	}
	void Unset( uint64 hashB ){
		// 探索中フラグをおろす。
		SHEK_TABLE* ptable = &shek_table[hashB&Hash::Mask()];
		ptable->cnt = 0;
	}
	void Set( Shogi* pshogi );
	void Unset( Shogi* pshogi );
};

/************************************************
DfPnTable df-pn用ハッシュテーブル
************************************************/
struct DFPN_TABLE{
	uint64 hash; // ハッシュ値
	int pn; // 証明数
	int dn; // 反証数
};

class DfPnTable : public HashHandler{
private:
	DFPN_TABLE* dfpn_table; // ハッシュテーブル

public:
	DfPnTable();
	~DfPnTable();
	void Resize();
	void Init(){
		memset( dfpn_table, 0, sizeof(DFPN_TABLE) * Hash::Size() );
	}
	DFPN_TABLE* GetTable( uint64 hash, int check ){
		// 攻め方と玉方で使うテーブルの位置を変える。
		if( check ){
			return &dfpn_table[hash&Hash::Mask()];
		}
		else{
			return &dfpn_table[(hash>>32)&Hash::Mask()];
		}
	}
	void Entry( uint64 hash, int check, int pn, int dn ){
		DFPN_TABLE* ptable = GetTable( hash, check );
		ptable->hash = hash;
		ptable->pn = pn;
		ptable->dn = dn;
	}
	int GetValue( uint64 hash, int check, int& pn, int& dn ){
		DFPN_TABLE* ptable = GetTable( hash, check );
		if( ptable->hash == hash ){
			pn = ptable->pn;
			dn = ptable->dn;
			return 1;
		}
		else{
			pn = 1;
			dn = 1;
			return 0;
		}
	}
};

/************************************************
HashSize
************************************************/
class HashSize{
public:
	// 1要素あたりのデータ量
	static unsigned BytesPerSize( int thread_num,
		CONS_TYPE ctype = CONS_SINGLE, int cons_num = 1 )
	{
		switch( ctype ){
		case CONS_PARALLEL:
			return sizeof(EVAL_TABLE) * cons_num
				+  sizeof(HASH_TABLE) * cons_num
				+  sizeof(SHEK_TABLE) * TREE_NUM(thread_num) * cons_num
				+  sizeof(DFPN_TABLE) * TREE_NUM(thread_num) * cons_num;
		case CONS_SERIAL:
			return sizeof(EVAL_TABLE) * cons_num
				+  sizeof(HASH_TABLE) * cons_num
				+  sizeof(SHEK_TABLE) * TREE_NUM(thread_num)
				+  sizeof(DFPN_TABLE) * TREE_NUM(thread_num);
		case CONS_SINGLE:
		default:
			return sizeof(EVAL_TABLE)
				+  sizeof(HASH_TABLE)
				+  sizeof(SHEK_TABLE) * TREE_NUM(thread_num)
				+  sizeof(DFPN_TABLE) * TREE_NUM(thread_num);
			
		}
	}

	// MBytes => 2のべき乗
	static unsigned MBytesToPow2( unsigned mbytes, int thread_num,
		CONS_TYPE ctype = CONS_SINGLE, int cons_num = 1 )
	{
		return BytesToPow2( mbytes * 1024 * 1024, thread_num, ctype, cons_num );
	}
	// kBytes => 2のべき乗
	static unsigned KBytesToPow2( unsigned kbytes, int thread_num,
		CONS_TYPE ctype = CONS_SINGLE, int cons_num = 1 )
	{
		return BytesToPow2( kbytes * 1024, thread_num, ctype, cons_num );
	}
	// Bytes => 2のべき乗
	static unsigned BytesToPow2( unsigned bytes, int thread_num,
		CONS_TYPE ctype = CONS_SINGLE, int cons_num = 1 )
	{
		unsigned size = bytes / BytesPerSize( thread_num, ctype, cons_num );
		return max( getLastBit( size ) - 1, 1 );
	}

	// 要素数 => MBytes
	static unsigned SizeToMBytes( unsigned size, int thread_num,
		CONS_TYPE ctype = CONS_SINGLE, int cons_num = 1 )
	{
		return SizeToBytes( size, thread_num, ctype, cons_num ) / 1024 / 1024;
	}
	// 要素数 => kBytes
	static unsigned SizeToKBytes( unsigned size, int thread_num,
		CONS_TYPE ctype = CONS_SINGLE, int cons_num = 1 )
	{
		return SizeToBytes( size, thread_num, ctype, cons_num ) / 1024;
	}
	// 要素数 => Bytes
	static unsigned SizeToBytes( unsigned size, int thread_num,
		CONS_TYPE ctype = CONS_SINGLE, int cons_num = 1 )
	{
		return size * BytesPerSize( thread_num, ctype, cons_num );
	}
};

/************************************************
探索情報
************************************************/
struct THINK_CNTS{
	uint64   node;           // node
	uint64   hash_entry;     // ハッシュの登録回数
	uint64   hash_collision; // ハッシュの衝突回数
	uint64   hash_cut;       // ハッシュの参照回数
#if RAZORING
	uint64   razoring;       // razoring回数
#endif
#if STATIC_NULL_MOVE
	uint64   s_nullmove_cut; // static null move cut回数
#endif
	uint64   nullmove_cut;   // null move cut回数
	uint64   futility_cut;   // futility cut回数
	uint64   e_futility_cut; // extended futility cut回数
#if MOVE_COUNT_PRUNING
	uint64   move_count_cut; // move count cut回数
#endif
#if SINGULAR_EXTENSION
	uint64   singular_ext;   // singular extensions
#endif
	uint64   split;          // split回数
	uint64   split_failed;   // split失敗回数
	void init(){
		memset( this, 0, sizeof(THINK_CNTS) );
	}
	void add( THINK_CNTS& cnts ){
		node           += cnts.node;
		hash_entry     += cnts.hash_entry;
		hash_collision += cnts.hash_collision;
		hash_cut       += cnts.hash_cut;
#if RAZORING
		razoring       += cnts.razoring;
#endif
#if STATIC_NULL_MOVE
		s_nullmove_cut += cnts.s_nullmove_cut;
#endif
		nullmove_cut   += cnts.nullmove_cut;
		futility_cut   += cnts.futility_cut;
		e_futility_cut += cnts.e_futility_cut;
#if MOVE_COUNT_PRUNING
		move_count_cut += cnts.move_count_cut;
#endif
		split          += cnts.split;
		split_failed   += cnts.split_failed;
	}
};

struct THINK_INFO : THINK_CNTS{
	bool     useBook;        // 定跡を使用
	bool     bPVMove;
	int64    value;          // 評価値(なんで64bitにしたんだっけ?)
	uint64   nps;            // node/sec
	unsigned msec;           // msec
	unsigned depth;          // 通常探索の深さ
	unsigned unused_tree;    // Treeの最大利用数
};

/************************************************
Principal Variation
************************************************/
class PVMove{
private:
	int* bnum;                // 手の個数
	MOVE (*bmove)[MAX_DEPTH]; // 手順

public:
	PVMove();
	~PVMove();
	void Init( int depth ){
		if( depth < MAX_DEPTH ){
			bnum[depth] = 0;
		}
	}
	void GetMoves( Moves& moves ){
		moves.CopyFrom( bnum[0], bmove[0] );
	}
	bool GetBestMove( MOVE& move, int depth ){
		if( bnum[depth] > 0 ){
			move = bmove[depth][0];
			return true;
		}
		return false;
	}
	void Copy( int depth, PVMove* source, int depth2, const MOVE& move );
	void PrintPVMove( int mdepth, int depth, int value );

};

/************************************************
Killer move
************************************************/
class KillerMove{
private:
	MOVE move1;
	MOVE move2;

public:
	void Init(){
		move1.from = EMPTY; move1.value = VMIN;
		move2.from = EMPTY; move2.value = VMIN;
	}
	void Add( MOVE& move, int value ){
		if( value > move1.value ){
			move2 = move1;
			move1 = move;
			move1.value = value;
		}
		else if( value > move2.value ){
			move2 = move;
			move2.value = value;
		}
	}
	int GetMoves( Shogi* pshogi, Moves& moves){
		if( pshogi->IsLegalMove( move1 ) ){
			moves.Add( move1 );
		}
		if( pshogi->IsLegalMove( move2 ) ){
			moves.Add( move2 );
		}
		return moves.GetNumber();
	}
	bool GetMoves( Shogi* pshogi, MOVE& mv1, MOVE& mv2 ){
		if( pshogi->IsLegalMove( move1 ) ){ mv1 = move1; }
		else{ mv1.from = EMPTY; }

		if( pshogi->IsLegalMove( move2 ) ){ mv2 = move2; }
		else{ mv2.from = EMPTY; }

		return ( mv1.from != EMPTY || mv2.from != EMPTY );
	}
	void Copy( KillerMove& source ){
		move1 = source.move1;
		move2 = source.move2;
	}
	MOVE& GetMove1(){
		return move1;
	}
	MOVE& GetMove2(){
		return move2;
	}
};

/************************************************
History Huristics
************************************************/
#define HIST_SIZE			( DAI + GHI + 1 )
#define HIST_FRACTION		0x10000
#define APP_LIMIT			0x10000

#if HIST_FRACTION * APP_LIMIT - 1 > UINT_MAX
#error
#endif

class History{
private:
	unsigned (*history)[0x100]; // 最善手だった回数
	unsigned (*appear) [0x100]; // 出現回数

public:
	History(){
		history = new unsigned[HIST_SIZE][0x100];
		appear  = new unsigned[HIST_SIZE][0x100];
		Clear();
	}
	~History(){
		delete [] history;
		delete [] appear;
	}
	void Clear(){
		memset( history, 0, sizeof(unsigned) * HIST_SIZE * 0x100 );
		memset( appear , 0, sizeof(unsigned) * HIST_SIZE * 0x100 );
	}
	unsigned int Get( unsigned from, unsigned to ){
		//return ( (uint64)( history[from][to] + 1 ) * HIST_FRACTION ) / ( appear[from][to] + 1 );
		return ( ( history[from][to] + 1 ) * HIST_FRACTION ) / ( appear[from][to] + 1 );
	}
	void Good( unsigned from, unsigned to, int depth ){
		depth /= DINC / 4;
		if( depth <= 0 ){ depth = 1; }

		history[from][to] += depth;
	}
	void Appear( unsigned from, unsigned to, int depth ){
		// オーバーフローするとき最下位ビットを捨てる。
		// (historyは常にappear以下のはず)
		depth /= DINC / 4;
		if( depth <= 0 ){ depth = 1; }

		//if( appear[from][to] > UINT_MAX - (unsigned)depth ){
		if( appear[from][to] >= APP_LIMIT - (unsigned)depth ){
			history[from][to] /= 2U;
			appear[from][to] /= 2U;
		}
		appear[from][to] += depth;
	}
};

#define FUT_MGN					600  // futility pruning のマージン(深さ1)
#define FUT_MGN2				600  // futility pruning のマージン(深さ2)
#define FUT_DIFF				800
#define FUT_DIFF_KING			2400 

/************************************************
MovesStack 合法手スタック
************************************************/
enum{
	PHASE_NULL = 0,

	// 通常探索用
	PHASE_CAPTURE,
	PHASE_NOCAPTURE,

	// 3手詰み探索用
	PHASE_CAP_EVASION,
	PHASE_KING,
	PHASE_NOCAP_EVASION,
};

class MovesStack : public Moves{
private:
	int current;
	int phase;
	MOVE hash1;
	MOVE hash2;
	MOVE killer1;
	MOVE killer2;
public:
	MovesStack() : Moves(){
		current = 0;
		phase = PHASE_NULL;
		hash1.from = hash2.from = EMPTY;
		killer1.from = killer2.from = EMPTY;
	}
	void Init( int _phase = PHASE_NULL ){
		current = 0;
		phase = _phase;
		Moves::Init();
	}
	void InitPhase( MOVE& _hash1, MOVE& _hash2 ){
		Init( PHASE_CAPTURE );
		hash1 = _hash1;
		if( hash1.from != EMPTY ){ Add( hash1 ); }
		hash2 = _hash2;
		if( hash2.from != EMPTY ){ Add( hash2 ); }
		killer1.from = killer2.from = EMPTY;
	}
	void InitPhaseEvasion(){
		Init( PHASE_CAP_EVASION );
	}
	int GetCurrentNumber(){
		return current;
	}
	MOVE* GetCurrentMove(){
		if( current < GetNumber() ){
			return &GetMove( current++ );
		}
		return NULL;
	}
	int GetPhase(){
		return phase;
	}
	void SetPhase( int phase_new ){
		current = 0;
		phase = phase_new;
		Moves::Init();
	}
	void SortAfter(){
		Sort( current );
	}
	MOVE& GetHash1(){
		return hash1;
	}
	MOVE& GetHash2(){
		return hash2;
	}
	MOVE& GetKiller1(){
		return killer1;
	}
	MOVE& GetKiller2(){
		return killer2;
	}
};

/************************************************
TreeBase 探索木
************************************************/
class TreeBase{
public:
	ShogiEval* pshogi;            // 局面
	MovesStack* moves;            // 着手可能手
	MovesStack* pmoves;           // 現在深さの着手可能手
	ShekTable* shek_table;        // SHEK用ハッシュテーブル

public:
	TreeBase( const ShogiEval* pshogi_org = NULL );
	~TreeBase();

	// Shogi
	void Copy( const ShogiEval& org ){
		if( pshogi != NULL ){
#if USE_QUICKCOPY
			pshogi->QuickCopy( org );
#else
			pshogi->Copy( org );
#endif
		}
		else{
			pshogi = new ShogiEval( org );
		}

		// SHEK
		shek_table->Clear();
		shek_table->Set( pshogi );

		// 初期化
		pmoves = &(moves[0]);
	}
	int GetValueN(){
		return pshogi->GetValueN();
	}
	int IsRepetition(){
		return pshogi->IsRepetition();
	}
	bool IsShortRepetition(){
		return pshogi->IsShortRepetition();
	}
	bool IsCheck(){
		return pshogi->IsCheck();
	}
	int IsMate(){
		return pshogi->IsMate();
	}
	bool IsProtectedPiece(){
		return pshogi->IsProtectedPiece();
	}
	bool CapInterposition( MOVE& move ){
		return pshogi->CapInterposition( move );
	}
	bool IsLegalMove( MOVE& move ){
		return pshogi->IsLegalMove( move );
	}
	void GenerateMoves(){
		pshogi->GenerateMoves( *pmoves );
	}
	void GenerateCheck(){
		pshogi->GenerateCheck( *pmoves );
	}
	void GenerateTacticalMoves(){
		pshogi->GenerateTacticalMoves( *pmoves );
	}
	void GenerateCaptures(){
		pshogi->GenerateCaptures( *pmoves );
	}
	void GenerateNoCaptures(){
		pshogi->GenerateNoCaptures( *pmoves );
	}
	void GenerateEvasion(){
		pshogi->GenerateEvasion( *pmoves );
	}
	void GenerateCapEvasion(){
		pshogi->GenerateCapEvasion( *pmoves );
	}
	void GenerateNoCapEvasion( bool _no_pro = true ){
		pshogi->GenerateNoCapEvasion( *pmoves, _no_pro );
	}
	void GenerateMovesOu(){
		pshogi->GenerateMovesOu( *pmoves );
	}
	uint64 GetHash(){
		return pshogi->GetHash();
	}
	uint64 GetNextHash( MOVE& move ){
		uint64 hash;
		pshogi->MoveD( move );
		hash = pshogi->GetHash();
		pshogi->GoBack();
		return hash;
	}
	bool IsReply() const{
		return pshogi->IsReply();
	}
	bool IsTacticalMove( MOVE& move ) const{
		return pshogi->IsTacticalMove( move );
	}
	bool IsTactical() const{
		return pshogi->IsTactical();
	}
	bool IsCheckMove( MOVE& move ) const{
		return pshogi->IsCheckMove( move );
	}
	bool IsCapture() const{
		return pshogi->IsCapture();
	}
	bool IsCapture( MOVE& move ) const{
		return pshogi->IsCapture( move );
	}
	bool IsRecapture( MOVE& move ) const{
		return pshogi->IsRecapture( move );
	}
	bool IsKingMove() const{
		return pshogi->IsKingMove();
	}
	void MakeMove( MOVE& move, bool handler = true ){
		pshogi->MoveD( move, handler );
		pmoves++;
	}
	void UnmakeMove(){
		pshogi->GoBack();
		pmoves--;
	}
	int MakeNullMove(){
		return pshogi->NullMove();
	}
	void UnmakeNullMove(){
		pshogi->GoBack();
	}

	// Moves
	void InitMoves(){
		pmoves->Init();
	}
	void AddMove( MOVE& move ){
		pmoves->Add( move );
	}
	int GetMovesNumber(){
		return pmoves->GetNumber();
	}
	MOVE& GetMove( int i ){
		return (*pmoves)[i];
	}
	void SortMoves( int idx, int n ){
		pmoves->Sort( idx, n );
	}
	void StableSortMoves( int idx, int n ){
		pmoves->StableSort( idx, n );
	}
	void StableSortMoves(){
		pmoves->StableSort();
	}
	void SetMoveValue( int i , int value ){
		pmoves->SetValue( i, value );
	}
	void DeleteMove( int i ){
		pmoves->Remove( i );
	}
	void GetHash( MOVE& hash1, MOVE& hash2 ){
		hash1 = pmoves->GetHash1();
		hash2 = pmoves->GetHash2();
	}

	// SHEK
	int ShekCheck(){
		return shek_table->Check( pshogi->GetHashB(), pshogi->GetDai(), pshogi->GetSengo() );
	}
	void ShekSet(){
		shek_table->Set( pshogi->GetHashB(), pshogi->GetDai(), pshogi->GetSengo() );
	}
	void ShekUnset(){
		shek_table->Unset( pshogi->GetHashB() );
	}
};

/************************************************
Tree 探索木
************************************************/
#define TREE_DEBUG				"tdebug" // デバプリファイル

#ifdef WIN32
// mutexを使うとうまく動かないので、
// _InterlockedExchangeを使用。
//typedef HANDLE MUTEX_SPLIT;
typedef long MUTEX_SPLIT;
#else
typedef pthread_mutex_t MUTEX_SPLIT;
#endif

class Tree : public TreeBase{
public:
#if KILLER
	KillerMove* kmoves;           // キラームーブ
#endif
	PVMove* pvmove;               // Principal Variation
	int copy_node;                // コピーしたときのノード
	bool abort;                   // 探索の中断
	bool no_pro;                  // 利きの無い合駒の生成
#if SINGULAR_EXTENSION
	MOVE exmove;                  // 除外する手
	int exdepth;                  // 除外を適用する深さ
#endif

	// 並列探索

	// 子供に設定する情報
	int self;                     // 自分
	int parent;                   // 親
	int sibling;                  // 兄弟の数
	int worker;                   // WORKER番号
	int depth;                    // 深さ
	int rdepth;                   // 残り深さ
	int alpha;                    // alpha値
	int beta;                     // beta値
	unsigned state;               // ノードの状態

	// 親に設定する情報
	int value;                    // 評価値
	MOVE bmove;                   // 最善手
	int mvcnt;                    // 兄弟節点のカウント
#if MOVE_COUNT_PRUNING
	MOVE threat;                  // 脅威(詰めろ)
#endif

	// 排他
	MUTEX_SPLIT mutex_split;

	// 探索情報
	THINK_CNTS cnts;

public:
	Tree( const ShogiEval* pshogi_org = NULL );
	~Tree();

	// Debug用
	bool DebugPrintBan();
	bool DebugPrintMoves();
	bool DebugPrintDepth( int depth );

	// Shogi
	void Copy( const ShogiEval& org ){
		TreeBase::Copy( org );

		copy_node = pshogi->GetNumber();

		abort = false;
	}
	void SetNode( const Tree* tree, Tree* tree_p, int _worker, int _depth,
		int _rdepth, int _alpha, int _beta, unsigned _state
	){
		MOVE move;

		// 情報をセット
		parent = tree_p->self;
		worker = _worker;
		depth = _depth;
		rdepth = _rdepth;
		alpha = _alpha;
		beta = _beta;
		state = _state;

		// 初期化
		pmoves = &(moves[0]);
		abort = false;

#if KILLER
		// killer move
		kmoves[depth+1].Copy( tree->kmoves[depth+1] );
#endif

		// 同じノードへ移動
		// SHEKを更新しないといけないのでShogi::GoNodeを使わない。
		// 他のツリーと開始局面が一致していることを想定
		while( pshogi->GetNumber() > copy_node && pshogi->GoBack() ){
			// SHEK
			ShekUnset();
		}

		// 同じ指し手で局面を進める。
		while( pshogi->GetNumber() < tree->pshogi->GetNumber() ){
			ShekSet();
			tree->pshogi->GetMove( pshogi->GetNumber(), move );
			// TODO: MoveDだと王手判定しないから連続王手千日手が検出できない。
			pshogi->MoveD( move );
		}
	}
#if SINGULAR_EXTENSION
	void SetExclude( MOVE& move, int depth ){
		pmoves++;
		exmove = move;
		exdepth = depth;
	}
	void UnsetExclude(){
		pmoves--;
		exmove.from = EMPTY;
		exdepth = -1;
	}
	bool Singular(){
		return exdepth >= 0;
	}
	bool Exclude( MOVE& move, int depth ){
		return exdepth == depth && exmove.IsSame( move );
	}
#endif

	// Moves
	MOVE* GetNextMove( History* history
#if KILLER
		, int depth
#endif
	);
	MOVE* GetNextEvasion();
	void InitMovesGenerator( MOVE& hash1, MOVE& hash2 ){
		pmoves->InitPhase( hash1, hash2 );
	}
	void InitEvasionGenerator( bool _no_pro = true ){
		pmoves->InitPhaseEvasion();
		no_pro = _no_pro;
	}
	int SortSeeQuies();
	int SortSee();
	int SortSee( MOVE& hmove1, MOVE& hmove2
#if KILLER
		, MOVE& killer1, MOVE& killer2
#endif
	);
	void SortHistory( History* history,
		MOVE& hmove1, MOVE& hmove2
#if KILLER
		, MOVE& killer1, MOVE& killer2
#endif
	);
	int EstimateValue( MOVE& move );
	bool ConnectedThreat( MOVE& threat, MOVE& move );
	void GetKiller( MOVE& killer1, MOVE& killer2 ){
		killer1 = pmoves->GetKiller1();
		killer2 = pmoves->GetKiller2();
	}
	int GetPhase(){
		return pmoves->GetPhase();
	}

	// PV
	void InitNode( int depth ){
		pvmove->Init( depth );
	}
	bool GetBestMove( MOVE& move, int depth ){
		return pvmove->GetBestMove( move, depth );
	}
	void PVCopy( int depth, MOVE& move ){
		pvmove->Copy( depth, pvmove, depth + 1, move );
	}
	void PVCopy( Tree* tree, int depth, MOVE& move ){
		pvmove->Copy( depth, tree->pvmove, depth + 1, move );
	}
	void PrintPVMove( int mdepth, int depth, int value ){
		pvmove->PrintPVMove( mdepth, depth, value );
	}

	// killer move
#if KILLER
	void InitKiller( int depth ){
		kmoves[depth].Init();
	}
	void GetKiller( int depth, MOVE& killer1, MOVE& killer2 ){
		kmoves[depth].GetMoves( pshogi, killer1, killer2 );
		if( killer1.from != EMPTY ){ killer1.value += pshogi->CaptureValueN( killer1 ); }
		if( killer2.from != EMPTY ){ killer2.value += pshogi->CaptureValueN( killer2 ); }
	}
	void AddKiller( int depth, MOVE& move , int value ){
		int capv = pshogi->CaptureValueN( move );
		if( move.nari ){
			capv += pshogi->GetV_PromoteN( move.koma );
		}
		kmoves[depth].Add( move, value - capv );
	}
	bool IsKiller( MOVE& move ){
		return move.IsSame( pmoves->GetKiller1() )
			|| move.IsSame( pmoves->GetKiller2() );
	}
	MOVE& GetKiller1( int depth ){
		return kmoves[depth].GetMove1();
	}
	MOVE& GetKiller2( int depth ){
		return kmoves[depth].GetMove2();
	}
#endif
};

/************************************************
WORKER 並列探索
************************************************/
class Think;

#define MAX_THREADS				8
#define TREE_ROOT				((int)-2)
#define TREE_NULL				((int)-1)

struct WORKER{
#ifdef WIN32
	HANDLE hThread;
#else
	pthread_t tid;
#endif
	Think* pthink;
	int tree;
	int worker;
	bool job;
};

/************************************************
Think 思考ルーチン
************************************************/
// 打ち切りフラグ
// 探索を途中で打ち切ったら、
// ハッシュ表への登録はしないことに注意!!
#define INTERRUPT			( (tree->abort) || ( (iflag) != NULL && (*(iflag)) != 0 ) )

class Think : public ConstData{
private:
	Tree* trees;                  // 探索木
	int tree_num; 
	WORKER* workers;              // Wokers
	int worker_num;               // workerの総数
	int _worker_num;
	int idle_num;                 // idleなworkerの数
	int idle_tree;                // 空きtreeの数
	HashTable* hash_table;        // ハッシュテーブル
	DfPnTable* dfpn_table;        // DfPn用ハッシュテーブル
	History* history;             // history huristics
	int root_alpha;               // root局面のalpha値
	int root_beta;                // root局面のbeta値
	int root_depth;               // root局面の探索深さ
	int limit_max;                // 思考を打ち切る時間の上限
	int limit_min;                // 思考を打ち切る時間の目安
	uint64 limit_node;            // 探索を打ち切るノード数
	bool parallel;                // 並列化
	THINK_INFO info;              // 探索情報
	const int* iflag;             // 中断フラグへのポインタ
#ifndef NLEARN
	bool is_learning;             // 学習用
#endif
#if FUT_EXT
#define FUT_MOVE_MAX			100
	int fut_mgn[DINC*MAX_DEPTH][FUT_MOVE_MAX];
	                              // 深さ毎のマージ
	int gain[GRY+1][0x100];       // ゲイン
	int move_count[DINC*MAX_DEPTH];
#endif
	int thread_num;

	// 排他処理
	MUTEX_SPLIT mutex_split;

	enum{
		STATE_NON    = 0x0000,
		STATE_NULL   = 0x0001, // null move pruning
		STATE_RECAP  = 0x0002, // recapture
		STATE_MATE   = 0x0004, // mate threat
		STATE_ROOT   = 
			STATE_NULL | STATE_RECAP,
			// state of root node
	};

public:
	enum{
		SEARCH_NULL = 0x0000, // オプションなし
		SEARCH_DISP = 0x0001, // 画面表示
		SEARCH_EASY = 0x0002, // 簡単な局面での打ち切り
	};

	// think.cpp
private:
	void Init4Root( int alpha = VMIN, int beta = VMAX, int depth = MAX_DEPTH ){
		history->Clear();   // historyの初期化
		root_alpha = alpha; // rootのalpha値
		root_beta  = beta;  // rootのbeta値
		root_depth = depth; // rootの探索深さ
#if FUT_EXT
		memset( gain, 0, sizeof(gain) );
#endif
	}
#if FUT_EXT
	int GetFutMgn( int depth, int mvcnt ){
#if 1
		return fut_mgn[max(depth,0)][min(mvcnt,FUT_MOVE_MAX)];
#else
		return depth < 7 * DINC ? fut_mgn[max(depth,0)][min(mvcnt,FUT_MOVE_MAX)] : VMAX;
#endif
	}
	void UpdateGain( KOMA koma, int to, int before, int after ){
		gain[koma][to] = max( -( before + after ), gain[koma][to] - 1 );
	}
	int GetGain( KOMA koma, int to ){
		return gain[koma][to];
	}
#endif
#if RAZORING
	int GetRazorMgn( int depth ){
		return 192 + 2 * depth;
	}
#endif
#if MOVE_COUNT_PRUNING
	int GetMoveCount( int depth ){
		return depth < 7 * DINC ? move_count[max(depth,0)] : MAX_MOVES;
	}
#endif
	int Quiescence( Tree* tree, int depth, int qdepth, int alpha, int beta );
	int NegaMaxR( Tree* __restrict__ tree, int depth, int rdepth, int alpha, int beta, unsigned state );
	int IDeepening( Tree* tree, int depth, int rdepth, int alpha, int beta, MOVE* pbmove );
	void DfPn( Tree* tree, int check, int* ppn, int* pdf );
	bool Mate1Ply( Tree* tree, MOVE* pmove );
	bool Mate3Ply( Tree* tree );
	bool MateAnd( Tree* tree, bool captured );
public:
	Think( int _thread_num = 1 );
	virtual ~Think();
	HashTable* ReplaceHashTable( HashTable* hash_table_new ){
		HashTable* hash_table_old = hash_table;
		hash_table = hash_table_new;
		return hash_table_old;
	}
	void SetNumberOfThreads( int _thread_num );
	int GetNumberOfThreads(){
		return thread_num;
	}
	void IsLearning(){
#ifndef NLEARN
		is_learning = true;
#endif
	}
	void IsNoLearning(){
#ifndef NLEARN
		is_learning = false;
#endif
	}
	void PrintSearchInfo( const char* str );
#define PVS_ROOT		1
	int NegaMax( ShogiEval* pshogi, int mdepth,
		int alpha, int beta, THINK_INFO* _info, const int* _iflag )
	{
		int ret = VMAX;
		unsigned state = STATE_ROOT;
		memset( &info, 0, sizeof(THINK_INFO) );
		iflag = _iflag;
#if KILLER
		trees[0].kmoves[0].Init();
#endif
		//trees[0].shek_table->Set( pshogi );
		trees[0].Copy( *pshogi );
		Init4Root( alpha, beta, mdepth );
#if PVS_ROOT
		// PVS
		if( alpha > VMIN && beta > alpha + 1 ){
			ret = NegaMaxR( &(trees[0]), 0, mdepth * DINC,
				alpha, alpha + 1, state );
		}
		if( ret > alpha && beta > alpha + 1 ){
			state &= ~STATE_NULL;
#endif
			ret = NegaMaxR( &(trees[0]), 0, mdepth * DINC,
				alpha, beta, state );
#if PVS_ROOT
		}
#endif
		//trees[0].shek_table->Unset( pshogi );
		if( _info ){ *_info = info; }
		return ret;
	}
	int IDeepening( ShogiEval* pshogi, int mdepth,
		int alpha, int beta, MOVE* pmove,
		THINK_INFO* _info, const int* _iflag )
	{
		int ret;
		memset( &info, 0, sizeof(THINK_INFO) );
		iflag = _iflag;
#if KILLER
		trees[0].kmoves[0].Init();
#endif
		//trees[0].shek_table->Set( pshogi );
		trees[0].Copy( *pshogi );
		Init4Root( alpha, beta, mdepth );
		ret = IDeepening( &(trees[0]), 0, mdepth * DINC,
			alpha, beta, pmove );
		//trees[0].shek_table->Unset( pshogi );
		if( _info ){ *_info = info; }
		return ret;
	}
	int Search( ShogiEval* pshogi, MOVE* pmove, int mdepth,
		THINK_INFO* _info, const int* _iflag, unsigned opt );
	int DfPnSearch( ShogiEval* pshogi, MOVE* pmove,
		Moves* pmoves, THINK_INFO* _info, const int* _iflag );
	int PrincipalVariationDfPnOr( Tree* tree, int depth );
	int PrincipalVariationDfPnAnd( Tree* tree, int depth );
	void GetPVMove( Moves& moves ){
		trees[0].pvmove->GetMoves( moves );
	}
	void InitHashTable(){
		hash_table->Clear();
	}
	bool Mate1Ply( ShogiEval* pshogi, MOVE* pmove ){
		trees[0].Copy( *pshogi );
		return Mate1Ply( &(trees[0]), pmove );
	}
	bool Mate3Ply( ShogiEval* pshogi ){
		trees[0].Copy( *pshogi );
		return Mate3Ply( &(trees[0]) );
	}
#if INANIWA
	int GetInaValue( MOVE move );
	bool IsInaniwa( ShogiEval* pshogi, KOMA sengo );
#endif

	// thread.cpp
private:
	void MutexLockSplit( MUTEX_SPLIT& ms ){
#ifdef WIN32
		//WaitForSingleObject( hMutex_split, INFINITE );
		long l;
		while( 1 ){
			// _InterlockedExchange
			// ポインタが指す先の値を入れ替えて、
			// 入れ替える前の値を返す。
			// この操作は排他的に行われる。
			l = InterlockedExchange( &ms, 1 );

			// 入れ替え前が0だったら成功
			if( l == 0 ){ return; }

			// フラグが落ちるまで待つ。
			// VC2003以降、またはMinGWでは最適化されて動かない。
			//while( ms != 0 ) 
			//	;
		}
#else
		pthread_mutex_lock( &ms );
#endif
	}
	void MutexUnlockSplit( MUTEX_SPLIT& ms ){
#ifdef WIN32
		//ReleaseMutex( hMutex_split );
		ms = 0;
#else
		pthread_mutex_unlock( &ms );
#endif
	}
	void MutexLockSplit(){
		MutexLockSplit( mutex_split );
	}
	void MutexUnlockSplit(){
		MutexUnlockSplit( mutex_split );
	}
	void YieldTimeSlice(){
#ifdef WIN32
		Sleep( 0 );
#else
		sched_yield();
#endif
	}
	void CreateWorkers( ShogiEval* pshogi );
	void DeleteWorkers( /*ShogiEval* pshogi, */THINK_INFO& info );
	bool Split( Tree* tree, int depth, int rdepth,
		int value, int alpha, int beta, unsigned state, int mvcnt
#if MOVE_COUNT_PRUNING
		, MOVE& threat
#endif
		);
#ifdef WIN32
	static unsigned __stdcall WorkerThread( LPVOID p );
#else
	static void* WorkerThread( void* p );
#endif
	void WaitJob( WORKER* pw, Tree* suspend );
	void DoJob( Tree* tree );
	void SetAbort( int parent ){
		// parentから分岐した全ての子Treeを中断
		int t;
		Tree* tree;
		// 全てのTreeについて、
		MutexLockSplit();
		for( t = 1 ; t < tree_num ; t++ ){
			// 親をたどってparentに到達したら、
			for( tree = &trees[t] ; tree->parent >= 0 ; tree = &trees[tree->parent] ){
				if( tree->parent == parent ){
					// 中断フラグをたてる。　
					trees[t].abort = true;
				}
			}
		}
		MutexUnlockSplit();
	}
public:
	void DebugPrintWorkers();
	void PresetTrees( ShogiEval* pshogi );

	// limit.cpp
private:
	static int lim_iflag;
public:
	static void sigint( int );
#if defined(WIN32) && defined(_WIN64)
	static VOID CALLBACK sigtime( UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 );
#elif defined(WIN32)
	static VOID CALLBACK sigtime( UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2 );
#else
	static void sigtime( int );
#endif
	static void SetIFlag( int f ){
		lim_iflag = f;
	}
	void SetLimit( int limit ){
		if( limit > 0 ){
			limit_max = limit;
			limit_min = limit * 1 / 10 + 1;
		}
		else{
			limit_max = 0;
			limit_min = 0;
		}
	}
	void UnsetLimit(){
		limit_max = 0;
		limit_min = 0;
	}
	void SetNodeLimit( uint64 node ){
		limit_node = node;
	}
	void UnsetNodeLimit(){
		limit_node = ULLONG_MAX;
	}
	int SearchLimit( ShogiEval* pshogi, Book* pbook, MOVE* pmove, int mdepth,
		THINK_INFO* _info, unsigned opt );
	int DfPnSearchLimit( ShogiEval* pshogi, MOVE* pmove, Moves* pmoves,
		THINK_INFO* _info, unsigned limit_msec );
};

/************************************************
合議
************************************************/
class Consultation{
public:
	virtual ~Consultation(){
	}
	virtual void SetNumberOfSearchers( int _cons_num ) = 0;
	virtual void SetDeviation( double _deviation ) = 0;
	virtual void SetNumberOfThreads( int thread_num ) = 0;
	virtual int GetNumberOfThreads() = 0;
	virtual int EnemyTurnSearch( ShogiEval* pshogi, MOVE* pmove, int mdepth,
		THINK_INFO* _info, const int* _iflag, unsigned opt ) = 0;
	virtual void SetLimit( int limit ) = 0;
	virtual void UnsetLimit() = 0;
	virtual void SetNodeLimit( uint64 node ) = 0;
	virtual void UnsetNodeLimit() = 0;
	virtual int SearchLimit( ShogiEval* pshogi, Book* pbook, MOVE* pmove, int mdepth,
		THINK_INFO* _info, unsigned opt ) = 0;
	virtual void GetPVMove( Moves& moves ) = 0;
	virtual void SetIFlag( int f ) = 0;
	virtual void PresetTrees( ShogiEval* pshogi ) = 0;
};

#if 0
/************************************************
合議(並列実行)
************************************************/
class ConsultationParallel : public Consultation{
private:
	Think* thinks;
	int    cons_num;
	int    delegate;

private:
#ifdef WIN32
	static unsigned __stdcall SearchThread( LPVOID p );
#else
	static void* SearchThread( void* p );
#endif
#ifdef WIN32
	static unsigned __stdcall SearchLimitThread( LPVOID p );
#else
	static void* SearchLimitThread( void* p );
#endif
public:
	ConsultationParallel( int _cons_num = 2, int thread_num = 1 );
	~ConsultationParallel();
	void SetNumberOfSearchers( int _cons_num );
	void SetDeviation( double _deviation );
	void SetNumberOfThreads( int thread_num );
	int GetNumberOfThreads();
	int EnemyTurnSearch( ShogiEval* pshogi, MOVE* pmove, int mdepth,
		THINK_INFO* _info, const int* _iflag, unsigned opt );
	void SetLimit( int limit ){
		for( int i = 0 ; i < cons_num ; i++ ){
			thinks[i].SetLimit( limit );
		}
	}
	void UnsetLimit(){
		for( int i = 0 ; i < cons_num ; i++ ){
			thinks[i].UnsetLimit();
		}
	}
	int SearchLimit( ShogiEval* pshogi, Book* pbook, MOVE* pmove, int mdepth,
		THINK_INFO* _info, unsigned opt );
	void GetPVMove( Moves& moves ){
		thinks[delegate].GetPVMove( moves );
	}
};
#endif

/************************************************
合議(1スレッド実行)
************************************************/
class ConsultationSerial : public Consultation{
private:
	Think think;
	int   cons_num;
	Moves best_pv;
	double deviation;
	HashTable* hash_table_a;
	Evaluate* eval_a;
	bool eval_loaded;

public:
	ConsultationSerial( int _cons_num = 2, int _thread_num = 1 );
	~ConsultationSerial();
	void SetNumberOfSearchers( int _cons_num ){
		if( cons_num != _cons_num ){
			delete [] hash_table_a;
			delete [] eval_a;
			hash_table_a = new HashTable   [_cons_num];
			eval_a       = new EvaluateNull[_cons_num];
			eval_loaded  = false;
		}
		cons_num = _cons_num;
	}
	void SetDeviation( double _deviation ){
		deviation = _deviation;
	}
	void SetNumberOfThreads( int _thread_num ){
		think.SetNumberOfThreads( _thread_num );
	}
	int GetNumberOfThreads(){
		return think.GetNumberOfThreads();
	}
	int EnemyTurnSearch( ShogiEval* pshogi, MOVE* pmove, int mdepth,
		THINK_INFO* _info, const int* _iflag, unsigned opt ){
		return think.Search( pshogi, pmove, mdepth, _info, _iflag, opt );
	}
	void SetLimit( int limit ){
		think.SetLimit( limit );
	}
	void UnsetLimit(){
		think.UnsetLimit();
	}
	void SetNodeLimit( uint64 node ){
		think.SetNodeLimit( node );
	}
	void UnsetNodeLimit(){
		think.UnsetNodeLimit();
	}
	int SearchLimit( ShogiEval* pshogi, Book* pbook, MOVE* pmove, int mdepth,
		THINK_INFO* _info, unsigned opt );
	void GetPVMove( Moves& moves ){
		best_pv.CopyTo( moves );
	}
	void SetIFlag( int f ){
		think.SetIFlag( f );
	}
	void PresetTrees( ShogiEval* pshogi ){
		think.PresetTrees( pshogi );
	}
};

/************************************************
合議なし
************************************************/
class ConsultationSingle : public Consultation{
private:
	Think think;

public:
	ConsultationSingle( int _thread_num = 1 ) : think( _thread_num ){
	}
	~ConsultationSingle(){
	}
	void SetNumberOfSearchers( int ){
	}
	void SetDeviation( double ){
	}
	void SetNumberOfThreads( int _thread_num ){
		think.SetNumberOfThreads( _thread_num );
	}
	int GetNumberOfThreads(){
		return think.GetNumberOfThreads();
	}
	int EnemyTurnSearch( ShogiEval* pshogi, MOVE* pmove, int mdepth,
		THINK_INFO* _info, const int* _iflag, unsigned opt )
	{
		return think.Search( pshogi, pmove, mdepth, _info, _iflag, opt );
	}
	void SetLimit( int limit ){
		think.SetLimit( limit );
	}
	void UnsetLimit(){
		think.UnsetLimit();
	}
	void SetNodeLimit( uint64 node ){
		think.SetNodeLimit( node );
	}
	void UnsetNodeLimit(){
		think.UnsetNodeLimit();
	}
	int SearchLimit( ShogiEval* pshogi, Book* pbook, MOVE* pmove,
		int mdepth, THINK_INFO* _info, unsigned opt )
	{
		return think.SearchLimit( pshogi, pbook, pmove, mdepth, _info, opt );
	}
	void GetPVMove( Moves& moves ){
		think.GetPVMove( moves );
	}
	void SetIFlag( int f ){
		think.SetIFlag( f );
	}
	void PresetTrees( ShogiEval* pshogi ){
		think.PresetTrees( pshogi );
	}
};

/************************************************
投了
************************************************/
#define RESIGN_VALUE_MIN				500
#define RESIGN_VALUE_DEF				2500
class ResignController{
private:
	int limit;

public:
	ResignController( int _limit = RESIGN_VALUE_DEF ){
		if( !set_limit( _limit ) ){
			limit = RESIGN_VALUE_DEF;
		}
	}
	bool set_limit( int _limit ){
		if( _limit >= RESIGN_VALUE_MIN ){
			limit = _limit;
			return true;
		}
		else{
			return false;
		}
	}
	bool good( const THINK_INFO& info ) const{
		return -info.value < limit;
	}
	bool bad( const THINK_INFO& info ) const{
		return -info.value >= limit;
	}
};

/************************************************
対局の設定情報
************************************************/
struct MATCH_CONF{
	int com_s;   // 先手番がCOMか
	int com_g;   // 後手番がCOMか
	int depth;   // 読みの深さ
	int limit;   // 制限時間
	int node;    // 制限ノード数
	int resign;  // 投了のしきい値
	int csa;     // CSA形式で表示
	int repeat;  // 自己対局の回数
	int aquit;   // 自動でquit(コマンドモードに入らない。)
	char* fname; // 棋譜ファイル
};
#define DEF_CONF			{ 0, 0, DEF_DEPTH, DEF_LIMIT, U64(0), RESIGN_VALUE_DEF, 0, 0, 0, NULL }

/************************************************
Match 将棋の対局
************************************************/
class Match : public Think{
	// match.cpp
public:
	Match( int thread_num = 1 ) : Think( thread_num ){
	}
	int MatchConsole( MATCH_CONF* pconfig );
	int MatchSelf( MATCH_CONF* pconfig );
	int NextMove( MATCH_CONF* pconfig );
	int AgreementTest( const char* dir, MATCH_CONF* pconfig );

	int InputMove( MOVE* move, const char str[], KOMA sengo );
	int PrintMoves( Shogi* pshogi );
	int PrintCaptures( Shogi* pshogi );
	int PrintNoCaptures( Shogi* pshogi );
	int PrintTacticalMoves( Shogi* pshogi );
	int PrintCheck( Shogi* pshogi );
	int PrintBook( Shogi* pshogi, Book* pbook );
	int PrintBookEval( Shogi* pshogi, Book* pbook );
	void PrintThinkInfo( THINK_INFO* info );
	void PrintThinkInfoMate( THINK_INFO* info );
};

/************************************************
MatchCSA CSAプロトコルによる対局
************************************************/
#define FNAME_NCONF				"nconf"
#define FNAME_CSALOG			"csalog.csv"

#define CSA_PORT				4081

#define MATCH_CSA_LOGIN_OK		0x00000001
#define MATCH_CSA_LOGIN_INC		0x00000002
#define MATCH_CSA_LOGOUT		0x00000004
#define MATCH_CSA_START			0x00000008
#define MATCH_CSA_REJECT		0x00000010
#define MATCH_CSA_WIN			0x00000020
#define MATCH_CSA_LOSE			0x00000040
#define MATCH_CSA_DRAW			0x00000080
#define MATCH_CSA_CHUDAN		0x00000100
#define MATCH_CSA_SENTE			0x00000200
#define MATCH_CSA_GOTE			0x00000400
#define MATCH_CSA_SUMMARY		0x00000800
#define MATCH_CSA_SENNICHI		0x00001000
#define MATCH_CSA_OUTE_SEN		0x00002000
#define MATCH_CSA_ILLEGAL		0x00004000
#define MATCH_CSA_TIME_UP		0x00008000
#define MATCH_CSA_RESIGN		0x00010000
#define MATCH_CSA_JISHOGI		0x00020000
#define MATCH_CSA_ERROR			0x80000000

#define MATCH_CSA_MASK_END		( MATCH_CSA_WIN | MATCH_CSA_LOSE | MATCH_CSA_DRAW | MATCH_CSA_CHUDAN )
#define MATCH_CSA_MASK_MOVE		( MATCH_CSA_SENTE | MATCH_CSA_GOTE )

#define DEF_TIMEOUT			600

// 対局の結果
enum{
	RES_DRAW = 0x00U, // 引き分け/無効試合
	RES_WIN  = 0x01U, // 勝ち
	RES_LOSE = 0x02U, // 負け
	RES_REP  = 0x04U, // 千日手
	RES_TIME = 0x08U, // 時間切れ
	RES_ILL  = 0x10U, // 反則
	RES_ERR  = 0x20U, // エラー
};

typedef unsigned int GAME_RESULT;

// 対局情報
struct GAME_INFO{
	char game_name[256]; // 対局名
	char sente[256];     // 先手の対局者名
	char gote[256];      // 後手の対局者名
	KOMA sengo;          // 自分の手番
	KOMA to_move;        // 開始直後の手番
	GAME_RESULT result;  // 対局結果
};

class MatchCSA{
private:
	// 合議対応版
	Consultation* pcons;
	int cons_num;
	double deviation;
	uint64 limit_node;

#ifdef WIN32
	SOCKET sock;
#else
	int sock;
#endif
	char host[256];
	char user[256];
	char pass[256];

	int depth;
	int limit;            // 1手あたりの思考時間
	int repeat;           // 連続対局回数
	//int timeout;          // 受信時のタイムアウト
#ifdef POSIX
	unsigned keepalive;   // keep-alive on/off
	unsigned keepidle;    // keep-alive開始までのidle時間
	unsigned keepintvl;   // keep-aliveの間隔
	unsigned keepcnt;     // 接続断を判断するまでの回数
#endif
	vector<string> timer; // ログインスケジュール
	int floodgate;        // floodgateモード
	char kifu_dir[1024];  // 棋譜の保存先

	char* next;
	char buf[2048];
	char line[1024];
	char mstrS[256];
	char mstrG[256];

	unsigned int rflag;   // レシーブしたコマンド
	int eflag;            // 相手番思考終了フラグ

	CONS_TYPE consultationType; // 合議
	bool enemyTurnSearch; // 相手番思考

	GAME_INFO info;
	DateTime start;
	DateTime end;

	ShogiEval* pshogi;
	Book* pbook;

	ResignController resign; // 投了判定

	AvailableTime atime; // 持ち時間

#ifdef WIN32
	HANDLE hMutex;       // Mutex
#else
	pthread_mutex_t mutex;
#endif

	// csa.cpp
private:
	int StopFlag();
	int Timer();
	int LoadConfig();
	void SetResult( GAME_RESULT res = 0x00U );
	int WriteResult( int itr );
	int ReceiveMessage();
	int SendMessage( const char* str, const char* disp = NULL );
	int Connect();
	int Disconnect();
	int Login();
	int Logout();
	int SendAgree();
	int SendReject();
	int InputTime( const char* mstr );
	int SendMove( THINK_INFO* pthink_info );
	int SendToryo();
#ifdef WIN32
	static unsigned __stdcall EnemyTurnSearchThread( LPVOID p );
#else
	static void* EnemyTurnSearchThread( void* p );
#endif
#ifdef WIN32
	static unsigned __stdcall ReceiveThread( LPVOID p );
#else
	static void* ReceiveThread( void* p );
#endif
	unsigned int GetRecFlag( unsigned int mask );
	void MutexLock(){
#ifdef WIN32
		WaitForSingleObject( hMutex, INFINITE );
#else
		pthread_mutex_lock( &mutex );
#endif
	}
	void MutexUnlock(){
#ifdef WIN32
		ReleaseMutex( hMutex );
#else
		pthread_mutex_unlock( &mutex );
#endif
	}
	const char* MoveStrSelf(){
		if( info.sengo == SENTE ){
			return mstrS;
		}
		else{
			return mstrG;
		}
	}
	const char* MoveStrEnemy(){
		if( info.sengo == SENTE ){
			return mstrG;
		}
		else{
			return mstrS;
		}
	}
	int OutputFileCSA( const char* fname ){
		KIFU_INFO kinfo;
		kinfo.event = info.game_name;
		kinfo.sente = info.sente;
		kinfo.gote  = info.gote;
		kinfo.start = start;
		kinfo.end   = end;
		return pshogi->OutputFileCSA( fname, &kinfo, &atime );
	}
public:
	MatchCSA( int thread_num = 1 );
	~MatchCSA();
	int MatchNetwork();
};

/************************************************
学習
************************************************/
#define FNAME_LCONF			"lconf"       // 設定
#define FNAME_PVMOVE		"pvmove"      // PV-move
#define FNAME_GRADIENT		"gradient.gr" // 勾配
#define FNAME_LINFO			"learn.info"  // LEARN_INFO
#define FNAME_EVINFO 		"evinfo"      // Ev-info
#define FNAME_DETAIL 		"detail"      // detail

#define DEF_SWINDOW			256
#define DEF_STEPS			1
#define DEF_ADJUST			1
#define DEF_TNUM			1
#define DEF_RJRATE			1.0

// Experimentation ランダムな教師例選択
#define SELECT_RAND				0
#define SELECT_1ST_TIME_ONLY	0 // 初回のみ
// この下はいじらない。
#if !SELECT_RAND
#	undef SELECT_1ST_TIME_ONLY
#	define SELECT_1ST_TIME_ONLY		0
#endif

class LEARN_INFO{
public:
	int swindow;   // 探索窓
	int num;       // 局面数　
	int loss_base;
	double loss;   // 損失
	double loss0;
	int time;
	int total;
	int adjust;
	int record;

public:
	LEARN_INFO(){
		Init();
	}
	void Init(){
		memset( this, 0, sizeof(LEARN_INFO) );
		swindow = DEF_SWINDOW;
		record = INT_MAX;
	}
	bool Output( const char* fname );
	bool Input( const char* fname );
};

class Learn;

struct THREAD_INFO{
	unsigned int num;  // Thread number
	ShogiEval* pshogi;
	Param* pparam;
	Learn* plearn;
#ifdef WIN32
	HANDLE hMutex;
#else
	pthread_mutex_t* pmutex;
#endif
#if SELECT_1ST_TIME_ONLY
	string fname;
#endif
};

class Learn : public ConstData{
protected:
	FileList flist;  // List of CSA files
	Evaluate* eval;  // Evaluation
	LEARN_INFO info; // Information

	int mode;             // LMODE
	char directory[1024]; // directory of CSA files
	char gradients[1024]; // directory of .gr files
	int steps;            // steps
	int iteration;        // number of iteration
	int adjust;           // adjustments count
	int tnum;             // number of threads
	int swmin;            // Minimum of search window
	int swmax;            // maximum of search window
	int start;            // 開始ステップ
	int resume;           // resume mode
	bool detail;          // detail log

	int swindow;     // search window
	int fnum;        // number of analyzed files

public:
	enum{
		MODE_DEFAULT = 0,
		MODE_NORMAL,
		MODE_PVMOVE,
		MODE_GRADIENT,
		MODE_ADJUSTMENT,
	};

	// learn.cpp
private:
	int LearnNormal();
	int LearnPVMove();
	int LearnGradient();
	int LearnAdjust();
	int Learn1();
	void WriteMoves( ofstream& fout, const MOVE& move0, Moves& moves );
	int ReadMoves( ifstream& fin, Moves& moves );
	bool PVOk( THREAD_INFO& th, Moves& pv, int value, int alpha, int beta );
	int MakePVMoves( THREAD_INFO& th );
	int Learn2();
	int Gradient( THREAD_INFO& th );
	int GetDegreeOfFreedomS( double dps, const KOMA* ban,
		const int diffS, const int diffG,
		int addr, const int dir,
		float free[][289][9],
		float effect[][289][20]
		)
	{
		// 自由度と利きの計算
		KOMA koma;
		int cnt = 0;
		addr += dir;
		while( ( koma = ban[addr] ) != WALL ){
			if( koma & SENTE ){
				effect[Param::S][diffS][sente_num[koma]] += dps;
				effect[Param::G][diffG][ gote_num[koma]] -= dps;
				break;
			}
			cnt++;
			if( koma != EMPTY ){
				effect[Param::S][diffS][sente_num[koma]] += dps;
				effect[Param::G][diffG][ gote_num[koma]] -= dps;
				break;
			}
			addr += dir;
		}
		free[Param::S][diffS][cnt] += dps;
		free[Param::G][diffG][cnt] -= dps;

		return cnt;
	}
	int GetDegreeOfFreedomG( double dps, const KOMA* ban,
		const int diffS, const int diffG,
		int addr, const int dir,
		float free[][289][9],
		float effect[][289][20]
		)
	{
		// 自由度と利きの計算
		KOMA koma;
		int cnt = 0;
		addr += dir;
		while( ( koma = ban[addr] ) != WALL ){
			if( koma & GOTE ){
				effect[Param::G][diffS][sente_num[koma]] += dps;
				effect[Param::S][diffG][ gote_num[koma]] -= dps;
				break;
			}
			cnt++;
			if( koma != EMPTY ){
				effect[Param::G][diffS][sente_num[koma]] += dps;
				effect[Param::S][diffG][ gote_num[koma]] -= dps;
				break;
			}
			addr += dir;
		}
		free[Param::G][diffS][cnt] += dps;
		free[Param::S][diffG][cnt] -= dps;

		return cnt;
	}
	double Penalty( Param* p );
	int AdjustmentValue( int v, double d, double penalty );
protected:
	const char* GetBackupName(){
		static char backup[64];
		sprintf( backup, "evdata_%03d", iteration );
		return backup;
	}
	int LoadConfig();
	int ConfigLog();
	int LearnLog();
	double Loss( double x );
	double dLoss( double x );
	double dLossPC( double x, double depth );
	void IncParam( ShogiEval* pshogi, Param* p, double d );
	void IncParam2( ShogiEval* pshogi, Param* p, double d );
	void AdjustParam( Param* p );
public:
	Learn();
	virtual ~Learn();
	int LearnMain( int _mode = MODE_DEFAULT );
#ifdef WIN32
	static unsigned __stdcall Learn1Thread( LPVOID p );
	static unsigned __stdcall Learn2Thread( LPVOID p );
#else
	static void* Learn1Thread( void* p );
	static void* Learn2Thread( void* p );
#endif
};

/************************************************
PositionList 局面リスト 2010/09/23
************************************************/
struct POSITION{
	string fname; // 棋譜のファイル名
	int num;      // 局面までの手数
};

class PositionList{
private:
	vector<POSITION> pos_list;
	vector<POSITION>::iterator it;
public:
	PositionList(){
		it = pos_list.begin();
	}
	virtual ~PositionList(){
	}
	void Clear(){
		pos_list.clear();
		it = pos_list.begin();
	}
	void Enqueue( const string& fname, int num ){
		POSITION pos;
		pos.fname = fname;
		pos.num = num;
		pos_list.push_back( pos );
	}
	bool Dequeue( string& fname, int& num ){
		if( it != pos_list.end() ){
			fname = (*it).fname;
			num = (*it).num;
			it++;
			return true;
		}
		return false;
	}
	int Size(){
		return pos_list.size();
	}
	void Shuffle(){
		int size;
		int i, j;
		POSITION temp;

		size = pos_list.size();
		for( i = size - 1 ; i > 0 ; i-- ){
			j = (int)( gen_rand32() % ( i + 1 ) );
			if( i != j ){
				temp = pos_list[i];
				pos_list[i] = pos_list[j];
				pos_list[j] = temp;
			}
		}

		it = pos_list.begin();
	}
};

/************************************************
学習 Averaged Perceptron 2010/09/23
************************************************/
class LearnAP : public Learn{
private:
	PositionList pos_list; // 局面リスト
	Param* pparam;         // 現在のパラメータ
	Param* pcumurate;      // パラメータの累計
	int cum_cnt;           // 足し込んだ回数

public:
	LearnAP();
	virtual ~LearnAP();
	bool LearnAPMain();
	bool LearnAPMethod();
	void EnumeratePos();
	void AveragedPerceptron();
	void OutputAverage();
	int EvalMargin( int num );
};

#endif
