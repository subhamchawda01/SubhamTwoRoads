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

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate

sub CalcNextWorkingDateMult {
    my $input_date = shift;
    my $num_times = shift;

    my $retval = $input_date;
    while ( $num_times > 0 )
    {
	$retval = `$BINDIR/calc_next_day $retval`; # no chomp required
	while ( SkipWeirdDate ( $retval ) ||
		IsDateHoliday ( $retval ) )
	{
	    $retval = `$BINDIR/calc_next_day $retval`; # no chomp required
	}
	# maintains invariant that $retval is never a holiday
	$num_times --;
    }
    $retval;
}

1;
