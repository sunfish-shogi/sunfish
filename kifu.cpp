/* kifu.cpp
 * R.Kubo 2008-2010
 * 棋譜の入出力
 */

#include <fstream>
#include <sstream>
#include "shogi.h"

static char s_koma[][3] = {
	"FU", "KY", "KE", "GI", "KI", "KA", "HI", "OU",
	"TO", "NY", "NK", "NG", "  ", "UM", "RY",
};

string Shogi::GetStrMove( const MOVE& move, bool eol ){
/************************************************
指し手を文字列に変換。
************************************************/
	ostringstream str;

	if( move.from & PASS ){
		str << "PASS";

		return str.str();
	}

	str << Addr2X(move.to);
	str << Addr2Y(move.to);

	if( move.koma & SENTE )
		str << s_koma[move.koma-SENTE-1];
	else if( move.koma & GOTE )
		str << s_koma[move.koma-GOTE-1];
	else
		str << s_koma[move.koma-1];

	if( move.from & DAI ){
		// 持ち駒を打つ場合
		str << "(00";
	}
	else{
		// 盤上の駒を動かす場合
		if( move.nari )
			str << 'n';

		str << '(';
		str << Addr2X(move.from);
		str << Addr2Y(move.from);
	}
	str << ") ";

	if( eol ){
		str << '\n';
	}

	return str.str();
}

string Shogi::GetStrMove( bool eol ){
/************************************************
棋譜の指し手を文字列に変換。
************************************************/
	return GetStrMove( know, eol );
}

string Shogi::GetStrMove( int num, bool eol ){
/************************************************
棋譜の指し手を文字列に変換。(手数指定)
************************************************/
	ostringstream str;

	if( num <= knum || 0 < num ){
		num--;
		str << GetStrMove( kifu[num].move, eol );
	}

	return str.str();
}

void Shogi::PrintMove( const MOVE& move, bool eol ){
/************************************************
指し手の表示。
************************************************/
	cout << GetStrMove( move, eol );
}

int Shogi::PrintMove( bool eol ){
/************************************************
棋譜の指し手の表示。
************************************************/
	return PrintMove( know, eol );
}

int Shogi::PrintMove( int num, bool eol ){
/************************************************
棋譜の指し手の表示。(手数指定)
************************************************/
	if( num > knum || 0 >= num )
		return 0;

	num--;
	PrintMove( kifu[num].move, eol );

	return 1;
}

string Shogi::GetStrBan(
#if POSIX
	int current
#endif
){
/************************************************
局面を文字列として取得。
************************************************/
	KOMA koma;
	ostringstream str;
	int i, j;

	// 持ち駒の表示
	str << "GOTE (";
	for( koma = GHI ; koma >= GFU ; koma-- ){
		if( dai[koma] > 0 ){
			str << ' ';
			str << s_koma[koma-GOTE-1];
			if( dai[koma] > 1 )
				str << dai[koma];
		}
	}
	str << " )";
	str << '\n';

#if 0
	// 歩フラグのデバッグ用
	for( i = 9 ; i >= 1 ; i-- ){
		if( kifu[know].gfu & ( 1 << i ) )
			str << " + ";
		else
			str << "   ";
	}
	str << '\n';
#endif

	// 盤面の表示
	str << " 9  8  7  6  5  4  3  2  1 \n";
	for( j = 1 ; j <= 9 ; j++ ){
		for( i = 9 ; i >= 1 ; i-- ){
			koma = ban[BanAddr(i,j)];
#if POSIX
			if( current && current == BanAddr(i,j) ){
				str << "\x1b[31m";
			}
#endif
			if( koma & SENTE ){
				str << ' ';
				str << s_koma[koma-SENTE-1];
			}
			else if( koma & GOTE ){
				str << 'v';
				str << s_koma[koma-GOTE-1];
			}
			else
				str << " * ";
#if POSIX
			if( current ){ str << "\x1b[39m"; }
#endif
		}
		str << ' ';
		str << j;

#if 0
		// 利き情報のデバッグ用
		str << ' ';
		for( i = 9 ; i >= 1 ; i-- ){
			str.width(3);
			str << CountEffect( effectS[BanAddr(i,j)] );
		}
		str << ' ';
		for( i = 9 ; i >= 1 ; i-- ){
			str.width(3);
			str << CountEffect( effectG[BanAddr(i,j)] );
		}
#endif

#if 0
		// bitboardのデバッグ用
		str << ' ';
		for( i = 9 ; i >= 1 ; i-- ){
			str.width(3);
			str << bb[SKA].getBit(_addr[BanAddr(i,j)]);
		}
		str << ' ';
		for( i = 9 ; i >= 1 ; i-- ){
			str.width(3);
			str << bb[GKA].getBit(_addr[BanAddr(i,j)]);
		}
#endif

		str << '\n';
	}

#if 0
	// 歩フラグのデバッグ用
	for( i = 9 ; i >= 1 ; i-- ){
		if( kifu[know].sfu & ( 1 << i ) )
			str << " + ";
		else
			str << "   ";
	}
	str << '\n';
#endif

	// 持ち駒の表示
	str << "SENTE(";
	for( koma = SHI ; koma >= SFU ; koma-- ){
		if( dai[koma] > 0 ){
			str << ' ';
			str << s_koma[koma-SENTE-1];
			if( dai[koma] > 1 )
				str << dai[koma];
		}
	}
	str << " )";
	str << '\n';

	return str.str();
}

