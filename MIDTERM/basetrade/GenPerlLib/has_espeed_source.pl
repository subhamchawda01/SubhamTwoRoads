# \file GenPerlLib/has_espeed_source.pl
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

sub HasESPEEDSource
{
    my $shortcode_ = shift;
    my $retval = 0;

    given ( $shortcode_ )
    {
	when ( "usg_2Y" ) { $retval = 1; }
	when ( "usg_3Y" ) { $retval = 1; }
	when ( "usg_5Y" ) { $retval = 1; }
	when ( "usg_7Y" ) { $retval = 1; }
	when ( "usg_10Y" ) { $retval = 1; }
	when ( "usg_30Y" ) { $retval = 1; }
    }
    $retval;
}

1
