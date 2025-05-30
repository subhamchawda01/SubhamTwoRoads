# \file GenPerlLib/has_rtsmicex_source.pl
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

sub HasRTSMICEXSource
{
    my $shortcode_ = shift;
    my $retval = 0;

    given ( $shortcode_ )
    {
	when ( "RI_0" ) { $retval = 1; }
	when ( "Si_0" ) { $retval = 1; }
	when ( "ED_0" ) { $retval = 1; }
	when ( "MX_0" ) { $retval = 1; }
	when ( "GD_0" ) { $retval = 1; }
	when ( "LK_0" ) { $retval = 1; }
	when ( "GZ_0" ) { $retval = 1; }
	when ( "SR_0" ) { $retval = 1; }
	when ( "GM_0" ) { $retval = 1; }
	when ( "RN_0" ) { $retval = 1; }
	when ( "VB_0" ) { $retval = 1; }
	when ( "SN_0" ) { $retval = 1; }
	when ( "TT_0" ) { $retval = 1; }
	when ( "USD000000TOD" ) { $retval = 1; }
	when ( "USD000TODTOM" ) { $retval = 1; }
	when ( "USD000UTSTOM" ) { $retval = 1; }
	when ( "SBER" ) { $retval = 1; }
	when ( "MGNT" ) { $retval = 1; }
	when ( "GMKN" ) { $retval = 1; }
	when ( "LKOH" ) { $retval = 1; }
	when ( "GAZP" ) { $retval = 1; }
	when ( "VTBR" ) { $retval = 1; }
	when ( "ROSN" ) { $retval = 1; }
	when ( "SNGS" ) { $retval = 1; }
	when ( "TATN" ) { $retval = 1; }
	when ( "GAZGZFUT" ) { $retval = 1; }
	when ( "GAZGZ" ) { $retval = 1; }
	when ( "GMKNGMFUT" ) { $retval = 1; }
	when ( "GMKNGM" ) { $retval = 1; }
	when ( "LKOHLKFUT" ) { $retval = 1; }
	when ( "LKOHLK" ) { $retval = 1; }
	when ( "REQFUT" ) { $retval = 1; }
	when ( "REQFUTL" ) { $retval = 1; }
	when ( "REQL" ) { $retval = 1; }
	when ( "REQ" ) { $retval = 1; }
	when ( "RFUTL" ) { $retval = 1; }
	when ( "RFUT" ) { $retval = 1; }
	when ( "RRUBFUT" ) { $retval = 1; }
	when ( "RRUB" ) { $retval = 1; }
	when ( "SBERSRFUT" ) { $retval = 1; }
	when ( "SBERSR" ) { $retval = 1; }
    }
    $retval;
}

1
