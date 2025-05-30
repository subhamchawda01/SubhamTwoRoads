# \file GenPerlLib/is_strat_dir_in_timeperiod.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes inputs like :
# @est_brk_points = (0000,0800,1600)
#!/usr/bin/perl

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $output_file = $HOME_DIR."/strats_count.txt";

my $STRATS_BASE_DIR=$HOME_DIR."/modelling/strats";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $GET_UTC_HHMM_STR_EXEC_ = "$BIN_DIR/get_utc_hhmm_str" ;
if ( -e "$LIVE_BIN_DIR/get_utc_hhmm_str" )
{
    $GET_UTC_HHMM_STR_EXEC_ = "$LIVE_BIN_DIR/get_utc_hhmm_str" ;
}

require "$GENPERLLIB_DIR/is_strat_dir_in_timeperiod.pl";
require "$GENPERLLIB_DIR/get_time_zone_from_short_code.pl";

#my @counter;
#my $flag;
#my @prod_list;
#if( @ARGV > 1 )
#{    
#    while ( @ARGV )
#    {
#	push ( @est_brk_points , shift );
#    }
#    my @time_periods;

#    for ( my $i = 1 ; $i < scalar(@est_brk_points) ; $i ++ )
#    {
#	push ( @time_periods, "EST_" . sprintf("%04d" , @est_brk_points[$i - 1]) . "-"  . sprintf("%04d",@est_brk_points[$i]) ) ;
#	push ( @counter, 0 );
#   }
    
#    if ( @est_brk_points[0] == 0000 ) 
#    {
#	push ( @time_periods, "EST_" . sprintf("%04d" , @est_brk_points[$i - 1]) . "-"  . sprintf("%04d",2400) );
#    }
#    else
#    {
#	push ( @time_periods, "EST_" . sprintf("%04d" , @est_brk_points[$i - 1]) . "-"  . sprintf("%04d",@est_brk_points[0]) );
#    }
#    push ( @counter, 0 );
    

    opendir( my $l0dir, $STRATS_BASE_DIR );
#   my @times;
    my $st;
    my $et;

    open OUTFILEHANDLE, ">$output_file";

    while ( my $prod = readdir $l0dir )
    {
	next if $prod eq '.' or $prod eq '..';

	#push ( @prod_list , $prod );
	print OUTFILEHANDLE $prod,"\n";
	
	my $timezone = get_utc_from_short_code_time_zone ( $prod );

	@tza = split ( '\+', $timezone );

	if(scalar(@tza) != 2)
	{
	    @tza = split ( '-', $timezone);
	    @tza[1] = -@tza[1];
	}

	#print @tza[0],@tza[1],"\t";
	   

	$prod = $STRATS_BASE_DIR . "/" . $prod . "/";
	opendir (my $l1dir, $prod );

	while ( my $dirname = readdir $l1dir )
	{
	    next if $dirname eq '.' or $dirname eq '..';  

	    @words = split( '-', $dirname );
	    if ( $#words == 1)
	    {
		$st = $words[0];
		$et = $words[1];

		#print "$GET_UTC_HHMM_STR_EXEC_ $st";

		$st = `$GET_UTC_HHMM_STR_EXEC_ $st`;
		$et = `$GET_UTC_HHMM_STR_EXEC_ $et`;

		$st = sprintf( "%04d", $st - @tza[1]*100 );
		$et = sprintf( "%04d", $et - @tza[1]*100 );

		print OUTFILEHANDLE "\t", @tza[0], $st, "-";
		print OUTFILEHANDLE @tza[0], $et, "\t";

		$dirname = $prod . $dirname;
		my @files = <$dirname/*>;
		print OUTFILEHANDLE scalar(@files), "\n";
		
                #for ( my $j = 0 ; $j < scalar (@time_periods); $j++)
		#{
		#if( IsStratDirInTimePeriod ($dirname, @time_periods[$j]) == 1 )
		#{
		#    @counter[$j] = @counter[$j] + 1;
		#}	    		
		
	    }
	}
	closedir $l1dir;
    }
    closedir $l0dir;
#    print @counter;
#    print @time_periods;
#}
    

