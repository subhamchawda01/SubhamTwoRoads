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

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
if( -d $LIVE_BIN_DIR)   {$BINDIR=$LIVE_BIN_DIR;}
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate

sub CalcNextWorkingDateMult {
    my @this_words_ = @_;
    my $retval = 20110101;
    if ( $#this_words_ >= 1 )
    {
	my $input_date = $this_words_[0];
	my $num_times = $this_words_[1];
	$retval = $input_date;
	while ( $num_times > 0 )
	{
            my $arg_in_ = $retval;
	    $retval = `$BINDIR/calc_next_day $retval`; # no chomp required
            if(length($retval)<8) {die "calc_prev_day returned empty string from $arg_in_";}
	    while ( SkipWeirdDate ( $retval ) ||
		    IsDateHoliday ( $retval ) )
	    {
		$arg_in_ = $retval;
		$retval = `$BINDIR/calc_next_day $retval`; # no chomp required
		if(length($retval) < 8)	{die "calc_prev_day returned empty string from $arg_in_";}
	    }
	    # maintains invariant that $retval is never a holiday
	    $num_times --;
	}
    }
    if ( ! ( $retval ) )
    {
	$retval = 20110101;
    }
    $retval;
}

1;
