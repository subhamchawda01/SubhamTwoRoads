#!/usr/bin/perl

# \file ModelScripts/t_make_base_model.pl
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#      Suite 217, Level 2, Prestige Omega,
#      No 104, EPIP Zone, Whitefield,
#      Bangalore - 560066, India
#      +91 80 4060 0717
#
# This script takes a product indicator_corr_file start_date end_date

use strict;
use warnings;
use feature "switch"; # for given, when
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/get_default_timeout_values_for_shortcode.pl"; # GetDefaultTimeoutValuesForShortcode
require "$GENPERLLIB_DIR/get_avg_event_count_per_sec_for_shortcode.pl"; # GetAvgEventCountPerSecForShortcode
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

# start 
my $SCRIPTNAME="$0";
my $USAGE="$0 product indicator_corr_file corr_series_start_date corr_series_end_date start_time_ end_time_ datagen_trading_date_ pred_duration_ norm_algo_ min_corr ";

if ( scalar ( @ARGV ) < 10 )
{
    print $USAGE ,"\n";
    exit ( 0 ) ;
}

my $prod_ = $ARGV [ 0 ];
my $indicator_corr_file_ = $ARGV [ 1 ];
my $start_date_ = $ARGV [ 2 ];
my $end_date_ = $ARGV [ 3 ];
my $start_time_ = $ARGV [ 4 ];
my $end_time_ = $ARGV [ 5 ];

my $datagen_trading_date_ = GetIsoDateFromStrMin1 ( $ARGV [ 6 ] );

my $pred_time_ = $ARGV [ 7 ];
my $norm_algo_ = $ARGV [ 8 ];
my $min_corr_ = $ARGV [ 9 ];

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

