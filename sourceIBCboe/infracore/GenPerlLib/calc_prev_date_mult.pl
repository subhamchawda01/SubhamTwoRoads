# \file GenPerlLib/calc_prev_date_mult.pl
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

sub CalcPrevDateMult {
    my $input_date = shift;
    my $num_times = shift;

    my $retval = `$BINDIR/calc_prev_day $input_date $num_times`; # no chomp required
    $retval;
}

1;