void Shogi::PrintBan(
#if POSIX
	 bool useColor
#endif
){
/************************************************
局面の表示。
************************************************/
	cout << "------------------------------" << '\n';
#if POSIX
	if( useColor && know > 0 ){
		cout << GetStrBan( kifu[know-1].move.to );
	}
	else
#endif
	{
		cout << GetStrBan();
	}
	cout << "------------------------------" << '\n';
}

#ifndef NDEBUG
void Shogi::DebugPrintBitboard( uint96 _bb ){
	int i, j;

	for( j = 1 ; j <= 9 ; j++ ){
		for( i = 9 ; i >= 1 ; i-- ){
			cout.width(3);
			cout << _bb.getBit(_addr[BanAddr(i,j)]);
		}
		cout << '\n';
	}
	cout << '\n';
}

void Shogi::DebugPrintBitboard( KOMA koma ){
	if( koma & SENTE ){
		cout << "SENTE " << s_koma[koma-SENTE-1];
	}
	else{
		cout << "GOTE " << s_koma[koma-GOTE-1];
	}
	cout << '\n';

	DebugPrintBitboard( bb[koma] );
}
#endif

string Shogi::GetStrMoveCSA( const MOVE& move, KOMA sengo, bool eol ){
/************************************************
指し手を文字列に変換。
************************************************/
	KOMA koma;
	ostringstream str;

	if( sengo == SENTE )
		str << '+';
	else
		str << '-';

	if( move.from & PASS ){
		str << "0000EM";

		return str.str();
	}
	else if( move.from & DAI ){
		// 持ち駒を打つ場合
		str << "00";
	}
	else{
		// 盤上の駒を動かす場合
		str << Addr2X(move.from);
		str << Addr2Y(move.from);
	}

	str << Addr2X(move.to);
	str << Addr2Y(move.to);

	if( move.koma & SENTE )
		koma = move.koma-SENTE-1;
	else if( move.koma & GOTE )
		koma = move.koma-GOTE-1;
	else
		koma = move.koma-1;

	if( move.nari )
		koma += NARI;

	str << s_koma[koma];

	if( eol ){
		str << '\n';
	}

	return str.str();
}

string Shogi::GetStrMoveCSA( bool eol ){
/************************************************
棋譜の指し手を文字列に変換。
************************************************/
	return GetStrMoveCSA( know, eol );
}

string Shogi::GetStrMoveCSA( int num, bool eol ){
/************************************************
棋譜の指し手を文字列に変換。(手数指定)
************************************************/
	ostringstream str;

	if( num <= knum || 0 < num ){
		num--;
		str << GetStrMoveCSA( kifu[num].move, GetSengo( num ), eol );
	}

	return str.str();
}

void Shogi::PrintMoveCSA( const MOVE& move, KOMA sengo, bool eol ){
/************************************************
指し手の表示。
************************************************/
	cout << GetStrMoveCSA( move, sengo, eol );
}

