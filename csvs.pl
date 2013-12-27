#!/usr/bin/perl -w

while( $line = <> ){
	chomp $line;
	@cells = split( ',', $line );
	for( $i = 0 ; $i < @cells ; $i++ ){
		$cell = $cells[$i];
		if( $i >= @sum ){
			$sum[$i] = 0;
			$name[$i] = "";
		}

		if( $cell =~ /^[+-]?\d+$/ ){
			$sum[$i] += $cell;
		}
		else{
			$sum[$i] += 0;
			if( $name[$i] eq "" ){
				$name[$i] = $cell;
			}
		}
	}
}

for( $i = 0 ; $i < @name ; $i++ ){
	if( $i >= @sum ){
		$space_n[$i] = 0;
	}
	else{
		$len = length($name[$i]) - length($sum[$i]);
		if( $len >= 0 ){
			$space_n[$i] = 0;
			$space_s[$i] = $len;
		}
		else{
			$space_n[$i] = $len;
			$space_s[$i] = 0;
		}
	}
}

for( $i = 0 ; $i < @name ; $i++ ){
	for($j=0;$j<$space_n[$i];$j++){print ' ';}
	print $name[$i] . ',';
}
printf "\n";
for( $i = 0 ; $i < @sum ; $i++ ){
	for($j=0;$j<$space_s[$i];$j++){print ' ';}
	print $sum[$i] . ',';
}
print "\n";
