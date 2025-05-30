# \file GenPerlLib/get_trading_location_for_shortcode.pl
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

sub GetTradingLocationForShortcode
{
    my ( $shortcode_ , $date_ ) = @_;
    
    my $trading_location_ = "10.23.74.51";

    if ( index ( $shortcode_ , "DI1" ) >= 0 )
    {
      if ( index ( $shortcode_ , "DI1F" ) >= 0 )
      {
        return "10.23.23.13";
      }
      else
      {
        return "10.23.23.11";
      }
    }
    
    if ( index( $shortcode_, "BMFEQ") >= 0 )
    {
    	return "10.23.23.12";
    }

    if ( index ( $shortcode_, "BAX" ) >= 0 ) 
    {
      return "10.23.182.51";
    }    
    if ( index ( $shortcode_, "GE") >= 0 )
    {
      return "10.23.82.52";
    }

    if ( index ( $shortcode_, "VX" ) >= 0 )
    {
      return "10.23.74.61";
    }    

    if ( index ( $shortcode_, "FVS" ) >= 0 )
    {
      return "10.23.200.51";
    }

    if ( index ( $shortcode_, "IR_" ) == 0 )
	  { 
	    return "10.23.43.51";
	  }


    given ( $shortcode_ )
    {
	when ( "ZF_0" )
	{ 
	    $trading_location_ = "10.23.82.53";
	}
        when ( "USD000000TOD" )
	{ 
	    $trading_location_ = "172.18.244.107";
	}
	when ( "ZN_0" )
	{ 
	    if ( $date_ >= 20120528 && $date_ <= 20120603 )
	    {
		$trading_location_ = "10.23.196.54";
	    }
	    else
	    {
		$trading_location_ = "10.23.82.51";
	    }
	}
	when ( "ZB_0" )
	{ 
	    $trading_location_ = "10.23.82.54";
	}
	when ( "UB_0" )
	{ 
	    $trading_location_ = "10.23.82.52";
	}
        when ( "ZT_0" )
        {
          $trading_location_ = "10.23.82.52";
        }
        when ( "NKD_0")
        {
          $trading_location_ = "10.23.82.52";
        }
        when ( "NIY_0" )
        {
          $trading_location_ = "10.23.82.52";
        }
	when ( "CGB_0" )
	{ 
	    $trading_location_ = "10.23.182.51";
	}
	when ( "SXF_0" )
	{ 
	    $trading_location_ = "10.23.182.52";
	}
  when ( "XT_0" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}
  when ( "YT_0" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}
  when ( "AP_0" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}



        when ( "CGF_0" )
        {
            $trading_location_ = "10.23.182.51";
        }
        when ( "CGZ_0" )
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
	    $trading_location_ = "10.23.200.53";
	}
	when ( "FGBM_0" )
	{ 
	    if ( $date_ <= 20120605 )
	    {
		$trading_location_ = "10.23.200.53";
	    }
	    else
	    {
		$trading_location_ = "10.23.200.54";
	    }
	}
	when ( "FGBL_0" )
	{ 
	    if ( $date_ >= 20120509 )
	    {
		$trading_location_ = "10.23.200.51";
	    }
	    else
	    {
		$trading_location_ = "10.23.200.51";
	    }
	}
	when ( "FESX_0" )
	{ 
	    $trading_location_ = "10.23.200.53";
	}
	when ( "FOAT_0" )
	{ 
	    $trading_location_ = "10.23.200.51";
	}
        when ( "FOAM_0" )
        {
            $trading_location_ = "10.23.200.52";
        }
	when ( "FBTP_0" )
	{ 
	    $trading_location_ = "10.23.200.52";
	}
        when ( "FBTS_0" )
        {
            $trading_location_ = "10.23.200.52";
        }
	when ( "FDAX_0" )
	{ 
	    $trading_location_ = "10.23.200.53";
	}
        when ( "FGBX_0" ) 
        {
          $trading_location_ = "10.23.200.53"
        }
	when ( "BR_DOL_0" )
	{ 
	    $trading_location_ = "10.23.23.14";
	}
	when ( "BR_WDO_0" )
	{ 
	    $trading_location_ = "10.23.23.14";
	}
	when ( "BR_IND_0" )
	{ 
	    $trading_location_ = "10.23.23.13";
	}
	when ( "BR_WIN_0" )
	{ 
	    $trading_location_ = "10.23.23.12";
	}
	when ( "JFFCE_0" )
	{ 
	    $trading_location_ = "10.23.52.52";
	}
	when ( "KFFTI_0" )
	{ 
	    $trading_location_ = "10.23.52.52";
	}
	when ( "LFR_0" )
	{ 
	    if ( $date_ < 20120921 )
	    {
		$trading_location_ = "10.23.52.53";
	    }
	    else
	    {
		$trading_location_ = "10.23.52.51";
	    }
	}
	when ( "LFZ_0" )
	{ 
	    if ( $date_ <= 20120912 )
	    {
		$trading_location_ = "10.23.52.53";
	    }
	    else
	    {
		$trading_location_ = "10.23.52.51";
	    }
	}
	when ( "YFEBM_0" )
	{ 
	    $trading_location_ = "10.23.52.52";
	}
	when ( "YFEBM_1" )
	{ 
	    $trading_location_ = "10.23.52.52";
	}
	when ( "YFEBM_2" )
	{ 
	    $trading_location_ = "10.23.52.52";
	}
 	when ( "XFC_0" )
	{ 
	    $trading_location_ = "10.23.52.53";
	}
	when ( "XFC_1" )
	{ 
	    $trading_location_ = "10.23.52.53";
	}
	when ( "XFC_2" )
	{ 
	    $trading_location_ = "10.23.52.53";
	}
       
	when ( "LFI_0" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
		
	}
	when ( "LFI_1" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFI_2" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFI_3" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFI_4" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFI_5" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFI_6" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFI_7" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFI_8" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFI_9" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}

	}
	when ( "LFI_10" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}

	}
	when ( "LFI_11" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}

	}
	when ( "LFI_12" )
	{ 
		if ( $date_ < 20141103 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}

	when ( "LFL_0" )
	{ 
		if ( $date_ < 20141020 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFL_1" )
	{ 
		if ( $date_ < 20141020 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFL_2" )
	{ 
		if ( $date_ < 20141020 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFL_3" )
	{ 
		if ( $date_ < 20141020 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFL_4" )
	{ 
		if ( $date_ < 20141020 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFL_5" )
	{ 
		if ( $date_ < 20141020 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "LFL_6" )
	{ 
		if ( $date_ < 20141020 )
		{
			$trading_location_ = "10.23.52.53";
		}
		else
		{
			$trading_location_ = "10.23.52.51";
		}
	}
	when ( "HSI_0" )
	{ 
	    $trading_location_ = "10.152.224.145";
	}
	when ( "HHI_0" )
	{ 
	    $trading_location_ = "10.152.224.145";
	}
	when ( "MHI_0" )
	{ 
	    $trading_location_ = "10.152.224.145";
	}
	when ( "NKM_0" )
	{ 
	    $trading_location_ = "10.134.210.182";
	}
        when ( "NKMF_0" )
	{ 
	    $trading_location_ = "10.134.210.182";
	}
        when ( "TOPIX_0" )
	{ 
	    $trading_location_ = "10.134.210.182";
	}
        when ( "JGBL_0" )
	{ 
	    $trading_location_ = "10.134.210.184";
	}
	when ( "NKM_1" )
	{ 
	    $trading_location_ = "10.134.210.182";
	}
	when ( "NK_0" )
	{ 
	    $trading_location_ = "10.134.210.182";
	}
	when ( "RI_0" )
	{ 
	    $trading_location_ = "172.18.244.107";
	}
	when ( "Si_0" )
	{ 
	    $trading_location_ = "172.18.244.107";
	}
	when ( "ED_0" )
	{ 
	    $trading_location_ = "172.18.244.107";
	}
	when ( "GD_0" )
	{ 
	    $trading_location_ = "172.18.244.107";
	}
	when ( "USD000UTSTOM" )
	{ 
	    $trading_location_ = "172.18.244.107";
	}
	when ( "XT_0" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}
	when ( "YT_0" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}
	when ( "AP_0" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}
	when ( "IR_0" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}
	when ( "IR_1" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}
	when ( "IR_2" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}
	when ( "IR_3" )
	{ 
	    $trading_location_ = "10.23.43.51";
	}
	default
	{
	    $trading_location_ = "-na-";
	}
    }

    $trading_location_;
}

1
