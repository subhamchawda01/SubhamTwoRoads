$direction=1;
if ( @ARGV == 1 )
{
	$indx=shift;
	if($indx<0){
		$indx = -($indx + 1) ;
		$direction=-1;
	}
	else{
		$indx-- ;
		$direction=1;
	}

}
else{
	print "usage: exec  <indx>\n -ve number for decreasing order sorting, positive for increasing order sorting. index are counted from 1\n";
	exit (0);
}

sub numerically { my @x=split(/\s+/,$a); my @y=split(/\s+/,$b); if( abs($x[$indx])>abs($y[$indx]) ) {$direction} elsif ( abs($x[$indx])==abs($y[$indx]) ) {0} else {-$direction} } 

my @arr=();
while(<>)
{
	push(@arr, $_);
}

my@sorted = sort numerically @arr;

print @sorted ;
