/* shogi.cpp
 * R.Kubo 2008-2012
 * 将棋の局面と棋譜を管理するShogiクラスのメンバ
 */

#include "shogi.h"

Shogi::Shogi(){
/************************************************
コンストラクタ
平手の初期局面で初期化される。
************************************************/
	// ハッシュの初期化
	phash = new Hash();
	halloc = true;

	// 平手の局面
	SetBan( HIRATE );
}

Shogi::Shogi( const KOMA* _ban0, const int* dai0, KOMA sengo ){
/************************************************
コンストラクタ
任意の局面で初期化される。
************************************************/
	// ハッシュの初期化
	phash = new Hash();
	halloc = true;

	// 盤面の設定
	SetBan( _ban0, dai0, sengo );
}

Shogi::Shogi( TEAI teai ){
/************************************************
コンストラクタ
任意の局面で初期化される。
************************************************/
	// ハッシュの初期化
	phash = new Hash();
	halloc = true;

	// 盤面の設定
	SetBan( teai );
}

Shogi::Shogi( const Shogi& org ){
/************************************************
コンストラクタ
既存のインスタンスから局面をコピー
************************************************/
	halloc = false;
	Copy( org );
}

Shogi::~Shogi(){
/************************************************
デストラクタ
************************************************/
	if( halloc ){
		delete phash;
	}
}

void Shogi::UpdateBan(){
/************************************************
盤面を更新したときの処理
************************************************/
	int i, j;
	int addr;
	KOMA koma;

	knum = 0;
	know = 0;
	kifu[0].chdir = NOJUDGE_CHECK;

	// 利き
	InitEffect();

	// ハッシュ値
	kifu[0].hashB = (uint64)0;
	kifu[0].hashD = (uint64)0;

	// 歩の有無
	kifu[0].sfu = 0x00U;
	kifu[0].gfu = 0x00U;

	// bitboardの初期化
	for( i = 0 ; i <= GRY ; i++ ){
		bb[i] = 0U;
	}
	bb_a = 0U;

	for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
		for( i = 0x01 ; i <= 0x09 ; i += 0x01 ){
			addr = i + j;
			koma = ban[addr];

			if( koma != EMPTY ){
				// ハッシュ値
				kifu[0].hashB ^= phash->Ban( koma, addr );
				// bitboard
				bb[koma].setBit( _addr[addr] );
				bb_a.setBit( _addr[addr] );
			}

			// 歩の有無
			if( koma == SFU ){ kifu[0].sfu |= 1 << i; }
			if( koma == GFU ){ kifu[0].gfu |= 1 << i; }
		}
	}

	if( ksengo == GOTE ){
		kifu[0].hashB ^= phash->Sengo();
	}

	for( koma = SFU ; koma <= GHI ; koma++ ){
		kifu[0].hashD ^= phash->Dai( koma, dai[koma] );
	}
}

void Shogi::SetBan( const KOMA* _ban0, const int* dai0, KOMA sengo ){
/************************************************
任意の局面で初期化する。
現在の棋譜は全て削除される。
************************************************/
	int i;
	KOMA koma;

	sou = 0;
	gou = 0;

	// 駒を並べる
	for( i = 0 ; i < 16 * 13 ; i++ ){
		koma = _ban[i] = _ban0[i];
		if( koma == SOU ){
			sou = i - BAN_OFFSET;
		}
		else if( koma == GOU ){
			gou = i - BAN_OFFSET;
		}
	}

	ban = _ban + BAN_OFFSET;

	// 持駒
	for( i = 0 ; i < 64 ; i++ ){
		dai[i] = dai0[i];
	}

	// 手番
	ksengo = sengo;

	UpdateBan();
}

void Shogi::SetBan( TEAI teai ){
/************************************************
指定した手合で局面を初期化する。
現在の棋譜は全て削除される。
************************************************/
	int i;

	// 駒を並べる
	for( i = 0 ; i < 16 * 13 ; i++ ){
		_ban[i] = _hirate[i];
	}

	ban = _ban + BAN_OFFSET;

	switch( teai ){
	case HIRATE:
		break;

	case M10_OCHI:
		ban[BanAddr(4,9)] = EMPTY;
		ban[BanAddr(6,9)] = EMPTY;
	case M8_OCHI:
		ban[BanAddr(3,9)] = EMPTY;
		ban[BanAddr(7,9)] = EMPTY;
	case M6_OCHI:
		ban[BanAddr(2,9)] = EMPTY;
		ban[BanAddr(8,9)] = EMPTY;
	case M4_OCHI:
		ban[BanAddr(1,9)] = EMPTY;
		ban[BanAddr(9,9)] = EMPTY;
	case M2_OCHI:
		ban[BanAddr(2,8)] = EMPTY;
		ban[BanAddr(8,8)] = EMPTY;
		break;

	case HIKY_OCHI:
		ban[BanAddr(9,9)] = EMPTY;
	case HI_OCHI:
		ban[BanAddr(2,8)] = EMPTY;
		break;

	case KA_OCHI:
		ban[BanAddr(8,8)] = EMPTY;
		break;

	case KY_OCHI:
		ban[BanAddr(9,9)] = EMPTY;
		break;
	}

	// 持駒
	for( i = 0 ; i < 64 ; i++ )
		dai[i] = 0;

	// 玉の位置
	sou = BanAddr( 5, 9 );
	gou = BanAddr( 5, 1 );

	// 手番
	ksengo = SENTE;

	UpdateBan();
}

