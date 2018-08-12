/* generate.cpp
 * R.Kubo 2008-2010
 * 合法手生成
 */

#include "shogi.h"

void Shogi::SetGenInfo( GEN_INFO& info, unsigned flag ){
/************************************************
GEN_INFOの初期化
************************************************/
	// 生成する指し手の種類
	info.flag = flag;

	// 玉の位置
	if( ksengo == SENTE ){
		info.ou  = sou;
		info.ou2 = gou;
	}
	else{
		info.ou  = gou;
		info.ou2 = sou;
	}
}

void Shogi::SetGenInfoPin( GEN_INFO& info ){
/************************************************
GEN_INFOの初期化(王手とピン)
************************************************/
	// 既に調べた事がある場合はそれを使う。
	if( kifu[know].chdir == NOJUDGE_CHECK ){
		kifu[know].chdir = info.check = IsCheck( &info.cnt, &info.pin );
		kifu[know].cnt = info.cnt;
		kifu[know].pin = info.pin;
	}
	else{
		info.check = kifu[know].chdir;
		info.cnt = kifu[know].cnt;
		info.pin = kifu[know].pin;
	}
}

void Shogi::GenerateMovesDropFu( Moves& moves ){
/************************************************
歩を打つ手を列挙する。
************************************************/
	KOMA koma;
	int to;
	int i, j;

	koma = FU | ksengo;

	if( dai[koma] == 0 )
		return;

	for( i = 1 ; i <= 9 ; i++ ){
		// 二歩のチェック
		if( ksengo == SENTE ){
			if( kifu[know].sfu & ( 1 << i ) )
				continue;
		}
		else{
			if( kifu[know].gfu & ( 1 << i ) )
				continue;
		}

		for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
			to = i + j;
			if( ban[to] == EMPTY ){
				// 合法手チェック
				if( ksengo == SENTE ){
					if( to <= BanAddr(9,1) || ( to - 0x10 == gou && IsUchifu( to ) ) )
						continue;
				}
				else{
					if( to >= BanAddr(1,9) || ( to + 0x10 == sou && IsUchifu( to ) ) )
						continue;
				}

				// 着手可能手追加
				moves.Add( DAI + koma, to, 0, koma );
			}
		}
	}
}

void Shogi::GenerateMovesDropKy( Moves& moves ){
/************************************************
香車を打つ手を列挙する。
************************************************/
	KOMA koma;
	int to;
	int i, j;

	koma = KY | ksengo;

	if( dai[koma] == 0 )
		return;

	for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
		for( i = 1 ; i <= 9 ; i++ ){
			to = i + j;
			if( ban[to] == EMPTY ){
				// 合法手チェック
				if( ksengo == SENTE ){
					if( to <= BanAddr(9,1) )
						continue;
				}
				else{
					if( to >= BanAddr(1,9) )
						continue;
				}

				// 着手可能手追加
				moves.Add( DAI + koma, to, 0, koma );
			}
		}
	}
}

void Shogi::GenerateMovesDropKe( Moves& moves ){
/************************************************
桂馬を打つ手を列挙する。
************************************************/
	KOMA koma;
	int to;
	int i, j;

	koma = KE | ksengo;

	if( dai[koma] == 0 )
		return;

	for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
		for( i = 1 ; i <= 9 ; i++ ){
			to = i + j;
			if( ban[to] == EMPTY ){
				// 合法手チェック
				if( ksengo == SENTE ){
					if( to <= BanAddr(9,2) )
						continue;
				}
				else{
					if( to >= BanAddr(1,8) )
						continue;
				}

				// 着手可能手追加
				moves.Add( DAI + koma, to, 0, koma );
			}
		}
	}
}

void Shogi::GenerateMovesDrop( Moves& moves, KOMA koma ){
/************************************************
持ち駒を打つ手を列挙する。
************************************************/
	int to;
	int i, j;

	if( dai[koma] == 0 )
		return;

	for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
		for( i = 1 ; i <= 9 ; i++ ){
			to = i + j;
			if( ban[to] == EMPTY ){
				// 着手可能手追加
				moves.Add( DAI + koma, to, 0, koma );
			}
		}
	}
}

