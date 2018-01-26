/* effect.cpp
 * R.Kubo 2010
 * 盤面の利き情報
 */

#include "shogi.h"

void Shogi::InitEffect(){
/************************************************
利きの初期化
************************************************/
	int i, j;
	int addr, addr2;
	KOMA koma;
	int d;
	int dir;

	memset( _effectS, 0, sizeof(_effectS) );
	memset( _effectG, 0, sizeof(_effectG) );

	effectS = _effectS + BAN_OFFSET;
	effectG = _effectG + BAN_OFFSET;

	// 0で初期化
	for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
		for( i = 1 ; i <= 9 ; i++ ){
			addr = i + j;
			effectS[addr] = 0x00U;
			effectG[addr] = 0x00U;
		}
	}

	// 全てのマスを調べる。
	for( j = 0x10 ; j <= 0x90 ; j += 0x10 ){
		for( i = 1 ; i <= 9 ; i++ ){
			addr = i + j;

			// 駒があるか
			if( ( koma = ban[addr] ) != EMPTY ){
				// 各方向を調べる。
				for( d = 0 ; d < DNUM ; d++ ){
					// その方向に移動できるか
					switch( _mov[koma][d] ){
					case 1:
						// 1マスしか進めない場合
						dir = _dir[d];
						// 利き追加
						if( koma & SENTE )
							effectS[addr+dir] |= _dir2bit[dir];
						else
							effectG[addr+dir] |= _dir2bit[dir];
						break;
					case 2:
						// 何マスでも進める場合
						dir = _dir[d];
						for( addr2 = addr + dir ; ; addr2 += dir ){
							// 利き追加
							if( koma & SENTE )
								effectS[addr2] |= _dir2bitF[dir];
							else
								effectG[addr2] |= _dir2bitF[dir];

							// 駒があったら終了
							if( ban[addr2] != EMPTY )
								break;
						}
					}
				}
			}
		}
	}
}

void Shogi::SetEffect( int addr, int dir, unsigned int* __restrict__ effect, unsigned bit ){
/************************************************
飛び駒の利きを付ける。
************************************************/
	addr += dir;

	// 1
	effect[addr] |= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 2
	effect[addr] |= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 3
	effect[addr] |= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 4
	effect[addr] |= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 5
	effect[addr] |= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 6
	effect[addr] |= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 7
	effect[addr] |= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 8
	effect[addr] |= bit;
}

void Shogi::UnsetEffect( int addr, int dir, unsigned int* __restrict__ effect, unsigned bit ){
/************************************************
飛び駒の利きを消す。
************************************************/
	addr += dir;

	// 1
	effect[addr] -= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 2
	effect[addr] -= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 3
	effect[addr] -= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 4
	effect[addr] -= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 5
	effect[addr] -= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 6
	effect[addr] -= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 7
	effect[addr] -= bit;
	if( ban[addr] != EMPTY ) return;
	addr += dir;

	// 8
	effect[addr] -= bit;
}

// ビットシフト
#define S(x)				(0x01U<<(x))