void Shogi::SetBanEmpty(){
/************************************************
局面を初期化する。
現在の棋譜は全て削除される。
************************************************/
	int i;

	// 駒を並べる
	for( i = 0 ; i < 16 * 13 ; i++ ){
		_ban[i] = _empty[i];
	}

	ban = _ban + BAN_OFFSET;

	// 持駒
	for( i = 0 ; i < 64 ; i++ ){
		dai[i] = 0;
	}

	// 玉の位置
	sou = 0;
	gou = 0;

	// 手番
	ksengo = SENTE;

	UpdateBan();
}

void Shogi::QuickCopy( const Shogi& org ){
/************************************************
簡易コピー
************************************************/
	// 開始局面が同じなら指し手を入力する。
	if( kifu[0].hashB == org.kifu[0].hashB && kifu[0].hashD == org.kifu[0].hashD ){
		int i;

		// ハッシュ値が異なる最初の場所を見つける。
		for( i = 1 ; i < knum && i < org.knum ; i++ ){
			if( kifu[i].hashB != org.kifu[i].hashB || kifu[i].hashD != org.kifu[i].hashD ){
				break;
			}
		}
		i--;

		while( know < i && GoNext() )
			;

		while( know > i && GoBack() )
			;

		// 指し手をインプット
		for( ; i < org.knum ; i++ ){
			MoveD( org.kifu[i].move );
		}

		while( know > org.know && GoBack() )
			;
	}
	// 開始局面が異なる場合はCopyを呼ぶ。
	else{
		Copy( org );
	}
}

void Shogi::Copy( const Shogi& org ){
/************************************************
盤面のコピー
************************************************/
	int i;

	// 盤面
	memcpy( _ban, org._ban, sizeof(_ban) );
	ban = _ban + BAN_OFFSET;
	memcpy( dai, org.dai, sizeof(dai) );
	sou = org.sou;
	gou = org.gou;
	memcpy( bb, org.bb, sizeof(bb) );
	bb_a = org.bb_a;

	// 棋譜
	know = org.know;
	knum = org.knum;
	ksengo = org.ksengo;
	memcpy( kifu, org.kifu, sizeof(KIFU)*(knum+1) );

	// changeだけはアドレスを書き換えないと使えない。
	for( i = 0 ; i < knum ; i++ ){
		MOVE& move = kifu[i].move;

		// 取った駒
		if( kifu[i].change.flag & CHANGE_DAI ){
			//KOMA koma = ( move.koma ^ SENGO ) & ~NARI;
			KOMA koma = ( kifu[i].change.to ^ SENGO ) & ~NARI;
			kifu[i].change.pdai = (unsigned*)&dai[koma];
		}

		// 移動先
		if( kifu[i].change.flag & CHANGE_TO ){
			kifu[i].change.pto = &ban[move.to];
		}

		// 移動元
		if( kifu[i].change.flag & CHANGE_FROM ){
			if( move.from & DAI ){
				//kifu[i].change.pfrom = (unsigned*)&dai[move.koma];
				KOMA koma = kifu[i].change.to2;
				assert( ( koma >= SFU && koma <= SHI ) || ( koma >= GFU && koma <= GHI ) );
				kifu[i].change.pfrom = (unsigned*)&dai[koma];
			}
			else{
				kifu[i].change.pfrom = &ban[move.from];
			}
		}

		// 玉の位置
		if( kifu[i].change.flag & CHANGE_OU ){
			//if( move.koma == SOU ){
			if( kifu[i].change.from == SOU ){
				kifu[i].change.pou = (unsigned*)&sou;
			}
			//else if( move.koma == GOU ){
			else if( kifu[i].change.from == GOU ){
				kifu[i].change.pou = (unsigned*)&gou;
			}
		}
	}

	// ハッシュ
	// (コピー元で解放されると困ってしまう..)
	if( halloc ){
		delete phash;
	}
	phash = org.phash;
	halloc = false; // 自分がallocしたのかフラグ

	// 利き情報
	memcpy( _effectS, org._effectS, sizeof(_effectS) );
	memcpy( _effectG, org._effectG, sizeof(_effectG) );
	effectS = _effectS + BAN_OFFSET;
	effectG = _effectG + BAN_OFFSET;
}

int Shogi::MoveError( KOMA koma, int to ){
/************************************************
行き所のない駒かチェックする。
************************************************/
	if( koma == SFU || koma == SKY ){
		if( to <= BanAddr(9,1) )
			return 1;
	}
	else if( koma == SKE ){
		if( to <= BanAddr(9,2) )
			return 1;
	}
	else if( koma == GFU || koma == GKY ){
		if( to >= BanAddr(1,9) )
			return 1;
	}
	else if( koma == GKE ){
		if( to >= BanAddr(1,8) )
			return 1;
	}

	return 0;
}

