# \file GenPerlLib/get_time_zone_from_short_code.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2013
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use feature "switch";

sub get_utc_from_short_code_time_zone
{
    my ( $shortcode_ , $date_ ) = @_;
    
    my $time_zone_ = "UTC+0";

    if ( index ( $shortcode_ , "DI1" ) >= 0 )
    {
	return "BRT+3";
    }

    given ( $shortcode_ )
    {
	when ( "ZF_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "ZN_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "ZB_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "UB_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "CGB_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
        when ( "CGF_0" )
        {
            $time_zone_ = "EST+5";
        }
        when ( "CGZ_0" )
        {
            $time_zone_ = "EST+5";
        }
	when ( "BAX_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "BAX_1" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "BAX_2" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "BAX_3" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "BAX_4" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "BAX_5" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "FGBS_0" )
	{ 
	    $time_zone_ = "CET-1";
	}
	when ( "FGBM_0" )
	{ 
	    $time_zone_ = "CET-1";
	}
	when ( "FGBL_0" )
	{ 
	    $time_zone_ = "CET-1";
	}
        when ( "FGBX_0" )
        {
            $time_zone_ = "CET-1";
        }
	when ( "FESX_0" )
	{ 
	    $time_zone_ = "CET-1";
	}
	when ( "FSTB_0" )
	{ 
	    $time_zone_ = "CET-1";
	}
	when ( "FVS_3*" )
	{ 
	    $time_zone_ = "CET-1";
	}
	when ( "FOAT_0" )
	{ 
	    $time_zone_ = "CET-1";
	}
        when ( "FOAM_0" )
        {
            $time_zone_ = "CET-1";
        }
	when ( "FBTP_0" )
	{ 
	    $time_zone_ = "CET-1";
	}
        when ( "FBTS_0" )
        {
            $time_zone_ = "CET-1";
        }
	when ( "FDAX_0" )
	{ 
	    $time_zone_ = "CET-1";
	}
	when ( "BR_DOL_0" )
	{ 
	    $time_zone_ = "BRT+3";
	}
	when ( "BR_WDO_0" )
	{ 
	    $time_zone_ = "BRT+3";
	}
	when ( "BR_IND_0" )
	{
	    $time_zone_ = "BRT+3";
	}
	when ( "BR_WIN_0" )
	{ 
	    $time_zone_ = "BRT+3";
	}
	when ( "JFFCE_0" )
	{ 
	    $time_zone_ = "CET-1";
	}
	when ( "KFFTI_0" )
	{ 
	    $time_zone_ = "UTC+0";
	}
	when ( "LFR_0" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFZ_0" )
	{
	    $time_zone_ = "BST-1";
	}
	when ( "YFEBM_0" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "YFEBM_1" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "YFEBM_2" )
	{ 
	    $time_zone_ = "BST-1";
	}
 	when ( "XFC_0" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "XFC_1" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "XFC_2" )
	{
	    $time_zone_ = "BST-1";
	}       
	when ( "LFI_0" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFI_1" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFI_2" )
	{
	    $time_zone_ = "BST-1";
	}
	when ( "LFI_3" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFI_4" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFI_5" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFI_6" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFL_0" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFL_1" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFL_2" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "6S_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "6N_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "6E_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "6M_0" )
	{ 
	    $time_zone_ = "EST+5";
	}
	when ( "XFRC_0" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFL_3" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFL_4" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFL_5" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "LFL_6" )
	{ 
	    $time_zone_ = "BST-1";
	}
	when ( "HSI_0" )
	{
	    $time_zone_ = "HKT";
	}
	when ( "HHI_0" )
	{
	    $time_zone_ = "HKT";
	}
	when ( "LK_0" )
	{
	    $time_zone_ = "MSK-3";
	}
	when ( "ITUB4" )
	{
	    $time_zone_ = "BRT+3";
	}
	when ( "VALE5" )
	{
	    $time_zone_ = "BRT+3";
	}
	when ( "NQ_0" )
	{
	    $time_zone_ = "EST+5";
	}
	when ( "SP_LFI2_LFI3" )
	{
	    $time_zone_ = "BST-1";
	}
	when ( "SR_0" )
	{
	    $time_zone_ = "MSK-3";
	}
	when ( "ZT_0" )
	{
	    $time_zone_ = "EST+5";
	}
	when ( "YM_0" )
	{
	    $time_zone_ = "EST+5";
	}
	default
	{
	    $time_zone_ = "UTC+0";	    
	}
    }

    $time_zone_;
}

1
