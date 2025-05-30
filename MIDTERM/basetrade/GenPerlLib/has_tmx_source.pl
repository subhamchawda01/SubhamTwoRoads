# \file GenPerlLib/has_tmx_source.pl
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
use feature "switch";

sub HasTMXSource
{
    my $shortcode_ = shift;
    my $retval = 0;

    given ( $shortcode_ )
    {
    	when ("CGB_0" ) { $retval = 1; }
        when ("CGF_0" ) { $retval = 1; }
        when ("CGZ_0" ) { $retval = 1; }
	when ("CGB_1" ) { $retval = 1; }
	when ("SXF_0" ) { $retval = 1; }
	when ("BAX_0" ) { $retval = 1; }
	when ("BAX_1" ) { $retval = 1; }
	when ("BAX_2" ) { $retval = 1; }
	when ("BAX_3" ) { $retval = 1; }
	when ("BAX_4" ) { $retval = 1; }
	when ("BAX_5" ) { $retval = 1; }
	when ("BAX_6" ) { $retval = 1; }
	when ("BAX_7" ) { $retval = 1; }
	when ("CGB_1" ) { $retval = 1; }
	when ("CGB_2" ) { $retval = 1; }
	when ("CGB_3" ) { $retval = 1; }
	
	#spreads
	when ("SP_CGB0_CGB1" ) { $retval = 1; }
	when ("SP_CGB0_CGB2" ) { $retval = 1; }
	when ("SP_CGB0_CGB3" ) { $retval = 1; }
	when ("SP_CGB1_CGB2" ) { $retval = 1; }
	when ("SP_CGB2_CGB3" ) { $retval = 1; }
	when ("SP_BAX0_BAX1" ) { $retval = 1; }
	when ("SP_BAX0_BAX2" ) { $retval = 1; }
	when ("SP_BAX0_BAX3" ) { $retval = 1; }
	when ("SP_BAX1_BAX2" ) { $retval = 1; }
	when ("SP_BAX2_BAX3" ) { $retval = 1; }
	when ("SP_BAX3_BAX4" ) { $retval = 1; }
	when ("SP_BAX4_BAX5" ) { $retval = 1; }
	when ("SP_BAX5_BAX6" ) { $retval = 1; }
	
	#portfolios
    	when ( "CGBROLL" ) { $retval = 1; }
	when ( "BAXFLY1" ) { $retval = 1; }
	when ( "BAXFLY2" ) { $retval = 1; }
	when ( "BAXFLY3" ) { $retval = 1; }
	when ( "BAXFLY4" ) { $retval = 1; }
	when ( "BAXFLY5" ) { $retval = 1; }
	when ( "BAXMID" ) { $retval = 1; }
	when ( "BAXWHT" ) { $retval = 1; }
	when ( "CBEFUT" ) { $retval = 1; }
	when ( "CBFUT" ) { $retval = 1; }
	    	
    }
    $retval;
}

1