int Shogi::IsCheckNeglect( int addr, int to, int from ){
/************************************************
王手放置
移動しようとしている駒がピンされているか
addr : 玉
to   : 移動先
from : 移動元
************************************************/
	int fdir, tdir;
	int fdiff, tdiff;

	// これ要るのかな?
	if( addr == 0 || from & DAI ){
		return 0;
	}

	fdiff = addr - from;
	tdiff = addr - to;

	// addr(玉)とfromが1直線上にあって、
	// toがそれと同じ直線上にはないなら..
	if( ( fdir = _diff2dir[fdiff] ) != 0 && 
		( ( tdir = _diff2dir[tdiff] ) == 0 || fdir != tdir ) )
	{
		unsigned int* effect = ( ksengo == SENTE ? effectG : effectS );

		// 移動しようとしている駒がピンされているか
		if( effect[from] & _dir2bitF[fdir] ){
			for( int addr2 = from + fdir ; addr2 != addr ; addr2 += fdir ){
				if( ban[addr2] != EMPTY ){
					return 0;
				}
			}
			return 1;
		}
	}

	return 0;
}

int Shogi::IsCheckNeglect( int addr, int check, int to, int cnt ){
/************************************************
王手放置
王手が回避されたか調べる。
************************************************/
	int diff = to - addr;
	int dir;

	if( ( dir = _diff2dir[diff] ) != 0 && dir == check && diff / dir <= cnt )
		return 0;

	return 1;
}

int Shogi::IsCheckNeglect( int dir ){
/************************************************
玉を動かして王手放置になるか調べる。
************************************************/
	int ou;
	unsigned int* effect;

	if( ksengo == SENTE ){
		ou = sou;
		effect = effectG;
	}
	else{
		ou = gou;
		effect = effectS;
	}

	if( effect[ou] & _dir2bitF[dir] || effect[ou+dir] ){
		// 飛び駒で動けないか移動先に利きがある場合
		return 1;
	}

	return 0;
}

int Shogi::GetCheckDir(){
/************************************************
王手を掛けている駒への方向を調べる。
************************************************/
	unsigned e;

	if( ksengo == SENTE ){
		e = effectG[sou]; // 先手の玉に対する後手の利き
	}
	else{
		e = effectS[gou]; // 後手の玉に対する先手の利き
	}

	// 王手ではない。
	if( e == 0U ){
		return 0;
	}

	// 2つ以上のビットが立っている => 両王手
	if( e & ( e - 1 ) ){
		return DOUBLE_CHECK;
	}

	// 跳び駒の利きを他の駒と同じにする。
	if( e & (0xFF<<12) ){
		e >>= 12;
	}

	// 複数のビットが立っている場合と0の場合は除外済み
	return -_dir[getFirstBit(e)-1];
}

int Shogi::IsCheck( unsigned int effect[], int addr, int* pcnt, uint96* ppin ){
/************************************************
王手がかかっているか調べる。
同時に王手をかけている駒までの距離とピンも調べる。
************************************************/
	int check = 0;

	if( ppin ) (*ppin) = 0x00U;

	if( addr == 0 ){
		return 0;
	}

	// 2つ以上のビットが立っている => 両王手
	if( effect[addr] & ( effect[addr] - 1 ) ){
		if( pcnt ) (*pcnt) = 0;
		check = DOUBLE_CHECK;
	}
	else{
		int d;
		int dir;
		int addr2;

		for( d = 0 ; d < DNUM ; d++ ){
			dir = _dir[d];
			// 非跳び利き
			if( effect[addr] & _dir2bit[dir] ){
				// 王手している駒への距離( = 1 )
				if( pcnt ){ (*pcnt) = 1; }
				// 王手している駒への方向
				check = -dir;
				// ピンを調べないなら終了
				if( ppin == NULL ){ break; }
			}
			// 跳び利き
			else if( effect[addr] & _dir2bitF[dir] ){
				// 王手している駒までの距離を調べる。
				int cnt = 1;
				for( addr2 = addr - dir ; ban[addr2] == EMPTY ; addr2 -= dir ){
					cnt++;
				}
				if( pcnt ){ (*pcnt) = cnt; }
				// 王手している駒への方向
				check = -dir;
				// ピンを調べないなら終了
				if( ppin == NULL ){ break; }
			}
			// 利き無し
			else if( ppin && d < _DNUM ){
				// ピンを調べる。
				for( addr2 = addr - dir ; ban[addr2] == EMPTY ; addr2 -= dir )
					;

				if( ban[addr2] & ksengo && effect[addr2] & _dir2bitF[dir] ){
					(*ppin) |= _addr2bit[addr2];
				}
			}
		}
	}

	return check;
}

int Shogi::IsCheck( int* pcnt, uint96* ppin ){
/************************************************
王手がかかっているか調べる。
王手をかけている駒の方向を返す。
同時にピンされている駒を調べる。
************************************************/
	if( ksengo == SENTE ){
		return IsCheck( effectG, sou, pcnt, ppin );
	}
	else{
		return IsCheck( effectS, gou, pcnt, ppin );
	}
}

int Shogi::IsCheckMove( int addr, int to, KOMA koma ){
/************************************************
移動した駒によって王手がかかったか調べる。
************************************************/
	int d;
	int dir;
	int diff = addr - to;
	int m;
	int addr2;

	if( addr == 0 )
		return 0;

	if( ( dir = _diff2dir[diff] ) != 0 ){
		d = _idr[dir];
		if( ( m = _mov[koma][d] ) ){
			if( diff == dir )
				return 1;
			else if( m == 2 ){
				for( addr2 = to + dir ; ban[addr2] != WALL ; addr2 += dir ){
					if( addr2 == addr )
						return 1;
					else if( ban[addr2] != EMPTY )
						return 0;
				}
			}
		}
	}

	return 0;
}

