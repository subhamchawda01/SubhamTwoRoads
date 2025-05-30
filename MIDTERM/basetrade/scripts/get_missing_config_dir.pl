#! /usr/bin/perl


my @cfiles= `ls /home/dvctrader/modelling/indicatorwork/prod_configs/comb_config_*`;

@dlist_ = ( ) ;

chomp( @cfiles ) ;

for ( my $k = 0 ; $k < scalar ( @cfiles ) ; $k ++ )
{
    my $config = $ARGV[0] ;
    my @a = `grep SELF $cfiles[$k] | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
    my @b = `grep PREDDURATION $cfiles[$k] | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
    my @c = `grep FILTER $cfiles[$k] | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
    my @d = `grep TIMEPERIODSTRING $cfiles[$k] | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
    my @e = `grep DATAGEN_BASE_FUT_PAIR $cfiles[$k] | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
    my @f = `grep PREDALGO $cfiles[$k] | awk '{for ( i = 2 ; i <=NF ; i++ ) print \$i;}'`;
    
    chomp ( @a ) ;
    chomp ( @b ) ;
    chomp ( @c ) ;
    chomp ( @d ) ;
    chomp ( @e ) ;
    chomp ( @f ) ;
    

    
    for ( my $i = 0 ; $i < scalar ( @b ) ; $i++ )
    {
	for ( my $j = 0 ; $j < scalar ( @c ) ; $j++ )
	{
	    my $t =  $a[ 0 ]."_".$b[ $i ]."_".$c[ $j ]."_".$f[ 0 ]."_".$d[ 0 ]."_".$e[ 0 ]."_".$e[ 1 ] ;

	    
	    
	    if ( -e "/NAS1/indicatorwork/$t" )
	    {
#		push ( @dlist_, $t );
#		print $t, "\n";
	    }
	    else
	    {
		push ( @dlist_, $t );
		print $t, "\n";
		
	    }
	}
    }
}

