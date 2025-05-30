# \file GenPerlLib/get_avg_trade_count_per_sec_for_shortcode.pl
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

my $DATA_DIR = "/spare/local/tradeinfo/datageninfo/";

sub GetAvgTradeCountPerSecForShortcode
{
    my $shortcode_ = shift;
    my $data_file_ = $DATA_DIR."avg_l1events_trade_per_sec";
    my $avg_trade_count_per_sec_for_shortcode_ = 0.0;
    my $found = 0;

    if(-e $data_file_)
    {
        open(DATAFILEHANDLE, $data_file_);
        while(<DATAFILEHANDLE>)
        {
            chomp;
            #print "$_\n";
            my @words = split(/ /);
            if($words[0] eq $shortcode_)
            {
                $avg_trade_count_per_sec_for_shortcode_ = $words[2];
                $found = 1;
                last;
            }
        }
        close(DATAFILEHANDLE);
    }
    
    if($found == 0)
    {
        given ( $shortcode_ )
        {
        when ( "ZT_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.074; #60 day avg
        }
        when ( "ZF_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.38; #60 day avg
        }
        when ( "ZN_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.887; #60 day avg
        }
        when ( "ZB_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.733; #60 day avg
        }
        when ( "UB_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.293; #60 day avg
        }
        when ( "6A_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.6; #60 day avg
        }
        when ( "6B_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.6; #60 day avg
        }
        when ( "6E_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.44; #60 day avg
        }
        when ( "6J_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.17; #60 day avg
        }
        when ( "6M_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.09; #60 day avg
        }
        when ( "6N_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.14; #60 day avg
        }
        when ( "6S_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.27; #60 day avg
        }
        when ( "CL_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.9;
        }
        when ( "GC_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.33;
        }
        when ( "ES_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 3.3;
        }
        when ( "YM_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.24;
        }
        when ( "NQ_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.425; #60 day avg
        }
        when ( "CGB_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.316; #60 day avg
        }
        when ( "BAX_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.001; #60 day avg
        }
        when ( "BAX_1" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.007; #60 day avg
        }
        when ( "BAX_2" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.01; #60 day avg
        }
        when ( "BAX_3" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.024; #60 day avg
        }
        when ( "BAX_4" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.01; #60 day avg
        }
        when ( "BAX_5" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.01; #60 day avg
        }
        when ( "BAX_6" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.01;
        }
        when ( "SXF_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.25; #60 day avg
        }
        when ( "FGBS_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.1389; #60 day avg
        }
        when ( "FGBM_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.397; #60 day avg
        }
        when ( "FGBL_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.2763; #60 day avg
        }
        when ( "FGBX_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.124; #60 day avg
        }
        when ( "FESX_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.08; #60 day avg
        }
        when ( "FDAX_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.27; #60 day avg
        }
        when ( "FSMI_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.224; #60 day avg
        }
        when ( "FESB_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.03; #60 day avg
        }
        when ( "FBTP_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.272; #60 day avg
        }
        when ( "FOAT_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.229; #60 day avg
        }
        when ( "FSTB_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.0136; #60 day avg
        }
        when ( "FXXP_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.01; #60 day avg
        }
        when ( "BR_DOL_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.128; #60 day avg
        }
        when ( "BR_WDO_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.05; #60 day avg
        }
        when ( "BR_IND_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.2465; #60 day avg
        }
        when ( "BR_WIN_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.131; #60 day avg
        }
        when ( "DI1F15" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.03; #60 day avg
        }
        when ( "DI1F16" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.0443056; #60 day avg
        }
        when ( "DI1F17" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.0253056; #60 day avg
        }
        when ( "DI1F18" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.0250082; #60 day avg
        }
        when ( "DI1F19" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.01;
        }
        when ( "DI1F20" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.01;
        }
        when ( "DI1N14" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.029019; #60 day avg
        }
        when ( "DI1N15" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.014;
        }
        when ( "DI1N16" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.005;
        }
        when ( "DI1N17" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.002;
        }
        when ( "DI1N18" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.001;
        }
        when ( "DI1J15" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.001;
        }
        when ( "DI1J16" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.001;
        }
        when ( "DI1J17" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.001;
        }
        when ( "JFFCE_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.8; #60 day avg
        }
        when ( "KFFTI_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.316; #60 day avg
        }
        when ( "LFR_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.33; #60 day avg
        }
        when ( "LFZ_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.66; #60 day avg
        }
        when ( "LFI_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.019; #60 day avg
        }
        when ( "LFI_1" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.0286; #60 day avg
        }
        when ( "LFI_2" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.0315; #60 day avg
        }
        when ( "LFI_3" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.02775; #60 day avg
        }
        when ( "LFI_4" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.03533; #60 day avg
        }
        when ( "LFI_5" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.03344; #60 day avg
        }
        when ( "LFL_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.0211; #60 day avg
        }
        when ( "LFL_1" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.02; #60 day avg
        }
        when ( "LFL_2" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.015; #60 day avg
        }
        when ( "LFL_3" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.022; #60 day avg
        }
        when ( "LFL_4" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.0247; #60 day avg
        }
        when ( "LFL_5" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.02422; #60 day avg
        }
        when ( "HHI_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.648; #60 day avg
        }
        when ( "HSI_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 1.08; #60 day avg
        }
        when ( "MCH_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.12; #60 day avg
        }
        when ( "MHI_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.556; #60 day avg
        }
        when ( "NKM_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 2.077; #60 day avg
        }
        when ( "NK_0" )
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.3967; #60 day avg
        }
        default
        {
            $avg_trade_count_per_sec_for_shortcode_ = 0.1;
        }
        }
    }
    return $avg_trade_count_per_sec_for_shortcode_;
}

1
