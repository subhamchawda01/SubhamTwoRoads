#!/usr/bin/perl

# \file ModelScripts/pick_strats_and_show_simresults_for_date.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use Math::Complex; # sqrt
use FileHandle;

# TODO : Figure out how to factor in diversity weights.

sub LoadConfigFile;
sub SanityCheckConfigParams;

sub GetRealResults;
sub GetGlobalResults;

sub ComputeSimRealBias;
sub CompensateSimRealBias;

sub SortResultsByAlgo;
sub ScoreStrats;

sub PickStrats;

sub DiversifyAndScore;

sub ComputeOptimalMaxLosses;
sub CheckResetMaxLosses;

sub InstallProductionStratList;
sub RemoveIntermediateFiles;

sub IsTooInsample;
sub GetBaseStratNameFromPickedStratsList;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $LOCAL_PRODUCTION_STRAT_LIST_DIR = $HOME_DIR."/production_strat_list";
my $local_production_strat_list_file_ = "";

my $REMOTE_PRODUCTION_STRAT_LIST_DIR = "/home/dvctrader/production_strat_list";
my $remote_production_strat_list_file_ = "";

my $SIM_REAL_BIAS_DIR_ = "/spare/local/simrealbias";

my @intermediate_files_ = ( );

require "$GENPERLLIB_DIR/sqrt_sign.pl"; # SqrtSign
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr

require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams

#require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD CONFIGFILE DATE";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $mail_body_ = "";

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];

my $config_file_ = $ARGV [ 2 ];
my $date_to_run_for_ = $ARGV [ 3 ];

my $exchange_ = "";

my %strat_name_to_subset_index_ = ( );

my $sum_num_strats_to_install_ = 0;

my @num_strats_to_install_ = ( );
my $num_strat_files_to_install_ = 0;
my @total_size_to_run_ = ( );
my @size_per_strat_ = ( );
my $max_loss_per_unit_size_ = 0;
my @use_optimal_max_loss_per_unit_size_ = ( );
my $min_max_loss_per_unit_size_ = -1;
my $max_max_loss_per_unit_size_ = -1;
my $opentrade_loss_per_unit_size_ = 0;
my $short_term_loss_per_unit_size_ = 0;
my @min_volume_per_strat_ = ( );
my @volume_cut_off_ratio_ = ( );
my @ttc_cut_off_ratio_ = ( );
my @strats_to_keep_ = ( );
my @strats_to_exclude_ = ( );

my $intervals_to_compare_ = 0;
my %intervals_to_pick_from_ = ( );
my $longest_interval_ = 0;
my %diversity_type_to_weight_ = ( );

my @sort_algo_ = ( );
my $email_address_ = "";

my $install_picked_strats_ = 0;
my $install_location_ = "";
my $prod_query_start_id_ = 0;
my $exec_start_hhmm_ = 0;
my $exec_end_hhmm_ = 0;
my $onload_trade_exec_ = 0;
my $vma_trade_exec_ = 0;
my $affinity_trade_exec_ = 0;
my $use_sharpe_pnl_check_ = 0;

my $compensate_sim_real_bias_ = 0;
my $sim_real_bias_compensation_ = 0.5;

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
$yyyymmdd_ = $date_to_run_for_; 
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my @remote_strat_full_path_list_ = ( );

my @skip_dates_vector_ = ( );
my $skip_dates_file_ = "/spare/local/".$USER."/skip_dates_file_".$shortcode_ ;

LoadConfigFile ( $config_file_ );
SanityCheckConfigParams ( );

# A single config file now contains multiple sets of configs within it.
# This lets us do something like :
# Pick 1/3rd using ultra-strict TTC cutoffs
# Pick 1/3rd which is higher in risk
# Pick 1/3rd average strats
# etc.

my @picked_strats_ = ( );

my @all_strats_ = ( );
my %interval_to_real_results_ = ( );

my %interval_to_global_results_ = ( );

my %strat_name_to_sim_real_bias_ppc_ = ( );

my %interval_to_bias_adjusted_global_results_ = ( );

my %interval_to_strat_to_bias_ = ( );

my %strat_name_to_global_result_score_ = ( );

my %strat_name_to_diversity_adjusted_score_ = ( );
my %strat_name_to_is_diversity_adjusted_ = ( );

my @algo_sorted_strat_names_ = ( );
my @algo_sorted_strat_results_ = ( );

