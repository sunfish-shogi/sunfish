/* gencheck.cpp
 * R.Kubo 2008-2010
 * 王手生成
 */

#include "shogi.h"

void Shogi::GenerateCheckDrop( Moves& moves, GEN_INFO& info, int dir, unsigned flag, unsigned flag2 ){
/************************************************
駒打ちによる王手を列挙する。
************************************************/
	int addr = info.ou2 + dir;

	assert( !( flag  & (1U|(1U<<HI)) ) );
	assert( !( flag2 & (1U|(1U<<HI)) ) );

	if( ban[addr] != EMPTY ){
		return;
	}

	// 小駒(1マス動ける駒)
	if( flag != 0U ){
		for( int x = getFirstBit( flag ) ; x != 0 ; x = getFirstBit( flag ) ){
			removeFirstBit( flag );
			assert( x >= KY );
			assert( x <= HI );
			KOMA koma = x | ksengo;
			moves.Add( DAI + koma, addr, 0, koma );
		}
	}

	// 跳び駒
	if( flag2 != 0U ){
		for( addr += dir ; ban[addr] == EMPTY ; addr += dir ){
			unsigned f = flag2;
			for( int x = getFirstBit( f ) ; x != 0 ; x = getFirstBit( f ) ){
				removeFirstBit( f );
				assert( x >= KY );
				assert( x <= HI );
				KOMA koma = x | ksengo;
				moves.Add( DAI + koma, addr, 0, koma );
			}
		}
	}
}

void Shogi::GenerateCheckDrop( Moves& moves, GEN_INFO& info ){
/************************************************
駒打ちによる王手を列挙する。
(自玉に王手がかかっていない時のみ)
************************************************/
	// 歩
	{
		int i = info.ou2 % 0x10;
		if( ksengo == SENTE ){
			int addr = info.ou2 + 0x10;
			// 歩がある かつ マスが空いている かつ 二歩ではない
			if( dai[SFU] != 0 && ban[addr] == EMPTY && !( kifu[know].sfu & ( 1 << i ) ) ){
				// 打ち歩詰めのチェック
				if( !IsUchifu( addr ) ){
					// 指し手追加
					moves.Add( DAI + SFU, addr, 0, SFU );
				}
			}
		}
		else{
			int addr = info.ou2 - 0x10;
			// 歩がある かつ マスが空いている かつ 二歩ではない
			if( dai[GFU] != 0 && ban[addr] == EMPTY && !( kifu[know].gfu & ( 1 << i ) ) ){
				// 打ち歩詰めのチェック
				if( !IsUchifu( addr ) ){
					// 指し手追加
					moves.Add( DAI + GFU, addr, 0, GFU );
				}
			}
		}
	}

	// 歩以外
	{
		if( ksengo == SENTE ){
			unsigned flag = 0U;

			// 持っている駒
			for( KOMA koma = SKY ; koma <= SHI ; koma++ ){
				if( dai[koma] != 0 ){
					flag |= 1 << ( koma - SFU );
				}
			}

			if( flag != 0U ){
				GenerateCheckDrop( moves, info, +0x10     , flag & FLAG_FRONT , flag & FLAG_FRONT2  );
				GenerateCheckDrop( moves, info, +0x20+0x01, flag & FLAG_BISHOP, 0U                  );
				GenerateCheckDrop( moves, info, +0x20-0x01, flag & FLAG_BISHOP, 0U                  );
				GenerateCheckDrop( moves, info, +0x10+0x01, flag & FLAG_DIAG  , flag & FLAG_DIAG2   );
				GenerateCheckDrop( moves, info, +0x10-0x01, flag & FLAG_DIAG  , flag & FLAG_DIAG2   );
				GenerateCheckDrop( moves, info,      +0x01, flag & FLAG_SIDE  , flag & FLAG_SIDE2   );
				GenerateCheckDrop( moves, info,      -0x01, flag & FLAG_SIDE  , flag & FLAG_SIDE2   );
				GenerateCheckDrop( moves, info, -0x10+0x01, flag & FLAG_DIAGB , flag & FLAG_DIAGB2  );
				GenerateCheckDrop( moves, info, -0x10-0x01, flag & FLAG_DIAGB , flag & FLAG_DIAGB2  );
				GenerateCheckDrop( moves, info, -0x10     , flag & FLAG_BACK  , flag & FLAG_BACK2   );
			}
		}
		else{
			unsigned flag = 0U;

			// 持っている駒
			for( KOMA koma = GKY ; koma <= GHI ; koma++ ){
				if( dai[koma] != 0 ){
					flag |= 1 << ( koma - GFU );
				}
			}

			if( flag != 0U ){
				GenerateCheckDrop( moves, info, -0x10     , flag & FLAG_FRONT , flag & FLAG_FRONT2  );
				GenerateCheckDrop( moves, info, -0x20+0x01, flag & FLAG_BISHOP, 0U                  );
				GenerateCheckDrop( moves, info, -0x20-0x01, flag & FLAG_BISHOP, 0U                  );
				GenerateCheckDrop( moves, info, -0x10+0x01, flag & FLAG_DIAG  , flag & FLAG_DIAG2   );
				GenerateCheckDrop( moves, info, -0x10-0x01, flag & FLAG_DIAG  , flag & FLAG_DIAG2   );
				GenerateCheckDrop( moves, info,      +0x01, flag & FLAG_SIDE  , flag & FLAG_SIDE2   );
				GenerateCheckDrop( moves, info,      -0x01, flag & FLAG_SIDE  , flag & FLAG_SIDE2   );
				GenerateCheckDrop( moves, info, +0x10+0x01, flag & FLAG_DIAGB , flag & FLAG_DIAGB2  );
				GenerateCheckDrop( moves, info, +0x10-0x01, flag & FLAG_DIAGB , flag & FLAG_DIAGB2  );
				GenerateCheckDrop( moves, info, +0x10     , flag & FLAG_BACK  , flag & FLAG_BACK2   );
			}
		}
	}
}

