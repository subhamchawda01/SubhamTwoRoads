#!/usr/bin/perl

# \file ModelScripts/bootstrap_add_replace_perturb.pl
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
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib/";

require "$GENPERLLIB_DIR/sqrt_sign.pl"; # SqrtSign
#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

#require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
#require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
#require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
#require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr

require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

require "$GENPERLLIB_DIR/get_basepx_strat_first_model.pl"; # GetBasepxStratFirstModel

require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllPIDSTerminated

sub ReadConfigFile;
sub InitializeWorkDir;

sub RunBootstrapLoop;

sub InstallAndReportResults;

sub UpdateBaseResults;

sub RunAIM;
sub CopyToAIMTempDirectory;

sub RunFBR;
sub CopyToFBRTempDirectory;

sub RunFBMFS;
sub CopyToFBMFSTempDirectory;

sub NotImprovedThisIteration;
sub UpdateVars;

sub GetOutSampleDates;
sub CalcScores;
sub CompareResultLines;

sub GetModelFromResultLine;
sub GetOutsampleScoreFromResultLine;
sub GetOutsampleVolumeFromResultLine;
sub GetOutsampleTtcFromResultLine;

sub GetModelFromStrat;
sub GetParamFromStrat;
sub CopyStratModelParamToLocalDir;

sub CopyOriginalConfigToWorkDir;
sub SetConfigStratList;
sub SetConfigField;

sub RemoveIntermediateFiles;

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";

my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $TRADELOG_DIR = "/spare/local/logs/tradelogs/";

my $SPARE_HOME = "/spare/local/".$USER."/";
my $BARP_WORK_DIR = $SPARE_HOME."BARP/";

my $AIM_SCRIPT = $MODELSCRIPTS_DIR."/add_and_perturb_indicator.pl";
my $FBR_SCRIPT = $MODELSCRIPTS_DIR."/replace_and_perturb_indicator.pl";

my $FBMFS_SCRIPT = $MODELSCRIPTS_DIR."/run_find_best_model.pl";

my @intermediate_files_ = ( );

# start
my $USAGE="$0 CONFIGFILE";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $config_file_ = $ARGV [ 0 ];

my $shortcode_ = "" ;
my $input_strat_list_file_ = "" ;
my $original_strat_file_ = "" ;
my $original_model_file_ = "" ;
my $ilist_ = "" ;
my $datagen_start_hhmm_ = "" ;
my $datagen_end_hhmm_ = "" ;
my $datagen_msecs_timeout_ = 1000 ;
my $datagen_l1events_timeout_ = 15 ;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;
my $pred_duration_ = 32;
my $predalgo_ = "na_e3" ;
my $filter_name_ = "fsg1" ;

my $last_trading_date_ = GetIsoDateFromStrMin1("TODAY-1"); #date is TODAY-1 by default
my $num_prev_days_ = 45;
my $outsample_start_date_ = GetIsoDateFromStrMin1("TODAY-15"); #date is TODAY-1 by default
my $outsample_end_date_ = GetIsoDateFromStrMin1("TODAY-1");
my $change_fraction_ = 2;
my $max_number_iterations_ = 15;
my $max_number_bootstrap_iterations_ = 15;
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );
my $search_algo_ = "gradalpha3";
my $debug_ = 0;
my $cost_fn_ = "kCNAPnlAverage";
my $max_ttc_check_ = 1;
my $min_vol_check_ = 1;
my $max_ttc_ = 1;
my $min_vol_ = 1000000;
my $num_perturbations_ = 4;
my $retain_more_pert_ = 1;
my $batch_size_ = $num_prev_days_/3;
my $find_best_model_version_ = 0;
my $outsample_improvement_thresh_ = 0.1;
my $abs_outsample_improvement_thresh_ = 20;
my $aim_model_list_file_ = "";
my $install_strats_ = 1;
my $outsample_ttc_cuttoff_factor_ = 1.2;
my $outsample_vol_cuttoff_factor_ = 0.8;

my $num_installed_ = 0;

my @input_strat_list_ = ();
my @input_model_list_ = ();
my @potential_model_list_ = ();
my @potential_score_vec_ = ();
my @strats_done_ = ();
my @strats_to_do_ = ();
my %local_strat_filename_to_input_strat_filename_ = ( );
my $mail_address_ = "sghosh\@dvcnj.com";
my $mail_body_ = "";
my $config_string_ = "";
my $start_time_ = `date`;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $config_base_ = basename ( $config_file_ ); chomp ( $config_base_ );
my $main_work_dir_ = $BARP_WORK_DIR."/".$config_base_."/".$unique_gsm_id_."/"; 

my $work_dir_ = "" ;

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $main_work_dir_ )
    {
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$main_work_dir_ = $BARP_WORK_DIR."/".$config_base_."/".$unique_gsm_id_."/";
    }
    else
    {
	last;
    }
}

`mkdir -p $main_work_dir_ `;
print "main_work_dir_ : $main_work_dir_\n";

my $aim_work_dir_ = "";
my $fbr_work_dir_ = "";
my $fbmfs_work_dir_ = "";

# create a copy of the original config file and operate on that.
my $local_config_file_ = $main_work_dir_.$config_base_;

my $main_log_file_ = $main_work_dir_."main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "$0 Could not open log file : $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);
$mail_body_ = $mail_body_."logfile : $main_log_file_\n";

$mail_body_ = $mail_body_."start time : $start_time_\n-------------------------------------------------------------------------\n";

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

# The maps below maintain a table of results.
# a typical entry is like :
# strat_to_iteration_to_fbrresults [ STRATEGY_NAME ] [ ITERATION ] = "BEST_MODEL_NAME INSAMPLE_SCORE INSAMPLE_VOL INSAMPLE_TTC OUTSAMPLE_SCORE OUTSAMPLE_VOL OUTSAMPLE_TTC SECONDS_SPENT";
# STRATEGY_NAME is provided as a list in the config
# ITERATION is the iteration ( 1 , MAX_NUMBER_BOOTSTRAP_ITERATIONS )
# BEST_MODEL_NAME is the best model derived for this method , strat , iteration
# SECONDS_SPENT is the amount of time needed to optimize this , 
#               aim is to use this later to see which parts are holding up the system.
my %strat_to_iteration_to_baseresults_ = ( ); my %strat_to_iteration_to_basestrat_ = ( );
my %strat_to_iteration_to_aimresults_ = ( ); my %strat_to_iteration_to_aimstrat_ = ( );
my %strat_to_iteration_to_fbrresults_ = ( ); my %strat_to_iteration_to_fbrstrat_ = ( );
my %strat_to_iteration_to_fbmfsresults_ = ( ); my %strat_to_iteration_to_fbmfsstrat_ = ( );

my %iteration_to_strat_no_to_best_strat_ = ( );
my %iteration_to_strat_no_to_results_ = ( );

ReadConfigFile ( );

InitializeWorkDir ( );

RunBootstrapLoop ( );

InstallAndReportResults ( );

RemoveIntermediateFiles ( );

exit ( 0 );

sub RemoveIntermediateFiles
{
    foreach my $intermediate_file_ ( @intermediate_files_ )
    {
	my $exec_cmd_ = "rm -f $intermediate_file_";
	`$exec_cmd_`;
    }

    return;
}

