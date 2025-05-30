# \file GenPerlLib/get_strat_id_for_shortcode.pl
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
        when ( "6A_0" )
        { 
            $ret_strat_id_ = 16011;
        }
        when ( "6B_0" )
        { 
            $ret_strat_id_ = 16111;
        }
        when ( "6E_0" )
        { 
            $ret_strat_id_ = 16311;
        }
        when ( "6J_0" )
        { 
            $ret_strat_id_ = 16411;
        }
        when ( "6M_0" )
        { 
            $ret_strat_id_ = 16511;
        }
        when ( "6N_0" )
        { 
            $ret_strat_id_ = 16611;
        }
        when ( "6S_0" )
        { 
            $ret_strat_id_ = 16711;
        }
        when ( "CL_0" )
        { 
            $ret_strat_id_ = 16811;
        }
        when ( "GC_0" )
        { 
            $ret_strat_id_ = 16911;
        }
        when ( "NQ_0" )
        { 
            $ret_strat_id_ = 17011;
        }
        when ( "YM_0" )
        { 
            $ret_strat_id_ = 17111;
        }
        when ( "ES_0" )
        { 
            $ret_strat_id_ = 17211;
        }
        when ( "NKD_0" )
        { 
            $ret_strat_id_ = 18011;
        }
        when ( "NIY_0" )
        { 
            $ret_strat_id_ = 19011;
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
	when ( "FBTP_0" )
	{
	    $ret_strat_id_ = 8011;
	}
	when ( "FOAT_0" )
	{
	    $ret_strat_id_ = 9011;
	}
	when ( "FBTS_0" )
	{
	    $ret_strat_id_ = 1011;
	}
	when ( "FSMI_0" )
	{
	    $ret_strat_id_ = 1111;
	}
	when ( "FXXP_0" )
	{
	    $ret_strat_id_ = 1211;
	}
	when ( "FSTB_0" )
	{
	    $ret_strat_id_ = 1311;
	}
	when ( "FSTG_0" )
	{
	    $ret_strat_id_ = 1411;
	}
	when ( "SXF_0" )
	{
	    $ret_strat_id_ = 20011;
	}
	when ( "CGB_0" )
	{
	    $ret_strat_id_ = 21011;
	}
	when ( "CGF_0" )
	{
	    $ret_strat_id_ = 28011;
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
	when ( "BR_WDO_0" )
	{
	    $ret_strat_id_ = 33011;
	}
# DI1F
	when ( "DI1F15" )
	{
	    $ret_strat_id_ = 34011;
	}
	when ( "DI1F16" )
	{
	    $ret_strat_id_ = 37011;
	}
	when ( "DI1F17" )
	{
	    $ret_strat_id_ = 39011;
	}
	when ( "DI1F18" )
	{
	    $ret_strat_id_ = 36011;
	}
	when ( "DI1F19" )
	{
	    $ret_strat_id_ = 38011;
	}
# DI1N
	when ( "DI1N14" )
	{
	    $ret_strat_id_ = 35111;
	}
	when ( "DI1N15" )
	{
	    $ret_strat_id_ = 34111;
	}
	when ( "DI1N16" )
	{
	    $ret_strat_id_ = 37111;
	}
	when ( "DI1N17" )
	{
	    $ret_strat_id_ = 39111;
	}
	when ( "DI1N18" )
	{
	    $ret_strat_id_ = 36111;
	}
	when ( "DI1N19" )
	{
	    $ret_strat_id_ = 38111;
	}
# DI1G
	when ( "DI1G15" )
	{
	    $ret_strat_id_ = 35211;
	}
	when ( "DI1G16" )
	{
	    $ret_strat_id_ = 34211;
	}
	when ( "DI1G17" )
	{
	    $ret_strat_id_ = 37211;
	}
	when ( "DI1G18" )
	{
	    $ret_strat_id_ = 39211;
	}
	when ( "DI1G19" )
	{
	    $ret_strat_id_ = 36211;
	}
	when ( "DI1G20" )
	{
	    $ret_strat_id_ = 38211;
	}
# DI1J
	when ( "DI1J15" )
	{
	    $ret_strat_id_ = 35311;
	}
	when ( "DI1J16" )
	{
	    $ret_strat_id_ = 34311;
	}
	when ( "DI1J17" )
	{
	    $ret_strat_id_ = 37311;
	}
	when ( "DI1J18" )
	{
	    $ret_strat_id_ = 39311;
	}
	when ( "DI1J19" )
	{
	    $ret_strat_id_ = 36311;
	}
	when ( "DI1J20" )
	{
	    $ret_strat_id_ = 38311;
	}
#
	when ( "JFFCE_0" )
	{
	    $ret_strat_id_ = 40011;
	}
	when ( "YFEBM_0" )
	{
	    $ret_strat_id_ = 41011;
	}
	when ( "YFEBM_1" )
	{
	    $ret_strat_id_ = 42011;
	}
	when ( "YFEBM_2" )
	{
	    $ret_strat_id_ = 43011;
	}
	when ( "KFFTI_0" )
	{
	    $ret_strat_id_ = 50011;
	}
	when ( "LFR_0" )
	{
	    $ret_strat_id_ = 60011;
	}
	when ( "LFZ_0" )
	{
	    $ret_strat_id_ = 70011;
	}
	when ( "LFL_0" )
	{
	    $ret_strat_id_ = 80011;
	}
	when ( "LFL_1" )
	{
	    $ret_strat_id_ = 80111;
	}
	when ( "LFL_2" )
	{
	    $ret_strat_id_ = 80211;
	}
	when ( "LFL_3" )
	{
	    $ret_strat_id_ = 80311;
	}
	when ( "LFL_4" )
	{
	    $ret_strat_id_ = 80411;
	}
	when ( "LFL_5" )
	{
	    $ret_strat_id_ = 80511;
	}
	when ( "LFL_6" )
	{
	    $ret_strat_id_ = 80611;
	}
	when ( "LFI_0" )
	{
	    $ret_strat_id_ = 90011;
	}
	when ( "LFI_1" )
	{
	    $ret_strat_id_ = 90111;
	}
	when ( "LFI_2" )
	{
	    $ret_strat_id_ = 90211;
	}
	when ( "LFI_3" )
	{
	    $ret_strat_id_ = 90311;
	}
	when ( "LFI_4" )
	{
	    $ret_strat_id_ = 90411;
	}
	when ( "LFI_5" )
	{
	    $ret_strat_id_ = 90511;
	}
	when ( "LFI_6" )
	{
	    $ret_strat_id_ = 90611;
	}
	when ( "HHI_0" )
	{
	    $ret_strat_id_ = 100011
	}
	when ( "HSI_0" )
	{
	    $ret_strat_id_ = 110011
	}
	when ( "MCH_0" )
	{
	    $ret_strat_id_ = 120011
	}
	when ( "MHI_0" )
	{
	    $ret_strat_id_ = 130011
	}
	when ( "RI_0" )
	{
	    $ret_strat_id_ = 310011
	}
	when ( "Si_0" )
	{
	    $ret_strat_id_ = 320011
	}
	when ( "ED_0" )
	{
	    $ret_strat_id_ = 330011
	}
	when ( "MX_0" )
	{
	    $ret_strat_id_ = 340011
	}
	when ( "GD_0" )
	{
	    $ret_strat_id_ = 350011
	}
	when ( "LK_0" )
	{
	    $ret_strat_id_ = 360011
	}
	when ( "VB_0" )
	{
	    $ret_strat_id_ = 370011
	}
	when ( "GZ_0" )
	{
	    $ret_strat_id_ = 380011
	}
	when ( "SR_0" )
	{
	    $ret_strat_id_ = 390011
	}
	when ( "GM_0" )
	{
	    $ret_strat_id_ = 400011
	}
	when ( "RN_0" )
	{
	    $ret_strat_id_ = 410011
	}
	when ( "USD000000TOD" )
	{
	    $ret_strat_id_ = 510011
	}
	when ( "USD000UTSTOM" )
	{
	    $ret_strat_id_ = 520011
	}
	when ( "VTBR" )
	{
	    $ret_strat_id_ = 530011
	}
	when ( "SBER" )
	{
	    $ret_strat_id_ = 540011
	}
	when ( "MGNT" )
	{
	    $ret_strat_id_ = 550011
	}
	when ( "LKOH" )
	{
	    $ret_strat_id_ = 560011
	}
	when ( "GAZP" )
	{
	    $ret_strat_id_ = 570011
	}
	when ( "GMKN" )
	{
	    $ret_strat_id_ = 580011
	}
	when ( "ROSN" )
	{
	    $ret_strat_id_ = 590011
	}
	when ( "SNGS" )
	{
	    $ret_strat_id_ = 600011
	}
	when ( "TATN" )
	{
	    $ret_strat_id_ = 610011
	}
	when ( "NK_0" )
	{
	    $ret_strat_id_ = 210011
	}
	when ( "NKM_0" )
	{
	    $ret_strat_id_ = 220011
	}
	when ( "NKM_1" )
	{
	    $ret_strat_id_ = 230011
	}
	when ( "JGBL_0" )
	{
	    $ret_strat_id_ = 240011
	}
	when ( "TOPIX_0" )
	{
	    $ret_strat_id_ = 250011
	}
	when ( "FVS_0" )
	{
	    $ret_strat_id_ = 200011
	}
	when ( "FVS_1" )
	{
	    $ret_strat_id_ = 200111
	}
	when ( "FVS_2" )
	{
	    $ret_strat_id_ = 200211
	}
	when ( "FVS_3" )
	{
	    $ret_strat_id_ = 200311
	}
	when ( "FVS_4" )
	{
	    $ret_strat_id_ = 200411
	}
	when ( "FVS_5" )
	{
	    $ret_strat_id_ = 200511
	}
	when ( "FVS_6" )
	{
	    $ret_strat_id_ = 200611
	}
	when ( "FVS_7" )
	{
	    $ret_strat_id_ = 200711
	}
	when ( "VALE5" )
	{
	    $ret_strat_id_ = 210011
	}
	when ( "VALE3" )
	{
	    $ret_strat_id_ = 210111
	}
	when ( "ITUB4" )
	{
	    $ret_strat_id_ = 210211
	}
	default
	{
	    $ret_strat_id_ = 999911;
	}
    }
    $ret_strat_id_;
}

1