void Shogi::UpdateEffect( int addr ){
/************************************************
利き情報の差分計算
************************************************/
	assert( addr >= 0x11 );
	assert( addr <= 0x99 );
	assert( ban[addr] != EMPTY );
	assert( ban[addr] != WALL );

	// 他の駒について跳び駒の利きをとめる。
	{
		unsigned effect;
		int x;
		effect = effectS[addr] >> 12;
		for( x = getFirstBit( effect ) ; x != 0 ; x = getFirstBit( effect ) ){
			removeFirstBit( effect );
			UnsetEffect( addr, _dir[x-1], effectS, S(x+11) );
		}

		effect = effectG[addr] >> 12;
		for( x = getFirstBit( effect ) ; x != 0 ; x = getFirstBit( effect ) ){
			removeFirstBit( effect );
			UnsetEffect( addr, _dir[x-1], effectG, S(x+11) );
		}
	}

	// 移動した駒について利きをつける。
	switch( ban[addr] ){
	case SFU:
		SetEffect1Step( addr, -0x10     , effectS, S( 1) );
		break;
	case SKY:
		SetEffect     ( addr, -0x10     , effectS, S(13) );
		break;
	case SKE:
		SetEffect1Step( addr, -0x20+0x01, effectS, S(11) );
		SetEffect1Step( addr, -0x20-0x01, effectS, S(10) );
		break;
	case SGI:
		SetEffect1Step( addr, -0x10     , effectS, S( 1) );
		SetEffect1Step( addr, -0x10-0x01, effectS, S( 0) );
		SetEffect1Step( addr, -0x10+0x01, effectS, S( 2) );
		SetEffect1Step( addr, +0x10-0x01, effectS, S( 5) );
		SetEffect1Step( addr, +0x10+0x01, effectS, S( 7) );
		break;
	case SKI: case STO: case SNY: case SNK: case SNG:
		SetEffect1Step( addr, -0x10     , effectS, S( 1) );
		SetEffect1Step( addr, -0x10-0x01, effectS, S( 0) );
		SetEffect1Step( addr, -0x10+0x01, effectS, S( 2) );
		SetEffect1Step( addr,      -0x01, effectS, S( 3) );
		SetEffect1Step( addr,      +0x01, effectS, S( 4) );
		SetEffect1Step( addr, +0x10     , effectS, S( 6) );
		break;
	case SKA:
		SetEffect     ( addr, -0x10-0x01, effectS, S(12) );
		SetEffect     ( addr, -0x10+0x01, effectS, S(14) );
		SetEffect     ( addr, +0x10-0x01, effectS, S(17) );
		SetEffect     ( addr, +0x10+0x01, effectS, S(19) );
		break;
	case SHI:
		SetEffect     ( addr, -0x10     , effectS, S(13) );
		SetEffect     ( addr, +0x10     , effectS, S(18) );
		SetEffect     ( addr,      -0x01, effectS, S(15) );
		SetEffect     ( addr,      +0x01, effectS, S(16) );
		break;
	case SUM:
		SetEffect     ( addr, -0x10-0x01, effectS, S(12) );
		SetEffect     ( addr, -0x10+0x01, effectS, S(14) );
		SetEffect     ( addr, +0x10-0x01, effectS, S(17) );
		SetEffect     ( addr, +0x10+0x01, effectS, S(19) );
		SetEffect1Step( addr, -0x10     , effectS, S( 1) );
		SetEffect1Step( addr, +0x10     , effectS, S( 6) );
		SetEffect1Step( addr,      -0x01, effectS, S( 3) );
		SetEffect1Step( addr,      +0x01, effectS, S( 4) );
		break;
	case SRY:
		SetEffect     ( addr, -0x10     , effectS, S(13) );
		SetEffect     ( addr, +0x10     , effectS, S(18) );
		SetEffect     ( addr,      -0x01, effectS, S(15) );
		SetEffect     ( addr,      +0x01, effectS, S(16) );
		SetEffect1Step( addr, -0x10-0x01, effectS, S( 0) );
		SetEffect1Step( addr, -0x10+0x01, effectS, S( 2) );
		SetEffect1Step( addr, +0x10-0x01, effectS, S( 5) );
		SetEffect1Step( addr, +0x10+0x01, effectS, S( 7) );
		break;
	case SOU:
		SetEffect1Step( addr, -0x10-0x01, effectS, S( 0) );
		SetEffect1Step( addr, -0x10     , effectS, S( 1) );
		SetEffect1Step( addr, -0x10+0x01, effectS, S( 2) );
		SetEffect1Step( addr,      -0x01, effectS, S( 3) );
		SetEffect1Step( addr,      +0x01, effectS, S( 4) );
		SetEffect1Step( addr, +0x10-0x01, effectS, S( 5) );
		SetEffect1Step( addr, +0x10     , effectS, S( 6) );
		SetEffect1Step( addr, +0x10+0x01, effectS, S( 7) );
		break;
	case GFU:
		SetEffect1Step( addr, +0x10     , effectG, S( 6) );
		break;
	case GKY:
		SetEffect     ( addr, +0x10     , effectG, S(18) );
		break;
	case GKE:
		SetEffect1Step( addr, +0x20+0x01, effectG, S( 9) );
		SetEffect1Step( addr, +0x20-0x01, effectG, S( 8) );
		break;
	case GGI:
		SetEffect1Step( addr, +0x10     , effectG, S( 6) );
		SetEffect1Step( addr, +0x10-0x01, effectG, S( 5) );
		SetEffect1Step( addr, +0x10+0x01, effectG, S( 7) );
		SetEffect1Step( addr, -0x10-0x01, effectG, S( 0) );
		SetEffect1Step( addr, -0x10+0x01, effectG, S( 2) );
		break;
	case GKI: case GTO: case GNY: case GNK: case GNG:
		SetEffect1Step( addr, +0x10     , effectG, S( 6) );
		SetEffect1Step( addr, +0x10-0x01, effectG, S( 5) );
		SetEffect1Step( addr, +0x10+0x01, effectG, S( 7) );
		SetEffect1Step( addr,      -0x01, effectG, S( 3) );
		SetEffect1Step( addr,      +0x01, effectG, S( 4) );
		SetEffect1Step( addr, -0x10     , effectG, S( 1) );
		break;
	case GKA:
		SetEffect     ( addr, +0x10-0x01, effectG, S(17) );
		SetEffect     ( addr, +0x10+0x01, effectG, S(19) );
		SetEffect     ( addr, -0x10-0x01, effectG, S(12) );
		SetEffect     ( addr, -0x10+0x01, effectG, S(14) );
		break;
	case GHI:
		SetEffect     ( addr, +0x10     , effectG, S(18) );
		SetEffect     ( addr, -0x10     , effectG, S(13) );
		SetEffect     ( addr,      -0x01, effectG, S(15) );
		SetEffect     ( addr,      +0x01, effectG, S(16) );
		break;
	case GUM:
		SetEffect     ( addr, +0x10-0x01, effectG, S(17) );
		SetEffect     ( addr, +0x10+0x01, effectG, S(19) );
		SetEffect     ( addr, -0x10-0x01, effectG, S(12) );
		SetEffect     ( addr, -0x10+0x01, effectG, S(14) );
		SetEffect1Step( addr, +0x10     , effectG, S( 6) );
		SetEffect1Step( addr, -0x10     , effectG, S( 1) );
		SetEffect1Step( addr,      -0x01, effectG, S( 3) );
		SetEffect1Step( addr,      +0x01, effectG, S( 4) );
		break;
	case GRY:
		SetEffect     ( addr, +0x10     , effectG, S(18) );
		SetEffect     ( addr, -0x10     , effectG, S(13) );
		SetEffect     ( addr,      -0x01, effectG, S(15) );
		SetEffect     ( addr,      +0x01, effectG, S(16) );
		SetEffect1Step( addr, +0x10-0x01, effectG, S( 5) );
		SetEffect1Step( addr, +0x10+0x01, effectG, S( 7) );
		SetEffect1Step( addr, -0x10-0x01, effectG, S( 0) );
		SetEffect1Step( addr, -0x10+0x01, effectG, S( 2) );
		break;
	case GOU:
		SetEffect1Step( addr, -0x10-0x01, effectG, S( 0) );
		SetEffect1Step( addr, -0x10     , effectG, S( 1) );
		SetEffect1Step( addr, -0x10+0x01, effectG, S( 2) );
		SetEffect1Step( addr,      -0x01, effectG, S( 3) );
		SetEffect1Step( addr,      +0x01, effectG, S( 4) );
		SetEffect1Step( addr, +0x10-0x01, effectG, S( 5) );
		SetEffect1Step( addr, +0x10     , effectG, S( 6) );
		SetEffect1Step( addr, +0x10+0x01, effectG, S( 7) );
		break;
	}
}

