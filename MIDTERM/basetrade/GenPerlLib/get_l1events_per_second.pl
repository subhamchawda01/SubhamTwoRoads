# \file GenPerlLib/get_l1events_per_second.pl
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

sub GetL1EventsPerSecond
{
    my $shortcode_ = shift;
    my $period_ = shift;
    my $retval = 10;

    given ( $shortcode_ )
    {
	when ( "ZT_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 3; }
	    else
	    { $retval = 2; }
	}
	when ( "ZF_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 30; }
	    else
	    { $retval = 4; }
	}
	when ( "ZN_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 40; }
	    else
	    { $retval = 7; }
	}
	when ( "ZB_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 25; }
	    else
	    { $retval = 3; }
	}
	when ( "UB_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 18; }
	    else
	    { $retval = 1; }
	}
	when ( "FGBS_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 6; }
	    else
	    { $retval = 7; }
	}
	when ( "FGBM_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 10; }
	    else
	    { $retval = 10; }
	}
	when ( "FGBL_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 11; }
	    else
	    { $retval = 11; }
	}
	when ( "FGBX_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 2; }
	}
	when ( "FESX_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 23; }
	    else
	    { $retval = 15; }
	}
	when ( "FDAX_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 15; }
	    else
	    { $retval = 7; }
	}
	when ( "SXF_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 5; }
	    else
	    { $retval = 1; }
	}
	when ( "CGB_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 3; }
	    else
	    { $retval = 1; }
	}
	when ( "BAX_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "BAX_1" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "BAX_2" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "BAX_3" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "BAX_4" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "BAX_5" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "BR_DOL_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "BR_IND_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 2; }
	    else
	    { $retval = 1; }
	}
	when ( "BR_WIN_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 2; }
	    else
	    { $retval = 2; }
	}
	when ( "DI1F15" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "DI1F16" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "DI1N14" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "DI1F19" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "DI1N14" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "DI1F17" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	when ( "BR_WDO_0" )
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 1; }
	    else
	    { $retval = 1; }
	}
	default
	{
	    if ( $period_ eq "US_MORN_DAY" )
	    { $retval = 10; }
	    else
	    { $retval = 10; }
	}
    }
    $retval;
}

1
