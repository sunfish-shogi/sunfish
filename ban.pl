#!/usr/bin/perl -w
# gdbでShogi::_banを表示した内容を置き換える。

@koma = ( ' FU', ' KY', ' KE', ' GI', ' KI', ' KA', ' HI', ' OU', ' TO', ' NY', ' NK', ' NG', '   ', ' UM', ' RY', '   ',
          'vFU', 'vKY', 'vKE', 'vGI', 'vKI', 'vKA', 'vHI', 'vOU', 'vTO', 'vNY', 'vNK', 'vNG', '   ', 'vUM', 'vRY' );

$br = 0;
$line = '';

while( <STDIN> ){
	chomp $_;
	@ban = split( ',', $_ );
	foreach (@ban){
		$knum = $_;
		if( $knum == 64 ){
			if( $br == 0 ){
				print "$line\n";
				$br = 1;
				$line = '';
			}
		}
		elsif( $knum == 0 ){
			$line = ' * '.$line;
			$br = 0;
		}
		else{
			$line = $koma[$knum-17].$line;
			$br = 0;
		}
	}
	print "$line\n";
}