void Shogi::GenerateMovesDrop( Moves& moves ){
/************************************************
持ち駒を打つ手を列挙する。
************************************************/
	GenerateMovesDropFu( moves );
	GenerateMovesDropKy( moves );
	GenerateMovesDropKe( moves );
	if( ksengo == SENTE ){
		GenerateMovesDrop( moves, SGI );
		GenerateMovesDrop( moves, SKI );
		GenerateMovesDrop( moves, SKA );
		GenerateMovesDrop( moves, SHI );
	}
	else{
		GenerateMovesDrop( moves, GGI );
		GenerateMovesDrop( moves, GKI );
		GenerateMovesDrop( moves, GKA );
		GenerateMovesDrop( moves, GHI );
	}
}

void Shogi::GenerateMoves1Step( Moves& moves, GEN_INFO& info, int dir ){
/************************************************
任意の方向への着手可能手を列挙する。
************************************************/
	int from; // 移動元
	int to; // 移動先
	int n;
	int flag;
	bool nz;

	if( info.dir_p != 0 && dir != info.dir_p && dir != -info.dir_p )
		return;

	from = info.addr;
	to = info.addr + dir;
	if( ban[to] == EMPTY || ban[to] & (ksengo^SENGO) ){
		if(    ( info.flag & MOVE_CAPTURE   && ban[to] != EMPTY )
			|| ( info.flag & MOVE_NOCAPTURE && ban[to] == EMPTY )
			// 新しいGenerateCheckOnBoard内で空き王手のチェックは行う。
			/*|| ( info.flag & MOVE_CHECK && IsDiscCheckMove( info.ou2, to, from ) )*/
		){
			flag = 1;
		}
		else{
			flag = 0;
		}

		// 着手可能手追加
		n = 0;
		if( !( info.koma & NARI ) && info.koma != SKI && info.koma != GKI &&
			( ( ksengo == SENTE && ( to <= BanAddr(9,3) || from <= BanAddr(9,3) ) ) ||
			  ( ksengo == GOTE  && ( to >= BanAddr(1,7) || from >= BanAddr(1,7) ) ) ) )
		{
			// 成る場合
			if( flag || info.flag & MOVE_PROMOTE ||
				( info.flag & MOVE_CHECK && IsCheckMove( info.ou2, to, info.koma + NARI ) ) ||
				( info.flag & MOVE_NCHECK && !IsCheckMove( info.ou2, to, info.koma + NARI ) )
			){
				moves.Add( from, to, 1, info.koma );
			}
			n = 1;
		}
		nz = !n || Narazu( info.koma, to );
		if( !MoveError( info.koma, to ) &&
			// 王手の場合でも不成は生成しないように変更
			( /*info.flag & MOVE_CHECK ||*/ nz ) )
		{
			// 成らない場合
			if( flag || info.flag & MOVE_NOPROMOTE ||
				( info.flag & MOVE_CHECK && IsCheckMove( info.ou2, to, info.koma ) ) ||
				( nz && info.flag & MOVE_NCHECK && !IsCheckMove( info.ou2, to, info.koma ) )
			){
				moves.Add( from, to, 0, info.koma );
			}
		}
	}
}

