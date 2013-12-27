#!/usr/bin/perl -w

use Tk;
use encoding 'utf-8';

# サイズ
$board_width  = 410;
$board_height = 454;
$hand_width   = 128;
$hand_height  = 192;
$hand_s_left  = $board_width;
$hand_g_left  = $board_width;
$hand_s_top   = $board_height - $hand_height;
$hand_g_top   = 0;
$width        = $board_width  + $hand_width;
$height       = $board_height;

$piece_width  = 43;
$piece_height = 48;

@hand_left    = (   0,  0, 64,  0, 64,  0, 64, );
@hand_top     = ( 144, 96, 96, 48, 48,  0,  0, );
@hand_space   = (   5,  7,  7,  7,  7, 21, 21, );

# 画像
@img_piece = ();
@img_piece_name = (
	"Pictures/Sfu.gif",
	"Pictures/Skyo.gif",
	"Pictures/Skei.gif",
	"Pictures/Sgin.gif",
	"Pictures/Skin.gif",
	"Pictures/Skaku.gif",
	"Pictures/Shi.gif",
	"Pictures/Sou.gif",
	"Pictures/Sto.gif",
	"Pictures/Snkyo.gif",
	"Pictures/Snkei.gif",
	"Pictures/Sngin.gif",
	"",
	"Pictures/Suma.gif",
	"Pictures/Sryu.gif",
	"",
	"Pictures/Gfu.gif",
	"Pictures/Gkyo.gif",
	"Pictures/Gkei.gif",
	"Pictures/Ggin.gif",
	"Pictures/Gkin.gif",
	"Pictures/Gkaku.gif",
	"Pictures/Ghi.gif",
	"Pictures/Gou.gif",
	"Pictures/Gto.gif",
	"Pictures/Gnkyo.gif",
	"Pictures/Gnkei.gif",
	"Pictures/Gngin.gif",
	"",
	"Pictures/Guma.gif",
	"Pictures/Gryu.gif",
	"",
);

#盤面
@board = (
	[34,35,36,37,40,37,36,35,34,],
	[ 0,39, 0, 0, 0, 0, 0,38, 0,],
	[33,33,33,33,33,33,33,33,33,],
	[ 0, 0, 0, 0, 0, 0, 0, 0, 0,],
	[ 0, 0, 0, 0, 0, 0, 0, 0, 0,],
	[ 0, 0, 0, 0, 0, 0, 0, 0, 0,],
	[17,17,17,17,17,17,17,17,17,],
	[ 0,22, 0, 0, 0, 0, 0,23, 0,],
	[18,19,20,21,24,21,20,19,18,],
);
@hand_s = ( 0, 0, 0, 0, 0, 0, 0 );
@hand_g = ( 0, 0, 0, 0, 0, 0, 0 );

# 指し手
@move_from = ();
@move_to   = ();
@move_pro  = ();
@move_val  = ();
$best_from = 0;
$best_to   = 0;
$best_pro  = 0;

# auto flush
#$| = 1;

# 更新を知らせるシグナル
local $SIG{ 'USR1' } = sub{
	&load_detail( $kifu_num );
	&draw_board();
};

# シグナルの送信
sub sig_usr1{
	kill( 'USR1', getppid() );
}

# 起動時引数の処理
$kifu_num = -1;
if( @ARGV >= 1 ){
	$kifu_num = $ARGV[0];
	&load_detail( $kifu_num );
}

# ウィンドウ生成
$top = MainWindow->new();

# 矢印の描画
$arrow = 1;

# 画面ロック
$lock = 0;

$canvas = $top->Canvas( width => $width, height => $height )
	->pack( -anchor => "nw", -side => "bottom", -expand => 1 );

$top->Button( -text => 'Adjust', -command => [\&button_adjust]
	)->pack( -anchor => "nw", -side => "left" );
$top->Button( -text => 'Reload', -command => [\&button_reload]
	)->pack( -anchor => "nw", -side => "left" );
$top->Checkbutton( -text => 'Lock', -command => [\&change_lock],
	-variable => \$lock, -onvalue => 1, -offvalue => 0 )
	->pack( -anchor => "nw", -side => "left" );
$top->Checkbutton( -text => 'Arrows', -command => [\&change_arrow],
	-variable => \$arrow, -onvalue => 1, -offvalue => 0 )
	->pack( -anchor => "nw", -side => "left" );

