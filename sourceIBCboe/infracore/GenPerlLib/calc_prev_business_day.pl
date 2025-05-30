# \file GenPerlLib/calc_next_working_date_mult.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;
use POSIX qw(strftime);

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $PEXEC_DIR="/home/pengine/prod/live_execs";
my $PSCRIPT_DIR="/home/pengine/prod/live_scripts";

require "$PSCRIPT_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

sub CalcPrevBusinessDay {
    my $input_date = shift;

    my $retval = $input_date;
    my $run_ = 1;

    while ( $run_ )
    {
	$retval = `$PEXEC_DIR/calc_prev_day $retval`; # no chomp required

	my ( $year, $month, $day ) = BreakDateYYYYMMDD ( $retval );
	my $wday = strftime( "%u", 0, 0, 0, $day, $month -1, $year - 1900, -1, -1, -1 );

	if ( $wday == 6 || $wday == 7 )
	{
	    $run_ = 1 ;
	}
	else
	{
	    $run_ = 0 ;
	}
    }
    $retval;
}

1;
