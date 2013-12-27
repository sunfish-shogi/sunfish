/* shogi2.cpp
 * R.Kubo 2010-2012
 * Shogiクラスに局面評価を追加したShogiEvalクラス
 */

#include "shogi.h"

ShogiEval::ShogiEval( Evaluate* _eval ) : Shogi(){
/************************************************
コンストラクタ
平手の初期局面で初期化される。
************************************************/
	// 局面評価用
	if( _eval == NULL ){
		eval = new Evaluate();
		evalloc = true;
	}
	else{
		eval = _eval;
		evalloc = false;
	}

	UpdateBan();
}

ShogiEval::ShogiEval( const char* evdata,
	Evaluate* _eval ) : Shogi(){
/************************************************
コンストラクタ(評価データ指定)
平手の初期局面で初期化される。
************************************************/
	// 局面評価用
	if( _eval == NULL ){
		eval = new Evaluate( evdata );
		evalloc = true;
	}
	else{
		eval = _eval;
		evalloc = false;
	}

	UpdateBan();
}

ShogiEval::ShogiEval( const KOMA* _ban0, const int* dai0,
	KOMA sengo, Evaluate* _eval )
	: Shogi( _ban0, dai0, sengo ){
/************************************************
コンストラクタ
任意の局面で初期化される。
************************************************/
	// 局面評価用
	if( _eval == NULL ){
		eval = new Evaluate();
		evalloc = true;
	}
	else{
		eval = _eval;
		evalloc = false;
	}

	UpdateBan();
}

ShogiEval::ShogiEval( const KOMA* _ban0, const int* dai0,
	KOMA sengo, const char* evdata, Evaluate* _eval )
	: Shogi( _ban0, dai0, sengo ) {
/************************************************
コンストラクタ(評価データ指定)
任意の局面で初期化される。
************************************************/
	// 局面評価用
	if( _eval == NULL ){
		eval = new Evaluate( evdata );
		evalloc = true;
	}
	else{
		eval = _eval;
		evalloc = false;
	}

	UpdateBan();
}

ShogiEval::ShogiEval( TEAI teai, Evaluate* _eval )
	: Shogi( teai ){
/************************************************
コンストラクタ
任意の局面で初期化される。
************************************************/
	// 局面評価用
	if( _eval == NULL ){
		eval = new Evaluate();
		evalloc = true;
	}
	else{
		eval = _eval;
		evalloc = false;
	}

	UpdateBan();
}

ShogiEval::ShogiEval( TEAI teai, const char* evdata,
	Evaluate* _eval ) : Shogi( teai ){
/************************************************
コンストラクタ(評価データ指定)
任意の局面で初期化される。
************************************************/
	// 局面評価用
	if( _eval == NULL ){
		eval = new Evaluate( evdata );
		evalloc = true;
	}
	else{
		eval = _eval;
		evalloc = false;
	}

	UpdateBan();
}

ShogiEval::ShogiEval( const ShogiEval& org ) : Shogi( org ){
/************************************************
コンストラクタ
既存のインスタンスから局面をコピー
************************************************/
	// Shogi::Shogi(org)から
	// Shogi::Copy(org)が呼ばれるので
	// 残りの処理をここでやる。

	// Shogi::Copy(org)がvirtualになっていても
	// ShogiEval::ShogiEval(org)が終わるまでは
	// ShogiEval::Copy(org)は呼ばれない。

	// 評価値
	memcpy( evhist, org.evhist, sizeof(evhist[0]) * ( knum + 1 ) );

	// Evaluateクラス
	// (コピー元クラスで解放されると困ってしまう..)
	eval = org.eval;
	evalloc = false;
}

ShogiEval::ShogiEval( ShogiEval& org, Evaluate* _eval ){
/************************************************
コンストラクタ
既存のインスタンスから局面をコピー
Evaluateが異なる場合用
************************************************/
	int num = org.GetNumber();

	// Evaluateクラス
	if( _eval == NULL ){
		eval = new Evaluate();
		evalloc = true;
	}
	else{
		eval = _eval;
		evalloc = false;
	}

	// 局面を戻す。
	while( org.GoBack() )
		;

	// 盤面コピー
	SetBan( org._ban, org.dai, org.ksengo );

	// 局面を進めながらコピー
	while( org.GetNumber() < num ){
		MoveD( org.kifu[org.know].move );
		org.GoNext();
	}
}

