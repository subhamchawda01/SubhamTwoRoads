# \file GenPerlLib/get_trading_location_for_shortcode.pl
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

sub GetTradingLocationForShortcode
{
    my $shortcode_ = shift;

    my $trading_location_ = "10.23.199.51";

    given ( $shortcode_ )
    {
	when ( "ZF_0" )
	{ 
	    $trading_location_ = "10.23.196.53";
	}
	when ( "ZN_0" )
	{ 
	    $trading_location_ = "10.23.196.51";
	}
	when ( "ZB_0" )
	{ 
	    $trading_location_ = "10.23.196.52";
	}
	when ( "UB_0" )
	{ 
	    $trading_location_ = "10.23.196.52";
	}
	when ( "CGB_0" )
	{ 
	    $trading_location_ = "10.23.182.51";
	}
	when ( "BAX_0" )
	{ 
	    $trading_location_ = "10.23.182.52";
	}
	when ( "BAX_1" )
	{ 
	    $trading_location_ = "10.23.182.52";
	}
	when ( "BAX_2" )
	{ 
	    $trading_location_ = "10.23.182.52";
	}
	when ( "BAX_3" )
	{ 
	    $trading_location_ = "10.23.182.52";
	}
	when ( "BAX_4" )
	{ 
	    $trading_location_ = "10.23.182.52";
	}
	when ( "BAX_5" )
	{ 
	    $trading_location_ = "10.23.182.52";
	}
	when ( "FGBS_0" )
	{ 
	    $trading_location_ = "10.23.200.51"
	}
	when ( "FGBM_0" )
	{ 
	    $trading_location_ = "10.23.200.53";
	}
	when ( "FGBL_0" )
	{ 
	    $trading_location_ = "10.23.200.52";
	}
	when ( "FESX_0" )
	{ 
	    $trading_location_ = "10.23.200.52";
	}
	when ( "BR_DOL_0" )
	{ 
	    $trading_location_ = "10.23.23.12";
	}
	when ( "BR_WDO_0" )
	{ 
	    $trading_location_ = "10.23.23.12";
	}
	when ( "BR_IND_0" )
	{ 
	    $trading_location_ = "10.220.40.1";
	}
	when ( "BR_WIN_0" )
	{ 
	    $trading_location_ = "10.220.40.1";
	}
	default
	{
	    $trading_location_ = "10.23.199.51";
	}
    }

    $trading_location_;
}

1