int Shogi::PrintMoveCSA( bool eol ){
/************************************************
指し手の表示。(CSA形式)
************************************************/
	return PrintMoveCSA( know, eol );
}

int Shogi::PrintMoveCSA( int num, bool eol ){
/************************************************
指し手の表示。(手数指定)(CSA形式)
************************************************/
	if( num > knum || 0 >= num )
		return 0;

	num--;
	PrintMoveCSA( kifu[num].move, GetSengo( num ), eol );

	return 1;
}

string Shogi::GetStrBanCSA(
#if POSIX
	int current
#endif
){
/************************************************
局面を文字列として取得。(CSA形式)
************************************************/
	KOMA koma;
	ostringstream str;
	int i, j;

	// 盤面の表示
	for( j = 1 ; j <= 9 ; j++ ){
		str << 'P';
		str << j;
		for( i = 9 ; i >= 1 ; i-- ){
			koma = ban[BanAddr(i,j)];
#if POSIX
			if( current && current == BanAddr(i,j) ){
				str << "\x1b[31m";
			}
#endif
			if( koma & SENTE ){
				str << '+';
				str << s_koma[koma-SENTE-1];
			}
			else if( koma & GOTE ){
				str << '-';
				str << s_koma[koma-GOTE-1];
			}
			else
				str << " * ";
#if POSIX
			if( current ){ str << "\x1b[39m"; }
#endif
		}
		str << '\n';
	}

	// 持ち駒の表示
	str << "P+";
	for( koma = SHI ; koma >= SFU ; koma-- ){
		for( i = 0 ; i < dai[koma] ; i++ ){
			str << "00";
			str << s_koma[koma-SENTE-1];
		}
	}
	str << '\n';

	str << "P-";
	for( koma = GHI ; koma >= GFU ; koma-- ){
		for( i = 0 ; i < dai[koma] ; i++ ){
			str << "00";
			str << s_koma[koma-GOTE-1];
		}
	}
	str << '\n';

	// 手番の表示
	if( ksengo == SENTE )
		str << '+';
	else
		str << '-';
	str << '\n';

	return str.str();
}

void Shogi::PrintBanCSA(
#if POSIX
	 bool useColor
#endif
){
/************************************************
局面の表示。(CSA形式)
************************************************/
#if POSIX
	if( useColor && know > 0 ){
		cout << GetStrBanCSA( kifu[know-1].move.to );
	}
	else
#endif
	{
		cout << GetStrBanCSA();
	}
}

string Shogi::GetStrKifuCSA( KIFU_INFO* kinfo, AvailableTime* atime ){
/************************************************
棋譜を取得。(CSA形式)
************************************************/
	int know0 = know;
	ostringstream str;

	// 棋譜情報
	if( kinfo != NULL ){
		char tstr[32];
		str << "N+"           << kinfo->sente << '\n';
		str << "N-"           << kinfo->gote  << '\n';
		str << "$EVENT:"      << kinfo->event << '\n';
		sprintf( tstr, "%04d/%02d/%02d %02d:%02d:%02d",
			kinfo->start.year, kinfo->start.month, kinfo->start.day,
			kinfo->start.hour, kinfo->start.min  , kinfo->start.sec );
		str << "$START_TIME:" << tstr << '\n';
		sprintf( tstr, "%04d/%02d/%02d %02d:%02d:%02d",
			kinfo->end.year, kinfo->end.month, kinfo->end.day,
			kinfo->end.hour, kinfo->end.min  , kinfo->end.sec );
		str << "$END_TIME:"   << tstr << '\n';
	}

	// 開始局面へ移動
	while( GoBack() )
		;

	// 開始局面
	str << GetStrBanCSA();

	// 指し手
	while( GoNext() ){
		str << GetStrMoveCSA( true );
		if( atime != NULL ){
			str << 'T' << (unsigned)atime->GetUsedTime( know - 1 ) << '\n';
		}
	}

	// 元居た位置に戻る。
	while( know > know0 && GoBack() )
		;

	return str.str();
}