void Shogi::UpdateEffectR( int addr ){
/************************************************
利き情報の差分計算(駒がなくなった場合)
************************************************/
	assert( addr >= 0x11 );
	assert( addr <= 0x99 );
	assert( ban[addr] != EMPTY );
	assert( ban[addr] != WALL );

	// 移動した駒について利きを消す。
	switch( ban[addr] ){
	case SFU:
		UnsetEffect1Step( addr, -0x10     , effectS, S( 1) );
		break;
	case SKY:
		UnsetEffect     ( addr, -0x10     , effectS, S(13) );
		break;
	case SKE:
		UnsetEffect1Step( addr, -0x20+0x01, effectS, S(11) );
		UnsetEffect1Step( addr, -0x20-0x01, effectS, S(10) );
		break;
	case SGI:
		UnsetEffect1Step( addr, -0x10     , effectS, S( 1) );
		UnsetEffect1Step( addr, -0x10-0x01, effectS, S( 0) );
		UnsetEffect1Step( addr, -0x10+0x01, effectS, S( 2) );
		UnsetEffect1Step( addr, +0x10-0x01, effectS, S( 5) );
		UnsetEffect1Step( addr, +0x10+0x01, effectS, S( 7) );
		break;
	case SKI: case STO: case SNY: case SNK: case SNG:
		UnsetEffect1Step( addr, -0x10     , effectS, S( 1) );
		UnsetEffect1Step( addr, -0x10-0x01, effectS, S( 0) );
		UnsetEffect1Step( addr, -0x10+0x01, effectS, S( 2) );
		UnsetEffect1Step( addr,      -0x01, effectS, S( 3) );
		UnsetEffect1Step( addr,      +0x01, effectS, S( 4) );
		UnsetEffect1Step( addr, +0x10     , effectS, S( 6) );
		break;
	case SKA:
		UnsetEffect     ( addr, -0x10-0x01, effectS, S(12) );
		UnsetEffect     ( addr, -0x10+0x01, effectS, S(14) );
		UnsetEffect     ( addr, +0x10-0x01, effectS, S(17) );
		UnsetEffect     ( addr, +0x10+0x01, effectS, S(19) );
		break;
	case SHI:
		UnsetEffect     ( addr, -0x10     , effectS, S(13) );
		UnsetEffect     ( addr, +0x10     , effectS, S(18) );
		UnsetEffect     ( addr,      -0x01, effectS, S(15) );
		UnsetEffect     ( addr,      +0x01, effectS, S(16) );
		break;
	case SUM:
		UnsetEffect     ( addr, -0x10-0x01, effectS, S(12) );
		UnsetEffect     ( addr, -0x10+0x01, effectS, S(14) );
		UnsetEffect     ( addr, +0x10-0x01, effectS, S(17) );
		UnsetEffect     ( addr, +0x10+0x01, effectS, S(19) );
		UnsetEffect1Step( addr, -0x10     , effectS, S( 1) );
		UnsetEffect1Step( addr, +0x10     , effectS, S( 6) );
		UnsetEffect1Step( addr,      -0x01, effectS, S( 3) );
		UnsetEffect1Step( addr,      +0x01, effectS, S( 4) );
		break;
	case SRY:
		UnsetEffect     ( addr, -0x10     , effectS, S(13) );
		UnsetEffect     ( addr, +0x10     , effectS, S(18) );
		UnsetEffect     ( addr,      -0x01, effectS, S(15) );
		UnsetEffect     ( addr,      +0x01, effectS, S(16) );
		UnsetEffect1Step( addr, -0x10-0x01, effectS, S( 0) );
		UnsetEffect1Step( addr, -0x10+0x01, effectS, S( 2) );
		UnsetEffect1Step( addr, +0x10-0x01, effectS, S( 5) );
		UnsetEffect1Step( addr, +0x10+0x01, effectS, S( 7) );
		break;
	case SOU:
		UnsetEffect1Step( addr, -0x10-0x01, effectS, S( 0) );
		UnsetEffect1Step( addr, -0x10     , effectS, S( 1) );
		UnsetEffect1Step( addr, -0x10+0x01, effectS, S( 2) );
		UnsetEffect1Step( addr,      -0x01, effectS, S( 3) );
		UnsetEffect1Step( addr,      +0x01, effectS, S( 4) );
		UnsetEffect1Step( addr, +0x10-0x01, effectS, S( 5) );
		UnsetEffect1Step( addr, +0x10     , effectS, S( 6) );
		UnsetEffect1Step( addr, +0x10+0x01, effectS, S( 7) );
		break;
	case GFU:
		UnsetEffect1Step( addr, +0x10     , effectG, S( 6) );
		break;
	case GKY:
		UnsetEffect     ( addr, +0x10     , effectG, S(18) );
		break;
	case GKE:
		UnsetEffect1Step( addr, +0x20+0x01, effectG, S( 9) );
		UnsetEffect1Step( addr, +0x20-0x01, effectG, S( 8) );
		break;
	case GGI:
		UnsetEffect1Step( addr, +0x10     , effectG, S( 6) );
		UnsetEffect1Step( addr, +0x10-0x01, effectG, S( 5) );
		UnsetEffect1Step( addr, +0x10+0x01, effectG, S( 7) );
		UnsetEffect1Step( addr, -0x10-0x01, effectG, S( 0) );
		UnsetEffect1Step( addr, -0x10+0x01, effectG, S( 2) );
		break;
	case GKI: case GTO: case GNY: case GNK: case GNG:
		UnsetEffect1Step( addr, +0x10     , effectG, S( 6) );
		UnsetEffect1Step( addr, +0x10-0x01, effectG, S( 5) );
		UnsetEffect1Step( addr, +0x10+0x01, effectG, S( 7) );
		UnsetEffect1Step( addr,      -0x01, effectG, S( 3) );
		UnsetEffect1Step( addr,      +0x01, effectG, S( 4) );
		UnsetEffect1Step( addr, -0x10     , effectG, S( 1) );
		break;
	case GKA:
		UnsetEffect     ( addr, +0x10-0x01, effectG, S(17) );
		UnsetEffect     ( addr, +0x10+0x01, effectG, S(19) );
		UnsetEffect     ( addr, -0x10-0x01, effectG, S(12) );
		UnsetEffect     ( addr, -0x10+0x01, effectG, S(14) );
		break;
	case GHI:
		UnsetEffect     ( addr, +0x10     , effectG, S(18) );
		UnsetEffect     ( addr, -0x10     , effectG, S(13) );
		UnsetEffect     ( addr,      -0x01, effectG, S(15) );
		UnsetEffect     ( addr,      +0x01, effectG, S(16) );
		break;
	case GUM:
		UnsetEffect     ( addr, +0x10-0x01, effectG, S(17) );
		UnsetEffect     ( addr, +0x10+0x01, effectG, S(19) );
		UnsetEffect     ( addr, -0x10-0x01, effectG, S(12) );
		UnsetEffect     ( addr, -0x10+0x01, effectG, S(14) );
		UnsetEffect1Step( addr, +0x10     , effectG, S( 6) );
		UnsetEffect1Step( addr, -0x10     , effectG, S( 1) );
		UnsetEffect1Step( addr,      -0x01, effectG, S( 3) );
		UnsetEffect1Step( addr,      +0x01, effectG, S( 4) );
		break;
	case GRY:
		UnsetEffect     ( addr, +0x10     , effectG, S(18) );
		UnsetEffect     ( addr, -0x10     , effectG, S(13) );
		UnsetEffect     ( addr,      -0x01, effectG, S(15) );
		UnsetEffect     ( addr,      +0x01, effectG, S(16) );
		UnsetEffect1Step( addr, +0x10-0x01, effectG, S( 5) );
		UnsetEffect1Step( addr, +0x10+0x01, effectG, S( 7) );
		UnsetEffect1Step( addr, -0x10-0x01, effectG, S( 0) );
		UnsetEffect1Step( addr, -0x10+0x01, effectG, S( 2) );
		break;
	case GOU:
		UnsetEffect1Step( addr, -0x10-0x01, effectG, S( 0) );
		UnsetEffect1Step( addr, -0x10     , effectG, S( 1) );
		UnsetEffect1Step( addr, -0x10+0x01, effectG, S( 2) );
		UnsetEffect1Step( addr,      -0x01, effectG, S( 3) );
		UnsetEffect1Step( addr,      +0x01, effectG, S( 4) );
		UnsetEffect1Step( addr, +0x10-0x01, effectG, S( 5) );
		UnsetEffect1Step( addr, +0x10     , effectG, S( 6) );
		UnsetEffect1Step( addr, +0x10+0x01, effectG, S( 7) );
		break;
	}

	// 他の駒について飛び駒の利きを延長する。
	{
		unsigned effect;
		int x;
		effect = effectS[addr] >> 12;
		for( x = getFirstBit( effect ) ; x != 0 ; x = getFirstBit( effect ) ){
			removeFirstBit( effect );
			SetEffect( addr, _dir[x-1], effectS, S(x+11) );
		}

		effect = effectG[addr] >> 12;
		for( x = getFirstBit( effect ) ; x != 0 ; x = getFirstBit( effect ) ){
			removeFirstBit( effect );
			SetEffect( addr, _dir[x-1], effectG, S(x+11) );
		}
	}
}