ShogiEval::~ShogiEval(){
/************************************************
デストラクタ
************************************************/
	if( evalloc ){
		delete eval;
	}
}

void ShogiEval::UpdateBan(){
/************************************************
盤面を更新したときの処理
************************************************/
	Shogi::UpdateBan();

	// 評価値
	evhist[0].material = eval->GetValue0( this );
	if( sou != 0 && gou != 0 ){
		eval->GenerateListOU<OU>( &evhist[0], this );
		eval->GetValue1_dis2_ns( &evhist[0], this );
	}
}

void ShogiEval::Copy( const ShogiEval& org ){
/************************************************
盤面のコピー
************************************************/
	Shogi::Copy( (Shogi&)org );

	// 評価値
	memcpy( evhist, org.evhist, sizeof(evhist[0]) * ( knum + 1 ) );

	// Evaluateクラス
	// (コピー元クラスで解放されると困ってしまう..)
	if( evalloc ){
		delete eval;
	}
	eval = org.eval;
	evalloc = false;
}

void ShogiEval::ValueList( VLIST sv[], VLIST gv[], int to ){
/************************************************
駒の交換値リストを取得
************************************************/
	int addr;
	KOMA koma;
	int d;
	int dir;
	int eff_s = effectS[to];
	int eff_g = effectG[to];

	int sn = 0;
	int gn = 0;

	// 盤上
	for( d = 0 ; d < DNUM ; d++ ){
		dir = _dir[d];
		// 駒の利きがあるか
		if( eff_s & _dir2bit[-dir] || eff_s & _dir2bitF[-dir]
			|| eff_g & _dir2bit[-dir] || eff_g & _dir2bitF[-dir] )
		{
			for( addr = to + dir ; ban[addr] != WALL ; addr += dir ){
				if( ( koma = ban[addr] ) != EMPTY ){
					// リストへ追加
					if( koma & SENTE ){
						if( ( sv[sn].koma = koma ) == SOU )
							sv[sn].value = VMAX;
						else
							sv[sn].value = eval->GetV_BAN2( koma );
						sn++;
					}
					else{
						if( ( gv[gn].koma = koma ) == GOU )
							gv[gn].value = VMAX;
						else
							gv[gn].value = -( eval->GetV_BAN2( koma ) );
						gn++;
					}
					goto lab_break;
				}
			}
lab_break:
			;
		}
	}

	if( sn >= 2 ) qsort( sv, sn, sizeof(VLIST), ValueComp );
	if( gn >= 2 ) qsort( gv, gn, sizeof(VLIST), ValueComp );

	sv[sn].koma = EMPTY;
	gv[gn].koma = EMPTY;
}

int ShogiEval::see( VLIST* ps, VLIST* pg, int v0 ){
/************************************************
Static Exchange Evaluation
************************************************/
	int v;

	if( ps[0].koma == EMPTY )
		return 0;

	v = -see( pg, ps + 1, ps[0].value );
	if( v >= 0 )
		return v0 + v;

	return 0;
}

int ShogiEval::StaticExchangeN(){
/************************************************
駒の交換値
************************************************/
	int v;
	KOMA koma;
	VLIST sv[32], gv[32];

	if( !IsKingMove() && know > 0 && kifu[know-1].move.from != PASS )
	{
		// 利いている駒のリストを生成
		ValueList( sv, gv, kifu[know-1].move.to );

		koma = ban[kifu[know-1].move.to];
		v = abs( eval->GetV_BAN2( koma ) );

		// Static Exchange Evaluation
		if( ksengo == SENTE )
			return see( sv, gv, v );
		else
			return see( gv, sv, v );
	}

	return 0;
}

void ShogiEval::GoNextFrom( int addr ){
/************************************************
駒の移動元
************************************************/
	if( ban[addr] & 0x07 ){
		eval->GetValueDiff<-1>( &evhist[know], this, addr );
	}
}

void ShogiEval::GoNextTo( int addr ){
/************************************************
駒の移動先
************************************************/
	if     ( ban[addr] == SOU ){
		eval->GenerateListOU<SOU>( &evhist[know], this );
		eval->GetValueDiffSOU( &evhist[know], this );
	}
	else if( ban[addr] == GOU ){
		eval->GenerateListOU<GOU>( &evhist[know], this );
		eval->GetValueDiffGOU( &evhist[know], this );
	}
	else{
		eval->GetValueDiff<+1>( &evhist[know], this, addr );
	}
}