int Shogi::PrintKifuCSA( KIFU_INFO* kinfo, AvailableTime* atime ){
/************************************************
棋譜の表示。(CSA形式)
************************************************/
	cout << GetStrKifuCSA( kinfo, atime );
	return knum;
}

string Shogi::GetStrMoveNum( const MOVE& move, bool eol, char delimiter ){
/************************************************
指し手を文字列に変換。(数字形式)
************************************************/
	ostringstream str;

	if( move.from & PASS ){
		str << "00" << delimiter << "00" << delimiter << "0";

		return str.str();
	}

	// 移動元
	str << Addr2X(move.to);
	str << Addr2Y(move.to);
	str << delimiter;

	// 移動先, 成/不成
	if( move.from & DAI ){
		// 持ち駒を打つ場合
		str << move.from << delimiter << "0";
	}
	else{
		// 盤上の駒を動かす場合
		str << Addr2X(move.from);
		str << Addr2Y(move.from);
		str << delimiter;
		if( move.nari ){ str << "1"; }
		else           { str << "0"; }
	}

	if( eol ){ str << '\n'; }

	return str.str();
}

string Shogi::GetStrBanNum( char delimiter ){
/************************************************
局面を文字列として取得。(数字形式)
************************************************/
	KOMA koma;
	ostringstream str;
	int i, j;

	// 盤面
	str << "board\n";
	for( j = 1 ; j <= 9 ; j++ ){
		for( i = 9 ; i >= 1 ; i-- ){
			str << ban[BanAddr(i,j)] << delimiter;
		}
		str << '\n';
	}

	// 持ち駒の表示
	for( koma = SFU ; koma <= SHI ; koma++ ){
		str << dai[koma] << delimiter;
	}
	str << '\n';

	for( koma = GFU ; koma <= GHI ; koma++ ){
		str << dai[koma] << delimiter;
	}
	str << '\n';

	return str.str();
}

KOMA Shogi::GetKomaCSA( const char* str ){
/************************************************
2文字から駒の種類を取得。(CSA形式)
************************************************/
	KOMA koma = EMPTY;
	int i;

	for( i = 0 ; i < 15 ; i++ ){
		if( str[0] == s_koma[i][0] && str[1] == s_koma[i][1] ){
			koma = FU + i;
			break;
		}
	}

	return koma;
}

void Shogi::RemoveKomaCSA( const char* str ){
/************************************************
駒を落とす。(CSA形式)
************************************************/
	const char* e;
	int i, j;

	e = str + strlen( str );

	while( str < e ){
		i = str[0] - '0';
		j = str[1] - '0';

		if( i >= 1 && i <= 9 && j >= 1 && j <= 9 ){
			// 指定された位置の駒を落とす。
			ban[BanAddr(i,j)] = EMPTY;
		}

		str += 4;
	}
}

void Shogi::SetKomaAllCSA( KOMA sengo ){
/************************************************
残りの駒全てを駒台に配置。(CSA形式)
************************************************/
	int cnt[HI] = { 18, 4, 4, 4, 4, 2, 2 };
	KOMA koma;
	int i, j;

	// 盤上の駒の数をカウント
	for( i = 1 ; i <= 9 ; i++ ){
		for( j = 1 ; j <= 9 ; j++ ){
			// 0x07 が歩から飛車を取り出すマスク
			koma = ban[BanAddr(i,j)] & 0x07;
			if( koma != EMPTY ){
				cnt[koma-FU]--;
			}
		}
	}

	// 持駒の数をカウント
	for( koma = FU ; koma <= HI ; koma++ ){
		cnt[koma-FU] -= dai[koma+SENTE];
		cnt[koma-FU] -= dai[koma+GOTE];

		if( cnt[koma-FU] > 0 ){
			// 残りの全ての駒を駒台へ
			dai[koma+sengo] += cnt[koma-FU];
		}
	}
}