for ( my $t_subset_index_ = 0 ; $t_subset_index_ <= $#num_strats_to_install_ ; $t_subset_index_ ++ )
{
    @all_strats_ = ( );
    %interval_to_real_results_ = ( );
    # GetRealResults ( );

    %interval_to_global_results_ = ( );
    GetGlobalResults ( $t_subset_index_ );

    # Sim-real bias per contract.
    # -ve => Sim pessimistic
    # +ve => Sim optimistic
    %strat_name_to_sim_real_bias_ppc_ = ( );
    #ComputeSimRealBias ( );

    %interval_to_bias_adjusted_global_results_ = ( );
    #CompensateSimRealBias ( );

    %interval_to_strat_to_bias_ = ( );
    ComputePerIntervalSimRealBias ( $t_subset_index_ );
    CompensatePerIntervalSimRealBias ( $t_subset_index_ );

    %strat_name_to_global_result_score_ = ( );

    %strat_name_to_diversity_adjusted_score_ = ( );
    %strat_name_to_is_diversity_adjusted_ = ( );

    @algo_sorted_strat_names_ = ( );
    @algo_sorted_strat_results_ = ( );
    ScoreStrats ( $t_subset_index_ );

    PickStrats ( $t_subset_index_ );
    
    ShowResultsForDate () ;
}

my %strat_name_to_optimal_max_loss_ = ( );
ComputeOptimalMaxLosses ( );

CheckResetMaxLosses ( );

RemoveIntermediateFiles ( );

exit ( 0 );

sub LoadConfigFile
{
    my ( $t_config_file_ ) = @_;

    open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
    my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
    close ( CONFIG_FILE );

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_." > CONFIG_FILE=".$t_config_file_."\n";

    my $current_param_ = "";
    foreach my $config_file_lines_ ( @config_file_lines_ )
    {
	if ( index ( $config_file_lines_ , "#" ) == 0 ) # not ignoring lines with # not at the beginning
	{
	    next;
	}

	my @t_words_ = split ( ' ' , $config_file_lines_ );

	if ( $#t_words_ < 0 )
	{
	    $current_param_ = "";
	    next;
	}

	if ( ! $current_param_ )
	{
	    $current_param_ = $t_words_ [ 0 ];
	    next;
	}
	else
	{
	    given ( $current_param_ )
	    {
		when ( "SHORTCODE" )
		{
		    my $t_shortcode_ = $t_words_ [ 0 ];
		    if ( $t_shortcode_ ne $shortcode_ )
		    {
			PrintStacktraceAndDie ( "$t_shortcode_ in config file != $shortcode_" );
		    }
		    $mail_body_ = $mail_body_." \t > SHORTCODE=".$t_shortcode_."\n";
		}

		when ( "EXCHANGE" )
		{
		    $exchange_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > EXCHANGE=".$exchange_."\n";
		}

		when ( "NUM_STRATS_TO_INSTALL" )
		{
		    my $t_num_strats_to_install_ = $t_words_ [ 0 ];
		    push ( @num_strats_to_install_ , $t_num_strats_to_install_ );

		    $mail_body_ = $mail_body_." \t > NUM_STRATS_TO_INSTALL=".$t_num_strats_to_install_."\n";
		}

		when ( "NUM_STRAT_FILES_TO_INSTALL" )
		{
		    $num_strat_files_to_install_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > NUM_STRAT_FILES_TO_INSTALL=".$num_strat_files_to_install_."\n";
		}

		when ( "TOTAL_SIZE_TO_RUN" )
		{
		    my $t_total_size_to_run_ = $t_words_ [ 0 ];
		    push ( @total_size_to_run_ , $t_total_size_to_run_ );

		    $mail_body_ = $mail_body_." \t > TOTAL_SIZE_TO_RUN=".$t_total_size_to_run_."\n";
		}

		when ( "USE_OPTIMAL_MAX_LOSS_PER_UNIT_SIZE" )
		{
		    my $t_use_optimal_max_loss_per_unit_size_ = $t_words_ [ 0 ];
		    push ( @use_optimal_max_loss_per_unit_size_ , $t_use_optimal_max_loss_per_unit_size_ );

		    $mail_body_ = $mail_body_." \t > USE_OPTIMAL_MAX_LOSS_PER_UNIT_SIZE=".$t_use_optimal_max_loss_per_unit_size_."\n";
		}

		when ( "MIN_MAX_LOSS_PER_UNIT_SIZE" )
		{
		    $min_max_loss_per_unit_size_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MIN_MAX_LOSS_PER_UNIT_SIZE=".$min_max_loss_per_unit_size_."\n";
		}

		when ( "MAX_MAX_LOSS_PER_UNIT_SIZE" )
		{
		    $max_max_loss_per_unit_size_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MAX_MAX_LOSS_PER_UNIT_SIZE=".$max_max_loss_per_unit_size_."\n";
		}

		when ( "MAX_LOSS_PER_UNIT_SIZE" )
		{ # This overrides the two above.
		    $max_loss_per_unit_size_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > MAX_LOSS_PER_UNIT_SIZE=".$max_loss_per_unit_size_."\n";
		}

		when ( "OPENTRADE_LOSS_PER_UNIT_SIZE" )
		{
		    $opentrade_loss_per_unit_size_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > OPENTRADE_LOSS_PER_UNIT_SIZE=".$opentrade_loss_per_unit_size_."\n";
		}

		when ( "SHORT_TERM_LOSS_PER_UNIT_SIZE" )
		{
		    $short_term_loss_per_unit_size_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > SHORT_TERM_LOSS_PER_UNIT_SIZE=".$short_term_loss_per_unit_size_."\n";
		}

		when ( "MIN_VOLUME_PER_STRAT" )
		{
		    my $t_min_volume_per_strat_ = $t_words_ [ 0 ];
		    push ( @min_volume_per_strat_ , $t_min_volume_per_strat_ );

		    $mail_body_ = $mail_body_." \t > MIN_VOLUME_PER_STRAT=".$t_min_volume_per_strat_."\n";
		}

		when ( "VOLUME_CUT_OFF_RATIO" )
		{
		    my $t_volume_cut_off_ratio_ = $t_words_[ 0 ];
		    push ( @volume_cut_off_ratio_ , $t_volume_cut_off_ratio_ );

		    $mail_body_ = $mail_body_." \t > VOLUME_CUT_OFF_RATIO=".$t_volume_cut_off_ratio_."\n";
		}

		when ( "TTC_CUT_OFF_RATIO" )
		{
		    my $t_ttc_cut_off_ratio_ = $t_words_[ 0 ];
		    push ( @ttc_cut_off_ratio_ , $t_ttc_cut_off_ratio_ );

		    $mail_body_ = $mail_body_." \t > TTC_CUT_OFF_RATIO=".$t_ttc_cut_off_ratio_."\n";
		}

		when ( "STRATS_TO_KEEP" )
		{
		    if ( ! FindItemFromVec ( $t_words_ [ 0 ] , @strats_to_keep_ ) )
		    {
			push ( @strats_to_keep_ , $t_words_ [ 0 ] );
			$mail_body_ = $mail_body_." \t > STRATS_TO_KEEP=".$t_words_ [ 0 ]."\n";
		    }
		}

		when ( "STRATS_TO_EXCLUDE" )
		{
		    if ( ! FindItemFromVec ( $t_words_ [ 0 ] , @strats_to_exclude_ ) )
		    {
			push ( @strats_to_exclude_ , $t_words_ [ 0 ] );
			$mail_body_ = $mail_body_." \t > STRATS_TO_EXCLUDE=".$t_words_ [ 0 ]."\n";
		    }
		}

		when ( "INTERVALS_TO_PICK_FROM" )
		{
		    if ( ! exists ( $intervals_to_pick_from_ { $t_words_ [ 0 ] } ) )
		    {
			$intervals_to_pick_from_ { $t_words_ [ 0 ] } = $t_words_ [ 1 ];

			$longest_interval_ = max ( $longest_interval_ , $t_words_ [ 0 ] );

			$mail_body_ = $mail_body_." \t > INTERVALS_TO_PICK_FROM=[".$t_words_ [ 0 ]."]=".$t_words_ [ 1 ]."\n";
		    }
		}
		
		when ( "SKIP_RESULTS_FOR_DAYS" )
		{
			push ( @skip_dates_vector_ , $t_words_ [0] );
			$mail_body_ = $mail_body_." \t > SKIP_RESULTS_FOR_DAYS=".$t_words_ [ 0 ]."\n";
		}

		when ( "DIVERSITY_SCORES" )
		{
		    if ( ! exists ( $diversity_type_to_weight_ { $t_words_ [ 0 ] } ) )
		    {
			$diversity_type_to_weight_ { $t_words_ [ 0 ] } = $t_words_ [ 1 ];

			$mail_body_ = $mail_body_." \t > DIVERSITY_SCORES=[".$t_words_ [ 0 ]."]=".$t_words_ [ 1 ]."\n";
		    }
		}
		when ( "USE_SHARPE_PNL_CHECK" )
		{
		    $use_sharpe_pnl_check_ = $t_words_ [0];
		    print "Using USE_SHARPE_PNL_CHECK_: $use_sharpe_pnl_check_\n";
		}
		when ( "SORT_ALGO" )
		{
		    my $t_sort_algo_ = $t_words_ [ 0 ];
		    push ( @sort_algo_ , $t_sort_algo_ );

		    $mail_body_ = $mail_body_." \t > SORT_ALGO=".$t_sort_algo_."\n";
		}

		when ( "EMAIL_ADDRESS" )
		{
		    $email_address_ = $t_words_ [ 0 ];
		}

		when ( "INSTALL_PICKED_STRATS" )
		{
		    $install_picked_strats_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > INSTALL_PICKED_STRATS=".$install_picked_strats_."\n";
		}

		when ( "INSTALL_LOCATION" )
		{
		    $install_location_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > INSTALL_PICKED_STRATS=".$install_location_."\n";
		}

		when ( "PROD_QUERY_START_ID" )
		{
		    $prod_query_start_id_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > PROD_QUERY_START_ID=".$prod_query_start_id_."\n";
		}

		when ( "EXEC_START_HHMM" )
		{
		    $exec_start_hhmm_ = GetUTCHHMMStr ( $t_words_ [ 0 ], $yyyymmdd_ );
		    if ( $exec_start_hhmm_ < 1000 )
		    { # 0710 -> 710 , causing substrs to fail when installing cron.
			$exec_start_hhmm_ = "0".$exec_start_hhmm_;
		    }

		    $mail_body_ = $mail_body_." \t > EXEC_START_HHMM=".$exec_start_hhmm_."\n";    
		}

		when ( "EXEC_END_HHMM" )
		{
		    $exec_end_hhmm_ = GetUTCHHMMStr ( $t_words_ [ 0 ], $yyyymmdd_ );
		    if ( $exec_end_hhmm_ < 1000 )
		    { # 0710 -> 710 , causing substrs to fail when installing cron.
			$exec_end_hhmm_ = "0".$exec_end_hhmm_;
		    }

		    $mail_body_ = $mail_body_." \t > EXEC_END_HHMM=".$exec_end_hhmm_."\n";    
		}

		when ( "ONLOAD_TRADE_EXEC" )
		{
		    $onload_trade_exec_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > ONLOAD_TRADE_EXEC=".$onload_trade_exec_."\n";
		}

		when ( "VMA_TRADE_EXEC" )
		{
		    $vma_trade_exec_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > VMA_TRADE_EXEC=".$vma_trade_exec_."\n";
		}

		when ( "AFFINITY_TRADE_EXEC" )
		{
		    $affinity_trade_exec_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > AFFINITY_TRADE_EXEC=".$affinity_trade_exec_."\n";
		}

		when ( "COMPENSATE_SIM_REAL_BIAS" )
		{
		    $compensate_sim_real_bias_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > COMPENSATE_SIM_REAL_BIAS=".$compensate_sim_real_bias_."\n";
		}

		when ( "SIM_REAL_BIAS_COMPENSATION" )
		{
		    $sim_real_bias_compensation_ = $t_words_ [ 0 ];
		    $mail_body_ = $mail_body_." \t > SIM_REAL_BIAS_COMPENSATION=".$sim_real_bias_compensation_."\n";
		}
	    }
	}
    }
    
    if ( $#skip_dates_vector_ >= 0 )
    {
   		my $file_handle_ = FileHandle->new;
		$file_handle_->open ( "> $skip_dates_file_ " ) or PrintStacktraceAndDie ( "Could not open $skip_dates_file_ for writing\n" );
    	for ( my $i_ = 0; $i_ <= $#skip_dates_vector_; $i_ ++ )
    	{
			print $file_handle_ "$skip_dates_vector_[$i_]\n";
    	}
    	$file_handle_->close;
    	push ( @intermediate_files_ , $skip_dates_file_ );
    }
    else
    {
    	$skip_dates_file_ = "INVALIDFILE";
    }

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";

    return;
}

sub SanityCheckConfigParams
{
    if ( $#num_strats_to_install_ != $#total_size_to_run_ )
    {
	PrintStacktraceAndDie ( "#NUM_STRATS_TO_INSTALL != #TOTAL_SIZE_TO_RUN" );
    }

    if ( $#num_strats_to_install_ != $#use_optimal_max_loss_per_unit_size_ )
    {
	PrintStacktraceAndDie ( "#NUM_STRATS_TO_INSTALL != #TOTAL_SIZE_TO_RUN" );
    }

    if ( $#num_strats_to_install_ != $#use_optimal_max_loss_per_unit_size_ )
    {
	PrintStacktraceAndDie ( "#NUM_STRATS_TO_INSTALL != #TOTAL_SIZE_TO_RUN" );
    }

    if ( $#num_strats_to_install_ != $#volume_cut_off_ratio_ && 
	$#num_strats_to_install_ != $#min_volume_per_strat_ )
    {
	PrintStacktraceAndDie ( "#NUM_STRATS_TO_INSTALL != #MIN_VOLUME_PER_STRAT && #NUM_STRATS_TO_INSTALL != #VOLUME_CUT_OFF_RATIO" );
    }

    if ( $#num_strats_to_install_ != $#sort_algo_ )
    {
	PrintStacktraceAndDie ( "#NUM_STRATS_TO_INSTALL != #SORT_ALGO" );
    }

    $sum_num_strats_to_install_ = GetAverage ( \@num_strats_to_install_ ) * ( $#num_strats_to_install_ + 1 );

    if ( $sum_num_strats_to_install_ <= 0 )
    {
	PrintStacktraceAndDie ( "SUM_NUM_STRATS_TO_INSTALL=".$sum_num_strats_to_install_ );
    }

    for ( my $i = 0 ; $i <= $#num_strats_to_install_ ; $i ++ )
    {
	if ( $total_size_to_run_ [ $i ] < $num_strats_to_install_ [ $i ] )
	{
	    PrintStacktraceAndDie ( "TOTAL_SIZE_TO_RUN=".$total_size_to_run_ [ $i ]." < NUM_STRATS_TO_INSTALL=".$num_strats_to_install_ [ $i ] );
	}
    }

    # Assuming t_s_t_r is a multiple of n_s_t_i
    for ( my $i = 0 ; $i <= $#num_strats_to_install_ ; $i ++ )
    {
	push ( @size_per_strat_ , ( $total_size_to_run_ [ $i ] / $num_strats_to_install_ [ $i ] ) );
    }

    if ( ( $max_loss_per_unit_size_ <= 0 || $max_loss_per_unit_size_ > 10000 ) &&
	 ( $min_max_loss_per_unit_size_ < 0 || $max_max_loss_per_unit_size_ <= 0 ) )
    { # Pointless sanity check really.
	PrintStacktraceAndDie ( "MAX_LOSS_PER_UNIT_SIZE=".$max_loss_per_unit_size_." ".
				"MIN_MAX_LOSS_PER_UNIT_SIZE=".$min_max_loss_per_unit_size_." ".
				"MAX_MAX_LOSS_PER_UNIT_SIZE=".$max_max_loss_per_unit_size_ );
    }
    
    if ( $opentrade_loss_per_unit_size_ <= 0 || $opentrade_loss_per_unit_size_ > 10000 )
    { # Pointless sanity check really.
	PrintStacktraceAndDie ( "OPENTRADE_LOSS_PER_UNIT_SIZE=".$opentrade_loss_per_unit_size_ );
    }

    if ( $short_term_loss_per_unit_size_ <= 0 || $short_term_loss_per_unit_size_ > 10000 )
    { # Pointless sanity check really.
	PrintStacktraceAndDie ( "SHORT_TERM_LOSS_PER_UNIT_SIZE=".$short_term_loss_per_unit_size_ );
    }

    $local_production_strat_list_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$shortcode_.".".$timeperiod_.".".basename ( $config_file_ );

    push ( @intermediate_files_ , $local_production_strat_list_file_ );

    $remote_production_strat_list_file_ = $REMOTE_PRODUCTION_STRAT_LIST_DIR."/".$shortcode_.".".$timeperiod_.".".basename ( $config_file_ );

    # Normalize interval weights
    my $sum_interval_weights_ = 0;

    foreach my $interval_ ( sort { $a <=> $b }
    				  keys %intervals_to_pick_from_ )
    {
	$sum_interval_weights_ += $intervals_to_pick_from_ { $interval_ };
    }
    foreach my $interval_ ( sort { $a <=> $b }
    				  keys %intervals_to_pick_from_ )
    {
	$intervals_to_pick_from_ { $interval_ } /= $sum_interval_weights_;
    }

    # Normalize diversity scores
    my $sum_diversity_weights_ = 0;

    foreach my $diversity_type_ ( keys %diversity_type_to_weight_ )
    {
	$sum_diversity_weights_ += $diversity_type_to_weight_ { $diversity_type_ };
    }

    foreach my $diversity_type_ ( keys %diversity_type_to_weight_ )
    {
	$diversity_type_to_weight_ { $diversity_type_ } /= $sum_diversity_weights_;
    }

    return;
}

sub GetRealResults
{
    my $RANK_HIST_QUERIES = $SCRIPTS_DIR."/rank_hist_queries.pl";

    foreach my $interval_ ( sort { $a <=> $b }
			    keys %intervals_to_pick_from_ )
    {
	my $exec_cmd_ = "$RANK_HIST_QUERIES $timeperiod_ $shortcode_ TODAY-$interval_ TODAY";
	my @rank_hist_queries_lines_ = `$exec_cmd_`;

	foreach my $rank_hist_line_ ( @rank_hist_queries_lines_ )
	{
	    # 372 738 w_strategy_ilist_CGB_0_US_Msin_Msin_I2_noeu_1_na_e3_20111209_20120209_EST_820_EST_1455_500_2_1_fsr.5_3_FSHLR_0.01_0_0_0.75.tt_EST_820_EST_1455.pfi_7 1

	    my @t_result_words_ = split ( ' ' , $rank_hist_line_ );

	    my $strat_name_ = $t_result_words_ [ 2 ];
	    my $t_num_days_run_ = $t_result_words_ [ 3 ];

	    if ( $t_num_days_run_ >= 0.3 * $interval_ )
	    { # This strat was run in real for atleast 30 % of the days constituting this interval.
		my $pnl_vol_line_ = sprintf ( "%d %d" , $t_result_words_ [ 0 ] , $t_result_words_ [ 1 ] );

		$interval_to_real_results_ { $interval_ } { $strat_name_ } = $pnl_vol_line_;

		if ( ! FindItemFromVec ( $strat_name_ , @all_strats_ ) )
		{
		    push ( @all_strats_ , $strat_name_ );
		}
	    }
	}
    }
}

sub GetGlobalResults
{
    my ( $t_subset_index_ ) = @_;

    my $SS_NOC = $SCRIPTS_DIR."/ss_noc.sh";

    foreach my $interval_ ( sort { $a <=> $b }
			    keys %intervals_to_pick_from_ )
    {
    my $t_end_date = CalcPrevWorkingDateMult ( $date_to_run_for_ , 1 );
	my $exec_cmd_ = $SS_NOC." $shortcode_ $timeperiod_ ".( $interval_ - 1 )." $t_end_date 1 $skip_dates_file_";
	my @ss_noc_results_ = `$exec_cmd_`;

	my $volume_cut_off_ = 0;
	my $ttc_cut_off_ = 0;
	foreach my $ss_noc_result_line_ ( @ss_noc_results_ )
	{
	    my @ss_noc_result_words_ = split ( ' ' , $ss_noc_result_line_ );
	    if( $ss_noc_result_words_[ 1 ] > $volume_cut_off_ )
	    {
		$volume_cut_off_ = $ss_noc_result_words_[ 1 ];
	    }
	    $ttc_cut_off_+= $ss_noc_result_words_[ 6 ];
	}
	if ( $#ss_noc_results_ < 0 ) { next; }
	$ttc_cut_off_/= ( $#ss_noc_results_ + 1 );

	if ( $#volume_cut_off_ratio_ >= $t_subset_index_ )
	{
	    $volume_cut_off_ *=  $volume_cut_off_ratio_ [ $t_subset_index_ ];
	}
	else
	{
	    $volume_cut_off_ = $min_volume_per_strat_ [ $t_subset_index_ ];
	}
	if ( $#ttc_cut_off_ratio_ >= $t_subset_index_ )
	{
	    $ttc_cut_off_ *= $ttc_cut_off_ratio_[ $t_subset_index_ ];
	}
	else
	{
	    $ttc_cut_off_ = 0;
	}

	foreach my $ss_noc_result_line_ ( @ss_noc_results_ )
	{
	    # -84 644 -0.08 w_hv_ilist_CGB_0_US_Mkt_Mkt_J0_8_na_t3_20111226_20120220_EST_820_EST_1455_500_2_1_fst.5_FSLR_0.01_0_0_0.75.tt_EST_820_EST_1455.pfi_7

	    my @ss_noc_result_words_ = split ( ' ' , $ss_noc_result_line_ );

	    if( $ss_noc_result_words_[ 1 ] < $volume_cut_off_ )
	    {
		next;
	    }

	    if ( ! $volume_cut_off_ && ( $ss_noc_result_words_ [ 1 ] < $min_volume_per_strat_ [ $t_subset_index_ ] ) )
	    {
		next;
	    }

	    if( ( $ttc_cut_off_ > 0 ) && ( $ss_noc_result_words_[ 6 ] > $ttc_cut_off_ ) )
	    {
		next;
	    }

	    if ( $ss_noc_result_words_ [ 6 ] <= 0 )
	    {
		print "ERROR ttc<=0 $exec_cmd_\n";
		$ss_noc_result_words_ [ 6 ] = 100000;
	    }

	    my $t_ss_noc_result_ = sprintf ( "%d %d %0.2f %d %0.2f %d" , $ss_noc_result_words_ [ 0 ] , $ss_noc_result_words_ [ 1 ] , $ss_noc_result_words_ [ 2 ] , $ss_noc_result_words_ [ 4 ] , $ss_noc_result_words_ [ 5 ] , $ss_noc_result_words_ [ 6 ] );
	    my $t_strat_name_ = $ss_noc_result_words_ [ 3 ];

	    if ( index ( $t_strat_name_ , "EARTH" ) >= 0 )
	    { # Ignore EARTH stuff till ready.
		print "Ignoring EARTH strategy : ".$t_strat_name_."\n";
		next;
	    }
            if ( index ( $t_strat_name_ , "LASSO" ) >= 0 )
            { # Ignore LASSO stuff till ready.
                print "Ignoring LASSO strategy : ".$t_strat_name_."\n";
                next;
            }
	    
	    if ( IsTooInsample ( $t_strat_name_ ) )
	    { # Use the weights to see if this strat is "too" insample for the periods.
		next;
	    }

	    if ( FindItemFromVec ( $t_strat_name_ , @strats_to_exclude_ ) )
	    { # We were explicitly instructed not to pick this strat.
		next;
	    }

	    if ( FindItemFromVec ( $t_strat_name_ , @picked_strats_ ) )
	    { # Already picked as part of an earlier subset
		next;
	    }

	    $interval_to_global_results_ { $interval_ } { $t_strat_name_ } = $t_ss_noc_result_;

	    if ( ! FindItemFromVec ( $t_strat_name_ , @all_strats_ ) )
	    {
		push ( @all_strats_ , $t_strat_name_ );
	    }
	}
    }
}

sub ScoreStrats
{
    my ( $t_subset_index_ ) = @_;

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_." # ScoreStrats ( $t_subset_index_ )\n\n";

    # Remove strats that did not have results in all intervals.
    my %strat_name_to_result_count_ = ( );
    foreach my $interval_ ( sort { $a <=> $b }
			    keys %intervals_to_pick_from_ )
    {
	foreach my $strat_name_ ( @all_strats_ )
	{
	    if ( ! exists ( $strat_name_to_result_count_ { $strat_name_ } ) )
	    {
		$strat_name_to_result_count_ { $strat_name_ } = 0;
	    }

	    if ( exists ( $interval_to_bias_adjusted_global_results_ { $interval_ } { $strat_name_ } ) )
	    {
		$strat_name_to_result_count_ { $strat_name_ } ++;
	    }
	}
    }

    @all_strats_ = ( );
    foreach my $strat_name_ ( keys %strat_name_to_result_count_ )
    {
	if ( $strat_name_to_result_count_ { $strat_name_ } == keys %intervals_to_pick_from_ )
	{
	    if ( ! FindItemFromVec ( $strat_name_ , @all_strats_ ) )
	    {
		push ( @all_strats_ , $strat_name_ );
	    }
	}
    }

    $mail_body_ = $mail_body_."No. of strats to be scored: $#all_strats_\n\n";

    foreach my $interval_ ( sort { $a <=> $b }
			    keys %intervals_to_pick_from_ )
    {
	foreach my $strat_name_ ( @all_strats_ )
	{
	    my $t_global_result_ = "";

	    if ( $compensate_sim_real_bias_ )
	    {
		if ( exists ( $interval_to_bias_adjusted_global_results_ { $interval_ } { $strat_name_ } ) )
		{
		    $t_global_result_ = $interval_to_bias_adjusted_global_results_ { $interval_ } { $strat_name_ };
		}
	    }
	    else
	    {
		if ( exists ( $interval_to_global_results_ { $interval_ } { $strat_name_ } ) )
		{
		    $t_global_result_ = $interval_to_global_results_ { $interval_ } { $strat_name_ };
		}
	    }

	    if ( $t_global_result_ )
	    {
		my @global_result_words_ = split ( ' ' , $t_global_result_ );

		my $t_weighted_pnl_ = $global_result_words_ [ 0 ] * $intervals_to_pick_from_ { $interval_ };
		my $t_weighted_vol_ = $global_result_words_ [ 1 ] * $intervals_to_pick_from_ { $interval_ };
		my $t_weighted_dd_ = $global_result_words_ [ 2 ] * $intervals_to_pick_from_ { $interval_ };
		my $t_weighted_pnl_stdev_ = $global_result_words_ [ 3 ] * $intervals_to_pick_from_ { $interval_ };
		my $t_weighted_pnl_sharpe_ = $global_result_words_ [ 4 ] * $intervals_to_pick_from_ { $interval_ };
		my $t_weighted_ttc_ = $global_result_words_ [ 5 ] * $intervals_to_pick_from_ { $interval_ };

		my $t_existing_result_line_ = "0 0 0 0 0 0";
		if ( exists ( $strat_name_to_global_result_score_ { $strat_name_ } ) )
		{
		    $t_existing_result_line_ = $strat_name_to_global_result_score_ { $strat_name_ };
		}

		# Combine this result line with the existing result line for this strat.
		my @t_existing_result_words_ = split ( ' ' , $t_existing_result_line_ );
		$t_weighted_pnl_ += $t_existing_result_words_ [ 0 ];
		$t_weighted_vol_ += $t_existing_result_words_ [ 1 ];
		$t_weighted_dd_ += $t_existing_result_words_ [ 2 ];
		$t_weighted_pnl_stdev_ += $t_existing_result_words_ [ 3 ];
		$t_weighted_pnl_sharpe_ += $t_existing_result_words_ [ 4 ];
		$t_weighted_ttc_ += $t_existing_result_words_ [ 5 ];

		$strat_name_to_global_result_score_ { $strat_name_ } = sprintf ( "%d %d %0.2f %d %0.2f %d" , $t_weighted_pnl_ , $t_weighted_vol_ , $t_weighted_dd_ , $t_weighted_pnl_stdev_ , $t_weighted_pnl_sharpe_ , $t_weighted_ttc_ );
	    }
	}
    }

    foreach my $strat_name_ ( @all_strats_ )
    {
	if ( exists ( $strat_name_to_global_result_score_ { $strat_name_ } ) )
	{
	    push ( @algo_sorted_strat_names_ , $strat_name_ );
	    push ( @algo_sorted_strat_results_ , $strat_name_to_global_result_score_ { $strat_name_ } );
	}
    }

    SortResultsByAlgo ( $t_subset_index_ );

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";

    return;
}

sub getSharpe # input array
{
    my $n_ = $#_;
    my $s_=0;
    my $sqs_=0;
    foreach my $t_( @_ ){
	$s_ += $t_; $sqs_+=$t_*$t_;
    }
    #return ($s_ * $s_ )/($n_ * sqrt($sqs_*$n_ - $s_*$s_));
    return ($s_ / sqrt($sqs_*$n_ - $s_ * $s_));
}

sub SortResultsByAlgo
{
    my ( $t_subset_index_ ) = @_;

    my $t_sort_algo_ = $sort_algo_ [ $t_subset_index_ ];

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_." # SortResultsByAlgo ( $t_subset_index_ = $t_sort_algo_ )\n\n";

    my %strat_name_to_algo_sorted_score_ = ( );

    for ( my $i = 0 ; $i <= $#algo_sorted_strat_names_ ; $i ++ )
    {
	my $t_strat_name_ = $algo_sorted_strat_names_ [ $i ];
	my $t_result_line_ = $algo_sorted_strat_results_ [ $i ];

	my @t_result_words_ = split ( ' ' , $t_result_line_ );

	my $t_pnl_ = $t_result_words_ [ 0 ];
	my $t_vol_ = $t_result_words_ [ 1 ];
	my $t_dd_ = $t_result_words_ [ 2 ];
	my $t_pnl_stdev_ = $t_result_words_ [ 3 ];
	my $t_pnl_sharpe_ = $t_result_words_ [ 4 ];
	my $t_ttc_ = $t_result_words_ [ 5 ];

	given ( $t_sort_algo_ )
	{
	    when ( "PNL" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_;
	    }
	    when ( "PNL_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_ );
	    }
	    when ( "PNL_VOL" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_ * $t_vol_;
	    }
	    when ( "PNL_VOL_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ );
	    }
	    when ( "PNL_DD" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_ * abs ( $t_dd_ );
	    }
	    when ( "PNL_SQRT_DD" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_ * sqrt ( abs ( $t_dd_ ) );
	    }
	    when ( "PNL_VOL_SQRT_BY_DD" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_ * sqrt ( abs ( $t_vol_ ) ) * abs ( $t_dd_ ) ;
	    }
	    when ( "PNL_VOL_SQRT_BY_DD_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_ ) * sqrt ( abs ( $t_vol_ ) ) * sqrt ( abs ( $t_dd_ ) ) ; # sign(pnl)*sqrt(abs(pnl)) * sqrt(abs(volume)) * sqrt(max(0,pnl_dd_ratio))
	    }
	    when ( "PNL_VOL_SQRT_BY_TTC_BY_DD_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = ( SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / $t_ttc_;
	    }
	    when ( "PNL_VOL_SQRT_BY_TTC_SQRT_BY_DD_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = ( SqrtSign ( $t_pnl_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / sqrt ( $t_ttc_ );
	    }
	    when ( "PNL_SHARPE" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_;
	    }
	    when ( "PNL_SHARPE_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_sharpe_ );
	    }
	    when ( "PNL_SHARPE_VOL" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_ * $t_vol_;
	    }
	    when ( "PNL_SHARPE_VOL_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ );
	    }
	    when ( "PNL_SHARPE_DD" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_ * abs ( $t_dd_ );
	    }
	    when ( "PNL_SHARPE_SQRT_DD" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_ * sqrt ( abs ( $t_dd_ ) );
	    }
	    when ( "PNL_SHARPE_VOL_SQRT_BY_DD" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = $t_pnl_sharpe_ * sqrt ( $t_vol_ ) * abs ( $t_dd_ ) ;
	    }
	    when ( "PNL_SHARPE_VOL_SQRT_BY_DD_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ; # sign(pnl_sharpe)*sqrt(abs(pnl_sharpe)) * sqrt(abs(volume)) * sqrt(max(0,pnl_sharpe_dd_ratio))
	    }
	    when ( "PNL_SHARPE_VOL_SQRT_BY_TTC_BY_DD_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = ( SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / $t_ttc_;
	    }
	    when ( "PNL_SHARPE_VOL_SQRT_BY_TTC_SQRT_BY_DD_SQRT" )
	    {
		$strat_name_to_algo_sorted_score_ { $t_strat_name_ } = ( SqrtSign ( $t_pnl_sharpe_ ) * sqrt ( $t_vol_ ) * sqrt ( abs ( $t_dd_ ) ) ) / sqrt ( $t_ttc_ );
	    }

	    default
	    {
		PrintStacktraceAndDie ( "SORT_ALGO=".$t_sort_algo_." NOT AVAILABLE" );
	    }
	}
    }

    @algo_sorted_strat_names_ = ( );
    @algo_sorted_strat_results_ = ( );
    my %SelectedPoolPNL_ = ( );my %s_ = ( ); my $sharpe_=-1.0, my $sharpe_cutoff_ = 0.0;
    my $end_yyyymmdd_ = CalcPrevWorkingDateMult ( $yyyymmdd_ , 10 );
    foreach my $strat_name_ ( sort { $strat_name_to_algo_sorted_score_ { $b } <=> $strat_name_to_algo_sorted_score_ { $a } }
			      keys %strat_name_to_algo_sorted_score_ )
    {
	if ( $#algo_sorted_strat_results_ > 2 * $num_strats_to_install_ [ $t_subset_index_ ] )
	{
	    # Consider at most 2 times the no. of starts to be picked.
	    last;
	}
	
	if ( $#algo_sorted_strat_results_ <= $num_strats_to_install_[ $t_subset_index_ ]) {
	    # sharpe of last 15 days data..
	    my @t_res_=`/home/dvctrader/LiveExec/bin/summarize_single_strategy_results $shortcode_ $strat_name_ ~/ec2_globalresults $end_yyyymmdd_ $yyyymmdd_ | egrep '^201[123]' | cut -d' ' -f 1,2`;
	    for(@t_res_){
		chomp;
		my @t_ = split(/ /);
		if ( ! $SelectedPoolPNL_{$t_[0]} ){ $SelectedPoolPNL_{$t_[0]} = 0.0; }
		$s_{$t_[0]} = $SelectedPoolPNL_{$t_[0]} + $t_[1];
		#print "$t_[0] $s_{$t_[0]} $SelectedPoolPNL_{$t_[0]}\n";
	    }
	    
	    my $new_sharpe_ =  getSharpe(values %s_) ; 
	    if ( ! $sharpe_cutoff_ ) { $sharpe_cutoff_ = $new_sharpe_; }
	    $sharpe_cutoff_ = ($sharpe_cutoff_ + $new_sharpe_);$sharpe_cutoff_ *= $new_sharpe_>0?0.45:1.0;
	    #print "$sharpe_ -- $new_sharpe_\n";
	    if ( $use_sharpe_pnl_check_ && $new_sharpe_ < $sharpe_cutoff_ ) { next; }
	    while (my ($key, $value) = each(%s_)) { $SelectedPoolPNL_{$key} = $value; }
	    $sharpe_ = $new_sharpe_;
	}
	push ( @algo_sorted_strat_names_ , $strat_name_ );
	push ( @algo_sorted_strat_results_ , $strat_name_to_global_result_score_ { $strat_name_ } );

	my $t_int_algo_sorted_score_ = sprintf ( "%d" , $strat_name_to_algo_sorted_score_ { $strat_name_ } );
	$mail_body_ = $mail_body_."\t ".$t_int_algo_sorted_score_." ".$strat_name_." ".$strat_name_to_global_result_score_ { $strat_name_ }."\n";

	foreach my $interval_ ( sort { $a <=> $b }
				keys %interval_to_real_results_ )
	{
	    if ( exists ( $interval_to_real_results_ { $interval_ } { $strat_name_ } ) )
	    {
		$mail_body_ = $mail_body_."\t REAL-RESULT $interval_ ".$interval_to_real_results_ { $interval_ } { $strat_name_ }."\n";
	    }
	}

	foreach my $interval_ ( sort { $a <=> $b }
				keys %interval_to_global_results_ )
	{
	    if ( exists ( $interval_to_bias_adjusted_global_results_ { $interval_ } { $strat_name_ } ) )
	    {
		$mail_body_ = $mail_body_."\t GLOB-RESULT $interval_ ".$interval_to_global_results_ { $interval_ } { $strat_name_ }."\n";
	    }
	}

	foreach my $interval_ ( sort { $a <=> $b }
				keys %interval_to_bias_adjusted_global_results_ )
	{
	    if ( exists ( $interval_to_bias_adjusted_global_results_ { $interval_ } { $strat_name_ } ) )
	    {
		$mail_body_ = $mail_body_."\t BIAS-ADJ-GLOB-RESULT $interval_ ".$interval_to_bias_adjusted_global_results_ { $interval_ } { $strat_name_ }."\n";
	    }
	}
    }
    print "Final Sharpe: $sharpe_\n";
    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";

    return;
}

sub PickStrats
{
    my ( $t_subset_index_ ) = @_;

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_." # PickStrats ( $t_subset_index_ )\n\n";

    $mail_body_ = $mail_body_."\n\n\t PICKED :\n";

    # First pick strats from STRATS_TO_KEEP list.
    foreach my $strat_name_ ( @strats_to_keep_ )
    {
	if ( ! FindItemFromVec ( $strat_name_ , @picked_strats_ ) )
	{
	    push ( @picked_strats_ , $strat_name_ );

	    $mail_body_ = $mail_body_."\t\t ".$strat_name_."\n";

	    $strat_name_to_subset_index_ { $strat_name_ } = $t_subset_index_;
	}
    }

    # Factor in diversity , so that we don't end up with a bad case ,
    # where all top strats are almost the same.
    # DiversifyAndScore ( );

    my $num_picked_this_subset_ = 0;
    for ( my $i = 0 ; $i <= $#algo_sorted_strat_names_ && $num_picked_this_subset_ < $num_strats_to_install_ [ $t_subset_index_ ] && ( $#picked_strats_ + 1 ) < $sum_num_strats_to_install_ ; $i ++ , $num_picked_this_subset_ ++ )
    {
	my $t_strat_name_ = $algo_sorted_strat_names_ [ $i ];

	if ( ! FindItemFromVec ( $t_strat_name_ , @picked_strats_ ) )
	{
	    push ( @picked_strats_ , $t_strat_name_ );

	    $mail_body_ = $mail_body_."\t\t ".$t_strat_name_."\n";

	    $strat_name_to_subset_index_ { $t_strat_name_ } = $t_subset_index_;
	}
    }

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    
    return;
}

sub RemoveIntermediateFiles
{
    foreach my $intermediate_file_ ( @intermediate_files_ )
    {
	my $exec_cmd_ = "rm -f $intermediate_file_";
#	print $exec_cmd_."\n";
	`$exec_cmd_`;
    }

    return;
}

sub DiversifyAndScore
{
    # Best pick gets a free ride.
    $strat_name_to_diversity_adjusted_score_ { $algo_sorted_strat_names_ [ 0 ] } = $algo_sorted_strat_results_ [ 0 ];
    $strat_name_to_is_diversity_adjusted_ { $algo_sorted_strat_names_ [ 0 ] } = 1;

    while ( keys ( %strat_name_to_is_diversity_adjusted_ ) != $#algo_sorted_strat_names_ )
    { # While all strats have not been diversity adjusted.
	for ( my $i = 0 ; $i <= $#algo_sorted_strat_names_ ; $i ++ )
	{
	    my $t_strat_name_ = $algo_sorted_strat_names_ [ $i ];

	    if ( exists ( $strat_name_to_is_diversity_adjusted_ { $t_strat_name_ } ) )
	    {
		# Already considered , skip.
		next;
	    }

	    # Get the diversity score when this strat is added to
	    # the list of picked strats.
	    my $t_incremental_diversity_score_ = GetIncrementalDiversityScore ( $t_strat_name_ );
	}
    }

    return;
}

sub GetIncrementalDiversityScore
{
    my ( $next_strat_name_ ) = @_;

    my $diversity_score_ = 0;

    # Comparing this strat against all strats in %strat_name_to_is_diversity_adjusted_ ,
    # find out how "similar" it is to the existing pool of strats.
    foreach my $pool_strat_name_ ( keys ( %strat_name_to_is_diversity_adjusted_ ) )
    {
	
    }

    return $diversity_score_;
}

sub IsTooInsample
{
    my ( $t_strat_name_ ) = @_;

    my $last_insample_date_ = GetInsampleDate ( $t_strat_name_ );

    my $total_weight_insample_ = 0;

    if ( $last_insample_date_ > 20110101 )
    {
	foreach my $interval_ ( sort { $a <=> $b }
				keys %intervals_to_pick_from_ )
	{
	    my $outsample_barrier_ = CalcPrevWorkingDateMult ( $yyyymmdd_ , ( ( $interval_ * 2 ) / 3 ) );

	    if ( $last_insample_date_ > $outsample_barrier_ )
	    {
		$total_weight_insample_ += $intervals_to_pick_from_ { $interval_ };
	    }
	}
    }

    return ( $total_weight_insample_ >= 0.5 );
}

sub ComputeSimRealBias
{
    foreach my $strat_name_ ( @all_strats_ )
    {
	my $t_sim_real_bias_sum_ = 0;
	my $t_sim_real_bias_count_ = 0;

	foreach my $interval_ ( sort { $a <=> $b }
				keys %intervals_to_pick_from_ )
	{
	    if ( exists ( $interval_to_global_results_ { $interval_ } { $strat_name_ } ) &&
		 exists ( $interval_to_real_results_ { $interval_ } { $strat_name_ } ) )
	    {
		my $t_sim_ppc_ = 0.0;
		my $t_real_ppc_ = 0.0;

		{
		    my $t_sim_result_line_ = $interval_to_global_results_ { $interval_ } { $strat_name_ };
		    my @t_sim_result_words_ = split ( ' ' , $t_sim_result_line_ );
		    my $t_pnl_ = $t_sim_result_words_ [ 0 ];
		    my $t_vol_ = $t_sim_result_words_ [ 1 ];

		    $t_sim_ppc_ = $t_pnl_ / $t_vol_;
		}

		{
		    my $t_real_result_line_ = $interval_to_real_results_ { $interval_ } { $strat_name_ };

		    my @t_real_result_words_ = split ( ' ' , $t_real_result_line_ );
		    my $t_pnl_ = $t_real_result_words_ [ 0 ];
		    my $t_vol_ = $t_real_result_words_ [ 1 ];

		    $t_real_ppc_ = $t_pnl_ / $t_vol_;
		}

		# -ve if sim_ppc < real_ppc ( Sim pessimistic )
		# +ve if sim_ppc > real_ppc ( Sim optimistic )
		$t_sim_real_bias_sum_ += ( $t_sim_ppc_ - $t_real_ppc_ );
		$t_sim_real_bias_count_ ++;
	    }
	}

	if ( $t_sim_real_bias_count_ > 0 )
	{
	    $strat_name_to_sim_real_bias_ppc_ { $strat_name_ } = ( $t_sim_real_bias_sum_ / $t_sim_real_bias_count_ );
	}
    }

    return;
}

sub CompensateSimRealBias
{
    foreach my $strat_name_ ( @all_strats_ )
    {
	my $sim_real_bias_ppc_ = 0;
	if ( exists ( $strat_name_to_sim_real_bias_ppc_ { $strat_name_ } ) )
	{
	    $sim_real_bias_ppc_ = $strat_name_to_sim_real_bias_ppc_ { $strat_name_ };
	}

	foreach my $interval_ ( sort { $a <=> $b }
				keys %intervals_to_pick_from_ )
	{
	    if ( exists ( $interval_to_global_results_ { $interval_ } { $strat_name_ } ) )
	    {
		my @global_result_words_= split ( ' ' , $interval_to_global_results_ { $interval_ } { $strat_name_ } );

		# Adjust the pnl ,
		# strictly speaking the volume may suffer from sim-real bias too ,
		# but is not being compensated.

		$global_result_words_ [ 0 ] -= ( $sim_real_bias_compensation_ * ( $sim_real_bias_ppc_ * $global_result_words_ [ 1 ] ) );

		my $t_global_result_ = sprintf ( "%d %d %0.2f %d %0.2f %d %0.2f" , $global_result_words_ [ 0 ] , $global_result_words_ [ 1 ] , $global_result_words_ [ 2 ] , $global_result_words_ [ 3 ] , $global_result_words_ [ 4 ] , $global_result_words_ [ 5 ] , $sim_real_bias_ppc_ );

		$interval_to_bias_adjusted_global_results_ { $interval_ } { $strat_name_ } = $t_global_result_;
	    }
	}
    }
}

sub ComputeOptimalMaxLosses
{
    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    $mail_body_ = $mail_body_." # ComputeOptimalMaxLosses\n\n";

    my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl";
    my $ratio_num_times_max_loss_hit = 1.0;
    foreach my $picked_strat_ ( @picked_strats_ )
    {
	my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shortcode_ $timeperiod_ $longest_interval_ $ratio_num_times_max_loss_hit 100 $picked_strat_";
	my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

	# Consider the first result within range and not hitting max-loss too often
	foreach my $max_loss_line_ ( @exec_output_ )
	{
	    if ( index ( $max_loss_line_ , "=>" ) >= 0 ||
		 index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 )
	    {
		next;
	    }

	    my @max_loss_words_ = split ( ' ' , $max_loss_line_ );
	    if ( $#max_loss_words_ >= 2 )
	    {
		my $max_loss_ = $max_loss_words_ [ 0 ];
		my $num_max_loss_hits_ = $max_loss_words_ [ 2 ];

		if ( $num_max_loss_hits_ < $longest_interval_ * 0.1 )
		{
		    $strat_name_to_optimal_max_loss_ { $picked_strat_ } = max ( min ( $max_loss_ , $max_max_loss_per_unit_size_ ) , $min_max_loss_per_unit_size_ );
		    last;
		}
	    }
	}

	if ( ! exists ( $strat_name_to_optimal_max_loss_ { $picked_strat_ } ) )
	{
	    $strat_name_to_optimal_max_loss_ { $picked_strat_ } = $min_max_loss_per_unit_size_;
	}

	$mail_body_ = $mail_body_." ".$strat_name_to_optimal_max_loss_ { $picked_strat_ }." ".$picked_strat_."\n";
    }

    $mail_body_ = $mail_body_."\n---------------------------------------------------------------------------------------------------------------------\n";
    
    return;
}

sub GetBaseStratNameFromPickedStratsList
{
    my ( $strat_name_ ) = @_;

    my $base_strat_name_ = "";

    foreach my $t_base_strat_name_ ( @picked_strats_ )
    {
	if ( index ( $strat_name_ , $t_base_strat_name_ ) >= 0 )
	{
	    $base_strat_name_ = $t_base_strat_name_;
	    last;
	}
    }

    return $base_strat_name_;
}

sub CheckResetMaxLosses
{
    # Use user provided max-losses.
    # Purge computed max-losses.
    foreach my $strat_name_ ( keys %strat_name_to_optimal_max_loss_ )
    {
	if ( $max_max_loss_per_unit_size_ && 
	     ! $use_optimal_max_loss_per_unit_size_ [ $strat_name_to_subset_index_ { $strat_name_ } ] )
	{

	    $strat_name_to_optimal_max_loss_ { $strat_name_ } = $max_loss_per_unit_size_;
	}
    }

    return;
}

sub ComputePerIntervalSimRealBias
{
    foreach my $interval_ ( sort { $a<=>$b } keys %intervals_to_pick_from_ )
    {
	my %strat_to_count_ = ( );
	my $total_interval_bias_ = 0;
	my $total_interval_count_ = 0;

	#my $t_yyyymmdd_ = GetIsoDateFromStrMin1( "TODAY" );
	my $t_yyyymmdd_ = `date +%Y%m%d`; chomp ( $t_yyyymmdd_ );
	$t_yyyymmdd_ = $date_to_run_for_;
	for ( my $i = 0 ; $i < $interval_ ; $i++ )
	{
	    $t_yyyymmdd_ = CalcPrevWorkingDateMult( $t_yyyymmdd_ , 1 );
	    while ( (!ValidDate($t_yyyymmdd_)) || SkipWeirdDate($t_yyyymmdd_) || IsDateHoliday($t_yyyymmdd_) )
	    {
		$t_yyyymmdd_ = CalcPrevWorkingDateMult( $t_yyyymmdd_ , 1 );
	    }
	    my @t_bias_lines_ = GetSimRealBiasForDate( $t_yyyymmdd_ );
	    foreach my $t_bias_line_ ( @t_bias_lines_ )
	    {
		my @t_bias_words_ = split( ' ' , $t_bias_line_ );
		my $t_strat_name_ = $t_bias_words_[ 0 ];
		if( ! ( exists( $strat_to_count_{ $t_strat_name_ } ) ) )
		{
		    $strat_to_count_{ $t_strat_name_ } = 0;
		    $interval_to_strat_to_bias_{ $interval_}{ $t_strat_name_ } = 0;
		}
		$strat_to_count_{ $t_strat_name_ }++;
		$interval_to_strat_to_bias_{ $interval_ }{ $t_strat_name_ } += $t_bias_words_[ 3 ];
		$total_interval_count_++;
		$total_interval_bias_ += $t_bias_words_[ 3 ];
	    }
	}
	foreach my $t_strat_name_ ( keys %strat_to_count_ )
	{
	    $interval_to_strat_to_bias_{ $interval_ }{ $t_strat_name_ } /= $strat_to_count_{ $t_strat_name_ };
	}
	if( $total_interval_count_ > 0 )
	{
	    $interval_to_strat_to_bias_{ $interval_ }{ "AVERAGE_BIAS" } = $total_interval_bias_ / $total_interval_count_;
	}
	else
	{
	    $interval_to_strat_to_bias_{ $interval_ }{ "AVERAGE_BIAS" } = 0;
	}
    }
}

sub CompensatePerIntervalSimRealBias
{
    foreach my $strat_name_ ( @all_strats_ )
    {
	foreach my $interval_ ( sort { $a<=>$b } keys %intervals_to_pick_from_ )
	{
	    my $sim_real_bias_ppc_ = 0;
	    if ( exists ( $interval_to_strat_to_bias_{ $interval_ }{ $strat_name_ } ) )
	    {
		$sim_real_bias_ppc_ = $interval_to_strat_to_bias_{ $interval_ }{ $strat_name_ };
	    }
	    else
	    {
		$sim_real_bias_ppc_ = $interval_to_strat_to_bias_{ $interval_ }{ "AVERAGE_BIAS" };
	    }

	    if( exists ( $interval_to_global_results_{ $interval_ }{ $strat_name_ } ) )
	    {
		my @global_result_words_ = split( ' ' , $interval_to_global_results_{ $interval_ }{ $strat_name_ } );
		$global_result_words_[ 0 ] -= ( $sim_real_bias_compensation_ * ( $sim_real_bias_ppc_ * $global_result_words_ [ 1 ] ) );
		my $t_global_result_ = sprintf ( "%d %d %0.2f %d %0.2f %d %0.2f" , $global_result_words_ [ 0 ] , $global_result_words_ [ 1 ] , $global_result_words_ [ 2 ] , $global_result_words_ [ 3 ] , $global_result_words_ [ 4 ] , $global_result_words_ [ 5 ] , $sim_real_bias_ppc_ );

		$interval_to_bias_adjusted_global_results_{ $interval_ }{ $strat_name_ } = $t_global_result_;
	    }
	}
    }
}

sub GetSimRealBiasForDate
{
    my ( $yyyymmdd_ ) = @_ ;

    my @t_bias_lines_ = ( );    

    my $bias_file_name_ = "$SIM_REAL_BIAS_DIR_/$shortcode_/".substr( $yyyymmdd_ , 0 , 4 )."/".substr( $yyyymmdd_ , 4 , 2 )."/".substr( $yyyymmdd_ , 6 , 2 )."/sim_real_bias.txt";

    if( ExistsWithSize ( $bias_file_name_ ) )
    {
	open ( BIAS_FILE , "<" , $bias_file_name_ ) or PrintStacktraceAndDie( "Could Not Open $bias_file_name_\n" );

	@t_bias_lines_ = <BIAS_FILE>;
	chomp( @t_bias_lines_ );

	close( BIAS_FILE );
    }

    return @t_bias_lines_ ;
}

sub ShowResultsForDate
{
	#print "$mail_body_\n";
	for ( my $i = 0; $i <= $#picked_strats_; $i++ )
	{
		my $t_strat_name_ = $picked_strats_[ $i ];
		my $exec_cmd = "$LIVE_BIN_DIR/summarize_single_strategy_results $shortcode_ $t_strat_name_ /NAS1/ec2_globalresults/ $date_to_run_for_ $date_to_run_for_";
		#print "$exec_cmd\n";
		my @final_res_lines_ = `$exec_cmd`;
		print "@final_res_lines_\n";
	}
}
