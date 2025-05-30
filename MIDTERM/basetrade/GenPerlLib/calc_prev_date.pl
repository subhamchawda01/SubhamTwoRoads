# \file GenPerlLib/calc_prev_date.pl
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

sub CalcPrevDate {
    my @this_words_ = @_;
    my $retval = 20110101;
    if ( $#this_words_ >= 0 )
    {
	my $input_date = $this_words_[0];
	$retval = `$BINDIR/calc_prev_day $input_date`; # no chomp required
    }
    else
    {
	die "CalcPrevDate called with less than 1 arg: $#this_words_";
    }
    $retval;
}

1;
