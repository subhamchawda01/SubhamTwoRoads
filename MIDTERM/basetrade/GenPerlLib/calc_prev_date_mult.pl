# \file GenPerlLib/calc_prev_date_mult.pl
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

sub CalcPrevDateMult {
    my $input_date = shift;
    my $num_times = shift;

    my $retval = `$BINDIR/calc_prev_day $input_date $num_times`; # no chomp required
    $retval;
}

1;