int Shogi::IsDiscCheckMove( int addr, int to, int from ){
/************************************************
開き王手がかかったか調べる。
************************************************/
	KOMA koma;
	int d;
	int dir, dir2;
	int diff = from - addr;
	int diff2 = to - addr;
	int addr2;

	if( addr == 0 || from & DAI )
		return 0;

	if( ( dir = _diff2dir[diff] ) != 0 && diff / dir < 8 ){
		if( ( dir2 = _diff2dir[diff2] ) != 0 && dir == dir2 )
			return 0;

		// 間に余計な駒がないか調べる。
		for( addr2 = addr + dir ; ban[addr2] != WALL ; addr2 += dir ){
			if( addr2 == from )
				break;
			else if( ban[addr2] != EMPTY )
				return 0;
		}

		// 王の方向に利いている駒があるか調べる。
		for( addr2 += dir ; ban[addr2] != WALL ; addr2 += dir ){
			if( ( koma = ban[addr2] ) != EMPTY ){
				if( koma & ksengo ){
					d = _idr[-dir];
					if( _mov[koma][d] == 2 )
						return 1;
				}
				return 0;
			}
		}
	}

	return 0;
}

int Shogi::IsUchifu( int to ){
/************************************************
打ち歩で詰む局面か。
************************************************/
	int addr;
	KOMA koma;
	int d, d2;
	int dir, dir2;
	int ou;
	int c;
	int ret = 1;
	unsigned* effect;

	assert( ban[to] == EMPTY );

	// 歩を打つ。
	ban[to] = FU | ksengo;
	UpdateEffect( to );

	// 手番を入れ替える。
	ksengo ^= SENGO;

	// 玉の位置
	if( ksengo == SENTE ){
		ou = sou;
		effect = effectG;
	}
	else{
		ou = gou;
		effect = effectS;
	}

	// 盤上
	for( d = 0 ; d < DNUM ; d++ ){
		// 歩を取れるか調べる。
		// 王を動かす手はここでは見ない。
		dir = _dir[d];

		if( ou == ( addr = to + dir ) )
			continue;

		c = 0;
		for( ; ban[addr] != WALL ; addr += dir ){
			c++;
			if( ( koma = ban[addr] ) != EMPTY ){
				// 自分の駒
				if( koma & ksengo ){
					d2 = _idr[-dir];
					if( _mov[koma][d2] == 2 ||
						( _mov[koma][d2] != 0 && c == 1 ) )
					{
						// 王手放置
						//if( IsCheckNeglect( ou, to, addr ) ){
						if( ( dir2 = _diff2dir[ou-addr] ) == ou - addr &&
							effect[addr] & _dir2bitF[dir2] )
						{
							break;
						}

						// 王手した歩を取れる。
						ret = 0;
						goto lab_end;
					}
				}
				break;
			}
		}
	}

	// 王を動かす手
	// 8方向を調べる。
	for( d = 0 ; d < _DNUM ; d++ ){
		dir = _dir[d];

		addr = ou + dir;
		if( ksengo == SENTE ){
			// 先手
			if( ban[addr] == EMPTY || ban[addr] & GOTE ){
				if( effectG[addr] == 0x00 ) {
					// 王が逃げられる。
					ret = 0;
					goto lab_end;
				}
			}
		}
		else{
			// 後手
			if( ban[addr] == EMPTY || ban[addr] & SENTE ){
				if( effectS[addr] == 0x00 ) {
					// 王が逃げられる。
					ret = 0;
					goto lab_end;
				}
			}
		}
	}

lab_end:
	// 手番を元に戻す。
	ksengo ^= SENGO;

	// 打った歩を消す。
	UpdateEffectR( to );
	ban[to] = EMPTY;

	return ret;
}

bool Shogi::CapInterposition( MOVE& move ){
/************************************************
直前の無駄な合駒を取る王手を生成する。
合法手かどうかの確認はしない。
************************************************/
	int ou = ( ksengo == SENTE ? gou : sou );
	int dir;

	if( know < 2 ){ return false; }

	// 移動先
	move.to   = kifu[know-1].move.to;

	// 移動元
	dir = _diff2dir[move.to-ou];
	for( move.from = move.to + dir ; ban[move.from] == EMPTY ; move.from += dir )
		;

	// 動かす駒
	move.koma = ban[move.from];

	// 成れるなら成る。
	if( !( move.koma & NARI ) && // 成り駒でなくて
		( ( ksengo == SENTE && ( move.to <= BanAddr(9,3) || move.from <= BanAddr(9,3) ) ) ||
		  ( ksengo == GOTE  && ( move.to >= BanAddr(1,7) || move.from >= BanAddr(1,7) ) ) ) )
		// 敵陣3段目以内なら
	{
		move.nari = 1;
	}
	else{
		move.nari = 0;
	}

	return true;
}