void Shogi::GenerateCheckDisc( Moves& moves, GEN_INFO& info, int dir, unsigned* effect ){
/************************************************
開き王手を生成する。 (両王手は除く)
GenerateCheckDisc以外からは使用不可
dir : 玉から見た方向
************************************************/
	// 空いているマスを飛ばす。
	for( info.addr = info.ou2 + dir ; ban[info.addr] == EMPTY ; info.addr += dir )
		;

	// 玉以外の自分の駒で、相手玉へ向かう利きがある場合
	if( ( info.koma = ban[info.addr] ) & ksengo && info.koma & 0x07 &&
		effect[info.addr] & _dir2bitF[-dir] )
	{
		// ピン
		if( ( info.pin & _addr2bit[info.addr] ) == 0x00U ){
			info.dir_p = 0;
		}
		else{
			info.dir_p = _diff2dir[info.ou-info.addr];
		}

		for( int d = 0 ; d < DNUM ; d++ ){
			int dir2 = _dir[d];
			if( dir2 != dir && dir2 != -dir ){
				switch( _mov[info.koma][d] ){
				case 1:
					GenerateMoves1Step( moves, info, dir2 );
					break;
				case 2:
					GenerateMovesStraight( moves, info, dir2 );
					break;
				}
			}
		}
	}
}

void Shogi::GenerateCheckDisc( Moves& moves, GEN_INFO& info ){
/************************************************
開き王手を生成する。 (両王手は除く)
(自玉に王手がかかっていない時のみ)
************************************************/
	// 両王手を生成しないためにフラグを変える。
	unsigned flag = info.flag;
	info.flag = MOVE_NCHECK;

	// 玉が跳び駒のライン上に居るならその方向へ調べる。
	if( ksengo == SENTE ){
		uint96 bb_ka = bb[SUM] | bb[SKA];
		uint96 bb_hi = bb[SRY] | bb[SHI];
		uint96 bb_ky = bb_hi | bb[SKY];
		int a = _addr[gou];
		if( ( bb_ka & _dbits_lu[a] ) != 0U ){ GenerateCheckDisc( moves, info, -17, effectS ); }
		if( ( bb_hi & _dbits_up[a] ) != 0U ){ GenerateCheckDisc( moves, info, -16, effectS ); }
		if( ( bb_ka & _dbits_ru[a] ) != 0U ){ GenerateCheckDisc( moves, info, -15, effectS ); }
		if( ( bb_hi & _dbits_lt[a] ) != 0U ){ GenerateCheckDisc( moves, info, - 1, effectS ); }
		if( ( bb_hi & _dbits_rt[a] ) != 0U ){ GenerateCheckDisc( moves, info, + 1, effectS ); }
		if( ( bb_ka & _dbits_ld[a] ) != 0U ){ GenerateCheckDisc( moves, info, +15, effectS ); }
		if( ( bb_ky & _dbits_dn[a] ) != 0U ){ GenerateCheckDisc( moves, info, +16, effectS ); }
		if( ( bb_ka & _dbits_rd[a] ) != 0U ){ GenerateCheckDisc( moves, info, +17, effectS ); }
	}
	else{
		uint96 bb_ka = bb[GUM] | bb[GKA];
		uint96 bb_hi = bb[GRY] | bb[GHI];
		uint96 bb_ky = bb_hi | bb[GKY];
		int a = _addr[sou];
		if( ( bb_ka & _dbits_lu[a] ) != 0U ){ GenerateCheckDisc( moves, info, -17, effectG ); }
		if( ( bb_ky & _dbits_up[a] ) != 0U ){ GenerateCheckDisc( moves, info, -16, effectG ); }
		if( ( bb_ka & _dbits_ru[a] ) != 0U ){ GenerateCheckDisc( moves, info, -15, effectG ); }
		if( ( bb_hi & _dbits_lt[a] ) != 0U ){ GenerateCheckDisc( moves, info, - 1, effectG ); }
		if( ( bb_hi & _dbits_rt[a] ) != 0U ){ GenerateCheckDisc( moves, info, + 1, effectG ); }
		if( ( bb_ka & _dbits_ld[a] ) != 0U ){ GenerateCheckDisc( moves, info, +15, effectG ); }
		if( ( bb_hi & _dbits_dn[a] ) != 0U ){ GenerateCheckDisc( moves, info, +16, effectG ); }
		if( ( bb_ka & _dbits_rd[a] ) != 0U ){ GenerateCheckDisc( moves, info, +17, effectG ); }
	}

	info.flag = flag; // 元に戻す。
}

