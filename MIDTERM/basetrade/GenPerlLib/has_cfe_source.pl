# \file GenPerlLib/has_cfe_source.pl
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

sub HasCFESource
{
    my $shortcode_ = shift;
    my $retval = 0;

    given ( $shortcode_ )
    {
    	when ("VX_0" ) { $retval = 1; }
    	when ("VX_1" ) { $retval = 1; }
    	when ("VX_2" ) { $retval = 1; }
    	when ("VX_3" ) { $retval = 1; }
    	when ("VX_4" ) { $retval = 1; }
    	when ("VX_5" ) { $retval = 1; }
    	when ("VX_6" ) { $retval = 1; }
    	when ("VX_7" ) { $retval = 1; }
    	when ("SP_VX0_VX1" ) { $retval = 1; }
    	when ("SP_VX1_VX2" ) { $retval = 1; }
    	when ("SP_VX2_VX3" ) { $retval = 1; }
    	when ("SP_VX0_VX2" ) { $retval = 1; }
    	when ("SP_VX0_VX3" ) { $retval = 1; }
    	when ("SP_VX0_VX3" ) { $retval = 1; }
    	when ("SP_VX1_VX3" ) { $retval = 1; }
    	when ("SP_VX0_VX4" ) { $retval = 1; }
    	when ("SP_VX1_VX4" ) { $retval = 1; }
    	when ("SP_VX0_VX5" ) { $retval = 1; }
    	when ("SP_VX3_VX4" ) { $retval = 1; }
    	when ("SP_VX1_VX5" ) { $retval = 1; }
    	when ("SP_VX2_VX4" ) { $retval = 1; }
    	when ("SP_VX2_VX5" ) { $retval = 1; }
    	when ("SP_VX4_VX5" ) { $retval = 1; }
    	when ("SP_VX0_VX6" ) { $retval = 1; }
    	when ("SP_VX3_VX5" ) { $retval = 1; }
    	when ("SP_VX1_VX6" ) { $retval = 1; }
    	when ("SP_VX2_VX6" ) { $retval = 1; }
    	when ("SP_VX5_VX6" ) { $retval = 1; }
    	when ("SP_VX3_VX6" ) { $retval = 1; }
    	when ("SP_VX4_VX6" ) { $retval = 1; }
    	when ("SP_VX7_VX0" ) { $retval = 1; }
    	when ("SP_VX7_VX3" ) { $retval = 1; }
    	when ("SP_VX7_VX1" ) { $retval = 1; }
    	when ("SP_VX7_VX4" ) { $retval = 1; }
    	when ("SP_VX7_VX5" ) { $retval = 1; }
    	when ("SP_VX7_VX2" ) { $retval = 1; }
    	when ("SP_VX7_VX6" ) { $retval = 1; }
    	when ("SP_VX0_VX1_VX2" ) { $retval = 1; }
    }

    $retval;
}

1