void Shogi::GenerateMovesStraight( Moves& moves, GEN_INFO& info, int dir ){
/************************************************
飛び駒による任意の方向への着手可能手を列挙する。
************************************************/
	int from; // 移動元
	int to; // 移動先
	int n;
	int flag;
	bool nz;

	if( info.dir_p != 0 && dir != info.dir_p && dir != -info.dir_p )
		return;

	from = info.addr;
	for( to = info.addr + dir ; ban[to] != WALL ; to += dir ){
		// 自分の駒があったら終了
		if( ban[to] & ksengo )
			break;

		if(    ( info.flag & MOVE_CAPTURE   && ban[to] != EMPTY )
			|| ( info.flag & MOVE_NOCAPTURE && ban[to] == EMPTY )
			// 新しいGenerateCheckOnBoard内で空き王手のチェックは行う。
			/*|| ( info.flag & MOVE_CHECK && IsDiscCheckMove( info.ou2, to, from ) )*/
		){
			flag = 1;
		}
		else{
			flag = 0;
		}

		// 着手可能手追加
		n = 0;
		if( !( info.koma & NARI ) && info.koma != SKI && info.koma != GKI &&
			( ( ksengo == SENTE && ( to <= BanAddr(9,3) || from <= BanAddr(9,3) ) ) ||
			  ( ksengo == GOTE  && ( to >= BanAddr(1,7) || from >= BanAddr(1,7) ) ) ) )
		{
			// 成る場合
			if( flag || info.flag & MOVE_PROMOTE ||
				( info.flag & MOVE_CHECK && IsCheckMove( info.ou2, to, info.koma + NARI ) ) ||
				( info.flag & MOVE_NCHECK && !IsCheckMove( info.ou2, to, info.koma + NARI ) )
			){
				moves.Add( from, to, 1, info.koma );
			}
			n = 1;
		}
		nz = !n || Narazu( info.koma, to );
		if( !MoveError( info.koma, to ) &&
			// 王手の場合でも不成は生成しないように変更
			( /*info.flag & MOVE_CHECK ||*/ nz ) )
		{
			// 成らない場合
			if( flag || info.flag & MOVE_NOPROMOTE ||
				( info.flag & MOVE_CHECK && IsCheckMove( info.ou2, to, info.koma ) ) ||
				( nz && info.flag & MOVE_NCHECK && !IsCheckMove( info.ou2, to, info.koma ) )
			){
				moves.Add( from, to, 0, info.koma );
			}
		}
		// 相手の駒があったら終了
		if( ban[to] )
			break;
	}
}

