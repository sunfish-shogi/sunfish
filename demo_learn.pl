#!/usr/bin/perl -w

use Tk;
use encoding 'utf-8';

# auto flush
#$| = 1;

# データと初期化
$kifu_length = 0;
@move_num    = ();
@move_val    = ([]);
@best_move   = ();
@fail_high   = ();
@fail_low    = ();

$searchWindow = 256;

# 更新を知らせるシグナル
local $SIG{ 'USR1' } = sub{
	&load_detail();
	&draw_canvas();
	&sig_usr1();
};

# PIDの一覧
@pid_list = ();

# シグナルの送信
sub sig_usr1{
	foreach my $pid ( @pid_list ){
		kill( 'USR1', $pid );
	}
}

# 表示形式
$viewtype = 1;

# 図形サイズ
$def_oval_size   = 10;
$def_oval_size_s = 5;
$def_rect_size   = 10;

# キャンバスのサイズ
$width  = 500;
$height = 300;

# マウスカーソル
$mouse_kifu = -1;
$mouse_click = -1;

# 乱数の種 (テスト用)
#srand( time() );

# 起動時引数の処理
foreach $arg( @ARGV ){
	if( $arg =~ /\d+x\d+/ ){
		print $arg. "\n";
		( $width, $height ) = split( 'x', $arg );
		print $width . "\n";
		print $height . "\n";
	}
}

# ウィンドウ生成
$top = MainWindow->new();

# キャンバスの作成
$canvas = $top->Canvas( width => "$width", height => "$height" )
	->pack( -anchor => "nw", -side => "bottom", -expand => 1 );

# マウス操作
$canvas->Tk::bind( '<Button>', [ \&mouse_button, Ev('x'), Ev('y') ] );
$canvas->Tk::bind( '<Motion>', [ \&mouse_motion, Ev('x'), Ev('y') ] );
$canvas->Tk::bind( '<Leave>' , [ \&mouse_leave ]  );

# キャンバスの初期化
&init_canvas();

# ボタンの配置
$top->Button( -text => 'Search', -command => [\&button_search] )
	->pack( -anchor => "nw", -side => "left");
$top->Button( -text => 'Adjust', -command => [\&button_adjust])
	->pack( -anchor => "nw", -side => "left" );
$top->Button( -text => 'Reload', -command => [\&button_reload])
	->pack( -anchor => "nw", -side => "left" );
$top->Button( -text => 'Clear', -command => [\&button_clear])
	->pack( -anchor => "nw", -side => "left" );
$top->Radiobutton( -text => 'Oder', -command => [\&change_viewtype],
	-variable => \$viewtype, -value => "0")
	->pack( -anchor => "nw", -side => "left" );
$top->Radiobutton( -text => 'Distribution', -command => [\&change_viewtype],
	-variable => \$viewtype, -value => "1")
	->pack( -anchor => "nw", -side => "left" );

# メインループ
MainLoop();

# キャンバスの初期化
sub init_canvas{
	&load_detail();
	&draw_canvas();
}

