#!/usr/bin/perl -w

$BLACK = 0;
$WHITE = 1;

$turn = $BLACK;
$time_b = 0;
$time_w = 0;
while($line = <>){
	chomp $line;
	if( $line =~ /^T/ ){
		$time = substr( $line, 1 );
		if( $turn == $BLACK ){
			$time_b += $time;
			$turn = $WHITE;
		}
		else{
			$time_w += $time;
			$turn = $BLACK;
		}
	}
}
print "black : ".$time_b.", white : ".$time_w."\n";
