# \file GenPerlLib/get_pred_dur_strat.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
# This script takes inputs :
# STRATFILE
# and returns the pred duration of the strat

use strict;
use warnings;

sub GetPredDurStrat
{
    my $retval = 0;
    my ($stratfile_) = @_;

    my @swords_ = split ( '_', $stratfile_ );
    for ( my $i = 1; $i <= $#swords_ ; $i ++ )
    {
	if ( ( $swords_[$i] eq "na" ) ||
	     ( $swords_[$i] eq "ac" ) )
	{
	    $retval = $swords_[$i-1];
	    last;
	}
    }
    $retval;
}

1
