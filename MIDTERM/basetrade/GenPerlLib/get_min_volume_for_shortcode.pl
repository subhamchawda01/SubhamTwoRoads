# \file GenPerlLib/get_min_pnl_volume_for_shortcode.pl
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

sub GetMinVolumeForShortcode
{
    my $shortcode_ = shift;

    my $min_volume_ = 0;

    given ( $shortcode_ )
    {
	when ( "ZF_0" )
	{ 
	    $min_volume_ = 4500;
	}
	when ( "ZN_0" )
	{ 
	    $min_volume_ = 1800;
	}
	when ( "ZB_0" )
	{ 
	    $min_volume_ = 1500;
	}
	when ( "UB_0" )
	{ 
	    $min_volume_ = 180;
	}
	when ( "CGB_0" )
	{ 
	    $min_volume_ = 180;
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
	    $min_volume_ = 80;
	}
	when ( "FGBM_0" )
	{ 
	    $min_volume_ = 250;
	}
	when ( "FGBL_0" )
	{
	    $min_volume_ = 80;
	}
	when ( "FESX_0" )
	{ 
	    $min_volume_ = 8000;
	}
	when ( "BR_DOL_0" )
	{
	    $min_volume_ = 10000;
	}
	when ( "BR_WDO_0" )
	{ 
	}
	when ( "BR_IND_0" )
	{ 
	    $min_volume_ = 400;
	}
	when ( "BR_WIN_0" )
	{ 
	    $min_volume_ = 1000;
	}
	default
	{
	    $min_volume_ = 0;
	}
    }

    $min_volume_;
}

1
