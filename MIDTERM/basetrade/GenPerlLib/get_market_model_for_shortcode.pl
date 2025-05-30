#!/usr/bin/perl
# \file GenPerlLib/get_market_model_for_shortcode.pl
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

sub GetMarketModelForShortcode
{
    my $shortcode_ = shift;
    my $market_model_index_ = 2;

    if ( index ( $shortcode_ , "DI1" ) >= 0 )
    {
	return 1;
    }

    given ( $shortcode_ )
    {
        when ( "ZT_0" )
        {
            $market_model_index_ = 1;
        }
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
	    $market_model_index_ = 2;
	}
	when ( "6A_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "6B_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "6E_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "6J_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "6M_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "6N_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "6S_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "CL_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "GC_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "NQ_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "YM_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "ES_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "CGB_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "SXF_0" )
	{ 
	    $market_model_index_ = 5;
	}
        when ( "CGF_0" )
        {
            $market_model_index_ = 3;
        }
        when ( "CGZ_0" )
        {
            $market_model_index_ = 3;
        }
	when ( "FGBS_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FBTP_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FBTS_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FSMI_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FOAT_0" )
	{ 
	    $market_model_index_ = 3;
	}
        when ( "FOAM_0" )
        {
            $market_model_index_ = 3;
        }
	when ( "FXXP_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FSTG_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FSTB_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FGBM_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "FGBL_0" )
	{ 
	    $market_model_index_ = 4;
	}
        when ( "FGBX_0" )
        {
            $market_model_index_ = 4;
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
	    $market_model_index_ = 2;
	}
	when ( "BR_WIN_0" )
	{ 
	    $market_model_index_ = 1;
	}
	when ( "JFFCE_0" )
	{
	    $market_model_index_ = 3;
	}
	when ( "KFFTI_0" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFL_0" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFL_1" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFL_2" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFL_3" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFL_4" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFL_5" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFL_6" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFZ_0" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFR_0" )
	{
	    $market_model_index_ = 3;
	}
	when ( "YFEBM_0" )
	{
	    $market_model_index_ = 3;
	}
	when ( "YFEBM_1" )
	{
	    $market_model_index_ = 3;
	}
	when ( "YFEBM_2" )
	{
	    $market_model_index_ = 3;
	}
	when ( "XFC_0" )
	{
	    $market_model_index_ = 3;
	}
	when ( "XFC_1" )
	{
	    $market_model_index_ = 3;
	}
	when ( "XFRC_0" )
	{
	    $market_model_index_ = 3;
	}
	when ( "XFRC_1" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_0" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_1" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_2" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_3" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_4" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_5" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_6" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_7" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_8" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_9" )
	{
	    $market_model_index_ = 3;
	}
	when ( "LFI_10" )
	{
	    $market_model_index_ = 3;
	}
        when ( "LFI_11" )
        {
            $market_model_index_ = 3;
        }
        when ( "LFI_12" )
        {
            $market_model_index_ = 3;
        }
	when ( "LFI06" )
	{
	    $market_model_index_ = 3;
	}
	when ( "NK_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "NKM_0" )
	{ 
	    $market_model_index_ = 3;
	}
	when ( "NKM_1" )
	{ 
	    $market_model_index_ = 3;
	}
	default
	{
	    $market_model_index_ = 2;
	}
    }

    $market_model_index_;
}

if ( $0 eq __FILE__ ) {
    print GetMarketModelForShortcode ( $ARGV[0] );
}

1