sub ReadConfigFile
{
    print $main_log_file_handle_ "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";

    print $main_log_file_handle_ "ReadConfigFile $config_file_\n";
    open CONFIGFILEHANDLE, "+< $config_file_ " or PrintStacktraceAndDie ( "$0 Could not open config file : $config_file_ for reading\n" );
    
    my $current_instruction_="";
    my $current_instruction_set_ = 0;

    while ( my $thisline_ = <CONFIGFILEHANDLE> )
    {
        chomp($thisline_);
        my @thisline_words_  = split(' ', $thisline_);
        if($#thisline_words_ < 0)
        {
            $current_instruction_="";
            $current_instruction_set_ = 0;
            print $main_log_file_handle_ "\n";
            next;
        }
        if(substr ( $thisline_words_[0], 0, 1) eq '#')  {next;}
        if($current_instruction_set_ == 0)
        {
            $current_instruction_ = $thisline_words_[0];
            $current_instruction_set_ = 1;
            print $main_log_file_handle_ "current_instruction_ set to $current_instruction_\n";
            next;
        }
        given($current_instruction_)
        {
            print $main_log_file_handle_ "current_instruction_ : $current_instruction_\n";
            print $main_log_file_handle_ "current_instruction_value_ : $thisline_words_[0]\n";
            when("SHORTCODE")
            {
                $shortcode_ = $thisline_words_[0];
                print $main_log_file_handle_ "shortcode_ : $shortcode_\n";
            }
            when("ILIST")
            {
                $ilist_ = $thisline_words_[0];
                print $main_log_file_handle_ "ilist_ : $ilist_\n";
            }

            when("DATAGEN_START_END_HHMM")
            {
                $datagen_start_hhmm_ = $thisline_words_[0];
		$datagen_end_hhmm_ = $thisline_words_[1];
                print $main_log_file_handle_ "datagen_start_hhmm_ : $datagen_start_hhmm_\n";
		print $main_log_file_handle_ "datagen_end_hhmm_ : $datagen_end_hhmm_\n";
            }
            when("DATAGEN_TIMEOUT")
            {
                print $main_log_file_handle_ "datagen_msecs_timeout_ : $datagen_msecs_timeout_ , datagen_l1events_timeout_ : $datagen_l1events_timeout_, datagen_num_trades_timeout_ : $datagen_num_trades_timeout_\n";
            }
            when("TO_PRINT_ON_ECO")
            {
                $to_print_on_economic_times_ = $thisline_words_[0];
                print $main_log_file_handle_ "to_print_on_economic_times_ : $to_print_on_economic_times_\n";
            }
            when("PREDDURATION")
            {
                $pred_duration_ = $thisline_words_[0];
                print $main_log_file_handle_ "pred_duration_ : $pred_duration_\n";
            }
            when("PREDALGO")
            {
                $predalgo_ = $thisline_words_[0];
                print $main_log_file_handle_ "predalgo_ : $predalgo_\n";
            }
            when("DEP_BASED_FILTERS")
            {
                $filter_name_ = $thisline_words_[0];
                print $main_log_file_handle_ "filter_name_ : $filter_name_\n";
            }

            when("LAST_TRADING_DATE")
            {
                $last_trading_date_ = GetIsoDateFromStrMin1($thisline_words_[0]);
                print $main_log_file_handle_ "last_trading_date_ : $last_trading_date_\n";
            }
            when("NUM_PREV_DAYS")
            {
                $num_prev_days_ = $thisline_words_[0];
            }            
            when("CHANGE_FRACTION")
            {
                $change_fraction_ = $thisline_words_[0];
            }            
            when("MAX_NUMBER_ITERATIONS")
            {
                $max_number_iterations_ = $thisline_words_[0];
            }            
            when("MAX_NUMBER_BOOTSTRAP_ITERATIONS")
            {
                $max_number_bootstrap_iterations_ = $thisline_words_[0];
            }            
            when("MAX_CORES_TO_USE_IN_PARALLEL")
            {
                $MAX_CORES_TO_USE_IN_PARALLEL = $thisline_words_[0];
            }            
            when("SEARCH_ALGO")
            {
                $search_algo_ = $thisline_words_[0];
            }            
            when("DEBUG")
            {
                $debug_ = $thisline_words_[0];
            }            
            when("COST_FN")
            {
                $cost_fn_ = $thisline_words_[0];
            }     
            when("MAX_TTC_CHECK")
            {
                $max_ttc_check_ = $thisline_words_[0];
            }     
            when("MIN_VOL_CHECK")
            {
                $min_vol_check_ = $thisline_words_[0];
            }     
            when("MAX_TTC")
            {
                $max_ttc_ = $thisline_words_[0];
            }
            when("MIN_VOL")
            {
                $min_vol_ = $thisline_words_[0];
            }
            when("NUM_PERTURBATIONS")
            {
                $num_perturbations_ = $thisline_words_[0];
            }
            when("RETAIN_MORE_PERT")
            {
                $retain_more_pert_ = $thisline_words_[0];
            }
            when("BATCH_SIZE")
            {
                $batch_size_ = $thisline_words_[0];
            }
            when("FIND_BEST_MODEL_VERSION")
            {
                $find_best_model_version_ = $thisline_words_[0];
            }
            when("INPUT_STRAT_LIST_FILE")
            {
                $input_strat_list_file_ = $thisline_words_[0];
                open STRATFILEHANDLE, "< $input_strat_list_file_ " or PrintStacktraceAndDie ( "$0 Could not open INPUT_STRAT_LIST_FILE : $input_strat_list_file_ for reading\n" );
                while(my $strat_ = <STRATFILEHANDLE>)
                {
                    chomp($strat_);
		    if ( ExistsWithSize ( $strat_ ) )
		    { # if already given a full path
			if ( HasEARTHModel ( $strat_ ) )
			{
			    print $main_log_file_handle_ "Ignoring EARTH model strategy $strat_\n";
			}
			else
			{
			    push ( @strats_to_do_ , $strat_ );
			}
		    }
		    else
		    {
			my $strat_full_path_ = `find $HOME_DIR/modelling/strats/ -name $strat_`; chomp($strat_full_path_);
			if( ExistsWithSize ($strat_full_path_) )
			{ 
			    if ( HasEARTHModel ( $strat_full_path_ ) )
			    {
				print $main_log_file_handle_ "Ignoring EARTH model strategy $strat_full_path_\n";
			    }
			    else
			    {
				push ( @strats_to_do_ , $strat_ );
			    }
			}
		    }
                }
                close STRATFILEHANDLE;

            }
            when("STRATS_DONE")
            {
		if ( ExistsWithSize ( $thisline_words_ [ 0 ] ) )
		{
		    push ( @strats_done_ , $thisline_words_ [ 0 ] );
		}
	    }
            when("OUTSAMPLE_IMPROVEMENT_THRESH")
            {
                $outsample_improvement_thresh_ = $thisline_words_[0];
            }
            when("ABS_OUTSAMPLE_IMPROVEMENT_THRESH")
            {
                $abs_outsample_improvement_thresh_ = $thisline_words_[0];
            }
            when("OUTSAMPLE_START_DATE")
            {
                $outsample_start_date_ = GetIsoDateFromStrMin1($thisline_words_[0]);
            }
            when("OUTSAMPLE_END_DATE")
            {
                $outsample_end_date_ = GetIsoDateFromStrMin1($thisline_words_[0]);
            }
            when("MAIL_ADDRESS")
            {
                $mail_address_ = $thisline_words_[0];
            }
            when("INSTALL_STRATS")
            {
                $install_strats_ = $thisline_words_[0];
            }
            when("OUTSAMPLE_TTC_CUTTOFF_FACTOR")
            {
                $outsample_ttc_cuttoff_factor_ = $thisline_words_[0];
            }
            when("OUTSAMPLE_VOL_CUTTOFF_FACTOR")
            {
                $outsample_vol_cuttoff_factor_ = $thisline_words_[0];
            }
            default
            {
            }
        }
    }
    my $fbm_version_string_ = $find_best_model_version_?"v1:find_best_model_for_strategy_var_pert_single_indicator.pl":"v0:find_best_model_for_strategy_random_dates_single_indicator.pl";
    $config_string_ = $config_string_."FIND_BEST_MODEL_SINGLE_INDICATOR_VERSION : $fbm_version_string_\n";
    $config_string_ = $config_string_."COST_FN : $cost_fn_\n";
    
    close CONFIGFILEHANDLE;

    if ($#strats_done_ == -1 && `grep -ci "STRATS_DONE" $config_file_` == 0)   #To be used to keep track of strats already run
    {
	`echo "" >> $config_file_`;
	`echo "STRATS_DONE" >> $config_file_`;
    }

    for (my $strat_id_ = 0; $strat_id_ <= $#strats_to_do_; $strat_id_++)
    {
	my $temp_strat_ = $strats_to_do_[$strat_id_];
	if ($temp_strat_ ~~ @strats_done_)
	{
	    print $main_log_file_handle_ "Strat already done: $temp_strat_\n";
	}
	else
	{
	    push(@input_strat_list_, $temp_strat_);
	    print $main_log_file_handle_ "Strat running for the first time: $temp_strat_\n";
	}
    }

    return;
}

sub InitializeWorkDir
{
    print $main_log_file_handle_ "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    print $main_log_file_handle_ "InitializeWorkDir $main_work_dir_\n\n";

    my $exec_cmd_ = "";

    # copy config file to work dir
    CopyOriginalConfigToWorkDir ( $config_file_ );

    # copy initial strategies , models & params.
    $exec_cmd_ = "mkdir -p ".$main_work_dir_."strats ".$main_work_dir_."models ".$main_work_dir_."params";
    print $main_log_file_handle_ "- ".$exec_cmd_."\n";
    `$exec_cmd_`;
    for ( my $t_strategy_no_ = 0 ; $t_strategy_no_ <= $#strats_to_do_ ; $t_strategy_no_ ++ )
    {
	my $t_strategy_ = $strats_to_do_ [ $t_strategy_no_ ];

	CopyStratModelParamToLocalDir ( $t_strategy_ , $main_work_dir_."strats" , $main_work_dir_."models" , $main_work_dir_."params" );

	my $input_strat_filename_ = $strats_to_do_ [ $t_strategy_no_ ];

	$strats_to_do_ [ $t_strategy_no_ ] = $main_work_dir_."strats/".basename ( $t_strategy_ );

	$local_strat_filename_to_input_strat_filename_ { $strats_to_do_ [ $t_strategy_no_ ] } = $input_strat_filename_;
    }

    return;
}

sub RunBootstrapLoop
{
    print $main_log_file_handle_ "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    print $main_log_file_handle_ "RunBootstrapLoop\n\n";

    for ( my $current_strat_no_ = 0 ; $current_strat_no_ <= $#strats_to_do_ ; $current_strat_no_ ++ )
    {
	my $current_strat_ = $strats_to_do_ [ $current_strat_no_ ];

	print $main_log_file_handle_ ">>>> Current strat : ".$current_strat_."\n\n";

	for ( my $current_iter_ = 1 ; $current_iter_ <= $max_number_bootstrap_iterations_ ; $current_iter_ ++ )
	{
	    print $main_log_file_handle_ "\t>>>> Iteration : ".$current_iter_."\n\n";
	    $work_dir_ = $main_work_dir_."strat_file_s".$current_strat_no_."_i".$current_iter_."/"; `mkdir -p $work_dir_`;
	    $aim_work_dir_ = $main_work_dir_."AIM_s".$current_strat_no_."_i".$current_iter_; `mkdir -p $aim_work_dir_`;
	    $fbr_work_dir_ = $main_work_dir_."FBR_s".$current_strat_no_."_i".$current_iter_; `mkdir -p $fbr_work_dir_`;
	    $fbmfs_work_dir_ = $main_work_dir_."FBMFS_s".$current_strat_no_."_i".$current_iter_; `mkdir -p $fbmfs_work_dir_`;
	    
	    if ( $current_iter_ == 1 )
	    { # first iteration , set best_results for iteration = 0 as baseresults for iteration = 1
		UpdateBaseResults ( $current_strat_ , $current_iter_ , $current_strat_no_ );
		UpdateVars ( $current_strat_ , $current_iter_ , $current_strat_no_ );

		$iteration_to_strat_no_to_results_ { 0 } { $current_strat_no_ } = $strat_to_iteration_to_baseresults_ { $current_strat_ } { $current_iter_ };
		$iteration_to_strat_no_to_best_strat_ { 0 } { $current_strat_no_ } = $strat_to_iteration_to_basestrat_ { $current_strat_ } { $current_iter_ };
	    }

	    # run add_and_perturb and find the best output.
	    RunAIM ( $current_strat_ , $current_iter_ , $current_strat_no_ );
	    UpdateVars ( $current_strat_ , $current_iter_ , $current_strat_no_ );
	    
	    # run replace_and_perturb and find the best output.
	    RunFBR ( $current_strat_ , $current_iter_ , $current_strat_no_ );
	    UpdateVars ( $current_strat_ , $current_iter_ , $current_strat_no_ );
	    
	    # run fbmfs on the best output.
	    RunFBMFS ( $current_strat_ , $current_iter_ , $current_strat_no_ );
	    UpdateVars ( $current_strat_ , $current_iter_ , $current_strat_no_ );
	    
	    # check if any improvements were made this iteration over the last iteration
	    if ( NotImprovedThisIteration ( $current_strat_ , $current_iter_ , $current_strat_no_ ) )
	    { # no improvements , break.
		last;
	    }
	}
    }

    return;
}

sub RunAIM
{
    my ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ ) = @_;
    
    if ( exists ( $strat_to_iteration_to_aimresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    { # Check if somehow these results are already available.
	return;
    }

    print $main_log_file_handle_ "\n +RunAIM ( $t_current_strat_ , $t_current_iter_ )\n";

    CopyOriginalConfigToWorkDir ( $config_file_ );

    # use the best strat so far in this iteration as the seed to this iteration.
    my $t_aim_seed_strat_ = CopyToAIMTempDirectory ( $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ } { $t_current_strat_no_ } );
    my $t_aim_strat_list_ = $aim_work_dir_."/strat_list.txt";
    SetConfigStratList ( $local_config_file_ , $t_aim_seed_strat_ , $t_aim_strat_list_ );
    SetConfigField ( $local_config_file_ , "OUTSAMPLE_IMPROVEMENT_THRESH" , ( ( $outsample_improvement_thresh_ / 2 ) / $max_number_bootstrap_iterations_ ) );
    SetConfigField ( $local_config_file_ , "ABS_OUTSAMPLE_IMPROVEMENT_THRESH" , ( ( $abs_outsample_improvement_thresh_ / 2 ) / $max_number_bootstrap_iterations_ ) );

    my $exec_cmd_ = $AIM_SCRIPT." ".$local_config_file_;
    print $main_log_file_handle_ "$exec_cmd_\n";
    my @exec_cmd_output_ = `$exec_cmd_ 2>&1`; chomp ( @exec_cmd_output_ );
    print $main_log_file_handle_ join ( "\n" , @exec_cmd_output_ )."\n";

    foreach my $line_ ( @exec_cmd_output_ )
    {
    	if ( index ( $line_ , "OUTPUT " ) >= 0 )
    	{
    	    my @line_words_ = split ( ' ' , $line_ ); chomp ( @line_words_ );
    	    if ( $#line_words_ >= 1 &&
		 $line_words_ [ 0 ] eq "OUTPUT" && 
    		 $line_words_ [ 1 ] eq "NO_INSTALLATION" )
    	    { # seed model could not be improved.
		# set aim results same as base results this iteration.
		$strat_to_iteration_to_aimstrat_ { $t_current_strat_ } { $t_current_iter_ } = $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ } { $t_current_strat_no_ };
		$strat_to_iteration_to_aimresults_ { $t_current_strat_ } { $t_current_iter_ } = $iteration_to_strat_no_to_results_ { $t_current_iter_ } { $t_current_strat_no_ };

		print $main_log_file_handle_ "No improvements , storing baseresults\n";
		print $main_log_file_handle_ " strat_to_iteration_to_aimresults_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$strat_to_iteration_to_aimresults_ { $t_current_strat_ } { $t_current_iter_ }."\n";
		print $main_log_file_handle_ " strat_to_iteration_to_aimstrat_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$strat_to_iteration_to_aimstrat_ { $t_current_strat_ } { $t_current_iter_ }."\n";
    	    }
    	    elsif ( $#line_words_ >= 1 &&
		    $line_words_ [ 0 ] eq "OUTPUT" && 
    		    $line_words_ [ 1 ] eq "INSTALLATION" )
    	    { # pick up improved model
		my $aim_opt_strat_ = $line_words_ [ 2 ];

		print $main_log_file_handle_ "Improvements ! Computing aimresults ( $aim_opt_strat_ )\n";
		UpdateAIMResults ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ , $aim_opt_strat_ );
    	    }
    	}
    }

    return;
}

sub RunFBR
{
    my ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ ) = @_;
    
    if ( exists ( $strat_to_iteration_to_fbrresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    { # Check if somehow these results are already available.
	return;
    }

    print $main_log_file_handle_ "\n +RunFBR ( $t_current_strat_ , $t_current_iter_ )\n";

    CopyOriginalConfigToWorkDir ( $config_file_ );

    # use the best strat from this iteration as the seed to this iteration.
    my $t_fbr_seed_strat_ = CopyToFBRTempDirectory ( $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ } { $t_current_strat_no_ } );
    my $t_fbr_strat_list_ = $fbr_work_dir_."/strat_list.txt";
    SetConfigStratList ( $local_config_file_ , $t_fbr_seed_strat_ , $t_fbr_strat_list_ );
    SetConfigField ( $local_config_file_ , "OUTSAMPLE_IMPROVEMENT_THRESH" , ( ( $outsample_improvement_thresh_ / 2 ) / $max_number_bootstrap_iterations_ ) );
    SetConfigField ( $local_config_file_ , "ABS_OUTSAMPLE_IMPROVEMENT_THRESH" , ( ( $abs_outsample_improvement_thresh_ / 2 ) / $max_number_bootstrap_iterations_ ) );

    my $exec_cmd_ = $FBR_SCRIPT." ".$local_config_file_;
    print $main_log_file_handle_ "$exec_cmd_\n";
    my @exec_cmd_output_ = `$exec_cmd_ 2>&1`; chomp ( @exec_cmd_output_ );
    print $main_log_file_handle_ join ( "\n" , @exec_cmd_output_ )."\n";

    foreach my $line_ ( @exec_cmd_output_ )
    {
    	if ( index ( $line_ , "OUTPUT " ) >= 0 )
    	{
    	    my @line_words_ = split ( ' ' , $line_ ); chomp ( @line_words_ );
    	    if ( $#line_words_ >= 1 &&
		 $line_words_ [ 0 ] eq "OUTPUT" && 
    		 $line_words_ [ 1 ] eq "NO_INSTALLATION" )
    	    { # seed model could not be improved.
		# set fbr results same as base results this iteration.
		$strat_to_iteration_to_fbrstrat_ { $t_current_strat_ } { $t_current_iter_ } = $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ } { $t_current_strat_no_ };
		$strat_to_iteration_to_fbrresults_ { $t_current_strat_ } { $t_current_iter_ } = $iteration_to_strat_no_to_results_ { $t_current_iter_ } { $t_current_strat_no_ };

		print $main_log_file_handle_ "No improvements , storing baseresults\n";
		print $main_log_file_handle_ " strat_to_iteration_to_fbrresults_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$strat_to_iteration_to_fbrresults_ { $t_current_strat_ } { $t_current_iter_ }."\n";
		print $main_log_file_handle_ " strat_to_iteration_to_fbrstrat_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$strat_to_iteration_to_fbrstrat_ { $t_current_strat_ } { $t_current_iter_ }."\n";
    	    }
    	    elsif ( $#line_words_ >= 1 &&
		    $line_words_ [ 0 ] eq "OUTPUT" && 
    		    $line_words_ [ 1 ] eq "INSTALLATION" )
    	    { # pick up improved model
		my $fbr_opt_strat_ = $line_words_ [ 2 ];

		print $main_log_file_handle_ "Improvements ! Computing fbrresults ( $fbr_opt_strat_ )\n";
		UpdateFBRResults ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ , $fbr_opt_strat_ );
    	    }
    	}
    }

    return;
}

sub RunFBMFS
{
    my ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ ) = @_;

    if ( exists ( $strat_to_iteration_to_fbmfsresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    { # Check if somehow these results are already available.
	return;
    }

    print $main_log_file_handle_ "\n +RunFBMFS ( $t_current_strat_ , $t_current_iter_ )\n";

    CopyOriginalConfigToWorkDir ( $config_file_ );
    # use the best strat so far in this iteration as the seed to this iteration.
    my $t_fbmfs_seed_strat_ = CopyToFBMFSTempDirectory ( $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ } { $t_current_strat_no_ } );
    my $t_fbmfs_strat_list_ = $fbmfs_work_dir_."/strat_list.txt";
    SetConfigStratList ( $local_config_file_ , $t_fbmfs_seed_strat_ , $t_fbmfs_strat_list_ );
    SetConfigField ( $local_config_file_ , "OUTSAMPLE_IMPROVEMENT_THRESH" , ( ( $outsample_improvement_thresh_ / 2 ) / $max_number_bootstrap_iterations_ ) );
    SetConfigField ( $local_config_file_ , "ABS_OUTSAMPLE_IMPROVEMENT_THRESH" , ( ( $abs_outsample_improvement_thresh_ / 2 ) / $max_number_bootstrap_iterations_ ) );

    my $exec_cmd_ = $FBMFS_SCRIPT." ".$local_config_file_;
    print $main_log_file_handle_ "$exec_cmd_\n";
    my @exec_cmd_output_ = `$exec_cmd_ 2>&1`; chomp ( @exec_cmd_output_ );
    print $main_log_file_handle_ join ( "\n" , @exec_cmd_output_ )."\n";
    
    foreach my $line_ ( @exec_cmd_output_ )
    {
    	if ( index ( $line_ , "OUTPUT " ) >= 0 )
    	{
    	    my @line_words_ = split ( ' ' , $line_ ); chomp ( @line_words_ );
    	    if ( $#line_words_ >= 1 &&
		 $line_words_ [ 0 ] eq "OUTPUT" && 
    		 $line_words_ [ 1 ] eq "NO_INSTALLATION" )
    	    { # seed model could not be improved.
		# set fbmfs results same as base results this iteration.
		$strat_to_iteration_to_fbmfsstrat_ { $t_current_strat_ } { $t_current_iter_ } = $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ } { $t_current_strat_no_ };
		$strat_to_iteration_to_fbmfsresults_ { $t_current_strat_ } { $t_current_iter_ } = $iteration_to_strat_no_to_results_ { $t_current_iter_ } { $t_current_strat_no_ };

		print $main_log_file_handle_ "No improvements , storing baseresults\n";
		print $main_log_file_handle_ " strat_to_iteration_to_fbmfsresults_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$strat_to_iteration_to_fbmfsresults_ { $t_current_strat_ } { $t_current_iter_ }."\n";
		print $main_log_file_handle_ " strat_to_iteration_to_fbmfsstrat_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$strat_to_iteration_to_fbmfsstrat_ { $t_current_strat_ } { $t_current_iter_ }."\n";
    	    }
    	    elsif ( $#line_words_ >= 1 &&
		    $line_words_ [ 0 ] eq "OUTPUT" && 
    		    $line_words_ [ 1 ] eq "INSTALLATION" )
    	    { # pick up improved model
		my $fbmfs_opt_strat_ = $line_words_ [ 2 ];

		print $main_log_file_handle_ "Improvements ! Computing fbmfsresults ( $fbmfs_opt_strat_ )\n";
		UpdateFBMFSResults ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ , $fbmfs_opt_strat_ );
    	    }
    	}
    }

    return;
}