void Shogi::GenerateMovesOnBoard( Moves& moves, GEN_INFO& info ){
/************************************************
盤上の駒を移動する手を列挙する。
************************************************/
	int i, j;

	if( ksengo == SENTE ){
		// 先手番
		for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
			for( i = 1 ; i <= 9 ; i++ ){
				info.addr = i + j;

				if( ( info.koma = ban[info.addr] ) & SENTE ){
					if( ( info.pin & _addr2bit[info.addr] ) == 0x00U ){
						// ピンされていない。
						info.dir_p = 0;
					}
					else{
						// ピンされている。
						info.dir_p = _diff2dir[info.ou-info.addr];
					}

					switch( info.koma ){
					case SFU:
						GenerateMoves1Step   ( moves, info, -0x10      );
						break;
					case SKY:
						GenerateMovesStraight( moves, info, -0x10      );
						break;
					case SKE:
						GenerateMoves1Step   ( moves, info, -0x20+0x01 );
						GenerateMoves1Step   ( moves, info, -0x20-0x01 );
						break;
					case SGI:
						GenerateMoves1Step   ( moves, info, -0x10      );
						GenerateMoves1Step   ( moves, info, -0x10-0x01 );
						GenerateMoves1Step   ( moves, info, -0x10+0x01 );
						GenerateMoves1Step   ( moves, info, +0x10-0x01 );
						GenerateMoves1Step   ( moves, info, +0x10+0x01 );
						break;
					case SKI: case STO: case SNY: case SNK: case SNG:
						GenerateMoves1Step   ( moves, info, -0x10      );
						GenerateMoves1Step   ( moves, info, -0x10-0x01 );
						GenerateMoves1Step   ( moves, info, -0x10+0x01 );
						GenerateMoves1Step   ( moves, info,      -0x01 );
						GenerateMoves1Step   ( moves, info,      +0x01 );
						GenerateMoves1Step   ( moves, info, +0x10      );
						break;
					case SKA:
						GenerateMovesStraight( moves, info, -0x10-0x01 );
						GenerateMovesStraight( moves, info, -0x10+0x01 );
						GenerateMovesStraight( moves, info, +0x10-0x01 );
						GenerateMovesStraight( moves, info, +0x10+0x01 );
						break;
					case SHI:
						GenerateMovesStraight( moves, info, -0x10      );
						GenerateMovesStraight( moves, info, +0x10      );
						GenerateMovesStraight( moves, info,      -0x01 );
						GenerateMovesStraight( moves, info,      +0x01 );
						break;
					case SUM:
						GenerateMovesStraight( moves, info, -0x10-0x01 );
						GenerateMovesStraight( moves, info, -0x10+0x01 );
						GenerateMovesStraight( moves, info, +0x10-0x01 );
						GenerateMovesStraight( moves, info, +0x10+0x01 );
						GenerateMoves1Step   ( moves, info, -0x10      );
						GenerateMoves1Step   ( moves, info, +0x10      );
						GenerateMoves1Step   ( moves, info,      -0x01 );
						GenerateMoves1Step   ( moves, info,      +0x01 );
						break;
					case SRY:
						GenerateMovesStraight( moves, info, -0x10      );
						GenerateMovesStraight( moves, info, +0x10      );
						GenerateMovesStraight( moves, info,      -0x01 );
						GenerateMovesStraight( moves, info,      +0x01 );
						GenerateMoves1Step   ( moves, info, -0x10-0x01 );
						GenerateMoves1Step   ( moves, info, -0x10+0x01 );
						GenerateMoves1Step   ( moves, info, +0x10-0x01 );
						GenerateMoves1Step   ( moves, info, +0x10+0x01 );
						break;
					}
				}
			}
		}
	}
	else{
		// 後手番
		for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
			for( i = 1 ; i <= 9 ; i++ ){
				info.addr = i + j;

				if( ( info.koma = ban[info.addr] ) & GOTE ){
					if( ( info.pin & _addr2bit[info.addr] ) == 0x00U ){
						// ピンされていない。
						info.dir_p = 0;
					}
					else{
						// ピンされている。
						info.dir_p = _diff2dir[info.ou-info.addr];
					}

					switch( info.koma ){
					case GFU:
						GenerateMoves1Step   ( moves, info, +0x10      );
						break;
					case GKY:
						GenerateMovesStraight( moves, info, +0x10      );
						break;
					case GKE:
						GenerateMoves1Step   ( moves, info, +0x20+0x01 );
						GenerateMoves1Step   ( moves, info, +0x20-0x01 );
						break;
					case GGI:
						GenerateMoves1Step   ( moves, info, +0x10      );
						GenerateMoves1Step   ( moves, info, +0x10-0x01 );
						GenerateMoves1Step   ( moves, info, +0x10+0x01 );
						GenerateMoves1Step   ( moves, info, -0x10-0x01 );
						GenerateMoves1Step   ( moves, info, -0x10+0x01 );
						break;
					case GKI: case GTO: case GNY: case GNK: case GNG:
						GenerateMoves1Step   ( moves, info, +0x10      );
						GenerateMoves1Step   ( moves, info, +0x10-0x01 );
						GenerateMoves1Step   ( moves, info, +0x10+0x01 );
						GenerateMoves1Step   ( moves, info,      -0x01 );
						GenerateMoves1Step   ( moves, info,      +0x01 );
						GenerateMoves1Step   ( moves, info, -0x10      );
						break;
					case GKA:
						GenerateMovesStraight( moves, info, +0x10-0x01 );
						GenerateMovesStraight( moves, info, +0x10+0x01 );
						GenerateMovesStraight( moves, info, -0x10-0x01 );
						GenerateMovesStraight( moves, info, -0x10+0x01 );
						break;
					case GHI:
						GenerateMovesStraight( moves, info, +0x10      );
						GenerateMovesStraight( moves, info, -0x10      );
						GenerateMovesStraight( moves, info,      -0x01 );
						GenerateMovesStraight( moves, info,      +0x01 );
						break;
					case GUM:
						GenerateMovesStraight( moves, info, +0x10-0x01 );
						GenerateMovesStraight( moves, info, +0x10+0x01 );
						GenerateMovesStraight( moves, info, -0x10-0x01 );
						GenerateMovesStraight( moves, info, -0x10+0x01 );
						GenerateMoves1Step   ( moves, info, +0x10      );
						GenerateMoves1Step   ( moves, info, -0x10      );
						GenerateMoves1Step   ( moves, info,      -0x01 );
						GenerateMoves1Step   ( moves, info,      +0x01 );
						break;
					case GRY:
						GenerateMovesStraight( moves, info, +0x10      );
						GenerateMovesStraight( moves, info, -0x10      );
						GenerateMovesStraight( moves, info,      -0x01 );
						GenerateMovesStraight( moves, info,      +0x01 );
						GenerateMoves1Step   ( moves, info, +0x10-0x01 );
						GenerateMoves1Step   ( moves, info, +0x10+0x01 );
						GenerateMoves1Step   ( moves, info, -0x10-0x01 );
						GenerateMoves1Step   ( moves, info, -0x10+0x01 );
						break;
					}
				}
			}
		}
	}
}

