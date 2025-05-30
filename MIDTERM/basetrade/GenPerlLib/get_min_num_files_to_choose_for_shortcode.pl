# \file GenPerlLib/get_min_num_files_to_choose_for_shortcode.pl
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

sub GetMinNumFilesToChooseForShortcode
{
    my $shortcode_ = shift;
    my $min_num_files_to_choose_ = 0;

    given ( $shortcode_ )
    {
	# EUREX 
	when ( "FGBS_0" )
	{ 
	    $min_num_files_to_choose_ = 0;
	}
	when ( "FGBM_0" )
	{ 
	    $min_num_files_to_choose_ = 0;
	}
	when ( "FGBL_0" )
	{
	    $min_num_files_to_choose_ = 0;
	}
	when ( "FESX_0" )
	{ 
	    $min_num_files_to_choose_ = 0;
	}
	when ( "FDAX_0" )
	{ 
	    $min_num_files_to_choose_ = 0;
	}

	# BMF
	when ( "BR_DOL_0" )
	{
	    $min_num_files_to_choose_ = 0;
	}
	when ( "BR_IND_0" )
	{ 
	    $min_num_files_to_choose_ = 0;
	}
	when ( "BR_WIN_0" )
	{ 
	    $min_num_files_to_choose_ = 0;
	}

	# TMX
	when ( "CGB_0" )
	{ 
	    $min_num_files_to_choose_ = 0;
	}
	when ( "SXF_0" )
	{ 
	    $min_num_files_to_choose_ = 1;
	}

	# CME
	when ( "UB_0" )
	{ 
	    $min_num_files_to_choose_ = 0;
	}

	when ( "ZB_0" )
	{ 
	    $min_num_files_to_choose_ = 0;
	}
	
        # LIFFE
	when ( "LFI_0" ) {  $min_num_files_to_choose_ = 0; }
	when ( "LFR_0" ) {  $min_num_files_to_choose_ = 0; }
	when ( "LFZ_0" ) {  $min_num_files_to_choose_ = 0; }
	
	default
	{
	    $min_num_files_to_choose_ = 0;
	}
    }

    return $min_num_files_to_choose_;
}

1
