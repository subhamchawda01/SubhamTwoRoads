# \file GenPerlLib/get_avg_event_count_per_sec_for_shortcode.pl
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

sub GetAvgEventCountPerSecForShortcode
{
    my $shortcode_ = shift;

    my $avg_event_count_per_sec_for_shortcode_ = 0.0;

    # TODO : Have these be read from a file ,
    #        and update the file periodically.
    given ( $shortcode_ )
    {
	when ( "ZF_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 9.3939593; # 45 days
	}
	when ( "ZN_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 15.5523731; # 45 days
	}
	when ( "ZB_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 12.389031; # 45 days
	}
	when ( "UB_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 7.0916646; # 45 days
	}
	when ( "ZT_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 1.3333554; # 45 days
	}
	when ( "6A_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 26.82516; # 45 days
	}
	when ( "6B_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 17.218256; # 45 days
	}
	when ( "6E_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 31.97166; # 45 days
	}
	when ( "6J_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 10.503270; # 45 days
	}
	when ( "6M_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 6.2466485; # 45 days
	}
	when ( "6N_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 11.5716304; # 45 days
	}
	when ( "6S_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 14.863236; # 45 days
	}
	when ( "NQ_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 27.128719; # 45 days
	}
	when ( "CGB_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 5.84139492; # 45 days
	}
	when ( "BAX_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 0.01367753; # 45 days
	}
	when ( "BAX_1" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 0.13982487; # 45 days
	}
	when ( "BAX_2" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 0.24104669; #45 days
	}
	when ( "BAX_3" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 0.23175724; #45 days
	}
	when ( "BAX_4" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 0.21777375; #45 days
	}
	when ( "BAX_5" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 0.167155797; #45 days
	}
	when ( "SXF_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 2.262650; #45 days
	}
	when ( "FGBS_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 2.95835950; #45 days
	}
	when ( "FGBM_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 11.05267109; #45 days
	}
	when ( "FGBL_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 16.980638; #45 days
	}
	when ( "FESX_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 17.1509541; #45 days
	}
	when ( "FDAX_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 15.277014; #45 days
	}
	when ( "FSMI_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 6.17313405; #45 days
	}
	when ( "FESB_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 0.9229367; #45 days
	}
	when ( "FBTP_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 0.78247987; #45 days
	}
	when ( "FOAT_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 0.55052737; #45 days
	}
	when ( "FSTB_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.8823711; #45 days
	}
	when ( "FXXP_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.9115297; #45 days
	}
	when ( "BR_DOL_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 2.9215056; # 45 days
	}
	when ( "BR_WDO_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 1.03491143; #45 days
	}
	when ( "BR_IND_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 4.17650563; #45 days
	}
	when ( "BR_WIN_0" )
	{ 
	    $avg_event_count_per_sec_for_shortcode_ = 14.5355958; #45 days
	}
	when ( "DI1F16" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.2153985; #45 days
	}
	when ( "DI1F15" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.02161634; #45 days
	}
	when ( "DI1F19" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.21738727; #45 days
	}
	when ( "DI1F17" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.22785628; #45 days
	}
	when ( "DI1N14" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.076634460; #45 days
	}
	when ( "DI1F18" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.1958494; #45 days
	}
	when ( "JFFCE_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 19.4291404; #45 days
	}
	when ( "KFFTI_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 11.6938425; #45 days
	}
	when ( "LFR_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 9.3277455; #45 days
	}
	when ( "LFZ_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 13.645442; #45 days
	}
	when ( "LFI_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.3220289; #45 days
	}
	when ( "LFI_1" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.4700161; #45 days
	}
	when ( "LFI_2" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.6015378; #45 days
	}
	when ( "LFI_3" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.747870370; #45 days
	}
	when ( "LFI_4" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.90598027; #45 days
	}
	when ( "LFI_5" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.90069243; #45 days
	}
	when ( "LFL_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.15198671; #45 days
	}
	when ( "LFL_1" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.264816827; #45 days
	}
	when ( "LFL_2" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.286417069; #45 days
	}
	when ( "LFL_3" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.342375201; #45 days
	}
	when ( "LFL_4" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.3800905; #45 days
	}
	when ( "LFL_5" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.31317028; #45 days
	}
	when ( "HHI_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 6.89500;
	}
	when ( "HSI_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 8.4174629;
	}
	when ( "MCH_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 1.95288;
	}
	when ( "MHI_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 9.503907;
	}
	when ( "NKM_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 4.719234;
	}
	when ( "NK_0" )
	{
	    $avg_event_count_per_sec_for_shortcode_ = 12.956438;
	}
	default
	{
	    $avg_event_count_per_sec_for_shortcode_ = 0.5;
	}
    }

    return $avg_event_count_per_sec_for_shortcode_;
}

1