void Shogi::GenerateMovesOu( Moves& moves, GEN_INFO& info, int dir ){
/************************************************
王の移動を生成
************************************************/
	int to;

	to = info.ou + dir;
	if( ban[to] == EMPTY || ban[to] & (ksengo^SENGO) ){
		if(    ( info.flag & MOVE_KING                          )
			|| ( info.flag & MOVE_CAPTURE   && ban[to] != EMPTY )
			|| ( info.flag & MOVE_NOCAPTURE && ban[to] == EMPTY )
			|| ( info.flag & MOVE_CHECK && IsDiscCheckMove( info.ou2, to, info.ou ) )
		){
			// 王手放置
			if( !IsCheckNeglect( dir ) ){
				// 着手可能手追加
				moves.Add( info.ou, to, 0, OU | ksengo );
			}
		}
	}
}

void Shogi::GenerateMovesOu( Moves& moves, GEN_INFO& info ){
/************************************************
王の移動を列挙する。
************************************************/
	if( info.ou != 0 ){
		GenerateMovesOu( moves, info, -0x10-0x01 );
		GenerateMovesOu( moves, info, -0x10      );
		GenerateMovesOu( moves, info, -0x10+0x01 );
		GenerateMovesOu( moves, info,      -0x01 );
		GenerateMovesOu( moves, info,      +0x01 );
		GenerateMovesOu( moves, info, +0x10-0x01 );
		GenerateMovesOu( moves, info, +0x10      );
		GenerateMovesOu( moves, info, +0x10+0x01 );
	}
}

void Shogi::GenerateMovesOu( Moves& moves ){
/************************************************
王の移動を列挙する。
************************************************/
	GEN_INFO info;

	// 生成する指し手の種類
	SetGenInfo( info, MOVE_ALL );

	// ピン..多分いらない
	//SetGenInfoPin( info );

	// 玉を動かす手
	GenerateMovesOu( moves, info );
}

void Shogi::GenerateMoves( Moves& moves ){
/************************************************
着手可能手を列挙する。
************************************************/
	GEN_INFO info;

	// 生成する指し手の種類
	SetGenInfo( info, MOVE_ALL );

	// ピン
	SetGenInfoPin( info );

	if( !info.check ){
		// 駒台
		GenerateMovesDrop( moves );

		// 盤上
		GenerateMovesOnBoard( moves, info );
	}
	else if( info.check != DOUBLE_CHECK ){
		GenerateCapEvasion( moves, info );
		GenerateNoCapEvasion( moves, info );
	}

	// 玉を動かす手
	GenerateMovesOu( moves, info );
}

void Shogi::GenerateCaptures( Moves& moves ){
/************************************************
駒を取る手を列挙する。
************************************************/
	GEN_INFO info;

	// 生成する指し手の種類
	SetGenInfo( info, MOVE_CAPTURE );

	// ピン
	SetGenInfoPin( info );

	if( !info.check ){
		// 盤上
		GenerateMovesOnBoard( moves, info );
	}
	else if( info.check != DOUBLE_CHECK ){
		GenerateCapEvasion( moves, info );
	}

	// 玉を動かす手
	GenerateMovesOu( moves, info );
}

void Shogi::GenerateNoCaptures( Moves& moves ){
/************************************************
駒を取らない手を列挙する。
************************************************/
	GEN_INFO info;

	// 生成する指し手の種類
	SetGenInfo( info, MOVE_NOCAPTURE );

	// ピン
	SetGenInfoPin( info );

	if( !info.check ){
		// 駒台
		GenerateMovesDrop( moves );

		// 盤上
		GenerateMovesOnBoard( moves, info );
	}
	else if( info.check != DOUBLE_CHECK ){
		GenerateNoCapEvasion( moves, info );
	}

	// 玉を動かす手
	GenerateMovesOu( moves, info );
}