# キャンバスへの描画
sub draw_canvas{
	# テスト用乱数
	#my $j;
	#for( $j = 0 ; $j < $kifu_length ; $j++ ){
	#	$best_move[$j] = int( rand() * $move_num[$j] );
	#	$fail_high[$j] = int( rand() * $move_num[$j] );
	#	$fail_low [$j] = int( rand() * $move_num[$j] );
	#}

	# 一旦全部消す。
	&delete_canvas();

	if( $kifu_length == 0 ){ return; }

	# common
	my $i;
	my $j;
	my $space = $width / $kifu_length;
	my $space2 = int( $space / 2 );

	# マウスでポイントしている局面
	if( $mouse_kifu != -1 ){
		my $left = $space * $mouse_kifu;
		my $right = $left + $space;
		$id = $canvas->create( 'rectangle', $left, 0, $right, $height );
		$canvas->itemconfigure( $id, -fill => '#ff0000', -width => '0' );
	}

	# 最後にクリックされた局面
	if( $mouse_click != -1 ){
		my $left = $space * $mouse_click;
		my $right = $left + $space;
		$id = $canvas->create( 'rectangle', $left, 0, $right, $height );
		$canvas->itemconfigure( $id, -fill => '#ffaaaa', -width => '0' );
	}

	if( $viewtype == 0 ){
		# oder view
		my $oval_size     = $def_oval_size;
		my $oval_size_min = int(($space2+1)*2/3);
		my $rect_size     = $def_rect_size;
		my $rect_size_min = int($space2/2)+1;

		# サイズ調整
		if( $oval_size > $oval_size_min ){ $oval_size = $oval_size_min; }
		if( $rect_size > $rect_size_min ){ $rect_size = $rect_size_min; }

		for( $i = 0 ; $i < $kifu_length ; $i++ ){
			my $left = $space * $i;
			my $right = $left + $space;
			my $center = $left + $space2;
			my $h;
			my $id;

			# 縦棒
			$canvas->create( 'line', $center, 0, $center, $height );

			# fail-high
			$h = &scale( $fail_high[$i], $move_num[$i], $height );
			$id = $canvas->create( 'rectangle', $center-$rect_size, 0, $center+$rect_size, $h );
			$canvas->itemconfigure( $id, -fill => '#00ffaa', -width => '1' );

			# fail-low
			$h = &scale( $move_num[$i]-$fail_low[$i], $move_num[$i], $height );
			$id = $canvas->create( 'rectangle', $center-$rect_size, $h, $center+$rect_size, $height );
			$canvas->itemconfigure( $id, -fill => '#00aaff', -width => '1' );

			# 棋譜の指し手
			$h = &scale( $best_move[$i], $move_num[$i], $height );
			$id = $canvas->create( 'oval', $center-$oval_size, $h-$oval_size, $center+$oval_size, $h+$oval_size );
			$canvas->itemconfigure( $id, -fill => '#ff0000', -width => '1' );
		}
	}
	elsif( $viewtype == 1 ){
		# oder view
		my $oval_size     = $def_oval_size_s;
		my $oval_size_min = int( $space2 / 3 ) + 1;
		my $rect_size     = $def_rect_size;
		my $rect_size_min = int($space2/2)+1;

		# サイズ調整
		if( $oval_size > $oval_size_min ){ $oval_size = $oval_size_min; }
		if( $rect_size > $rect_size_min ){ $rect_size = $rect_size_min; }

		for( $i = 0 ; $i < $kifu_length ; $i++ ){
			my $left = $space * $i;
			my $right = $left + $space;
			my $center = $left + $space2;
			my $middle = $height / 2;
			my $move_val_num = @{$move_val[$i]};
			my $h;
			my $l;
			my $id;

			# 縦棒
			#$canvas->create( 'line', $center, 0, $center, $height );

			# 横棒
			$id = $canvas->create( 'line', 0, $middle, $width, $middle );
			$canvas->itemconfigure( $id, -fill => 'red' );

			# fail-high
			$h = &scale( $fail_high[$i], $move_num[$i], $height );
			$id = $canvas->create( 'rectangle', $center-$rect_size, 0, $center+$rect_size, $h );
			$canvas->itemconfigure( $id, -fill => '#00ffaa', -width => '1' );

			# fail-low
			$h = &scale( $move_num[$i]-$fail_low[$i], $move_num[$i], $height );
			$id = $canvas->create( 'rectangle', $center-$rect_size, $h, $center+$rect_size, $height );
			$canvas->itemconfigure( $id, -fill => '#00aaff', -width => '1' );

			# 評価値の分布
			for( $j = 0 ; $j < $move_val_num ; $j++ ){
				$h = $middle - &scale( $move_val[$i][$j], $searchWindow*2, $height );
				$l = &scale( $j - $move_val_num/2, $move_val_num, $space2 );
				$id = $canvas->create( 'oval', $l+$center-$oval_size, $h-$oval_size, $l+$center+$oval_size, $h+$oval_size );
				#$canvas->itemconfigure( $id, -outline => 'orange', -width => '1' );
			}
		}
	}
}

