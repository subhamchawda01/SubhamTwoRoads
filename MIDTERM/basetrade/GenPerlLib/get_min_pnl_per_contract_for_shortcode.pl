# \file GenPerlLib/get_min_pnl_per_contract_for_shortcode.pl
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

sub GetMinPnlPerContractForShortcode
{
    my $shortcode_ = shift;

    my $min_pnl_per_contract_ = 1000.0;

    given ( $shortcode_ )
    {
	when ( "ZF_0" )
	{ 
	    $min_pnl_per_contract_ = -0.85;
	}
	when ( "ZN_0" )
	{ 
	    $min_pnl_per_contract_ = -0.85;
	}
	when ( "ZB_0" )
	{ 
	    $min_pnl_per_contract_ = -0.85
	}
	when ( "UB_0" )
	{ 
	    $min_pnl_per_contract_ = -0.55;
	}
	when ( "CGB_0" )
	{ 
	    $min_pnl_per_contract_ = 0.60;
	}
	when ( "BAX_0" )
	{ 
	}
	when ( "BAX_1" )
	{ 
	}
	when ( "BAX_2" )
	{ 
	}
	when ( "BAX_3" )
	{ 
	}
	when ( "BAX_4" )
	{ 
	}
	when ( "BAX_5" )
	{ 
	}
	when ( "FGBS_0" )
	{ 
	    $min_pnl_per_contract_ = -0.15;
	}
	when ( "FGBM_0" )
	{ 
	    $min_pnl_per_contract_ = -0.2;
	}
	when ( "FGBL_0" )
	{
	    $min_pnl_per_contract_ = -1.2;
	}
	when ( "FESX_0" )
	{ 
	    $min_pnl_per_contract_ = 0.70;
	}
	when ( "BR_DOL_0" )
	{
	    $min_pnl_per_contract_ = 0.80;
	}
	when ( "BR_WDO_0" )
	{ 
	}
	when ( "BR_IND_0" )
	{ 
	    $min_pnl_per_contract_ = -5.0; # IND sims are incredibly pessimistic.
	}
	when ( "BR_WIN_0" )
	{ 
	}
	default
	{
	}
    }

    $min_pnl_per_contract_;
}

1