void Shogi::SetKomaCSA( const char* str, KOMA sengo ){
/************************************************
文字列から駒を配置。(CSA形式)
************************************************/
	KOMA koma;
	const char* e;
	int i, j;

	e = str + strlen( str );

	while( str < e ){
		i = str[0] - '0';
		j = str[1] - '0';

		if( i == 0 && j == 0 ){
			// 持駒
			if( str[2] == 'A' && str[3] == 'L' ){
				// 残りの駒全て
				SetKomaAllCSA( sengo );
				return;
			}
			koma = GetKomaCSA( str + 2 );
			if( koma >= FU && koma <= HI ){
				dai[koma+sengo]++;
			}
		}
		else if( i >= 1 && i <= 9 && j >= 1 && j <= 9 ){
			// 盤上
			koma = GetKomaCSA( str + 2 );
			if( koma != EMPTY ){
				ban[BanAddr(i,j)] = koma + sengo;
			}
		}

		str += 4;
	}
}

int Shogi::InputMoveCSA( const char* str, KOMA sengo ){
/************************************************
文字列から指し手を読み込む。(CSA形式)
************************************************/
	MOVE move;
	int fx, fy, tx, ty;
	KOMA koma;

	if( strlen( str ) < 6 )
		return 0;

	fx = str[0] - '0';
	fy = str[1] - '0';
	tx = str[2] - '0';
	ty = str[3] - '0';
	koma = GetKomaCSA( str + 4 );

	if( tx < 1 || tx > 9 || ty < 1 || ty > 9 )
		return 0;

	move.to = BanAddr(tx,ty);

	if( fx == 0 && fy == 0 && koma >= FU && koma <= HI ){
		// 持ち駒を打つ場合
		move.from = koma | sengo | DAI;
		move.nari = 0;
	}
	else{
		if( fx < 1 || fx > 9 || fy < 1 || fy > 9 )
			return 0;

		// 盤上の駒を動かす場合
		move.from = BanAddr(fx,fy);

		if( !( ban[move.from] & NARI ) && koma & NARI )
			move.nari = 1;
		else
			move.nari = 0;
	}

	return Move( move );
}

int Shogi::InputLineCSA( const char* line, KIFU_INFO* kinfo, AvailableTime* atime ){
/************************************************
棋譜の1行読み込み。(CSA形式)
************************************************/
	KOMA koma;
	int addr;
	int i, j;
	const char* p;

	while( 1 ){
		// コメント行
		if( line[0] == '\'' ){
			return 1;
		}
		// 局面の設定
		else if( line[0] == 'P' ){
			// 先手の駒台
			if( line[1] == '+' ){
				SetKomaCSA( line + 2, SENTE );
			}
			// 後手の駒台
			else if( line[1] == '-' ){
				SetKomaCSA( line + 2, GOTE );
			}
			// 平手初期配置
			else if( line[1] == 'I' ){
				SetBan( HIRATE );

				// 駒落ち
				RemoveKomaCSA( &line[2] );
			}
			// 盤面
			else{
				if( strlen( line ) < 29 ){ return 0; }

				j = line[1] - '0';
				if( j < 1 || j > 9 ){ return 0; }

				for( i = 0 ; i < 9 ; i++ ){
					addr = BanAddr(9-i,j);

					p = line + 2 + i * 3;
					koma = GetKomaCSA( p + 1 );
					if     ( p[0] == '+' ){ koma |= SENTE; }
					else if( p[0] == '-' ){ koma |= GOTE; }
					ban[addr] = koma;

					if     ( koma == SOU ){ sou = addr; }
					else if( sou == addr ){ sou = 0; }

					if     ( koma == GOU ){ gou = addr; }
					else if( gou == addr ){ gou = 0; }
				}
			}

			UpdateBan();
		}
		else if( line[0] == '+' ){
			// 先手番
			if( line[1] == '\0' ){
				ksengo = SENTE;
				UpdateBan();
			}
			// 先手の指し手
			else{
				if( 0 == InputMoveCSA( line + 1, SENTE ) )
					return 0;
			}
		}
		else if( line[0] == '-' ){
			// 後手番
			if( line[1] == '\0' ){
				ksengo = GOTE;
				UpdateBan();
			}
			// 後手の指し手
			else{
				if( 0 == InputMoveCSA( line + 1, GOTE ) )
					return 0;
			}
		}
		// 棋譜情報
		else if( line[0] == '$' ){
			// 開始日時
			if     ( String::WildCard( line, "$START_TIME:*" ) ){
				if( kinfo != NULL ){
					char tstr[11];
					strncpy( tstr, String::GetSecondToken( line, ':' ), 10 );
					kinfo->start.year  = strtol( &tstr[0], NULL, 10 );
					kinfo->start.month = strtol( &tstr[5], NULL, 10 );
					kinfo->start.day   = strtol( &tstr[8], NULL, 10 );
				}
			}
			// 終了日時
			else if( String::WildCard( line, "$END_TIME:*" ) ){
				if( kinfo != NULL ){
					char tstr[11];
					strncpy( tstr, String::GetSecondToken( line, ':' ), 10 );
					tstr[10] = '\0';
					kinfo->end.year  = strtol( &tstr[0], NULL, 10 );
					kinfo->end.month = strtol( &tstr[5], NULL, 10 );
					kinfo->end.day   = strtol( &tstr[8], NULL, 10 );
				}
			}
			// 棋戦名
			else if( String::WildCard( line, "$EVENT:*" ) ){
				if( kinfo != NULL ){
					kinfo->event = String::GetSecondToken( line, ':' );
				}
			}
			// 持ち時間
			else if( String::WildCard( line, "$TIME_LIMIT:*" ) ){
				if( atime != NULL ){
					double sec_a, sec_r;
					char tstr[9];
					strncpy( tstr, String::GetSecondToken( line, ':' ), 8 );
					tstr[8] = '\0';
					sec_a  = strtol( &tstr[0], NULL, 10 ) * 60 * 60;
					sec_a += strtol( &tstr[3], NULL, 10 ) * 60;
					sec_r  = strtol( &tstr[6], NULL, 10 );
					atime->Reset( sec_a, sec_r );
				}
			}
		}
		// 対局者名
		else if( line[0] == 'N' ){
			// 先手の対局者名
			if     ( String::WildCard( line, "N+" ) ){
				if( kinfo != NULL ){ kinfo->sente = (&line[2]); }
			}
			// 後手の対局者名
			else if( String::WildCard( line, "N-" ) ){
				if( kinfo != NULL ){ kinfo->gote  = (&line[2]); }
			}
		}
		// 消費時間
		else if( line[0] == 'T' ){
			if( atime != NULL ){
				atime->Reduce( strtol( &line[1], NULL, 10 ) );
			}
		}

		// マルチステートメント
		if( NULL == ( p = strchr( line, ',' ) ) ){
			break;
		}

		line = p + 1;
	}

	return 1;
}

