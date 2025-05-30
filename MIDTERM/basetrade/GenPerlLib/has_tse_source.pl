# \file GenPerlLib/has_liffe_source.pl
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
use feature "switch";

sub HasTSESource
{
    my $shortcode_ = shift;
    my $retval = 0;

    given ( $shortcode_ )
    {
	when ( "JGB_0" ) { $retval = 1; }
	when ( "TPX_0" ) { $retval = 1; }
	when ( "MTP_0" ) { $retval = 1; }
	when ( "MJG_0" ) { $retval = 1; }
    }
    $retval;
}

1