my $sample_using_ = "EVT";
if ( $#ARGV  > 9 )
{
    $sample_using_ = $ARGV [ 10 ] ;
}

my $msecs_print_ = 1000 ;
my $l1events_print_ = 10 ;
my $num_trades_ = 0 ;
my $eco_mode_ = 0 ;

{
    ( $msecs_print_, $l1events_print_ ) = GetDefaultTimeoutValuesForShortcode ( $prod_ );
    $l1events_print_ = GetAvgEventCountPerSecForShortcode ( $prod_, $start_time_, $end_time_, $datagen_trading_date_ ) / 2.0;
    if ( $l1events_print_ < 1 )
    { # very slow products
	$msecs_print_ = max ( $msecs_print_, (1000/$l1events_print_) );
	$msecs_print_ = min ( 60000, $msecs_print_ );
    }
}

if ( $sample_using_ eq "EVT2" ) 
{
    $l1events_print_ = "c1" ;  # msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
}
elsif ( $sample_using_ eq "EVT3" )
{
    $l1events_print_ = "c2" ;  # msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
}
elsif ( $sample_using_ eq "EVT4" )
{
    $l1events_print_ = "c3" ;  # msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
}

my $work_dir_ = "/spare/local/$USER/MBM/$prod_/$unique_gsm_id_" ;
`mkdir -p $work_dir_` ;



my $step_1_exec_ = "$BIN_DIR/make_indicator_list_extended";
my $step_2_exec_ = "$MODELSCRIPTS_DIR/remove_correlated_indicators.pl";
my $step_3_exec_ = "$BIN_DIR/datagen";
my $step_4_exec_ = "$BIN_DIR/timed_data_to_reg_data";
my $step_5_exec_ = "$BIN_DIR/callFSLR";
my $step_6_exec_ = "$MODELSCRIPTS_DIR/place_coeffs_in_model.pl";


# step 1 ###
# Usage: make_indicator_list_extended SHORTCODE INDICATOR_RECORD_FILE DEPBASE DEPPRED START_YYYYMMDD END_YYMMDD MAX_CORR_THRESH -m MAX_NO -a <algorithm for comparision(0,1,2,3,4,5,6)> -i [definately choose nth (0,1..]
my $step1_common_ = "$step_1_exec_ $prod_ $indicator_corr_file_ MktSizeWPrice MktSizeWPrice $start_date_ $end_date_ 1.5 -m 200";
my $cmd_10_ = "$step1_common_ -a 0 > $work_dir_/step1.median.o" ;
my $cmd_11_ = "$step1_common_ -a 1 > $work_dir_/step1.mean.o" ;
my $cmd_12_ = "$step1_common_ -a 2 > $work_dir_/step1.sharpe.o" ;
my $cmd_13_ = "$step1_common_ -a 3 > $work_dir_/step1.mrs.o" ;
my $cmd_14_ = "$step1_common_ -a 4 > $work_dir_/step1.mxo.o" ;
my $cmd_15_ = "$step1_common_ -a 5 > $work_dir_/step1.srm.o" ;
my $cmd_16_ = "$step1_common_ -a 6 > $work_dir_/step1.srs.o" ;

`$cmd_10_` ; `$cmd_11_` ; `$cmd_12_`; `$cmd_13_`; `$cmd_14_`; `$cmd_15_`; `$cmd_16_`;

# step 2 ###
# Usage: remove_correlated_indicators.pl shortcode=na_shc modelfile cross_correlation_threshold max_num startdate enddate starthhmm endhhmm
my $cmd_20_ = "$step_2_exec_ $prod_ $work_dir_/step1.median.o 0.8 50 TODAY-3 TODAY-2 $start_time_ $end_time_ > $work_dir_/step2.median.ro" ;
my $cmd_21_ = "$step_2_exec_ $prod_ $work_dir_/step1.mean.o 0.8 50 TODAY-3 TODAY-2 $start_time_ $end_time_ > $work_dir_/step2.mean.ro" ;
my $cmd_22_ = "$step_2_exec_ $prod_ $work_dir_/step1.sharpe.o 0.8 50 TODAY-3 TODAY-2 $start_time_ $end_time_ > $work_dir_/step2.sharpe.ro" ;
my $cmd_23_ = "$step_2_exec_ $prod_ $work_dir_/step1.mrs.o 0.8 50 TODAY-3 TODAY-2 $start_time_ $end_time_ > $work_dir_/step2.mrs.ro" ;
my $cmd_24_ = "$step_2_exec_ $prod_ $work_dir_/step1.mxo.o 0.8 50 TODAY-3 TODAY-2 $start_time_ $end_time_ > $work_dir_/step2.mxo.ro" ;
my $cmd_25_ = "$step_2_exec_ $prod_ $work_dir_/step1.srm.o 0.8 50 TODAY-3 TODAY-2 $start_time_ $end_time_ > $work_dir_/step2.srm.ro" ;
my $cmd_26_ = "$step_2_exec_ $prod_ $work_dir_/step1.srs.o 0.8 50 TODAY-3 TODAY-2 $start_time_ $end_time_ > $work_dir_/step2.srs.ro" ;

`$cmd_20_` ; `$cmd_21_` ; `$cmd_22_`; `$cmd_23_`; `$cmd_24_`; `$cmd_25_`; `$cmd_26_`;

# step 3 ###
# Usage: datagen INDICATORLISTFILENAME TRADINGDATE UTC_STARTHHMM UTC_ENDHHMM PROGID OUTPUTFILENAME MSECS_PRINT l1EVENTS_PRINT NUM_TRADES_PRINT ECO_MODE
my $cmd_30_ = "$step_3_exec_ $work_dir_/step2.median.ro $datagen_trading_date_ $start_time_ $end_time_ 228971 $work_dir_/step3.median.dout $msecs_print_ $l1events_print_ $num_trades_ $eco_mode_";
my $cmd_31_ = "$step_3_exec_ $work_dir_/step2.mean.ro $datagen_trading_date_ $start_time_ $end_time_ 228971 $work_dir_/step3.mean.dout $msecs_print_ $l1events_print_ $num_trades_ $eco_mode_";
my $cmd_32_ = "$step_3_exec_ $work_dir_/step2.sharpe.ro $datagen_trading_date_ $start_time_ $end_time_ 228971 $work_dir_/step3.sharpe.dout $msecs_print_ $l1events_print_ $num_trades_ $eco_mode_";
my $cmd_33_ = "$step_3_exec_ $work_dir_/step2.mrs.ro $datagen_trading_date_ $start_time_ $end_time_ 228971 $work_dir_/step3.mrs.dout $msecs_print_ $l1events_print_ $num_trades_ $eco_mode_";
my $cmd_34_ = "$step_3_exec_ $work_dir_/step2.mxo.ro $datagen_trading_date_ $start_time_ $end_time_ 228971 $work_dir_/step3.mxo.dout $msecs_print_ $l1events_print_ $num_trades_ $eco_mode_";
my $cmd_35_ = "$step_3_exec_ $work_dir_/step2.srm.ro $datagen_trading_date_ $start_time_ $end_time_ 228971 $work_dir_/step3.srm.dout $msecs_print_ $l1events_print_ $num_trades_ $eco_mode_";
my $cmd_36_ = "$step_3_exec_ $work_dir_/step2.srs.ro $datagen_trading_date_ $start_time_ $end_time_ 228971 $work_dir_/step3.srs.dout $msecs_print_ $l1events_print_ $num_trades_ $eco_mode_";

`$cmd_30_` ; `$cmd_31_` ; `$cmd_32_`; `$cmd_33_`; `$cmd_34_`; `$cmd_35_`; `$cmd_36_`;

# step 4 ###
# Usage: timed_data_to_reg_data MODELFILENAME INPUTDATAFILENAME MSECS/EVENTS_TO_PREDICT NORMALIZING_ALGO OUTPUTDATAFILENAME <FILTER>?
my $cmd_40_ = "$step_4_exec_ $work_dir_/step2.median.ro $work_dir_/step3.median.dout $pred_time_ $norm_algo_ $work_dir_/step4.median.td";
my $cmd_41_ = "$step_4_exec_ $work_dir_/step2.mean.ro $work_dir_/step3.mean.dout $pred_time_ $norm_algo_ $work_dir_/step4.mean.td";
my $cmd_42_ = "$step_4_exec_ $work_dir_/step2.sharpe.ro $work_dir_/step3.sharpe.dout $pred_time_ $norm_algo_ $work_dir_/step4.sharpe.td";
my $cmd_43_ = "$step_4_exec_ $work_dir_/step2.mrs.ro $work_dir_/step3.mrs.dout $pred_time_ $norm_algo_ $work_dir_/step4.mrs.td";
my $cmd_44_ = "$step_4_exec_ $work_dir_/step2.mxo.ro $work_dir_/step3.mxo.dout $pred_time_ $norm_algo_ $work_dir_/step4.mxo.td";
my $cmd_45_ = "$step_4_exec_ $work_dir_/step2.srm.ro $work_dir_/step3.srm.dout $pred_time_ $norm_algo_ $work_dir_/step4.srm.td";
my $cmd_46_ = "$step_4_exec_ $work_dir_/step2.srs.ro $work_dir_/step3.srs.dout $pred_time_ $norm_algo_ $work_dir_/step4.srs.td";

`$cmd_40_` ; `$cmd_41_` ; `$cmd_42_`; `$cmd_43_`; `$cmd_44_`; `$cmd_45_`; `$cmd_46_`;

# step 5 ###
# Usage: callFSLR input_reg_data_file_name  min_correlation  first_indep_is_weight  mult_include_first_k_independants  max_indep_correlation regression_output_filename  max_model_size avoid_sharpe_check_filename [indicator_correlations_] 
my $cmd_50_ = "$step_5_exec_ $work_dir_/step4.median.td $min_corr_ 0 0 0.8 $work_dir_/step5.median.rgout 20";
my $cmd_51_ = "$step_5_exec_ $work_dir_/step4.mean.td $min_corr_ 0 0 0.8 $work_dir_/step5.mean.rgout 20";
my $cmd_52_ = "$step_5_exec_ $work_dir_/step4.sharpe.td $min_corr_ 0 0 0.8 $work_dir_/step5.sharpe.rgout 20";
my $cmd_53_ = "$step_5_exec_ $work_dir_/step4.mrs.td $min_corr_ 0 0 0.8 $work_dir_/step5.mrs.rgout 20";
my $cmd_54_ = "$step_5_exec_ $work_dir_/step4.mxo.td $min_corr_ 0 0 0.8 $work_dir_/step5.mxo.rgout 20";
my $cmd_55_ = "$step_5_exec_ $work_dir_/step4.srm.td $min_corr_ 0 0 0.8 $work_dir_/step5.srm.rgout 20";
my $cmd_56_ = "$step_5_exec_ $work_dir_/step4.srs.td $min_corr_ 0 0 0.8 $work_dir_/step5.srs.rgout 20";

`$cmd_50_` ; `$cmd_51_` ; `$cmd_52_`; `$cmd_53_`; `$cmd_54_`; `$cmd_55_`; `$cmd_56_`;

# step 6 ###
# Usage: place_coeffs_in_model.pl  output_model_filename  indicator_list_filename  regression_output_filename
my $cmd_60_ = "$step_6_exec_ $work_dir_/step6.median.model $work_dir_/step2.median.ro $work_dir_/step5.median.rgout 20";
my $cmd_61_ = "$step_6_exec_ $work_dir_/step6.mean.model $work_dir_/step2.mean.ro $work_dir_/step5.mean.rgout 20";
my $cmd_62_ = "$step_6_exec_ $work_dir_/step6.sharpe.model $work_dir_/step2.sharpe.ro $work_dir_/step5.sharpe.rgout 20";
my $cmd_63_ = "$step_6_exec_ $work_dir_/step6.mrs.model $work_dir_/step2.mrs.ro $work_dir_/step5.mrs.rgout 20";
my $cmd_64_ = "$step_6_exec_ $work_dir_/step6.mxo.model $work_dir_/step2.mxo.ro $work_dir_/step5.mxo.rgout 20";
my $cmd_65_ = "$step_6_exec_ $work_dir_/step6.srm.model $work_dir_/step2.srm.ro $work_dir_/step5.srm.rgout 20";
my $cmd_66_ = "$step_6_exec_ $work_dir_/step6.srs.model $work_dir_/step2.srs.ro $work_dir_/step5.srs.rgout 20";

`$cmd_60_` ; `$cmd_61_` ; `$cmd_62_`; `$cmd_63_`; `$cmd_64_`; `$cmd_65_`; `$cmd_66_`;

