# \file GenPerlLib/get_real_query_id_prefix_for_shortcode_time_period.pl
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

sub GetRealQueryIdPrefixForShortcodeTimePeriod
{
    my ( $shortcode_ , $timeperiod_ ) = @_;

    my $query_id_prefix_ = "";

    given ( $timeperiod_ )
    {
	when ( "US" )
	{
	    given ( $shortcode_ )
	    {
		when ( "FGBS_0" )
		{ $query_id_prefix_ = "20"; }
		
		when ( "FGBM_0" )
		{ $query_id_prefix_ = "30"; }
		
		when ( "FGBL_0" )
		{ $query_id_prefix_ = "40"; }
		
		when ( "FESX_0" )
		{ $query_id_prefix_ = "60"; }

		when ( "FOAT_0" )
		{ $query_id_prefix_ = "90"; }
		
		when ( "FDAX_0" )
		{ $query_id_prefix_ = ""; }

		when ( "ZF_0" )
		{ $query_id_prefix_ = "120"; }
		
		when ( "ZN_0" )
		{ $query_id_prefix_ = "130"; }
		
		when ( "ZB_0" )
		{ $query_id_prefix_ = "140"; }
		
		when ( "UB_0" )
		{ $query_id_prefix_ = "150"; }
		
		when ( "CGB_0" )
		{ $query_id_prefix_ = "210"; }
		
		when ( "BAX_1" )
		{ $query_id_prefix_ = "230"; }
		
		when ( "BAX_2" )
		{ $query_id_prefix_ = "240"; }
		
		when ( "BAX_3" )
		{ $query_id_prefix_ = "250"; }

		when ( "BAX_4" )
		{ $query_id_prefix_ = "260"; }

		when ( "BAX_5" )
		{ $query_id_prefix_ = "270"; }

		when ( "BR_DOL_0" )
		{ $query_id_prefix_ = "300"; }

		when ( "DI1F15" )
		{ $query_id_prefix_ = "350"; }

		when ( "DI1F16" )
		{ $query_id_prefix_ = "390"; }

		when ( "DI1F17" )
		{ $query_id_prefix_ = "380"; }

		when ( "DI1F18" )
		{ $query_id_prefix_ = "370"; }

		when ( "DI1F22" )
		{ $query_id_prefix_ = "360"; }

		when ( "DI1N14" )
		{ $query_id_prefix_ = "3080"; }

		when ( "DI1N15" )
		{ $query_id_prefix_ = "3081"; }

		when ( "BR_WIN_0" )
		{ $query_id_prefix_ = "320"; }

		when ( "BR_IND_0" )
		{ $query_id_prefix_ = "310"; }

		when ( "LFR_0" )
		{ $query_id_prefix_ = "600"; }

		when ( "FBTS_0" )
		{ $query_id_prefix_ = "10"; }

		when ( "FBTP_0" )
		{ $query_id_prefix_ = "80"; }

		when ( "LFZ_0" )
		{ $query_id_prefix_ = "700"; }

		when ( "NK_0" )
		{ $query_id_prefix_ = "21005"; }

		when ( "NKM_0" )
		{ $query_id_prefix_ = "22005"; }

    when ( "YFEBM_0" )
		{ $query_id_prefix_ = "410"; }
    when ( "YFEBM_1" )
		{ $query_id_prefix_ = "420"; }
    when ( "YFEBM_2" )
		{ $query_id_prefix_ = "430"; }
    when ( "SXF_0" )
		{ $query_id_prefix_ = "200"; }


		default
		{ $query_id_prefix_ = ""; }
	    }
	}

	when ( "EU" )
	{		
	    given ( $shortcode_ )
	    {
		when ( "FGBS_0" )
		{ $query_id_prefix_ = "3"; }

		when ( "FGBM_0" )
		{ $query_id_prefix_ = "1"; }

		when ( "FGBL_0" )
		{ $query_id_prefix_ = "4"; }
                
		when ( "FESX_0" )
		{ $query_id_prefix_ = "2"; }
                
		when ( "FDAX_0" )
		{ $query_id_prefix_ = ""; }

		when ( "FBTP_0" )
		{ $query_id_prefix_ = "8"; }

		when ( "FBTS_0" )
		{ $query_id_prefix_ = "16"; }
                
		when ( "ZF_0" )
		{ $query_id_prefix_ = "6"; }
                
		when ( "ZN_0" )
		{ $query_id_prefix_ = "9"; }
                
		when ( "ZB_0" )
		{ $query_id_prefix_ = "140"; }
                
		when ( "UB_0" )
		{ $query_id_prefix_ = "150"; }

		when ( "LFR_0" )
		{ $query_id_prefix_ = "12"; }

		when ( "LFZ_0" )
		{ $query_id_prefix_ = "13"; }
                
		when ( "NK_0" )
		{ $query_id_prefix_ = "21004"; }

		when ( "NKM_0" )
		{ $query_id_prefix_ = "22004"; }

		default
		{ $query_id_prefix_ = ""; }
	    }
	}
	when ( "AS" )
	{		
	    given ( $shortcode_ )
	    {
		when ( "NK_0" )
		{ $query_id_prefix_ = "21001"; }
                
		when ( "NKM_0" )
		{ $query_id_prefix_ = "22001"; }

		when ( "HHI_0" )
		{ $query_id_prefix_ = "1000"; }

		when ( "HSI_0" )
		{ $query_id_prefix_ = "1100"; }
	
		default
		{ $query_id_prefix_ = ""; }
	    }	
	}
	default
	{ $query_id_prefix_ = ""; }
    }

    return $query_id_prefix_;
}

1