void Shogi::GenerateEvasionBan( Moves& moves, GEN_INFO& info, bool eff ){
/************************************************
駒を取る手と移動合
eff は紐のついていない合駒を除去するために使用
利きを使えばその方向に駒があるかはわかるはず。=> 要検討
************************************************/
	int to = info.addr; // 移動先
	int from;           // 移動元
	int dir;            // 方向
	int b;              // ビット位置
	int dflag;          // 空き王手フラグ
	unsigned* effect = ( ksengo == SENTE ? effectS : effectG );
	unsigned e = effect[to];

	if( e == 0U ){ return; }

	// 利きを見て現在のマスに動ける駒があるか調べる。
	b = 0;
	for( int x = getFirstBit( e ) ; x != 0 ; x = getFirstBit( e >> b ) ){
		b += x;
		if( b <= 12 ){
			dir = _dir[b- 1];
			if( dir == info.check ){
				// 王様は除外
				continue;
			}
			from = to - dir;
		}
		else{
			dir = _dir[b-13];
			for( from = to - dir ; ban[from] == EMPTY ; from -= dir )
				;
		}

		// 紐がついているかチェック
		if( eff && !( effect[from] & _dir2bitF[dir] ) ){
			continue;
		}

		// ピンされていないか
		if( ( info.pin & _addr2bit[from] ) != 0x00U ){
			continue;
		}

		// 王手
		if( info.flag & MOVE_CHECK && IsDiscCheckMove( info.ou2, to, from ) ){
			dflag = 1;
		}
		else{
			dflag = 0;
		}

		// 着手可能手追加
		bool n = 0;
		KOMA koma = ban[from];
		if( !( koma & NARI ) && koma != SKI && koma != GKI &&
			( ( ksengo == SENTE && ( to <= BanAddr(9,3) || from <= BanAddr(9,3) ) ) ||
		  	( ksengo == GOTE  && ( to >= BanAddr(1,7) || from >= BanAddr(1,7) ) ) ) )
		{
			// 成る場合
			if( dflag || !( info.flag & MOVE_CHECK ) ||
				IsCheckMove( info.ou2, to, koma + NARI )
			){
				moves.Add( from, to, 1, koma );
				n = true;
			}
		}
		if( !MoveError( koma, to ) &&
			// 王手の場合でも不成は生成しないように変更
			( !n /*|| info.flag & MOVE_CHECK*/ || Narazu( koma, to) ) )
		{
			// 成らない場合
			if( dflag || !( info.flag & MOVE_CHECK ) ||
				IsCheckMove( info.ou2, to, koma )
			){
				moves.Add( from, to, 0, koma );
			}
		}
	}
}

void Shogi::GenerateEvasionDrop( Moves& moves, GEN_INFO& info, int to ){
/************************************************
持ち駒による合駒
************************************************/
	KOMA koma;
	int i;

	for( koma = ksengo + FU ; koma <= ksengo + HI ; koma++ ){
		if( dai[koma] > 0 && !MoveError( koma, to ) ){
			if( info.flag & MOVE_CHECK && !IsCheckMove( info.ou2, to, koma ) ){
				continue;
			}

			if( koma == SFU || koma == GFU ){
				// 二歩のチェック
				i = to % 0x10;
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
					if( to - 0x10 == gou && IsUchifu( to ) )
						goto lab_nifu;
				}
				else{
					if( to + 0x10 == sou && IsUchifu( to ) )
						goto lab_nifu;
				}
			}

			// 着手可能手追加
			moves.Add( DAI + koma, to, 0, koma );
		}
lab_nifu:
		;
	}
}