void Shogi::GenerateCheckNoDisc( Moves& moves, GEN_INFO& info ){
/************************************************
盤上の駒による(非開き)王手を列挙する。
(自玉に王手がかかっていない時のみ)
************************************************/
	{ // 小駒
		int i, j;
		int m, n;
		int i0, i1;
		int j0, j1;
	
		// 王手できる小駒が存在し得る範囲
		// 桂馬があるので段は先後で異なる。
		m = info.ou2 & 0x0F; // 筋
		i0 = max( 1, m-2 );  // 2マス左
		i1 = min( 9, m+2 );  // 2マス右

		n = info.ou2 & 0xF0; // 段
		if( ksengo == SENTE ){
			j0 = max( 0x10, n-0x20 ); // 2マス前方
			j1 = min( 0x90, n+0x40 ); // 4マス後方
		}
		else{
			j0 = max( 0x10, n-0x40 ); // 4マス前方
			j1 = min( 0x90, n+0x20 ); // 2マス後方
		}

		for( j = j0 ; j <= j1 ; j += 0x10 ){
			for( i = i0 ; i <= i1 ; i++ ){
				info.addr = i + j;
	
				if( ( info.koma = ban[info.addr] ) & ksengo &&
					info.koma & 0x07 && !isFly[info.koma] ) // 王様は除外
				{
					unsigned dflag = _diff2attack[info.koma]
						[info.addr/16-info.ou2/16][info.addr%16-info.ou2%16];
	
					if( dflag != 0U ){
						if( ( info.pin & _addr2bit[info.addr] ) == 0x00U ){
							// ピンされていない。
							info.dir_p = 0;
						}
						else{
							// ピンされている。
							info.dir_p = _diff2dir[info.ou-info.addr];
						}
	
						do{
							int dir, d;
	
							d = getFirstBit( dflag ) - 1;
							dir = _dir[d];
							removeFirstBit( dflag );
	
							GenerateMoves1Step( moves, info, dir );
						} while( dflag != 0U );
					}
				}
			}
		}
	}

	{ // 跳び駒
		int bit = 0;
		uint96 _bb;
		if( ksengo == SENTE ){
			_bb = bb[SKY] | bb[SKA] | bb[SHI] | bb[SUM] | bb[SRY];
		}
		else{
			_bb = bb[GKY] | bb[GKA] | bb[GHI] | bb[GUM] | bb[GRY];
		}
		while( ( bit = _bb.pickoutFirstBit() ) != 0 ){
			info.addr = _iaddr0[bit];
			info.koma = ban[info.addr];
#ifndef NDEBUG
			if( info.koma == EMPTY ){
				PrintBan();
				DebugPrintBitboard( ksengo | KY );
				DebugPrintBitboard( ksengo | KA );
				DebugPrintBitboard( ksengo | HI );
				DebugPrintBitboard( ksengo | UM );
				DebugPrintBitboard( ksengo | RY );
				PrintAddr( info.addr );
				cout << " is empty!!\n";
				abort();
			}
#endif
			unsigned dflag = _diff2attack[info.koma]
				[info.addr/16-info.ou2/16][info.addr%16-info.ou2%16];

			if( dflag != 0U ){
				if( ( info.pin & _addr2bit[info.addr] ) == 0x00U ){
					// ピンされていない。
					info.dir_p = 0;
				}
				else{
					// ピンされている。
					info.dir_p = _diff2dir[info.ou-info.addr];
				}

				do{
					int dir, d;

					d = getFirstBit( dflag ) - 1;
					dir = _dir[d];
					removeFirstBit( dflag );

					switch( _mov[info.koma][d] ){
					case 1:
						GenerateMoves1Step( moves, info, dir );
						break;
					case 2:
						GenerateMovesStraight( moves, info, dir );
						break;
					}
				} while( dflag != 0U );
			}
		}
	}
}

void Shogi::GenerateCheck( Moves& moves ){
/************************************************
王手を列挙する。
************************************************/
	GEN_INFO info;

	// 生成する指し手の種類
	SetGenInfo( info, MOVE_CHECK );

	// ピン
	SetGenInfoPin( info );

	if( !info.check ){
		// 王手生成ルーチン
		GenerateCheckDrop( moves, info );
		GenerateCheckNoDisc( moves, info );
		GenerateCheckDisc( moves, info );
	}
	else if( info.check != DOUBLE_CHECK ){
		// 王手回避ルーチン
		GenerateCapEvasion( moves, info );
		GenerateNoCapEvasion( moves, info );
	}

	// 玉を動かす手
	GenerateMovesOu( moves, info );
}
