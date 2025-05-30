# \file GenPerlLib/calc_next_working_date_mult.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use POSIX qw(strftime);

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
if( -d $LIVE_BIN_DIR)	{$BINDIR=$LIVE_BIN_DIR;}
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD

sub CalcNextBusinessDay {
    my $input_date = shift;

    my $retval = $input_date;
    my $run_ = 1;

    while ( $run_ )
    {
	$retval = `$BINDIR/calc_next_day $retval`; # no chomp required

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
