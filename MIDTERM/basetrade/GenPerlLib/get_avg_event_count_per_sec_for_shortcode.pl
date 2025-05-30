# \file GenPerlLib/get_avg_event_count_per_sec_for_shortcode.pl
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
use List::Util qw/max min/; # for max

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $script_ = $HOME_DIR."/".$REPO."_install/scripts/get_avg_samples.pl";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

sub GetAvgEventCountPerSecForShortcode
{
    my $start_time_ = 0000;
    my $end_time_ = 2350;
    my $end_date_ = GetIsoDateFromStrMin1("TODAY-1");
    my $look_back_period_ = 60;
    my $shortcode_ = "";

    $shortcode_ = shift;
    $start_time_ = shift || 0000;
    $end_time_ = shift || 2350;
    $end_date_ = shift || $end_date_;
    $look_back_period_ = shift || 60;

    my $found = 0;

    my $avg_event_count_per_sec_for_shortcode_ = 0.0;

    my $cmd_ = $script_." ".$shortcode_." ".$end_date_." ".$look_back_period_." ".$start_time_." ".$end_time_." 0 L1EVPerSec | awk '{ print \$3}'";
#    print $cmd_."\n";

    my $t_count_ = `$cmd_ 2>/dev/null`;
    chomp($t_count_);
    if($t_count_ > 0) {
	$found = 1;
	$avg_event_count_per_sec_for_shortcode_ = $t_count_;
    }

    if($found == 0)
    {
        given ( $shortcode_ )
        {
        when ( "ZT_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 4.00485; #60 day avg
        }
        when ( "ZF_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 6.31922; #60 day avg
        }
        when ( "ZN_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 10.1389; #60 day avg
        }
        when ( "ZB_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 9.07821; #60 day avg
        }
        when ( "UB_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 6.84867; #60 day avg
        }
        when ( "6A_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 15.4242; #60 day avg
        }
        when ( "6B_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 11.91423; #60 day avg
        }
        when ( "6E_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 20.6302; #60 day avg
        }
        when ( "6J_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 10.1647; #60 day avg
        }
        when ( "6M_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 4.50926; #60 day avg
        }
        when ( "6N_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 5.36794; #60 day avg
        }
        when ( "6S_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 7.92053; #60 day avg
        }
        when ( "CL_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 16.9915;
        }
        when ( "GC_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 8.37337;
        }
        when ( "ES_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 60.4811;
        }
        when ( "YM_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 22.0694;
        }
        when ( "NQ_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 21.6361; #60 day avg
        }
        when ( "CGB_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 3.59307; #60 day avg
        }
        when ( "BAX_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.0128449; #60 day avg
        }
        when ( "BAX_1" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.0496632; #60 day avg
        }
        when ( "BAX_2" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.0817818; #60 day avg
        }
        when ( "BAX_3" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.129788; #60 day avg
        }
        when ( "BAX_4" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.102787; #60 day avg
        }
        when ( "BAX_5" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.0854578; #60 day avg
        }
        when ( "BAX_6" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.0852778;
        }
        when ( "SXF_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 2.12013; #60 day avg
        }
        when ( "FGBS_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 2.12775; #60 day avg
        }
        when ( "FGBM_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 6.86722; #60 day avg
        }
        when ( "FGBL_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 11.0765; #60 day avg
        }
        when ( "FESX_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 10.9694; #60 day avg
        }
        when ( "FDAX_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 7.95523; #60 day avg
        }
        when ( "FSMI_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 2.18929; #60 day avg
        }
        when ( "FESB_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.645943; #60 day avg
        }
        when ( "FBTP_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 1.98775; #60 day avg
        }
        when ( "FOAT_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 1.34712; #60 day avg
        }
        when ( "FSTB_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.61272; #60 day avg
        }
        when ( "FXXP_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.488234; #60 day avg
        }
        when ( "BR_DOL_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 1.32626; #60 day avg
        }
        when ( "BR_WDO_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.615229; #60 day avg
        }
        when ( "BR_IND_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 3.62897; #60 day avg
        }
        when ( "BR_WIN_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 15.6171; #60 day avg
        }
        when ( "DI1F15" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.139975; #60 day avg
        }
        when ( "DI1F16" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.323612; #60 day avg
        }
        when ( "DI1F17" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.315533; #60 day avg
        }
        when ( "DI1F18" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.350082; #60 day avg
        }
        when ( "DI1F19" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.169965;
        }
        when ( "DI1F20" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.0386458;
        }
        when ( "DI1J15" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.0177778;
        }
        when ( "DI1J16" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.0801389;
        }
        when ( "DI1J17" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.00256944;
        }
        when ( "DI1N14" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.029019; #60 day avg
        }
        when ( "DI1N15" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.0822917;
        }
        when ( "DI1N16" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.000451389;
        }
        when ( "DI1N17" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.000972222;
        }
        when ( "DI1N18" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.000555556;
        }
        when ( "JFFCE_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 14.95637; #60 day avg
        }
        when ( "KFFTI_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 7.24249; #60 day avg
        }
        when ( "LFR_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 9.09025; #60 day avg
        }
        when ( "LFZ_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 11.66724; #60 day avg
        }
        when ( "LFI_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.281014; #60 day avg
        }
        when ( "LFI_1" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.388848; #60 day avg
        }
        when ( "LFI_2" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.473755; #60 day avg
        }
        when ( "LFI_3" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.655642; #60 day avg
        }
        when ( "LFI_4" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.835903; #60 day avg
        }
        when ( "LFI_5" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.873564; #60 day avg
        }
        when ( "LFL_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.1030503; #60 day avg
        }
        when ( "LFL_1" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.171704; #60 day avg
        }
        when ( "LFL_2" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.194253; #60 day avg
        }
        when ( "LFL_3" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.214795; #60 day avg
        }
        when ( "LFL_4" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.308222; #60 day avg
        }
        when ( "LFL_5" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.264263; #60 day avg
        }
        when ( "HHI_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 6.18269; #60 day avg
        }
        when ( "HSI_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 7.55141; #60 day avg
        }
        when ( "MCH_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 1.82985; #60 day avg
        }
        when ( "MHI_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 6.56788; #60 day avg
        }
        when ( "NKM_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 7.5319; #60 day avg
        }
        when ( "NK_0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 3.90856; #60 day avg
        }
	when ( "NSE_USDINR_FUT0" )
	{
            $avg_event_count_per_sec_for_shortcode_ = 3.917115; #60 day avg   
	}
        when ( "NSE_GIND10YR_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.322660; #60 day avg   
        }
        when ( "NSE_NIFTY_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 16.184289; #60 day avg   
        }
	when ( "NSE_EURINR_FUT0" )
	{
            $avg_event_count_per_sec_for_shortcode_ = 3.147496; #60 day avg   
	}
        when ( "NSE_GBPINR_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 2.789125; #60 day avg   
        }
        when ( "NSE_JPYINR_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 2.346402; #60 day avg   
        }
        when ( "NSE_BANKNIFTY_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 30.645905; #60 day avg   
        }
	when ( "NSE_USDINR_FUT1" )
	{
            $avg_event_count_per_sec_for_shortcode_ = 4.539161; #60 day avg   
	}
        when ( "NSE_GIND10YR_FUT1" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 1.088793; #60 day avg   
        }
        when ( "NSE_NIFTY_FUT1" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 24.488180; #60 day avg   
        }
	when ( "NSE_EURINR_FUT1" )
	{
            $avg_event_count_per_sec_for_shortcode_ = 3.256847; #60 day avg   
	}
        when ( "NSE_GBPINR_FUT1" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 3.204335; #60 day avg   
        }
        when ( "NSE_JPYINR_FUT1" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 2.9555174; #60 day avg   
        }
        when ( "NSE_BANKNIFTY_FUT1" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 72.908966; #60 day avg   
        }
	when ( "NSE_USDINR_FUT2" )
	{
            $avg_event_count_per_sec_for_shortcode_ = 2.566573; #60 day avg   
	}
        when ( "NSE_GIND10YR_FUT2" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 3.773781; #60 day avg   
        }
        when ( "NSE_NIFTY_FUT2" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 13.932852; #60 day avg   
        }
	when ( "NSE_EURINR_FUT2" )
	{
            $avg_event_count_per_sec_for_shortcode_ = 3.447714; #60 day avg   
	}
        when ( "NSE_GBPINR_FUT2" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 2.497744; #60 day avg   
        }
        when ( "NSE_JPYINR_FUT2" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 3.024922; #60 day avg   
        }
        when ( "NSE_BANKNIFTY_FUT2" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 52.647927; #60 day avg   
        }
        when ( "NSE_HDFCBANK_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 25.443146;
        }
        when ( "NSE_INFY_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 19.560563;
        }
        when ( "NSE_HDFC_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 27.736227;
        }
        when ( "NSE_ITC_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 6.950647;
        }
        when ( "NSE_ICICIBANK_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 13.395060;
        }
        when ( "NSE_RELIANCE_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 14.870039;
        }
        when ( "NSE_TCS_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 24.106969;
        }
        when ( "NSE_LT_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 25.593792;
        }
        when ( "NSE_SUNPHARMA_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 18.348451;
        }
        when ( "NSE_AXISBANK_FUT0" )
        {
            $avg_event_count_per_sec_for_shortcode_ = 11.754320;
        }
        when ( "NSE_BANKBARODA_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 15.797593;
        }
        when ( "NSE_BANKINDIA_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 5.941379;
        }
        when ( "NSE_KOTAKBANK_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 39.975302;
        }
        when ( "NSE_CANBK_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 10.087287;
        }
        when ( "NSE_FEDERALBNK_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 2.063804;
        }
        when ( "NSE_INDUSINDBK_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 19.074651;
        }
        when ( "NSE_PNB_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 5.191543;
        }
        when ( "NSE_SBIN_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 8.705822;
        }
        when ( "NSE_YESBANK_FUT0" )
        { 
             $avg_event_count_per_sec_for_shortcode_ = 23.797696;
        }
        when ( "NSE_ASIANPAINT_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 21.826985;
        }
        when ( "NSE_CAIRN_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 7.015170;
        }
        when ( "NSE_GAIL_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 11.468498;
        }
        when ( "NSE_GRASIM_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 13.142483;
        }
        when ( "NSE_HINDALCO_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 2.758237;
        }
        when ( "NSE_PNB_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 3.730811;
        }
        when ( "NSE_TATAPOWER_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 0.648779;
        }
        when ( "NSE_TATASTEEL_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 8.077794;
        }
        when ( "NSE_VEDL_FUT0" )
        {
             $avg_event_count_per_sec_for_shortcode_ = 2.788849;
        }
        default
        {
            $avg_event_count_per_sec_for_shortcode_ = 0.5;
        }
        }
    }

    return $avg_event_count_per_sec_for_shortcode_;
}

1