bool Shogi::IsLegalMove( MOVE& move ){
/************************************************
合法手かどうか確認
合法手ならmove.komaを設定してtrueを返す。
************************************************/
	int ou, check, cnt;

	if( move.from == EMPTY ){ return false; }

	// パスは合法手として扱う。
	if( move.from & PASS ){ return true; }

	ou = ( ksengo == SENTE ? sou : gou );
	if( kifu[know].chdir == NOJUDGE_CHECK ){
		kifu[know].chdir = check = IsCheck( &cnt, &kifu[know].pin );
		kifu[know].cnt = cnt;
	}
	else{
		check = kifu[know].chdir;
		cnt = kifu[know].cnt;
	}

	if( move.from & DAI ){
		// 持ち駒を打つ場合
		if( check == DOUBLE_CHECK    // 両王手
			|| ban[move.to] != EMPTY // 移動先に駒があるか
			|| move.nari             // 成り駒では打てない
		){
			return false;
		}

		KOMA koma = move.from - DAI;

		if( !( ksengo & koma ) // 先後の一致
			|| dai[koma] == 0 // 持ち駒があるか
		){
			return false;
		}

		// 二歩と打ち歩詰めのチェック
		if( koma == SFU ){
			if( kifu[know].sfu & ( 1 << Addr2X(move.to) ) ||
				( move.to - 0x10 == gou && IsUchifu( move.to ) ) )
			{
				return false;
			}
		}
		else if( koma == GFU ){
			if( kifu[know].gfu & ( 1 << Addr2X(move.to) ) ||
				( move.to + 0x10 == sou && IsUchifu( move.to ) ) )
			{
				return false;
			}
		}

		// 行き所のない駒
		if( !MoveError( koma, move.to ) ){
			// 王手放置
			if( !check || !IsCheckNeglect( ou, check, move.to, cnt ) ){
				move.koma = koma;
				return true;
			}
		}
	}
	else{
		// 盤上の駒を動かす場合
		KOMA koma = ban[move.from];

		if( !( ksengo & koma )       // 移動元に駒があって先後一致
			|| ban[move.to] & ksengo // 移動先に自分の駒がないか
		){
			return false;
		}

		// 成る手
		if( move.nari ){
			// 成れるかどうか
			if( koma & NARI || koma == SKI || koma == GKI ||
				( ksengo == SENTE && move.to > BanAddr(9,3) && move.from > BanAddr(9,3) ) ||
				( ksengo == GOTE  && move.to < BanAddr(1,7) && move.from < BanAddr(1,7) ) )
			{
				return false;
			}
		}
		else{
			// 行き所のない駒
			if( MoveError( koma, move.to ) ){
				return false;
			}
		}

		// 方向を特定
		int dir = _diff2dir[move.to-move.from];
		switch( _mov[koma][_idr[dir]] ){
		case 1: // 1マスだけ動ける場合
			if( move.from + dir == move.to ){ // 移動先が1マス先
				if( koma != SOU && koma != GOU ){
					if( check == DOUBLE_CHECK || // 両王手
						( check && IsCheckNeglect( ou, check, move.to, cnt ) ) ||
						IsCheckNeglect( ou, move.to, move.from ) )
					{
						return false;
					}
				}
				else if( IsCheckNeglect( dir ) ){
					return false;
				}
				move.koma = koma;
				return true; // ok!
			}
			break;
		case 2: // 何マスでも動ける場合
			if( check == DOUBLE_CHECK ){ return false; } // 両王手
			for( int addr = move.from + dir ; ban[addr] != WALL ; addr += dir ){
				if( ban[addr] & ksengo ){ break; } // 自分の駒があったら終了
				if( addr == move.to ){ // 移動先と一致
					if( ( check && IsCheckNeglect( ou, check, move.to, cnt ) ) ||
						IsCheckNeglect( ou, move.to, move.from ) )
					{
						return false;
					}
					move.koma = koma;
					return true; // ok!
				}
				// 相手の駒があったら終了
				if( ban[addr] ){ break; }
			}
			break;
		}
	}

	return false;
}

bool Shogi::Obstacle( KOMA koma, int from, int to ){
/************************************************
fromからtoまでの間に駒があるか確認
************************************************/
	int diff = to - from;
	int dir = _diff2dir[diff];

	if( dir != 0 ){
		switch( _mov[koma][_idr[dir]] ){
		case 1:
			// 距離が1の場合
			if( to == from + dir ){ return false; }
			break;

		case 2:
			// 距離が1の場合
			if( to == from + dir ){ return false; }

			switch( dir ){
			case  -1: if( ( _dbits_lt[_addr[from]] & _dbits_rt[_addr[from]] & bb_a ) == 0U ){ return false; } break;
			case   1: if( ( _dbits_rt[_addr[from]] & _dbits_lt[_addr[from]] & bb_a ) == 0U ){ return false; } break;
			case -16: if( ( _dbits_up[_addr[from]] & _dbits_dn[_addr[from]] & bb_a ) == 0U ){ return false; } break;
			case  16: if( ( _dbits_dn[_addr[from]] & _dbits_up[_addr[from]] & bb_a ) == 0U ){ return false; } break;
			case -17: if( ( _dbits_lu[_addr[from]] & _dbits_rd[_addr[from]] & bb_a ) == 0U ){ return false; } break;
			case -15: if( ( _dbits_ru[_addr[from]] & _dbits_ld[_addr[from]] & bb_a ) == 0U ){ return false; } break;
			case  15: if( ( _dbits_ld[_addr[from]] & _dbits_ru[_addr[from]] & bb_a ) == 0U ){ return false; } break;
			case  17: if( ( _dbits_rd[_addr[from]] & _dbits_lu[_addr[from]] & bb_a ) == 0U ){ return false; } break;
			}
			break;
		}
	}

	return true;
}