# スケールの変換
sub scale{
	my ( $value, $num, $size ) = @_;

	if( $num == 0 ){
		return 0;
	}
	else{
		return int( $value * $size / $num );
	}
}

# 図形の削除
sub delete_canvas{
	my @id_list = $canvas->find( "all" );

	foreach my $id ( @id_list ){
		$canvas->delete( $id );
	}
}

# 学習の初期化
sub init_learning{
	&load_detail();
	&draw_canvas();
}

# detailファイルのロード
sub load_detail{
	my $i;

	# 初期化
	$kifu_length = 0;
	@move_num = ();
	@move_val = ([]);
	@best_move = ();
	@fail_high = ();
	@fail_low = ();
	$mouse_kifu = -1;
	#$mouse_click = -1;

	open( FILE, "detail0" ) or return;

	while( $line = <FILE> ){
		chomp $line;

		# 盤面
		if( $line eq 'board' ){
			for( $i = 0 ; $i < 9 && <FILE> ; $i++ ){
			}
			<FILE>;
			<FILE>;

			$kifu_length++;
			unshift( @move_num , 0 );
			unshift( @best_move, 0 );
			unshift( @fail_high, 0 );
			unshift( @fail_low , 0 );
			unshift( @move_val , [] );
		}

		# fail-high, fail-lowの数
		elsif( $line =~/^fail-high/ ){
			(my $dummy,$fail_high[0]) = split( ',', $line );
			$move_num [0] += $fail_high[0];
			$best_move[0] += $fail_high[0];
		}
		elsif( $line =~/^fail-low/ ){
			(my $dummy,$fail_low[0]) = split( ',', $line );
			$move_num [0] += $fail_low[0];
		}

		# 指し手
		else{
			my @move = split( ',', $line );

			# 棋譜の指し手
			if( @move == 3 ){
				$move_num[0]++;
			}
			# その他の手
			elsif( @move == 4 ){
				unshift( @{$move_val[0]}, $move[3] );
				$move_num[0]++;

				# 棋譜の指し手より評価が高いか
				if( $move[3] >= 0 ){
					$best_move[0]++;
				}
			}
		}
	}

	#print "move_num \t" . $move_num [0] . "\n";
	#print "best_move\t" . $best_move[0] . "\n";
	#print "fail-high\t" . $fail_high[0] . "\n";
	#print "fail-low \t" . $fail_low [0] . "\n";

	close( FILE );
}

# "Search"ボタン
sub button_search{
	print "\'Search\' button was pushed.\n";
	print "./sunfish -lp\n";
	system( './sunfish', '-lp' );
	print "..done\n";
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

	&load_detail();
	&draw_canvas();
	&sig_usr1();
}

# "Reload"ボタン
sub button_reload{
	print "\'Reload\' button was pushed.\n";
	&load_detail();
	&draw_canvas();
}

# "Clear"ボタン
sub button_clear{
	print "\'Clear\' button was pushed.\n";
	print "rm evdata\n";
	system( 'rm', 'evdata' );
	print "rm detail0\n";
	system( 'rm', 'detail0' );

	&load_detail();
	&draw_canvas();
}

# "Oder", "Distribution"
sub change_viewtype{
	&draw_canvas();
}

# マウスクリック
sub mouse_button{
	my ( $self, $x, $y ) = @_;
	if( $kifu_length != 0 ){
		my $space = $width / $kifu_length;
		my $pid = fork;
		$mouse_click = $mouse_kifu = int( $x / $space );
		if( $pid == 0 ){
			my $index = $kifu_length - $mouse_click - 1;
			exec ( './demo_board.pl', "$index" );
		}
		else{
			push( @pid_list, $pid );
		}
	}
}

# マウス移動
sub mouse_motion{
	my ( $self, $x, $y ) = @_;

	if( $kifu_length != 0 ){
		my $space = $width / $kifu_length;
		$mouse_kifu = int( $x / $space );
		&draw_canvas();
	}
}

# マウス範囲外
sub mouse_leave{
	$mouse_kifu = -1;
	&draw_canvas();
}