sub GetModelFromResultLine
{
    my ( $t_result_line_ ) = @_;

    my @result_line_words_ = split ( ' ' , $t_result_line_ ); chomp ( @result_line_words_ );

    my $t_model_ = "";
    if ( $#result_line_words_ >= 0 )
    {
	$t_model_ = $result_line_words_ [ 0 ];
    }

    return $t_model_;
}

sub GetOutsampleScoreFromResultLine
{
    my ( $t_result_line_ ) = @_;

    my @result_line_words_ = split ( ' ' , $t_result_line_ ); chomp ( @result_line_words_ );

    my $t_score_ = "";
    if ( $#result_line_words_ >= 4 )
    {
	$t_score_ = $result_line_words_ [ 4 ];
    }

    return $t_score_;
}

sub GetOutsampleVolumeFromResultLine
{
    my ( $t_result_line_ ) = @_;

    my @result_line_words_ = split ( ' ' , $t_result_line_ ); chomp ( @result_line_words_ );

    my $t_volume_ = "";
    if ( $#result_line_words_ >= 5 )
    {
	$t_volume_ = $result_line_words_ [ 5 ];
    }

    return $t_volume_;
}

sub GetOutsampleTtcFromResultLine
{
    my ( $t_result_line_ ) = @_;

    my @result_line_words_ = split ( ' ' , $t_result_line_ ); chomp ( @result_line_words_ );

    my $t_ttc_ = "";
    if ( $#result_line_words_ >= 6 )
    {
	$t_ttc_ = $result_line_words_ [ 6 ];
    }

    return $t_ttc_;
}

sub GetParamFromStrat
{
    my ( $t_strat_ ) = @_;

    my @output_ = `cat $t_strat_`;
    chomp ( @output_ );
    my @strat_output_ = split ( ' ' , $output_ [ 0 ] ); chomp ( @strat_output_ );
    my $t_param_ = $strat_output_ [ 4 ];

    return $t_param_;
}

sub CopyStratModelParamToLocalDir
{
    my ( $t_strat_path_ , $strat_dir_ , $model_dir_ , $param_dir_ ) = @_;

    my $t_model_path_ = GetModelFromStrat ( $t_strat_path_ );
    my $t_param_path_ = GetParamFromStrat ( $t_strat_path_ );

    my $exec_cmd_ = "cp $t_strat_path_ $strat_dir_";
    print $main_log_file_handle_ $exec_cmd_."\n"; `$exec_cmd_`;

    my $t_local_strat_path_ = $strat_dir_."/".basename ( $t_strat_path_ );

    $exec_cmd_ = "cp $t_model_path_ $model_dir_";
    print $main_log_file_handle_ $exec_cmd_."\n"; `$exec_cmd_`;

    $exec_cmd_ = "cp $t_param_path_ $param_dir_";
    print $main_log_file_handle_ $exec_cmd_."\n"; `$exec_cmd_`;

    # edit the local copy to point to these copies of models & params.
    $exec_cmd_ = "cat ".$t_local_strat_path_." | awk '{ \$4=\"".$t_model_path_."\"; \$5=\"".$t_param_path_."\"; print \$0; }' > ".$t_local_strat_path_.".local";
    print $main_log_file_handle_ $exec_cmd_."\n"; `$exec_cmd_`;

    $exec_cmd_ = "mv ".$t_local_strat_path_.".local ".$t_local_strat_path_;
    print $main_log_file_handle_ $exec_cmd_."\n"; `$exec_cmd_`;

    return;
}

sub UpdateBaseResults
{
    my ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ ) = @_;

    if ( exists ( $strat_to_iteration_to_baseresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    { # check if base results are already available.
	return;
    }

    print $main_log_file_handle_ "\n +UpdateBaseResults ( $t_current_strat_ , $t_current_iter_ )\n";

    my $insample_results_line_ = RunSimAndComputeResultLine2 ( $t_current_strat_ , $last_trading_date_ , $num_prev_days_ , $t_current_strat_no_ );
    my $outsample_results_line_ = RunSimAndComputeResultLine ( $t_current_strat_ , $outsample_start_date_ , $outsample_end_date_ , $t_current_strat_no_ );

    my $result_line_ = basename ( GetModelFromStrat ( $t_current_strat_ ) )." ".$insample_results_line_." ".$outsample_results_line_." 0"; # seconds-spent = 0.

    $strat_to_iteration_to_baseresults_ { $t_current_strat_ } { $t_current_iter_ } = $result_line_;
    $strat_to_iteration_to_basestrat_ { $t_current_strat_ } { $t_current_iter_ } = $t_current_strat_;

    print $main_log_file_handle_ " strat_to_iteration_to_baseresults_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$result_line_."\n";
    print $main_log_file_handle_ " strat_to_iteration_to_basestrat_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$t_current_strat_."\n";

    return;
}

sub UpdateAIMResults
{
    my ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ , $t_aim_opt_strat_ ) = @_;

    if ( exists ( $strat_to_iteration_to_aimresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    { # check if aim results are already available.
	return;
    }

    print $main_log_file_handle_ "\n +UpdateAIMResults ( $t_current_strat_ , $t_current_iter_ , $t_aim_opt_strat_ )\n";

    my $insample_results_line_ = RunSimAndComputeResultLine2 ( $t_aim_opt_strat_ , $last_trading_date_ , $num_prev_days_ , $t_current_strat_no_ );
    my $outsample_results_line_ = RunSimAndComputeResultLine ( $t_aim_opt_strat_ , $outsample_start_date_ , $outsample_end_date_ , $t_current_strat_no_ );

    my $result_line_ = basename ( GetModelFromStrat ( $t_aim_opt_strat_ ) )." ".$insample_results_line_." ".$outsample_results_line_." 0"; # seconds-spent = 0.

    $strat_to_iteration_to_aimresults_ { $t_current_strat_ } { $t_current_iter_ } = $result_line_;
    $strat_to_iteration_to_aimstrat_ { $t_current_strat_ } { $t_current_iter_ } = $t_aim_opt_strat_;

    print $main_log_file_handle_ " strat_to_iteration_to_aimresults_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$result_line_."\n";
    print $main_log_file_handle_ " strat_to_iteration_to_aimstrat_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$t_aim_opt_strat_."\n";

    return;
}

sub UpdateFBRResults
{
    my ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ , $t_fbr_opt_strat_ ) = @_;

    if ( exists ( $strat_to_iteration_to_fbrresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    { # check if fbr results are already available.
	return;
    }

    print $main_log_file_handle_ "\n +UpdateFBRResults ( $t_current_strat_ , $t_current_iter_ , $t_fbr_opt_strat_ )\n";

    my $insample_results_line_ = RunSimAndComputeResultLine2 ( $t_fbr_opt_strat_ , $last_trading_date_ , $num_prev_days_ , $t_current_strat_no_ );
    my $outsample_results_line_ = RunSimAndComputeResultLine ( $t_fbr_opt_strat_ , $outsample_start_date_ , $outsample_end_date_ , $t_current_strat_no_ );

    my $result_line_ = basename ( GetModelFromStrat ( $t_fbr_opt_strat_ ) )." ".$insample_results_line_." ".$outsample_results_line_." 0"; # seconds-spent = 0.

    $strat_to_iteration_to_fbrresults_ { $t_current_strat_ } { $t_current_iter_ } = $result_line_;
    $strat_to_iteration_to_fbrstrat_ { $t_current_strat_ } { $t_current_iter_ } = $t_fbr_opt_strat_;

    print $main_log_file_handle_ " strat_to_iteration_to_fbrresults_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$result_line_."\n";
    print $main_log_file_handle_ " strat_to_iteration_to_fbrstrat_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$t_fbr_opt_strat_."\n";

    return;
}

sub UpdateFBMFSResults
{
    my ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ , $t_fbmfs_opt_strat_ ) = @_;

    if ( exists ( $strat_to_iteration_to_fbmfsresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    { # check if fbmfs results are already available.
	return;
    }

    print $main_log_file_handle_ "\n +UpdateFBMFSResults ( $t_current_strat_ , $t_current_iter_ , $t_fbmfs_opt_strat_ )\n";

    my $insample_results_line_ = RunSimAndComputeResultLine2 ( $t_fbmfs_opt_strat_ , $last_trading_date_ , $num_prev_days_ , $t_current_strat_no_ );
    my $outsample_results_line_ = RunSimAndComputeResultLine ( $t_fbmfs_opt_strat_ , $outsample_start_date_ , $outsample_end_date_ , $t_current_strat_no_ );

    my $result_line_ = basename ( GetModelFromStrat ( $t_fbmfs_opt_strat_ ) )." ".$insample_results_line_." ".$outsample_results_line_." 0"; # seconds-spent = 0.

    $strat_to_iteration_to_fbmfsresults_ { $t_current_strat_ } { $t_current_iter_ } = $result_line_;
    $strat_to_iteration_to_fbmfsstrat_ { $t_current_strat_ } { $t_current_iter_ } = $t_fbmfs_opt_strat_;

    print $main_log_file_handle_ " strat_to_iteration_to_fbmfsresults_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$result_line_."\n";
    print $main_log_file_handle_ " strat_to_iteration_to_fbmfsstrat_ [ ".basename ( $t_current_strat_ )." ] [ ".$t_current_iter_." ] = ".$t_fbmfs_opt_strat_."\n";

    return;
}

sub RunSimAndComputeResultLine
{
    my ( $t_current_strat_ , $start_date_ , $end_date_ , $t_current_strat_no_ ) = @_;

    my $result_line_ = "0 0 0";

    my @outsample_date_vec_ = ( );
    my $t_model_file_ = GetModelFromStrat ( $t_current_strat_ );

    GetOutSampleDates ( \@outsample_date_vec_ , $start_date_ , $end_date_ , $t_model_file_ );

    my @score_vec_ = ();
    my @vol_vec_ = ();
    my @ttc_vec_ = ();
    my @strat_list_ = ();

    push(@strat_list_, $t_current_strat_ );

    my $stats_string_ = CalcScores( \@strat_list_, \@score_vec_, \@vol_vec_, \@ttc_vec_, \@outsample_date_vec_, $t_current_strat_no_ );
    
    $result_line_ = $score_vec_ [ 0 ]." ".$vol_vec_ [ 0 ]." ".$ttc_vec_ [ 0 ];

    return $result_line_;
}

sub RunSimAndComputeResultLine2
{
    my ( $t_current_strat_ , $end_date_ , $num_prev_days_ , $t_current_strat_no_ ) = @_;

    my $start_date_ = CalcPrevWorkingDateMult ( $end_date_ , $num_prev_days_ );

    return RunSimAndComputeResultLine ( $t_current_strat_ , $start_date_ , $end_date_ , $t_current_strat_no_ );
}

sub CalcScores
{
    my ( $strat_list_, $stats_vec_ref_, $volume_vec_ref_, $ttc_vec_ref_, $date_vec_ref_, $strat_index_ ) = @_;
    my $local_results_base_dir = $work_dir_."/local_results_base_dir_".$strat_index_;

    my %models_list_map_ = ( );
    my $t_models_list_ = $work_dir_."/"."tmp_models_list_".$strat_index_;
    open OUTMODELLIST, "> $t_models_list_" or PrintStacktraceAndDie ( "Could not open output_models_list_filename_ $t_models_list_ for writing\n" );

    my $t_strat_filename_ = $work_dir_."tmp_strats_".$strat_index_;
    if(-e $t_strat_filename_)   {`rm -f $t_strat_filename_ `;}
    my $base_shortcode_ = `awk '{print \$2}' $$strat_list_[0]`; chomp($base_shortcode_);
    for (my $i =0; $i<=$#$strat_list_; $i++)
    {
        `cat $$strat_list_[$i] | awk '{\$8 = $i; print }' >> $t_strat_filename_`;
        my $t_model_file_ = `awk '{print \$4}' $$strat_list_[$i]`; chomp($t_model_file_);
        $t_model_file_ = `basename $t_model_file_`; chomp($t_model_file_);
        print OUTMODELLIST $t_model_file_."\n";
        $models_list_map_{$t_model_file_}=$i;
    }
    
    my @unique_sim_id_list_ = ( );
    my @independent_parallel_commands_ = ( );
    my @tradingdate_list_ = ( );
    my @temp_strategy_list_file_index_list_ = ( );
    my @temp_strategy_list_file_list_ = ( );
    my @temp_strategy_cat_file_list_ = ( );
    my @temp_strategy_output_file_list_ = ( );

    my @nonzero_tradingdate_vec_ = ( );
    my $start_date_ = "";
    my $end_date_ = "";

    for ( my $t_day_index_ = 0; $t_day_index_ <= $#$date_vec_ref_ ; $t_day_index_ ++ )
    {
        my $tradingdate_ = $$date_vec_ref_[$t_day_index_];
	push ( @tradingdate_list_ , $tradingdate_ );

	my $unique_sim_id_ = GetGlobalUniqueId ( );
	push ( @unique_sim_id_list_ , $unique_sim_id_ );

        my $t_sim_res_filename_ = $work_dir_."/"."sim_res_".$strat_index_."_".$tradingdate_;
	push ( @temp_strategy_output_file_list_ , $t_sim_res_filename_ );
	`> $t_sim_res_filename_`;

        my $market_model_index_ = GetMarketModelForShortcode ( $base_shortcode_ );
        my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ $unique_sim_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1 > $t_sim_res_filename_ 2>&1";
	push ( @independent_parallel_commands_ , $exec_cmd_ );

    }  

    for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; )
    {
	my @output_files_to_poll_this_run_ = ( );
	my @pids_to_poll_this_run_ = ( );

	my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
	for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ )
	{
	    my $t_sim_res_filename_ = $temp_strategy_output_file_list_ [ $command_index_ ];
	    my $exec_cmd_ = $independent_parallel_commands_ [ $command_index_ ];

	    push ( @output_files_to_poll_this_run_ , $t_sim_res_filename_ );

#	    print $main_log_file_handle_ $exec_cmd_."\n";
	    $exec_cmd_ = $exec_cmd_." & echo \$!";
	    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

	    if ( $#exec_cmd_output_ >= 0 )
	    {
		my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ 0 ] );
		if ( $#exec_cmd_output_words_ >= 0 )
		{
		    my $t_pid_ = $exec_cmd_output_words_ [ 0 ];
		    $t_pid_ =~ s/^\s+|\s+$//g;

		    push ( @pids_to_poll_this_run_ , $t_pid_ );
		}
	    }

	    $command_index_ ++;
	    sleep ( 1 );
	}	

	while ( ! AllPIDSTerminated ( @pids_to_poll_this_run_ ) )
	{ # there are still some sim-strats which haven't output SIMRESULT lines
	    sleep ( 1 );
	}
    }

    for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
    {
	my $t_sim_res_filename_ = $temp_strategy_output_file_list_ [ $command_index_ ];
	my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];
	my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];

	my @sim_strategy_output_lines_=();
	my %unique_id_to_pnlstats_map_ =();

	if ( ExistsWithSize ( $t_sim_res_filename_ ) )
	{	
	    push (@nonzero_tradingdate_vec_, $tradingdate_);
	    @sim_strategy_output_lines_=`cat $t_sim_res_filename_`;
	    #print $main_log_file_handle_ join(' ', @sim_strategy_output_lines_);
	}
	my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_sim_id_);
	my $this_logfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int($unique_sim_id_);
	#print $main_log_file_handle_ "trades file name ".$this_tradesfilename_."\n";
	if ( ExistsWithSize ( $this_tradesfilename_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_";
	    #      print $main_log_file_handle_ "$exec_cmd\n";
	    my @pnlstats_output_lines_ = `$exec_cmd`;
	    #      print $main_log_file_handle_ join(' ',@pnlstats_output_lines_);
	    for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
	    {
		my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
		if( $#rwords_ >= 1 )
		{
		    my $unique_sim_id_ = $rwords_[0];
		    splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_sim_id_
		    $unique_id_to_pnlstats_map_{$unique_sim_id_} = join ( ' ', @rwords_ );
		}
	    }
	    `rm -f $this_tradesfilename_`;
	    `rm -f $this_logfilename_`;
	}
	my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$strat_index_."_".$tradingdate_ ;
	open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );

	for ( my $t_sim_strategy_output_lines_index_ = 0, my $psindex_ = 0; $t_sim_strategy_output_lines_index_ <= $#sim_strategy_output_lines_; $t_sim_strategy_output_lines_index_ ++ )
	{
	    if ( $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] =~ /SIMRESULT/ )
	    { # SIMRESULT pnl volume sup% bestlevel% agg%
		my @rwords_ = split ( ' ', $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] );
		if ( $#rwords_ >= 2 )
		{
		    splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc
		    my $remaining_simresult_line_ = join ( ' ', @rwords_ );
		    if ( ( $rwords_[1] > 0 ) || # volume > 0
			 ( ( $base_shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day
		    {
			my $unique_sim_id_ = GetUniqueSimIdFromCatFile ( $t_strat_filename_, $psindex_ );
			if ( ! exists $unique_id_to_pnlstats_map_{$unique_sim_id_} )
			{
			    $unique_id_to_pnlstats_map_{$unique_sim_id_} = "0 0 0 0 0 0 0 0 0 0 0";
#                               PrintStacktraceAndDie ( "unique_id_to_pnlstats_map_ missing $unique_sim_id_ for listfile: $temp_results_list_file_ catfile: $temp_strategy_cat_file_ rline: $remaining_simresult_line_\n" );
			}
			#printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_ ;
			printf TRLF "%s %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_;
		    }
		}
		else
		{
		    PrintStacktraceAndDie ( "ERROR: SIMRESULT line has less than 3 words\n" );
		}
		$psindex_ ++;
	    }
	}
	close TRLF;
	if ( ExistsWithSize ( $temp_results_list_file_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database_2.pl $t_strat_filename_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir/$base_shortcode_"; # TODO init $local_results_base_dir
	    #print $main_log_file_handle_ "$exec_cmd\n";
	    my $this_local_results_database_file_ = `$exec_cmd`;
	    #push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
	    if ($end_date_ eq ""){  $end_date_ = $tradingdate_; }
	    $start_date_ = $tradingdate_;
	}
    }
    
    my $statistics_result_file_ = $work_dir_."/"."stats_res_file_".$strat_index_;
    my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $cost_fn_ > $statistics_result_file_";
    #print $main_log_file_handle_ "$exec_cmd\n";
    my $results_ = `$exec_cmd`;
    $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $cost_fn_ ";
    my $stats_string_ = `$exec_cmd`;
    for ( my $i = 0; $i <= $#$strat_list_ ; $i ++)
    {
	push ( @$stats_vec_ref_ , 0 );
	push ( @$volume_vec_ref_ , 0 );
	push ( @$ttc_vec_ref_ , 0 );
    }
    
#    if ( $DEBUG ) { $main_log_file_handle_->printf ( "Num days %s results = %d\n%s\n", $var_,( 1 + $#nonzero_tradingdate_vec_ ), join ( ' ', @nonzero_tradingdate_vec_ ) ) ; }
    if ( $#nonzero_tradingdate_vec_ >= 0 )
    { # at least 1 day
	if ( ExistsWithSize ($statistics_result_file_) )
	{
	    #if ( $DEBUG ) { $main_log_file_handle_->printf ( "reading result file %s\n", $statistics_result_file_ ) ; }
	    open(FILE, $statistics_result_file_) or die ("Unable to open statistics result file $statistics_result_file_");
	    my @t_stats_output_lines = <FILE>;
	    close(FILE);
	    for (my $j = 0; $j <= $#$strat_list_ ; $j ++ )
	    {
		my @t_stat_rwords_ = split ( ' ', $t_stats_output_lines[$j]);
		my $index = $models_list_map_{$t_stat_rwords_[1]};
		$$stats_vec_ref_[$index]=$t_stat_rwords_[-1];
		$$volume_vec_ref_[$index]=$t_stat_rwords_[4];
		$$ttc_vec_ref_[$index]=$t_stat_rwords_[9];
	    }

	}
    }
    return $stats_string_;
}

sub GetOutSampleDates
{
    @_ == 4 or die "GetOutSampleDates called with !=4 args\n";
    my ( $outsample_date_vec_ref_, $t_start_date_, $t_end_date_, $model_file_ ) = @_;

    my $tradingdate_ = $t_end_date_;
    while($tradingdate_ >= $t_start_date_)
    {
        my $exec_cmd_ = $GENPERLLIB_DIR."is_valid.pl $tradingdate_ $model_file_";
        my $is_valid_ = `$exec_cmd_`; chomp($is_valid_);
        if ( $is_valid_ == 1)
        {
            push ( @$outsample_date_vec_ref_, $tradingdate_ ); 
        }
        $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }

    return;
}

sub CopyOriginalConfigToWorkDir
{
    my ( $t_original_config_file_ ) = @_;

    my $exec_cmd_ = "cp ".$t_original_config_file_." ".$local_config_file_;
    print $main_log_file_handle_ $exec_cmd_."\n";
    `$exec_cmd_`;

    # edit local config file & add/set
    # NO_EMAIL = 1
    # INSTALL_STRATS = 0
    open ( my $file_handle_ , ">>" , $local_config_file_ );
    print $file_handle_ "\nNO_EMAIL\n1\n\nINSTALL_STRATS\n1\n";
    close ( $file_handle_ );

    return;
}

sub CopyToAIMTempDirectory
{
    my ( $t_strat_path_ ) = @_;

    my $exec_cmd_ = "";

    $exec_cmd_ = "mkdir -p ".$aim_work_dir_."/strats"; `$exec_cmd_`;
    $exec_cmd_ = "mkdir -p ".$aim_work_dir_."/models"; `$exec_cmd_`;
    $exec_cmd_ = "mkdir -p ".$aim_work_dir_."/params"; `$exec_cmd_`;

    CopyStratModelParamToLocalDir ( $t_strat_path_ , $aim_work_dir_."/strats" , $aim_work_dir_."/models" , $aim_work_dir_."/params" );

    my $aim_strat_path_ = $aim_work_dir_."/strats/".basename ( $t_strat_path_ );

    my $t_model_ = $aim_work_dir_."/models/".basename ( GetModelFromStrat ( $aim_strat_path_ ) );
    my $t_param_ = $aim_work_dir_."/params/".basename ( GetParamFromStrat ( $aim_strat_path_ ) );

    $exec_cmd_ = "cat $aim_strat_path_ | awk '{ \$4=\"".$t_model_."\"; \$5=\"".$t_param_."\"; print \$0; }' > ".$aim_strat_path_.".local";
    `$exec_cmd_`;
    $exec_cmd_ = "mv ".$aim_strat_path_.".local ".$aim_strat_path_;
    `$exec_cmd_`;

    return $aim_strat_path_;
}

sub CopyToFBRTempDirectory
{
    my ( $t_strat_path_ ) = @_;

    my $exec_cmd_ = "";

    $exec_cmd_ = "mkdir -p ".$fbr_work_dir_."/strats"; `$exec_cmd_`;
    $exec_cmd_ = "mkdir -p ".$fbr_work_dir_."/models"; `$exec_cmd_`;
    $exec_cmd_ = "mkdir -p ".$fbr_work_dir_."/params"; `$exec_cmd_`;

    CopyStratModelParamToLocalDir ( $t_strat_path_ , $fbr_work_dir_."/strats" , $fbr_work_dir_."/models" , $fbr_work_dir_."/params" );

    my $fbr_strat_path_ = $fbr_work_dir_."/strats/".basename ( $t_strat_path_ );

    my $t_model_ = $fbr_work_dir_."/models/".basename ( GetModelFromStrat ( $fbr_strat_path_ ) );
    my $t_param_ = $fbr_work_dir_."/params/".basename ( GetParamFromStrat ( $fbr_strat_path_ ) );

    $exec_cmd_ = "cat $fbr_strat_path_ | awk '{ \$4=\"".$t_model_."\"; \$5=\"".$t_param_."\"; print \$0; }' > ".$fbr_strat_path_.".local";
    `$exec_cmd_`;
    $exec_cmd_ = "mv ".$fbr_strat_path_.".local ".$fbr_strat_path_;
    `$exec_cmd_`;

    return $fbr_strat_path_;
}

sub CopyToFBMFSTempDirectory
{
    my ( $t_strat_path_ ) = @_;

    my $exec_cmd_ = "";

    $exec_cmd_ = "mkdir -p ".$fbmfs_work_dir_."/strats"; `$exec_cmd_`;
    $exec_cmd_ = "mkdir -p ".$fbmfs_work_dir_."/models"; `$exec_cmd_`;
    $exec_cmd_ = "mkdir -p ".$fbmfs_work_dir_."/params"; `$exec_cmd_`;

    CopyStratModelParamToLocalDir ( $t_strat_path_ , $fbmfs_work_dir_."/strats" , $fbmfs_work_dir_."/models" , $fbmfs_work_dir_."/params" );

    my $fbmfs_strat_path_ = $fbmfs_work_dir_."/strats/".basename ( $t_strat_path_ );

    my $t_model_ = $fbmfs_work_dir_."/models/".basename ( GetModelFromStrat ( $fbmfs_strat_path_ ) );
    my $t_param_ = $fbmfs_work_dir_."/params/".basename ( GetParamFromStrat ( $fbmfs_strat_path_ ) );

    $exec_cmd_ = "cat $fbmfs_strat_path_ | awk '{ \$4=\"".$t_model_."\"; \$5=\"".$t_param_."\"; print \$0; }' > ".$fbmfs_strat_path_.".local";
    `$exec_cmd_`;
    $exec_cmd_ = "mv ".$fbmfs_strat_path_.".local ".$fbmfs_strat_path_;
    `$exec_cmd_`;

    return $fbmfs_strat_path_;
}

sub SetConfigStratList
{
    my ( $t_config_file_ , $t_strat_file_ , $t_strat_file_list_ ) = @_;

    my $exec_cmd_ = "";
    open ( my $file_handle_ , ">" , $t_strat_file_list_ ) or PrintStacktraceAndDie ( "Could not open file $t_strat_file_list_" );

    print $file_handle_ $t_strat_file_."\n";

    close ( $file_handle_ );

    $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl $t_config_file_ INPUT_STRAT_LIST_FILE ".$t_strat_file_list_;
    print $main_log_file_handle_ $exec_cmd_."\n";
    `$exec_cmd_`;

    return;
}

sub SetConfigField
{
    my ( $t_config_file_ , $t_field_ , $t_value_ ) = @_;

    my $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl $t_config_file_ $t_field_ $t_value_";
    print $main_log_file_handle_ $exec_cmd_."\n";
    `$exec_cmd_`;

    return;
}

sub UpdateVars
{
    my ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ ) = @_;

    print $main_log_file_handle_ "\n +UpdateVars ( $t_current_strat_ , $t_current_iter_ )\n";

    my $t_baseresult_line_ = ""; my $t_baseresult_strat_ = "";
    my $t_aimresult_line_ = ""; my $t_aimresult_strat_ = "";
    my $t_fbrresult_line_ = ""; my $t_fbrresult_strat_ = "";
    my $t_fbmfsresult_line_ = ""; my $t_fbmfsresult_strat_ = "";

    my $t_best_result_line_ = ""; my $t_best_strat_ = "";

    if ( exists ( $strat_to_iteration_to_baseresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    {
	$t_baseresult_line_ = $strat_to_iteration_to_baseresults_ { $t_current_strat_ } { $t_current_iter_ };
	$t_baseresult_strat_ = $strat_to_iteration_to_basestrat_ { $t_current_strat_ } { $t_current_iter_ };

	print $main_log_file_handle_ "BaseResultLine ".$t_baseresult_line_."\n";
    }
    if ( exists ( $strat_to_iteration_to_aimresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    {
	$t_aimresult_line_ = $strat_to_iteration_to_aimresults_ { $t_current_strat_ } { $t_current_iter_ };
	$t_aimresult_strat_ = $strat_to_iteration_to_aimstrat_ { $t_current_strat_ } { $t_current_iter_ };

	print $main_log_file_handle_ "AimResultLine ".$t_aimresult_line_."\n";
    }
    if ( exists ( $strat_to_iteration_to_fbrresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    {
	$t_fbrresult_line_ = $strat_to_iteration_to_fbrresults_ { $t_current_strat_ } { $t_current_iter_ };
	$t_fbrresult_strat_ = $strat_to_iteration_to_fbrstrat_ { $t_current_strat_ } { $t_current_iter_ };

	print $main_log_file_handle_ "FbrResultLine ".$t_fbrresult_line_."\n";
    }
    if ( exists ( $strat_to_iteration_to_fbmfsresults_ { $t_current_strat_ } { $t_current_iter_ } ) )
    {
	$t_fbmfsresult_line_ = $strat_to_iteration_to_fbmfsresults_ { $t_current_strat_ } { $t_current_iter_ };
	$t_fbmfsresult_strat_ = $strat_to_iteration_to_fbmfsstrat_ { $t_current_strat_ } { $t_current_iter_ };

	print $main_log_file_handle_ "FbmfsResultLine ".$t_fbmfsresult_line_."\n";
    }
    if ( exists ( $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ } { $t_current_strat_no_ } ) &&
	 exists ( $iteration_to_strat_no_to_results_ { $t_current_iter_ } { $t_current_strat_no_ } ) )
    {
	$t_best_result_line_ = $iteration_to_strat_no_to_results_ { $t_current_iter_ } { $t_current_strat_no_ };
	$t_best_strat_ = $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ } { $t_current_strat_no_ };

	print $main_log_file_handle_ "So far BestResultLine ".$t_best_result_line_."\n";
    }
    elsif ( $t_baseresult_line_ )
    {
	$t_best_result_line_ = $t_baseresult_line_;
	$t_best_strat_ = $t_baseresult_strat_;

	print $main_log_file_handle_ "So far BestResultLine ".$t_best_result_line_."\n";
    }

    if ( ! $t_best_result_line_ )
    {
	return;
    }

    if ( CompareResultLines ( $t_best_result_line_ , $t_baseresult_line_ ) < 0 )
    { # $t_best_result_line_ < $t_baseresult_line_ , update
	$t_best_result_line_ = $t_baseresult_line_;
	$t_best_strat_ = $t_baseresult_strat_;
    }
    if ( CompareResultLines ( $t_best_result_line_ , $t_aimresult_line_ ) < 0 )
    { # $t_best_result_line_ < $t_aimresult_line_ , update
	$t_best_result_line_ = $t_aimresult_line_;
	$t_best_strat_ = $t_aimresult_strat_;
    }
    if ( CompareResultLines ( $t_best_result_line_ , $t_fbrresult_line_ ) < 0 )
    { # $t_best_result_line_ < $t_fbrresult_line_ , update
	$t_best_result_line_ = $t_fbrresult_line_;
	$t_best_strat_ = $t_fbrresult_strat_;
    }
    if ( CompareResultLines ( $t_best_result_line_ , $t_fbmfsresult_line_ ) < 0 )
    { # $t_best_result_line_ < $t_fbmfsresult_line_ , update
	$t_best_result_line_ = $t_fbmfsresult_line_;
	$t_best_strat_ = $t_fbmfsresult_strat_;
    }

    $iteration_to_strat_no_to_results_ { $t_current_iter_ } { $t_current_strat_no_ } = $t_best_result_line_;
    $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ } { $t_current_strat_no_ } = $t_best_strat_;

    # populate best & base results
    # for the next iteration
    $iteration_to_strat_no_to_results_ { $t_current_iter_ + 1 } { $t_current_strat_no_ } = $t_best_result_line_;
    $iteration_to_strat_no_to_best_strat_ { $t_current_iter_ + 1 } { $t_current_strat_no_ } = $t_best_strat_;

    $strat_to_iteration_to_baseresults_ { $t_current_strat_ } { $t_current_iter_ + 1 } = $t_best_result_line_;
    $strat_to_iteration_to_basestrat_ { $t_current_strat_ } { $t_current_iter_ + 1 } = $t_best_strat_;

    print $main_log_file_handle_ "New BestResultLine ".$t_best_result_line_."\n";

    return;
}

sub NotImprovedThisIteration
{ # TODO implement
    my ( $t_current_strat_ , $t_current_iter_ , $t_current_strat_no_ ) = @_;

    my $retval_ = 0; # default is improved.

    if ( CompareResultLines ( $iteration_to_strat_no_to_results_ { $t_current_iter_ } { $t_current_strat_no_ } , 
			      $iteration_to_strat_no_to_results_ { $t_current_iter_ - 1 } { $t_current_strat_no_ } ) 
	 <= 0 )
    { # this iteration yielded a score which was less than or equal to the best score in the previous iteration
	$retval_ = 1; # no improvement this iteration.
    }

    return $retval_;
}

sub CompareResultLines
{
    my ( $result_line_a_ , $result_line_b_ ) = @_;

    my @result_line_a_words_ = split ( ' ' , $result_line_a_ ); chomp ( @result_line_a_words_ );
    my @result_line_b_words_ = split ( ' ' , $result_line_b_ ); chomp ( @result_line_b_words_ );

    my $retval_ = 0; # could not compare / equal

    if ( $#result_line_a_words_ >= 6 && $#result_line_b_words_ >= 6 )
    {
	my $result_line_a_outsample_score_ = $result_line_a_words_ [ 4 ];
	my $result_line_b_outsample_score_ = $result_line_b_words_ [ 4 ];

	if ( $result_line_a_outsample_score_ < $result_line_b_outsample_score_ )
	{
	    $retval_ = -1;
	}
	elsif ( $result_line_a_outsample_score_ > $result_line_b_outsample_score_ )
	{
	    $retval_ = 1;
	}
    }

    return $retval_;
}

sub InstallAndReportResults
{
    for ( my $current_strat_no_ = 0 ; $current_strat_no_ <= $#strats_to_do_ ; $current_strat_no_ ++ )
    {
	my $current_strat_ = $strats_to_do_ [ $current_strat_no_ ];

	$mail_body_ = $mail_body_."\n";
	$mail_body_ = $mail_body_."++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
	$mail_body_ = $mail_body_.$current_strat_."\n";

	$mail_body_ = $mail_body_."BASERESULTS ".$strat_to_iteration_to_baseresults_ { $current_strat_ } { 1 }."\n\n";

	for ( my $current_iter_ = 1 ; $current_iter_ <= $max_number_bootstrap_iterations_ ; $current_iter_ ++ )
	{
	    if ( exists ( $strat_to_iteration_to_aimresults_ { $current_strat_ } { $current_iter_ } ) )
	    {
		$mail_body_ = $mail_body_."ITERATION ".$current_iter_." after AIM ".$strat_to_iteration_to_aimresults_ { $current_strat_ } { $current_iter_ }."\n";
	    }
	    if ( exists ( $strat_to_iteration_to_fbrresults_ { $current_strat_ } { $current_iter_ } ) )
	    {
		$mail_body_ = $mail_body_."ITERATION ".$current_iter_." after FBR ".$strat_to_iteration_to_fbrresults_ { $current_strat_ } { $current_iter_ }."\n";
	    }
	    if ( exists ( $strat_to_iteration_to_fbmfsresults_ { $current_strat_ } { $current_iter_ } ) )
	    {
		$mail_body_ = $mail_body_."ITERATION ".$current_iter_." after FBMFS ".$strat_to_iteration_to_fbmfsresults_ { $current_strat_ } { $current_iter_ }."\n";
	    }

	    if ( exists ( $iteration_to_strat_no_to_results_ { $current_iter_ } { $current_strat_no_ } ) )
	    {
		$mail_body_ = $mail_body_."ITERATION ".$current_iter_." best_strat ".$iteration_to_strat_no_to_results_ { $current_iter_ } { $current_strat_no_ }." ".$iteration_to_strat_no_to_best_strat_ { $current_iter_ } { $current_strat_no_ }."\n";
	    }
	}

	my $best_iteration_ = $max_number_bootstrap_iterations_;
	for ( ; ! exists ( $iteration_to_strat_no_to_best_strat_ { $best_iteration_ } { $current_strat_no_ } ) ; $best_iteration_ -- )
	{
	}

	my $t_baseresults_ = $strat_to_iteration_to_baseresults_ { $current_strat_ } { 1 };
	my $t_bestresults_ = $iteration_to_strat_no_to_results_ { $best_iteration_ } { $current_strat_no_ };

	my $best_minus_original_outsample_score_diff_ = GetOutsampleScoreFromResultLine ( $t_bestresults_ ) - GetOutsampleScoreFromResultLine ( $t_baseresults_ );

	my $improvement_thresh_ = max ( $abs_outsample_improvement_thresh_ , ( $outsample_improvement_thresh_ * abs ( GetOutsampleScoreFromResultLine ( $t_baseresults_ ) ) ) );
	my $volume_thresh_ = max ( 10 , $outsample_vol_cuttoff_factor_ * GetOutsampleVolumeFromResultLine ( $t_baseresults_ ) );
	my $ttc_thresh_ = $outsample_ttc_cuttoff_factor_ * GetOutsampleTtcFromResultLine ( $t_baseresults_ );

	my $to_install_this_ = 1;
        if ( $best_minus_original_outsample_score_diff_ <= $improvement_thresh_)
        {
            $to_install_this_ = 0;
            $mail_body_ = $mail_body_." can't satisfy score_thresh_\n";
        }
        if ( GetOutsampleVolumeFromResultLine ( $t_bestresults_ ) <= $volume_thresh_ )
        {
            $to_install_this_ = 0;
            $mail_body_ = $mail_body_." can't satisfy vol_thresh_\n";
        }
        if ( GetOutsampleTtcFromResultLine ( $t_bestresults_ ) >= $ttc_thresh_ )
        {
            $to_install_this_ = 0;
            $mail_body_ = $mail_body_." can't satisfy ttc_thresh_\n";
        }
        
        if ( $to_install_this_ == 1 )
        {
	    my $t_beststrat_ = $iteration_to_strat_no_to_best_strat_ { $best_iteration_ } { $current_strat_no_ };
	    my $original_strat_ = $local_strat_filename_to_input_strat_filename_ { $current_strat_ };

	    my $original_strat_dir_ = dirname ( $original_strat_ );
	    my $original_model_dir_ = dirname ( GetModelFromStrat ( $original_strat_ ) );

	    my $optimized_strat_filename_ = $original_strat_dir_."/w_barp_".$unique_gsm_id_."_".basename ( $current_strat_ );
	    my $optimized_model_filename_ = $original_model_dir_."/w_barp_".$unique_gsm_id_."_".basename ( GetModelFromStrat ( $current_strat_ ) );

	    my $t_optimized_model_filename_ = GetModelFromStrat ( $t_beststrat_ );

	    `cat $original_strat_ | awk '{ \$4=\"$optimized_model_filename_\"; print \$0; }' > $optimized_strat_filename_`;
	    `cp $t_optimized_model_filename_ $optimized_model_filename_`;

	    $mail_body_ = $mail_body_."\n\n";
	    $mail_body_ = $mail_body_." Installed strategy : $optimized_strat_filename_\n";
	    $mail_body_ = $mail_body_." Installed model : $optimized_model_filename_\n";

	    $mail_body_ = $mail_body_."\n\n";

	    $mail_body_ = $mail_body_." Original outsample performance [ ".GetOutsampleScoreFromResultLine ( $t_baseresults_ )." ".GetOutsampleVolumeFromResultLine ( $t_baseresults_ )." ".GetOutsampleTtcFromResultLine ( $t_baseresults_ )." ]\n";
	    $mail_body_ = $mail_body_." Optimized outsample performance [ ".GetOutsampleScoreFromResultLine ( $t_bestresults_ )." ".GetOutsampleVolumeFromResultLine ( $t_bestresults_ )." ".GetOutsampleTtcFromResultLine ( $t_bestresults_ )." ]\n";

	    $mail_body_ = $mail_body_."\n\n";
	}

	$mail_body_ = $mail_body_."++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    }

    SendMail ( $mail_body_ );
}

sub SendMail
{
    my ($t_mail_body_) = @_;
    if ( ( $mail_address_ ) &&
	 ( $t_mail_body_ ) )
    {
	open(MAIL, "|/usr/sbin/sendmail -t");
	
	my $hostname_=`hostname`;
	## Mail Header
	print MAIL "To: $mail_address_\n";
	print MAIL "From: $mail_address_\n";
	print MAIL "Subject: boot_arp ( $config_file_ ) $start_time_ $hostname_\n\n";
	## Mail Body
	print MAIL $t_mail_body_ ;
	
	close(MAIL);
	
	print $main_log_file_handle_ "Mail Sent to $mail_address_\n\n$t_mail_body_\n";
    }
    
}

sub HasEARTHModel
{
    my ( $strategy_filename_ ) = @_;

    my $has_earth_model_ = 0;

    my $model_filename_ = GetModelFromStrat ( $strategy_filename_ );

    my $num_non_linear_ = `grep -c \"NONLINEARCOMPONENT \" $model_filename_`; chomp ( $num_non_linear_ );
    if ( $num_non_linear_ > 0 )
    {
	$has_earth_model_ = 1;
    }

    return $has_earth_model_;
}

sub GetModelFromStrat
{
    my ( $t_strat_ ) = @_;

    my @output_ = `cat $t_strat_`;
    chomp ( @output_ );
    my @strat_output_ = split ( ' ' , $output_ [ 0 ] ); chomp ( @strat_output_ );
    my $t_model_ = $strat_output_ [ 3 ];

    return $t_model_;
}
