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

sub CalNumberOfWorkingDaysBetweenDates {

    my $input_start_date = shift;
    my $input_end_date = shift;
    my $retval = $input_start_date ;

    my $num_times_ = 0 ;

    while ( $retval < $input_end_date )
    {
	$retval = `$BINDIR/calc_next_day $retval`; # no chomp required

	while ( IsDateHoliday ( $retval ) )  
	{
	    $retval = `$BINDIR/calc_next_day $retval`; # no chomp required
	}
	# maintains invariant that $retval is never a holiday

	$num_times_ ++ ;
    }

    $num_times_ ;
}

1;
