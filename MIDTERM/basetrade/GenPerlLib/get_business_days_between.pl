# \file GenPerlLib/get_business_days_between.pl
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

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
if( -d $LIVE_BIN_DIR)   {$BINDIR=$LIVE_BIN_DIR;}
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday

sub GetBusinessDaysBetween {
    my $start_date = shift;
    my $end_date = shift;

    my $retval = 1;
    while ( $retval < 90 )
    {
	$end_date = `$BINDIR/calc_prev_day $end_date`; # no chomp required
	while ( IsDateHoliday ( $end_date ) )
	{
	    $end_date = `$BINDIR/calc_prev_day $end_date`; # no chomp required
	}
	# maintains invariant that $end_date is never a holiday
	if ( $end_date < $start_date )
	{
	    last;
	}
	$retval ++;
    }
    $retval;
}

1;