bool Shogi::IsStraight( int from, int to, int middle ){
/************************************************
from, middle, toが一直線に並んでいるか
************************************************/
	int diff  = to     - from;
	int dir   = _diff2dir[diff];
	int diff2 = middle - from;

	if( dir && dir == _diff2dir[diff2] // 方向が同じ
		&& diff / dir > diff2 /dir // from-middle間の方が近いか
	){
		return true;
	}
	return false;
}

template<bool handler>
int Shogi::MoveD( const MOVE& move ){
/************************************************
指し手を入力する。
move.fromがEMPTYであってはいけない。
************************************************/
#if 0
	if( know >= MAX_KIFU ){
		return 0;
	}
#endif

	kifu[know].move = move;
	kifu[know].move.koma = GoNext<handler>( kifu[know].move );
	knum = know;

	return 1;
}
template int Shogi::MoveD<true>( const MOVE& move );
template int Shogi::MoveD<false>( const MOVE& move );

template<bool handler>
int Shogi::Move( MOVE& move ){
/************************************************
指し手を入力する。(合法手チェックあり)
************************************************/
	if( know >= MAX_KIFU ){
		return 0;
	}

	if( !IsLegalMove( move ) ){
		return 0;
	}

	return MoveD<handler>( move );
}
template int Shogi::Move<true>( MOVE& move );
template int Shogi::Move<false>( MOVE& move );

template<bool handler>
int Shogi::NullMove(){
/************************************************
パスを入力する。
************************************************/
	KIFU* now  = &kifu[know];

#if 0
	if( know >= MAX_KIFU ){
		return 0;
	}
#endif

	// 王手がかかっていないか。
	if( now->chdir != NOJUDGE_CHECK ){
		if( now->chdir != 0 ){ return 0; }
	}
	else if( ( now->chdir = IsCheck( &now->cnt, &now->pin ) ) ){
		return 0;
	}

	now->move.from = PASS;
	now->move.to = 0;
	now->move.nari = 0;
	now->move.koma = EMPTY;
	GoNext<handler>( now->move );
	knum = know;

	return 1;
}
template int Shogi::NullMove<true>();
template int Shogi::NullMove<false>();

