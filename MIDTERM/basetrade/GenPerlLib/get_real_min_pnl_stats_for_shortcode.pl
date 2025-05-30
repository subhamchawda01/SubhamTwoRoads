# \file GenPerlLib/get_real_min_pnl_stats_for_shortcode.pl
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

sub GetRealMinPnlStatsForShortcode
{
    my $shortcode_ = shift;

    my $min_pnl_per_contract_ = 1000.0;
    my $min_volume_ = 100000.0;
    my $min_pnl_per_abs_pos_ = 100000.0;

    given ( $shortcode_ )
    {
	when ( "ZF_0" )
	{ 
	    $min_pnl_per_contract_ = 0.5;
	    $min_volume_ = 500;
	    $min_pnl_per_abs_pos_ = 120;
	}
	when ( "ZN_0" )
	{ 
	    $min_pnl_per_contract_ = 0.0;
	    $min_volume_ = 300;
	    $min_pnl_per_abs_pos_ = 120;
	}
	when ( "ZB_0" )
	{ 
	}
	when ( "UB_0" )
	{ 
	    $min_pnl_per_contract_ = 1.0;
	    $min_volume_ = 500;
	    $min_pnl_per_abs_pos_ = 400;
	}
	when ( "CGB_0" )
	{ 
	    $min_pnl_per_contract_ = 1.5;
	    $min_volume_ = 600;
	    $min_pnl_per_abs_pos_ = 500;
	}
	when ( "BAX_0" )
	{ 
	    $min_pnl_per_contract_ = 1.0;
	    $min_volume_ = 300;
	    $min_pnl_per_abs_pos_ = 120;
	}
	when ( "BAX_1" )
	{ 
	    $min_pnl_per_contract_ = 3.0;
	    $min_volume_ = 60;
	    $min_pnl_per_abs_pos_ = 42;
	}
	when ( "BAX_2" )
	{ 
	    $min_pnl_per_contract_ = 5.0;
	    $min_volume_ = 60;
	    $min_pnl_per_abs_pos_ = 50;
	}
	when ( "BAX_3" )
	{ 
	    $min_pnl_per_contract_ = 4.0;
	    $min_volume_ = 50;
	    $min_pnl_per_abs_pos_ = 40;
	}
	when ( "BAX_4" )
	{ 
	    $min_pnl_per_contract_ = 4.0;
	    $min_volume_ = 50;
	    $min_pnl_per_abs_pos_ = 40;
	}
	when ( "BAX_5" )
	{ 
	}
	when ( "FGBS_0" )
	{ 
	    $min_pnl_per_contract_ = 0.8;
	    $min_volume_ = 300;
	    $min_pnl_per_abs_pos_ = 85;
	}
	when ( "FGBM_0" )
	{ 
	    $min_pnl_per_contract_ = 0.8;
	    $min_volume_ = 300;
	    $min_pnl_per_abs_pos_ = 200;
	}
	when ( "FGBL_0" )
	{
	    $min_pnl_per_contract_ = 0.8;
	    $min_volume_ = 250;
	    $min_pnl_per_abs_pos_ = 110;
	}
	when ( "FESX_0" )
	{ 
	    $min_pnl_per_contract_ = 0.8;
	    $min_volume_ = 500;
	    $min_pnl_per_abs_pos_ = 120;
	}
	when ( "BR_DOL_0" )
	{
	    $min_pnl_per_contract_ = 0.5;
	    $min_volume_ = 2500;
	    $min_pnl_per_abs_pos_ = 700;
	}
	when ( "BR_WDO_0" )
	{ 
	}
	when ( "BR_IND_0" )
	{ 
	    $min_pnl_per_contract_ = -0.5;
	    $min_volume_ = 2500;
	    $min_pnl_per_abs_pos_ = 700;
	}
	when ( "BR_WIN_0" )
	{ 
	    $min_pnl_per_contract_ = 0.8;
	    $min_volume_ = 250;
	    $min_pnl_per_abs_pos_ = 400;
	}
	when ( "DI1F15" )
	{
	    $min_pnl_per_contract_ = 0.0;
	    $min_volume_ = 100;
	    $min_pnl_per_abs_pos_ = 20;
	}
	when ( "DI1F18" )
	{
	    $min_pnl_per_contract_ = 0.0;
	    $min_volume_ = 100;
	    $min_pnl_per_abs_pos_ = 40;
	}
	when ( "DI1F16" )
	{
	    $min_pnl_per_contract_ = 0.0;
	    $min_volume_ = 100;
	    $min_pnl_per_abs_pos_ = 40;
	}
	when ( "DI1N14" )
	{
	    $min_pnl_per_contract_ = 0.0;
	    $min_volume_ = 100;
	    $min_pnl_per_abs_pos_ = 400;
	}
	when ( "DI1F17" )
	{
	    $min_pnl_per_contract_ = 0.0;
	    $min_volume_ = 100;
	    $min_pnl_per_abs_pos_ = 50;
	}

	when ( "LFR_0" )
	{
	    $min_pnl_per_contract_ = 0.0;
	    $min_volume_ = 100;
	    $min_pnl_per_abs_pos_ = 300;
	}

	default
	{
	}
    }

    return ( $min_pnl_per_contract_ , $min_volume_ , $min_pnl_per_abs_pos_ );
}

1
