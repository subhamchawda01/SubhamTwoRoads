# \file GenPerlLib/has_hkfe_source.pl
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

sub HasHKFESource
{
    my $shortcode_ = shift;
    my $retval = 0;

    given ( $shortcode_ )
    {
    	when ("HHI_0" ) { $retval = 1; }
    	when ("MHI_0" ) { $retval = 1; }
    	when ("HSI_0" ) { $retval = 1; }
    	when ("MCH_0" ) { $retval = 1; }
    	when ("HKALL" ) { $retval = 1; }
    	when ("HKFUT" ) { $retval = 1; }
    	when ("HKOSEALL" ) { $retval = 1; }
    	when ("HKOSEINDX" ) { $retval = 1; }
    	when ("HKOSFUT" ) { $retval = 1; }
    	when ("UHKEQ" ) { $retval = 1; }
    	when ("UHKEQL" ) { $retval = 1; }
    	when ("UOSEHKEQ" ) { $retval = 1; }
    	when ("UOSEHKEQL" ) { $retval = 1; }
    }

    $retval;
}

1