&load_images();
&draw_board();

MainLoop();

# イメージのロード
sub load_images{
	my $i;

	for( $i = 0 ; $i < @img_piece_name ; $i++ ){
		if( $img_piece_name[$i] ne "" ){
			$img_piece[$i] = $top->Photo( -file => $img_piece_name[$i] );
		}
	}

	$img_board = $top->Photo( -file => 'Pictures/ban_kaya_a.gif' );
	$img_lines = $top->Photo( -file => 'Pictures/masu_dot_xy.gif' );
}

# 盤面の描画
sub draw_board{
	my $i;
	my $j;

	# キャンバスの初期化
	&delete_canvas();

	# 木目とマス目
	$canvas->create( 'image', 0, 0, -image => $img_board, -anchor => 'nw' );
	$canvas->create( 'image', 0, 0, -image => $img_lines, -anchor => 'nw' );

	# 盤上の駒
	for( $j = 0 ; $j < 9 ; $j++ ){
		for( $i = 0 ; $i < 9 ; $i++ ){
			my $koma = $board[$j][$i];
			my ( $x, $y ) = &board_xy( $i, $j );

			if( $koma != 0 ){
				$canvas->create( 'image', $x, $y, -image => $img_piece[$koma-17], -anchor => 'nw' );
			}
		}
	}

	# 持ち駒
	for( $i = 0 ; $i < 7 ; $i++ ){
		my $koma;

		# 先手
		$koma = $i;
		for( $j = $hand_s[$i] - 1 ; $j >= 0 ; $j-- ){
			my ( $x, $y ) = &hand_s_xy( $i, $j );
			$canvas->create( 'image', $x, $y, -image => $img_piece[$koma], -anchor => 'nw' );
		}

		# 後手
		$koma = $i + 16;
		for( $j = $hand_g[$i] - 1 ; $j >= 0 ; $j-- ){
			my ( $x, $y ) = &hand_g_xy( $i, $j );
			$canvas->create( 'image', $x, $y, -image => $img_piece[$koma], -anchor => 'nw' );
		}
	}

	if( $arrow != 0 ){
		# 指し手の矢印
		for( $i = 0 ; $i < @move_from ; $i++ ){
			my ( $x0, $y0 ) = &move_xy( $move_from[$i] );
			my ( $x1, $y1 ) = &move_xy( $move_to  [$i] );
			my $w = int( &sigmoid( $move_val[$i] ) * 12 ) + 1;
			if( $move_pro[$i] == 0 ){
				&draw_arrow( $x0, $y0, $x1, $y1, $w, '#0000ff' );
			}
			else{
				&draw_arrow( $x0, $y0, $x1, $y1, $w, '#00aa00' );
			}
		}

		# 最善手の矢印
		if( $best_from != 0 ){
			my ( $x0, $y0 ) = &move_xy( $best_from );
			my ( $x1, $y1 ) = &move_xy( $best_to   );
			&draw_arrow( $x0, $y0, $x1, $y1, 7, '#ff0000' );
		}
	}
}

# sigmoid
sub sigmoid{
	my ( $x ) = @_;
	return 1.0 / ( 1.0 + exp( -$x * 7.0 / 256 ) );
}

# 矢印
sub draw_arrow{
	my ( $x0, $y0, $x1, $y1, $w, $color ) = @_;
	my ( $x2, $y2 );
	my ( $x3, $y3 );
	my ( $x4, $y4 );
	my ( $ex, $ey );
	my ( $hx, $hy );
	my $d;
	my $size_x = 15;
	my $size_y = 3 + $w;
	my $id;

	# 矢印方向の単位ベクトル( $ex, $ey )
	$ex  = $x1 - $x0;
	$ey  = $y1 - $y0;
	$d   = sqrt( $ex * $ex + $ey * $ey );
	$ex /= $d;
	$ey /= $d;

	# 主線
	$x2 = int( $x0 + ( $x1 - $x0 ) * ( $d - $size_x ) / $d );
	$y2 = int( $y0 + ( $y1 - $y0 ) * ( $d - $size_x ) / $d );
	$id = $canvas->create( 'line', $x0, $y0, $x2 + $ex, $y2 + $ey );
	$canvas->itemconfigure( $id, -fill => $color, -width => $w );

	# 法線
	( $hx, $hy ) = ( -$ey, $ex );
	( $x3, $y3 ) = ( $x2 + $hx * $size_y, $y2 + $hy * $size_y );
	( $x4, $y4 ) = ( $x2 - $hx * $size_y, $y2 - $hy * $size_y );

	$id = $canvas->create( 'polygon', $x1, $y1, $x3, $y3, $x4, $y4 );
	$canvas->itemconfigure( $id, -fill => $color );
}