int Shogi::InputFileCSA( const char* fname, KIFU_INFO* kinfo, AvailableTime* atime ){
/************************************************
棋譜の読み込み。(CSA形式)
************************************************/
	ifstream fin;
	char line[1024];
	int l;

	// ファイルを開く。
	fin.open( fname );

	// ファイルがあるか。
	if( fin.fail() ){
		cerr << "Fail!..[" << fname << ']' << '\n';
		return 0;
	}

	if( kinfo != NULL ){
		kinfo->init();
	}

	if( atime != NULL ){
		atime->Reset( 0.0, 0.0 );
	}

	l = 1;
	SetBanEmpty();
	while( 1 ){
		fin.getline( line, sizeof(line) );
		if( fin.fail() || fin.eof() )
			break;
		if( !InputLineCSA( line, kinfo, atime ) ){
			cerr << "Read Error!" << '\n';
			cerr << "Line " << l << ':' << line << '\n';
			break;
		}
		l++;
	}

	if( fin.fail() )
		fin.clear();

	// ファイルを閉じる。
	fin.close();

	assert( sou == 0 || ban[sou] == SOU );
	assert( gou == 0 || ban[gou] == GOU );

	return 1;
}

int Shogi::OutputFileCSA( const char* fname, KIFU_INFO* kinfo, AvailableTime* atime ){
/************************************************
棋譜の書き込み。(CSA形式)
************************************************/
	ofstream fout;

	// ファイルを開く。
	fout.open( fname );

	// ファイルを開けたか。
	if( fout.fail() ){
		cerr << "Fail!..[" << fname << ']' << '\n';
		return 0;
	}

	fout << GetStrKifuCSA( kinfo, atime );

	if( fout.fail() )
		fout.clear();

	// ファイルを閉じる。
	fout.close();

	return 1;
}
