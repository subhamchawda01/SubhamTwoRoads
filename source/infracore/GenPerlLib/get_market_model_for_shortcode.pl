# \file GenPerlLib/get_market_model_for_shortcode.pl
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

sub GetMarketModelForShortcode
{
    my $shortcode_ = shift;
    my $market_model_index_ = 2;

    given ( $shortcode_ )
    {
	when ( "ZF_0" )
	{ 
	    $market_model_index_ = 1;
	}
	when ( "ZN_0" )
	{ 
	    $market_model_index_ = 1;
	}
	when ( "ZB_0" )
	{ 
	    $market_model_index_ = 2;
	}
	when ( "UB_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "CGB_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FGBS_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FOAT_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FGBM_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FGBL_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FESX_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "BR_DOL_0" )
	{ 
	    $market_model_index_ = 1;
	}
	when ( "BR_WDO_0" )
	{ 
	    $market_model_index_ = 1;
	}
	when ( "BR_IND_0" )
	{ 
	    $market_model_index_ = 0;
	}
	when ( "BR_WIN_0" )
	{ 
	    $market_model_index_ = 1;
	}
	default
	{
	    $market_model_index_ = 2;
	}
    }

    $market_model_index_;
}

1
