# \file GenPerlLib/get_strat_id_for_shortcode.pl
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

sub GetStratIdForShortcode
{
    my $shortcode_ = shift;
    my $ret_strat_id_ = 10011;

    given ( $shortcode_ )
    {
	when ( "ZT_0" )
	{
	    $ret_strat_id_ = 11011;
	}
	when ( "ZF_0" )
	{
	    $ret_strat_id_ = 12011;
	}
	when ( "ZN_0" )
	{
	    $ret_strat_id_ = 13011;
	}
	when ( "ZB_0" )
	{
	    $ret_strat_id_ = 14011;
	}
	when ( "UB_0" )
	{
	    $ret_strat_id_ = 15011;
	}
	when ( "FGBS_0" )
	{
	    $ret_strat_id_ = 2011;
	}
	when ( "FGBM_0" )
	{
	    $ret_strat_id_ = 3011;
	}
	when ( "FGBL_0" )
	{
	    $ret_strat_id_ = 4011;
	}
	when ( "FGBX_0" )
	{
	    $ret_strat_id_ = 5011;
	}
	when ( "FESX_0" )
	{
	    $ret_strat_id_ = 6011;
	}
	when ( "FDAX_0" )
	{
	    $ret_strat_id_ = 7011;
	}
	when ( "SXF_0" )
	{
	    $ret_strat_id_ = 20011;
	}
	when ( "CGB_0" )
	{
	    $ret_strat_id_ = 21011;
	}
	when ( "BAX_0" )
	{
	    $ret_strat_id_ = 22011;
	}
	when ( "BAX_1" )
	{
	    $ret_strat_id_ = 23011;
	}
	when ( "BAX_2" )
	{
	    $ret_strat_id_ = 24011;
	}
	when ( "BAX_3" )
	{
	    $ret_strat_id_ = 25011;
	}
	when ( "BAX_4" )
	{
	    $ret_strat_id_ = 26011;
	}
	when ( "BAX_5" )
	{
	    $ret_strat_id_ = 27011;
	}
	when ( "BR_DOL_0" )
	{
	    $ret_strat_id_ = 30011;
	}
	when ( "BR_IND_0" )
	{
	    $ret_strat_id_ = 31011;
	}
	when ( "BR_WIN_0" )
	{
	    $ret_strat_id_ = 32011;
	}
	when ( "DI1F16" )
	{
	    $ret_strat_id_ = 33011;
	}
	when ( "DI1F15" )
	{
	    $ret_strat_id_ = 34011;
	}
	when ( "DI1F19" )
	{
	    $ret_strat_id_ = 35011;
	}
	when ( "DI1F17" )
	{
	    $ret_strat_id_ = 36011;
	}
	when ( "BR_WDO_0" )
	{
	    $ret_strat_id_ = 40011;
	}
	default
	{
	    $ret_strat_id_ = 10011;
	}
    }
    $ret_strat_id_;
}

1