# 譜号 => 座標
sub move_xy{
	my ( $move ) = @_;
	my ( $x, $y ) = ( 0, 0 );

	# 駒台
	if( $move & 0x0100 ){
		# 先手駒台
		if( $move & 0x0010 ){
			($x,$y) = &hand_s_xy( ($move&0x000f)-1, 0 );
		}
		# 後手駒台
		else{
			($x,$y) = &hand_g_xy( ($move&0x000f)-1, 0 );
		}
	}
	# 盤上
	else{
		($x,$y) = &board_xy( 9-int($move/10), $move%10-1 );
	}

	$x += int( $piece_width  / 2 );
	$y += int( $piece_height / 2 );

	return ( $x, $y );
}

# 盤上の座標計算
sub board_xy{
	my ( $i, $j ) = @_;
	my ( $x, $y );

	$x = $piece_width  * $i + 12;
	$y = $piece_height * $j + 11;

	return ( $x, $y );
}

# 駒台の座標計算
sub hand_s_xy{
	my ( $koma, $num ) = @_;
	my ( $x, $y );

	$x = $hand_s_left + $hand_left[$koma] + $num * $hand_space[$koma];
	$y = $hand_s_top  + $hand_top [$koma];

	return ( $x, $y );
}

sub hand_g_xy{
	my ( $koma, $num ) = @_;
	my ( $x, $y );

	$x = $hand_g_left + $hand_width  - $hand_left[$koma] - $piece_width - $num * $hand_space[$koma];
	$y = $hand_g_top  + $hand_height - $hand_top [$koma] - $piece_height;

	return ( $x, $y );
}

# キャンバスのクリア
sub delete_canvas{
	my @id_list = $canvas->find( "all" );

	foreach my $id ( @id_list ){
		$canvas->delete( $id );
	}
}

# ロード
sub load_detail{
	my ( $num ) = @_;
	my $c;

	# ロック
	if( $lock != 0 ){ return; }

	@move_from = ();
	@move_to   = ();
	@move_pro  = ();
	@move_val  = ();
	$best_from = 0;
	$best_to   = 0;
	$best_pro  = 0;

	open( FILE, "detail0" ) or return;

	$c = 0;
	while( $line = <FILE> ){
		chomp $line;
		if( $line eq 'board' ){
			if( $c == $num ){
				my $j;

				# 盤面の読み込み
				for( $j = 0 ; $j < 9 ; $j++ ){
					$line = <FILE>;
					@{$board[$j]} = split( ',', $line );
				}
				$line = <FILE>;
				@hand_s = split( ',', $line );
				$line = <FILE>;
				@hand_g = split( ',', $line );

				# 指し手の読み込み
				while( $line = <FILE> ){
					chomp $line;
					if( $line eq 'board' ){ last; }

					my @data = split( ',', $line );

					if( @data == 4 ){
						unshift( @move_from, 0 );
						unshift( @move_to  , 0 );
						unshift( @move_pro , 0 );
						unshift( @move_val , 0 );
						( $move_to[0], $move_from[0],
							$move_pro[0], $move_val[0] ) = @data;
					}
					elsif( @data == 3 ){
						( $best_to, $best_from, $best_pro ) = @data;
					}
				}
				last;
			}
			$c++;
		}
	}

	close( FILE );
}

# "Adjust"ボタン
sub button_adjust{
	print "\'Adjust\' button was pushed.\n";
	print "./sunfish -lg\n";
	system( './sunfish', '-lg' );
	print "..done\n";
	print "./sunfish -la\n";
	system( './sunfish', '-la' );
	print "..done\n";

	&load_detail( $kifu_num );
	&draw_board();
	&sig_usr1();
}

# "Reload"ボタン
sub button_reload{
	print "\'Reload\' button was pushed.\n";
	&load_detail( $kifu_num );
	&draw_board();
}

# "Lock"チェックボタン
sub change_lock{
}

# "Arrow"チェックボタン
sub change_arrow{
	&draw_board();
}