void Shogi::GenerateCapEvasion( Moves& moves, GEN_INFO& info ){
/************************************************
王手をかけている駒を取る手の生成
両王手のときに呼び出してはいけない。
両王手の場合はGenerateMovesOuを呼ぶこと。
MOVE_CHECK以外の条件は全て無視(常に生成)とする。
MOVE_CHECKを指定している場合は王手のみを生成する。
************************************************/
	// 王手している駒の位置
	info.addr = info.ou + info.check * info.cnt;

	// 王手している駒を取る手を生成
	GenerateEvasionBan( moves, info );
}

void Shogi::GenerateNoCapEvasion( Moves& moves, GEN_INFO& info, bool no_pro ){
/************************************************
合駒による王手回避手生成
両王手のときに呼び出してはいけない。
両王手の場合はGenerateMovesOuを呼ぶこと。
MOVE_CHECK以外の条件は全て無視(常に生成)とする。
MOVE_CHECKを指定している場合は王手のみを生成する。
************************************************/
	int addr;
	int i;
	bool eff = false; // 利きが必要な場合
	unsigned* effect = ( ksengo == SENTE ? effectS : effectG );

	addr = info.ou;

	for( i = 1 ; i < info.cnt ; i++ ){
		// 王と王手している駒の間
		addr += info.check;
		unsigned e = effect[addr];

		// 利きがなければ移動できる駒はない。
		if( e ){
			// 紐のついていない合駒を生成しない場合
			if( !no_pro ){
				// 2つ以上の利きがあれば移動合いでも
				// 必ず紐がついている。
				if( e & ( e -1 ) ){ eff = false; }
				else              { eff = true; }
			}
			// else{ eff = false; } // eff == false のまま変化しない。

			info.addr = addr;
			GenerateEvasionBan( moves, info, eff );
		}

		// 紐なしでもOK または 利きあり
		if( no_pro || e ){
			// 駒台
			GenerateEvasionDrop( moves, info, addr );
		} 
	}
}

void Shogi::GenerateCapEvasion( Moves& moves ){
/************************************************
王手している駒を取る手を生成する。
************************************************/
	GEN_INFO info;

	// 生成する指し手の種類
	SetGenInfo( info, MOVE_ALL );

	// ピン
	SetGenInfoPin( info );

	// 両王手の場合は王の移動でしか回避できない。
	if( info.check && info.check != DOUBLE_CHECK ){
		// 王手回避手生成
		GenerateCapEvasion( moves, info );
	}
}

void Shogi::GenerateNoCapEvasion( Moves& moves, bool no_pro ){
/************************************************
合駒する手を生成する。
************************************************/
	GEN_INFO info;

	// 生成する指し手の種類
	SetGenInfo( info, MOVE_ALL );

	// ピン
	SetGenInfoPin( info );

	// 両王手の場合は王の移動でしか回避できない。
	if( info.check && info.check != DOUBLE_CHECK ){
		// 王手回避手生成
		GenerateNoCapEvasion( moves, info, no_pro );
	}
}

void Shogi::GenerateEvasion( Moves& moves ){
/************************************************
着手可能手を列挙する。
************************************************/
	GEN_INFO info;

	// 生成する指し手の種類
	SetGenInfo( info, MOVE_ALL );

	// ピン
	SetGenInfoPin( info );

	// 両王手の場合は王の移動でしか回避できない。
	if( info.check && info.check != DOUBLE_CHECK ){
		// 王手回避手生成
		GenerateCapEvasion( moves, info );
		GenerateNoCapEvasion( moves, info );
	}

	GenerateMovesOu( moves, info );
}

void Shogi::GenerateTacticalMoves( Moves& moves ){
/************************************************
戦略的な手を列挙する。
Capture, Promote, (King)
************************************************/
	GEN_INFO info;

	// 生成する指し手の種類
	SetGenInfo( info, MOVE_CAPTURE | MOVE_PROMOTE );

	// ピン
	SetGenInfoPin( info );

	if( !info.check ){
		// 盤上
		GenerateMovesOnBoard( moves, info );
	}
	else if( info.check != DOUBLE_CHECK ){
		GenerateCapEvasion( moves, info );
		GenerateNoCapEvasion( moves, info );
	}

	if( moves.GetNumber() == 0 ){
		// 玉を動かす手
		info.flag |= MOVE_KING;
		GenerateMovesOu( moves, info );
	}
}
