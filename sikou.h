/* sikou.h
 * R.Kubo 2009-2010
 * 将棋の思考ルーチン
 */

#ifndef _SIKOU_
#define _SIKOU_

// フォントサイズ
#define FONT_SIZE			14

// スペース
#define SPACE				5

// ウィンドウスタイル
#define ST_STATIC			( WS_CHILD | WS_VISIBLE | ES_LEFT )
#define ST_EDIT				( WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_RIGHT )
#define ST_LEDIT			( WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY )
#define ST_CHECK			( WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX )
#define ST_EDIT_R			( ST_EDIT | ES_READONLY )
#define ST_LIST				( LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY )

// サイズ
#define STATIC_LEFT			SPACE
#define STATIC_TOP(n)		( EDIT_TOP(n) + 1 )
#define STATIC_HEIGHT		( FONT_SIZE + 2 )
#define STATIC_WIDTH		80

#define EDIT_LEFT			( SPACE * 2 + STATIC_WIDTH )
#define EDIT_TOP(n)			( SPACE + ( EDIT_HEIGHT + SPACE ) * (n) )
#define EDIT_HEIGHT			( FONT_SIZE + 4 )
#define EDIT_WIDTH			180
#define LEDIT_HEIGHT		( ( FONT_SIZE + 4 ) * 12 )
#define LEDIT_WIDTH			( STATIC_WIDTH + SPACE + EDIT_WIDTH )

#define CHECK_WIDTH			( ( STATIC_WIDTH + EDIT_WIDTH - SPACE ) / 2 )
#define CHECK_LEFT			( CHECK_WIDTH + SPACE * 2 )
//#define CHECK_LEFT2			( CHECK_LEFT + CHECK_WIDTH + SPACE )

struct OBJECT{
	HWND hWnd;
	char ClassName[32];
	DWORD style;
	int left;
	int top;
	int width;
	int height;
	int leftW;
	int topW;
	int widthW;
	int heightW;
	int change;
	char DefaultString[128];
};

// ウィンドウサイズ
#define WINDOW_SIZE_X		( EDIT_LEFT + EDIT_WIDTH + SPACE )
#define WINDOW_SIZE_Y(n)	( EDIT_TOP(n) + LEDIT_HEIGHT + SPACE )

// ワイド表示追加サイズ
#define WINDOW_WIDE_X		( WINDOW_SIZE_X )
#define STATIC_LEFT_W		( STATIC_LEFT + WINDOW_WIDE_X )
#define EDIT_LEFT_W			( EDIT_LEFT + WINDOW_WIDE_X )
#define CHECK_LEFT_W		( CHECK_LEFT + WINDOW_WIDE_X )
#define LEDIT_HEIGHT_W		( LEDIT_HEIGHT + ( EDIT_HEIGHT + SPACE ) * 5 )
#define LEDIT_WIDTH_W		( LEDIT_WIDTH + WINDOW_WIDE_X )

#define WM_MYCLOSE			WM_USER + 1

#endif