template<bool handler>
KOMA Shogi::GoNext( const MOVE& __restrict__ move ){
/************************************************
指し手を進める。
move.fromがEMPTYであってはいけない。
************************************************/
	KOMA koma0;
	KIFU* __restrict__ now  = &kifu[know];
	KIFU* __restrict__ next = now + 1;

	know++;
	if( handler ){ GoNextHandler(); }

	next->hashB = now->hashB; // ハッシュ値(盤面)
	next->hashD = now->hashD; // ハッシュ値(持ち駒)
	next->sfu = now->sfu;     // 先手の歩の有無
	next->gfu = now->gfu;     // 後手の歩の有無

	if( move.from != PASS ){
		now->change.flag = CHANGE_FROM | CHANGE_TO; // 変更項目
		now->change.pto = &ban[move.to];            // 変更箇所
		now->change.to = ban[move.to];              // 元の値

		if( move.from & DAI ){
			// 持ち駒を打つ場合
			koma0 = move.from - DAI;
			now->change.pfrom = (unsigned*)&dai[koma0]; // 変更箇所
			now->change.from = (unsigned)dai[koma0];    // 元の値
			next->hashD ^= phash->Dai( koma0, dai[koma0] );

			dai[koma0]--;         // 値書き換え!!(持ち駒)
			ban[move.to] = koma0; // 値書き換え!!(移動先)
			UpdateEffect( move.to ); // 移動先の利きを付ける。
			bb[koma0].setBit(_addr[move.to]); // bitboard
			bb_a.setBit(_addr[move.to]); // bitboard(all)
			now->change.to2 = (unsigned)koma0; // 移動後の駒

			next->hashD ^= phash->Dai( koma0, dai[koma0] );
			next->hashB ^= phash->Ban( koma0, move.to );

			// 歩の有無
			if( koma0 == SFU ){
				next->sfu |= 1 << Addr2X(move.to);
			}
			else if( koma0 == GFU ){
				next->gfu |= 1 << Addr2X(move.to);
			}

			if( handler ){ GoNextTo( move.to ); } // 差分計算
		}
		else{
			// 盤上の駒を動かす場合
			KOMA cap   = ban[move.to]; // 取られた駒
			KOMA koma;                 // 動かした後の駒
			koma0 = ban[move.from];    // 動かした駒

			if( cap != EMPTY ){ // 駒を取ったか
				if     ( cap == SFU ){ // 先手の歩の有無
					next->sfu ^= 1 << Addr2X(move.to);
				}
				else if( cap == GFU ){ // 後手の歩の有無
					next->gfu ^= 1 << Addr2X(move.to);
				}

				UpdateEffectR( move.to ); // 取られる駒の利きを消す。
				bb[cap].unsetBit(_addr[move.to]); // bitboard
				bb_a.unsetBit(_addr[move.to]); // bitboard(all)

				next->hashB ^= phash->Ban( cap, move.to ); // ハッシュ(盤)
				if( handler ){ GoNextRemove( cap ); }
				cap = ( cap ^ SENGO ) & ~NARI; // 生の駒にして手番を変える。
				if( handler ){ GoNextAdd( cap ); }
				now->change.flag |= CHANGE_DAI; // 変更項目追加
				now->change.pdai = (unsigned*)&dai[cap]; // 変更箇所
				now->change.dai = (unsigned)dai[cap];    // 元の値
				next->hashD ^= phash->Dai( cap, dai[cap] ); // ハッシュ(持ち駒)
				dai[cap]++; // 値書き換え!!(取った駒)
				next->hashD ^= phash->Dai( cap, dai[cap] ); // ハッシュ(持ち駒)

				if( handler ){
					GoNextFrom( move.to ); // 差分計算
					ban[move.to] = EMPTY; // 値書き換え!!(移動先)
				}
			}

			if( handler ){ GoNextFrom( move.from ); } // 差分計算
			// ** sou, gouの書き換えより前に呼ぶこと **

			now->change.pfrom = &ban[move.from]; // 変更箇所
			now->change.from = koma0;            // 元の値
			next->hashB ^= phash->Ban( koma0, move.from ); // ハッシュ(盤)

			if( move.nari ){ // 成る場合
				if     ( koma0 == SFU ){ // 先手の歩の有無
					next->sfu ^= 1 << Addr2X(move.to);
				}
				else if( koma0 == GFU ){ // 後手の歩の有無
					next->gfu ^= 1 << Addr2X(move.to);
				}

				if( handler ){ GoNextRemove( koma0 ); }
				koma = koma0 | NARI; // 成り駒にする。
				if( handler ){ GoNextAdd( koma ); }
			}
			else{ // 成らない場合
				if     ( koma0 == SOU ){ // 先手玉の位置
					now->change.flag |= CHANGE_OU;
					now->change.pou = (unsigned*)&sou;
					now->change.ou = (unsigned)sou;
					sou = move.to;
				}
				else if( koma0 == GOU ){ // 後手玉の位置
					now->change.flag |= CHANGE_OU;
					now->change.pou = (unsigned*)&gou;
					now->change.ou = (unsigned)gou;
					gou = move.to;
				}
				koma = koma0; // そのまま
			}

			next->hashB ^= phash->Ban( koma, move.to ); // ハッシュ(盤)

			ban[move.to] = koma; // 値書き換え!!(移動先)
			bb[koma].setBit(_addr[move.to]); // bitboard
			bb_a.setBit(_addr[move.to]); // bitboard(all)
			UpdateEffect( move.to ); // 移動先の利きを付ける。
			now->change.to2 = (unsigned)koma; // 移動後の駒

			UpdateEffectR( move.from ); // 移動元の利きを消す。
			ban[move.from] = EMPTY; // 値書き換え!!(移動元)
			bb[koma0].unsetBit(_addr[move.from]); // bitboard
			bb_a.unsetBit(_addr[move.from]); // bitboard(all)

			if( handler ){ GoNextTo( move.to ); } // 差分計算
		}
	}
	else{ // パスの場合
		now->change.flag = 0x00U;  // 変更なし
		koma0 = EMPTY;
	}

	ksengo ^= SENGO;               // 手番の変更
	next->hashB ^= phash->Sengo(); // ハッシュ(手番)

	next->check = IsCheck();       // 王手(IsRepetitionで使用)
	next->chdir = NOJUDGE_CHECK;   // 王手方向

	return koma0;
}

int Shogi::GoBack(){
/************************************************
1手戻る。
************************************************/
	if( know == 0 ){ return 0; }

	KIFU* now  = &kifu[--know];
	MOVE& move = now->move;

	if( now->change.flag & CHANGE_FROM ){
		*(now->change.pfrom) = now->change.from;
		if( !( move.from & DAI ) ){
			UpdateEffect( move.from ); // 移動元の利きを付ける。
			bb[now->change.from].setBit(_addr[move.from]); // bitboard
			bb_a.setBit(_addr[move.from]); // bitboard(all)
		}
	}

	if( now->change.flag & CHANGE_TO ){
		UpdateEffectR( move.to ); // 移動先の利きを消す。
		bb[now->change.to2].unsetBit(_addr[move.to]); // bitboard
		bb_a.unsetBit(_addr[move.to]); // bitboard(all)
		*(now->change.pto) = now->change.to;
	}

	if( now->change.flag & CHANGE_DAI ){
		*(now->change.pdai) = now->change.dai;
		UpdateEffect( move.to ); // 取られた駒の利きを付ける。
		bb[now->change.to].setBit(_addr[move.to]); // bitboard
		bb_a.setBit(_addr[move.to]); // bitboard(all)
	}

	if( now->change.flag & CHANGE_OU ){
		*(now->change.pou) = now->change.ou;
	}

	ksengo ^= SENGO; // 手番の変更

	return 1;
}

