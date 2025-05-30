# \file GenPerlLib/get_strat_type.pl
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

sub GetStratType
{
    my $retval = "PriceBasedAggressiveTrading";
    my ($stratfilename_) = @_;

    if ( -e $stratfilename_ )
    {

	open STRATFILEHANDLE, "< $stratfilename_ " or die "$0 Could not open $stratfilename_\n" ;
	my @slines_ = <STRATFILEHANDLE>;
	close STRATFILEHANDLE;

	foreach my $t_sline_ ( @slines_ )
	{
	    chomp ( $t_sline_ );
	    my @swords_ = split ( ' ', $t_sline_ );
	    if ( $#swords_ >= 2 )
	    {
		$retval = $swords_[2];
		last;
	    }
	}
    }
    $retval;
}

1