int Shogi::IsMate(){
/************************************************
詰み判定(有効な差し手がない)
************************************************/
	int i;
	int addr, addr2;
	KOMA koma;
	int d, d2;
	int dir;
	int check;
	int c;
	int ou;

	// 玉の位置
	if( ksengo == SENTE ){ ou = sou; }
	else                { ou = gou; }

	// 王手
	check = GetCheckDir();

	// 詰み云々ではなく王手ではない。
	if( !check ){
		return 0;
	}

	// 玉を動かす手
	// 各方向を調べる。
	if( ou != 0 ){
		for( d = 0 ; d < _DNUM ; d++ ){
			dir = _dir[d];
			addr2 = ou + dir;
			if( ban[addr2] != WALL && !( ban[addr2] & ksengo ) ){
				// 王手放置
				if( !IsCheckNeglect( dir ) ){
					// 着手可能手が存在
					return 0;
				}
			}
		}
	}

	// 両王手でないなら玉以外の駒を動かす手
	if( check != DOUBLE_CHECK ){
		for( addr = ou + check ; ; addr += check ){
			// 盤上
			for( d = 0 ; d < DNUM ; d++ ){
				dir = _dir[d];
				if( dir == -check ){
					continue;
				}

				c = 0;
				for( addr2 = addr + dir ; ban[addr2] != WALL ; addr2 += dir ){
					c++;
					if( ( koma = ban[addr2] ) != EMPTY ){
						// 自分の駒
						if( koma & ksengo ){
							d2 = _idr[-dir];
							if( _mov[koma][d2] == 2 ||
								( _mov[koma][d2] != 0 && c == 1 ) )
							{
								// 王手放置
								if( IsCheckNeglect( ou, addr, addr2 ) ){
									goto lab_break;
								}

								// 着手可能手が存在
								return 0;
							}
						}
						goto lab_break;
					}
				}
lab_break:
				;
			}

			// 駒台
			if( ban[addr] == EMPTY ){
				for( koma = ksengo + FU ; koma <= ksengo + HI ; koma++ ){
					if( dai[koma] > 0 ){
						if( koma == SFU || koma == GFU ){
							// 二歩のチェック
							i = Addr2X(addr);
							if( ksengo == SENTE ){
								if( kifu[know].sfu & ( 1 << i ) )
									goto lab_nifu;
							}
							else{
								if( kifu[know].gfu & ( 1 << i ) )
									goto lab_nifu;
							}

							// 打ち歩詰めのチェック
							if( ksengo == SENTE ){
								if( addr - 0x10 == gou && IsUchifu( addr ) )
									goto lab_nifu;
							}
							else{
								if( addr + 0x10 == sou && IsUchifu( addr ) )
									goto lab_nifu;
							}
						}

						// 着手可能手が存在
						return 0;
					}
lab_nifu:
					;
				}
			}

			if( ban[addr] != EMPTY ){
				break;
			}
		} 
	}

	return 1;
}

int Shogi::DeleteNext(){
/************************************************
次の指し手以降を削除
************************************************/
	if( know >= knum )
		return 0;

	knum = know;

	return 1;
}

int Shogi::IsRepetition( int num ){
/************************************************
千日手判定
************************************************/
	int i;
	uint64 hash = kifu[know].hashB ^ kifu[know].hashD;
	int cnt = 0;
	int check1 = 1;
	int check2 = 1;
	int srep = 0;

	// 同一局面は2手前には存在しないはずなので
	// know - 4 から調べ始める. 2011 1/3
	for( i = know - 4 ; i >= 0 ; i -= 2 ){
		// 王手かどうか
		if( kifu[i  ].check == 0 ){ check1 = 0; }
		if( kifu[i+1].check == 0 ){ check2 = 0; }
        /* 本来NOJUDGE_CHECKの状態はない。
		 * GenerateMovesやIsLegalMoveを呼ばずにMoveDを
		 * 呼んだときのみNOJUDGE_CHECKのままになる。
		 */
		if( ( kifu[i].hashB ^ kifu[i].hashD ) == hash ){
			// 局面が一致
			cnt++;
			if( cnt >= 3 ){
				// 千日手
				if( check1 )
					return PC_WIN;
				else if( check2 )
					return PC_LOS;
				else
					return IS_REP;
			}
			else if( i >= know - num ){
				// 指定範囲内で繰り返しを発見
				srep = 1;
			}

			// 2手前に同一局面は存在しないはず 2011 1/3
			i -= 2;
		}
	}

	if( srep )
		return ST_REP;

	return 0;
}

bool Shogi::IsShortRepetition(){
/************************************************
同一局面の検出 2011 1/3
連続王手は関係なく同一局面が存在するかだけ調べる。
************************************************/
	int i;
	uint64 hash = kifu[know].hashB ^ kifu[know].hashD;

	for( i = know - 4 ; i >= 0 ; i -= 2 ){
		if( ( kifu[i].hashB ^ kifu[i].hashD ) == hash ){
			// 局面が一致
			return true;
		}
	}

	return false;
}

bool Shogi::GoNode( const Shogi* pshogi ){
/************************************************
同じ局面へ移動
(開始局面が同一でなければならない)
************************************************/
	// 同一局面まで戻る。
	while( GoBack() ){
		// 同一局面か
		if( kifu[know].hashB == pshogi->kifu[know].hashB &&
			kifu[know].hashD == pshogi->kifu[know].hashD
		){
			// 同じ指し手で局面を進める。
			while( know < pshogi->know ){
				MoveD( pshogi->kifu[know].move );
			}

			return true;
		}
	}

	return false;
}
