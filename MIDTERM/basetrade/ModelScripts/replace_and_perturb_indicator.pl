#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
#use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
#use Scalar::Util qw(looks_like_number);

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts/";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts/";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib/";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin/";
if ( ! -d $LIVE_BIN_DIR ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin/";
}
my $TRADELOG_DIR="/spare/local/logs/tradelogs/";

#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; #GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/array_ops.pl"; # GetSum GetIndexOfMaxValue
require "$GENPERLLIB_DIR/get_avg_event_count_per_sec_for_shortcode.pl"; # GetAvgEventCountPerSecForShortcode
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllPIDSTerminated

sub ReadConfigFile;
sub RunFindBestReplacement;
sub RunFindBestModel;
sub GetOutSampleDates;
sub SendMail;
sub CalcScores;

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
my $sampling_shortcodes_str_ = "" ;
my $pred_duration_ = 32;
my $predalgo_ = "na_e5" ;
my $filter_name_ = "fsr.5_3" ;


my $last_trading_date_ = GetIsoDateFromStrMin1("TODAY-1"); #date is TODAY-1 by default
my $num_prev_days_ = 45;
my $outsample_start_date_ = GetIsoDateFromStrMin1("TODAY-15"); #date is TODAY-1 by default
my $outsample_end_date_ = GetIsoDateFromStrMin1("TODAY-1");
my $change_fraction_ = 2;
my $max_number_iterations_ = 15;
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
my $fbr_model_list_file_ = "";
my $install_strats_ = 1;
my $outsample_ttc_cuttoff_factor_ = 1.2;
my $outsample_vol_cuttoff_factor_ = 0.8;
my $timeout_string_ = "" ;
my $use_fake_faster_data_ = rand(1) > 0.5 ? 1 : 0;  #using delays randomly, to set usage use param USE_FAKE_FASTER_DATA 

my $num_installed_ = 0;

my @input_strat_list_ = ();
my @input_model_list_ = ();
my @potential_model_list_ = ();
my @potential_score_vec_ = ();
my @strats_done_ = ();
my @strats_to_do_ = ();
my @sampling_shortcodes_ = ();
my @pc_cutoffs_ = ();

my $fbmfs_dir_ = $SPARE_HOME."PerturbReplacedIndicator";
my $mail_address_ = "nseall@tworoads.co.in";
my $mail_body_ = "";
my $config_string_ = "";
my $USAGE = "$0 config_file [workind_dir]";
my $start_time_ = `date`;
$mail_body_ = $mail_body_."start time : $start_time_\n-------------------------------------------------------------------------\n";

if($#ARGV < 0)
{
    print $USAGE."\n";
    exit(0);
}

my $config_file_ = $ARGV[0];
if(! -e $config_file_)
{
    print "config_file_ $config_file_ does not exist\n";
    exit(0);
}

my $config_base_ = basename ( $config_file_ ); chomp ( $config_base_ );

if ( $#ARGV >= 1 ) 
{ 
    $fbmfs_dir_ = $ARGV[1]; 
}

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $main_work_dir_ = $fbmfs_dir_."/".$config_base_."/".$unique_gsm_id_."/"; 

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $main_work_dir_ )
{
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    $main_work_dir_ = $fbmfs_dir_."/".$config_base_."/".$unique_gsm_id_."/";
}
else
{
    last;
}
}

`mkdir -p $main_work_dir_ `;
print "main_work_dir_ : $main_work_dir_\n";

my $fbr_work_dir_ = "" ;
my $work_dir_ = "" ;

my $main_log_file_ = $main_work_dir_."main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "$0 Could not open log file : $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);
$mail_body_ = $mail_body_."logfile : $main_log_file_\n";

ReadConfigFile();
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

for( my $strat_no_ = 0; $strat_no_ <=$#input_strat_list_; $strat_no_++ )
{
    $work_dir_ = $main_work_dir_."strat_file_".$strat_no_."/" ;
    $fbr_work_dir_ = $work_dir_."FBR/" ;
    `mkdir -p $fbr_work_dir_ `;

    $original_strat_file_ = $input_strat_list_[$strat_no_];
    $original_model_file_ = `less $original_strat_file_ | awk '{print \$4}' `;
    chomp($original_model_file_) ;
    print $main_log_file_handle_ "original_strat_file_$strat_no_ : $original_strat_file_ , original_model_file_$strat_no_ : $original_model_file_\n";
    
    RunFindBestReplacement();
    RunFindBestModel();
    push (@strats_done_, $original_strat_file_);

    my $config_reset_string_ = "";
    for (my $done_id_= 0; $done_id_<=$#strats_done_; $done_id_++)
    {
	$config_reset_string_ = "$config_reset_string_ $strats_done_[$done_id_]";
    }

    my $config_reset_cmd_ = $SCRIPTS_DIR."set_config_field.pl $config_file_ STRATS_DONE $config_reset_string_";
    print $main_log_file_handle_ "$config_reset_cmd_\n";
    `$config_reset_cmd_`;
}


$mail_body_ = "Total installed strats : $num_installed_($install_strats_)\n".$mail_body_;
my $end_time_ = `date`;
$mail_body_ = $mail_body_."\nend time : $end_time_\n-------------------------------------------------------------------------\n";


SendMail($config_string_.$mail_body_);

$main_log_file_handle_->close;
exit (0);

sub ReadConfigFile
{
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
                $timeout_string_ = $thisline_;
#		$datagen_msecs_timeout_ = $thisline_words_[0];
#		$datagen_l1events_timeout_ = $thisline_words_[1];
#		$datagen_num_trades_timeout_ = $thisline_words_[2];
#               print $main_log_file_handle_ "datagen_msecs_timeout_ : $datagen_msecs_timeout_ , datagen_l1events_timeout_ : $datagen_l1events_timeout_, datagen_num_trades_timeout_ : $datagen_num_trades_timeout_\n";
                print $main_log_file_handle_ "timeout_string_ : $thisline_\n"; 
            }
            when("TO_PRINT_ON_ECO")
            {
                $to_print_on_economic_times_ = $thisline_words_[0];
                print $main_log_file_handle_ "to_print_on_economic_times_ : $to_print_on_economic_times_\n";
            }
            when ("USE_FAKE_FASTER_DATA")             
            {
              $use_fake_faster_data_ = $thisline_words_[0]>0 ? 1 : 0;
              print $main_log_file_handle_ "use_fake_faster_data_ : $use_fake_faster_data_\n";                                                        
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
				push ( @strats_to_do_ , $strat_full_path_);
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
		print $main_log_file_handle_ $current_instruction_, " is ",join(' ', @thisline_words_ ),"\n";
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
		if ( $mail_address_ )
		{ # if not nullified because of NO_EMAIL , only then send email.
		    $mail_address_ = $thisline_words_[0];
		}
            }
	    when ( "NO_EMAIL" )
	    {
		$mail_address_ = "";
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

    my $timeout_set_already_ = 0; # signal need to set ahead
    my @timeout_string_words_ = split ( ' ', $timeout_string_ );
    if ( $timeout_string_words_[0] eq "TIM1" )
    {
        ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 100, 0, 0 );
        $timeout_set_already_ = 1;
    }
    elsif ( index ( $timeout_string_words_[0] , "EVT" ) != -1 )
    {
        ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 4000, 10, 0 );
        if ( $shortcode_ )
        {
            $datagen_l1events_timeout_ = max ( 1 ,int ( ( GetAvgEventCountPerSecForShortcode ( $shortcode_, $datagen_start_hhmm_, $datagen_end_hhmm_, $last_trading_date_ ) / 2.0 ) + 0.5 ) ) ;
            if ( $datagen_l1events_timeout_ < 2 )
            { # very slow products
                $datagen_msecs_timeout_ = max ( $datagen_msecs_timeout_, int ( 1000 / $datagen_l1events_timeout_) ) ;
                $datagen_msecs_timeout_ = min ( 60000, $datagen_msecs_timeout_ );
            }
        }
        if ( $timeout_string_words_[0] eq "EVT2" )
        {
            $datagen_l1events_timeout_ = "c1" ;  # msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
            if ( scalar ( @sampling_shortcodes_ ) > 0 )
            {
                print $main_log_file_handle_ "sampling codes specificed_\n";
                $sampling_shortcodes_str_ = "ADD_SAMPLING_CODES";
                for ( my $i = 0 ; $i < scalar ( @sampling_shortcodes_ ) ; $i ++ )
                {
                    $sampling_shortcodes_str_ .= " ".$sampling_shortcodes_[$i];
                }
                $sampling_shortcodes_str_ .= " -1";
            }
        }
        if ( $timeout_string_words_[0] eq "EVT3" )
        {
            $datagen_l1events_timeout_ = "c2" ;  # msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
            if ( scalar ( @sampling_shortcodes_ ) > 0 )
            {
                print $main_log_file_handle_ "sampling codes specificed_\n";
                $sampling_shortcodes_str_ = "ADD_SAMPLING_CODES";
                for ( my $i = 0 ; $i < scalar ( @sampling_shortcodes_ ) ; $i ++ )
                {
                    $sampling_shortcodes_str_ .= " ".$sampling_shortcodes_[$i];
                }
                $sampling_shortcodes_str_ .= " -1";
            }
        }
        if ( $timeout_string_words_[0] eq "EVT4" )
        {
            $datagen_l1events_timeout_ = "c3" ;  # msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
            if ( scalar ( @sampling_shortcodes_ ) > 0 )
            {
                print $main_log_file_handle_ "sampling codes specificed_\n";
                $sampling_shortcodes_str_ = "ADD_SAMPLING_CODES";
                for ( my $i = 0 ; $i < scalar ( @sampling_shortcodes_ ) ; $i ++ )
                {
                    $sampling_shortcodes_str_ .= " ".$sampling_shortcodes_[$i]." ".$pc_cutoffs_[$i];
                }
                $sampling_shortcodes_str_ .= " -1";
            }
        }
        $timeout_set_already_ = 1;
    }
    elsif ( $timeout_string_words_[0] eq "TRD1" )
    {
        ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 8000, 20, 1 );
        if ( $shortcode_ )
        {
            $datagen_l1events_timeout_ = max ( 1 ,int ( ( GetAvgEventCountPerSecForShortcode ( $shortcode_, $datagen_start_hhmm_, $datagen_end_hhmm_, $last_trading_date_ ) * 2.0 ) + 0.5) ) ;
            $datagen_num_trades_timeout_ = max ( 1, int ( GetAvgTradeCountPerSecForShortcode ( $shortcode_ ) ) ) ;

            if ( $datagen_l1events_timeout_ < 2 )
            { # very slow products
                $datagen_msecs_timeout_ = max ( $datagen_msecs_timeout_, int(1000/$datagen_l1events_timeout_) );
                $datagen_msecs_timeout_ = min ( 60000, $datagen_msecs_timeout_ );
            }
        }
        $timeout_set_already_ = 1;
    }
    if ( $#timeout_string_words_ >= 2 )
    {
        if ( $timeout_set_already_ == 0 )
        {
            $datagen_msecs_timeout_ = $timeout_string_words_[0];
            $datagen_l1events_timeout_ = $timeout_string_words_[1];
            $datagen_num_trades_timeout_ = $timeout_string_words_[2];
        }
    }

    my $fbm_version_string_ = $find_best_model_version_?"v1:find_best_model_for_strategy_var_pert_single_indicator.pl":"v0:find_best_model_for_strategy_random_dates_single_indicator.pl";
    $config_string_ = $config_string_."FIND_BEST_MODEL_SINGLE_INDICATOR_VERSION : $fbm_version_string_\n";
    $config_string_ = $config_string_."COST_FN : $cost_fn_\n";
    
    close CONFIGFILEHANDLE;
}

sub RunFindBestReplacement
{
    
    $fbr_model_list_file_ = $work_dir_."fbr_model_list_file";
    print $main_log_file_handle_ "Model list file with new indicators added $fbr_model_list_file_\n";
    my $exec_cmd_ = "" ;
    $exec_cmd_ = $MODELSCRIPTS_DIR."find_best_replacement_in_model.R $shortcode_ $original_strat_file_ $original_model_file_ $ilist_ $last_trading_date_ $num_prev_days_ $datagen_start_hhmm_ $datagen_end_hhmm_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ $pred_duration_ $predalgo_ $filter_name_ $fbr_work_dir_ $fbr_model_list_file_ $use_fake_faster_data_ $sampling_shortcodes_str_ ";

    print $main_log_file_handle_ "$exec_cmd_\n";
    my $output_lines_ = `$exec_cmd_`;
    print $main_log_file_handle_ "$output_lines_\n";
    @input_model_list_ = () ;
    open MODELFILEHANDLE, "< $fbr_model_list_file_ " or PrintStacktraceAndDie ( "$0 Could not open RIM_MODEL_LIST_FILE : $fbr_model_list_file_ for reading\n" );
    while(my $model_ = <MODELFILEHANDLE>)
    {
	chomp($model_);
	push(@input_model_list_, $model_);
#       $exec_cmd_ = "cat $original_strat_file_ | awk -vnmf=$model_ '{\$4 = nmf; print}' > $this_output_strat_file_";
#       print $main_log_file_handle_ $exec_cmd_."\n";
#       `$exec_cmd_`;
#       chomp($this_output_strat_file_);
    }
    close MODELFILEHANDLE;
}


sub RunFindBestModel
{
    @potential_model_list_ = () ;
    @potential_score_vec_ = () ;
    my $start_training_date_ = CalcPrevWorkingDateMult($last_trading_date_, $num_prev_days_);
    my $run_string_ = $num_prev_days_."_".$last_trading_date_."_F".$use_fake_faster_data_."_".substr($unique_gsm_id_, length($unique_gsm_id_) - 5, 5); #used for naming only, shortening uid for length issues 
    my $this_start_time_ = `date`;
    my $this_mail_body_ = "start time for this strat : $this_start_time_\n-----------------------------------\nlogfile : $main_log_file_\n";

    for (my $i=0; $i<=$#input_model_list_; $i++)
    {
        my $this_input_strat_file_ = $work_dir_."temp_strat_for_fbr_model_".$i; 
        my $exec_cmd_ = "" ;
        $exec_cmd_ = "cat $original_strat_file_ | awk -vnmf=$input_model_list_[$i] '{\$4 = nmf; print}' > $this_input_strat_file_";
        print $main_log_file_handle_ $exec_cmd_."\n";
        `$exec_cmd_`;
        chomp($this_input_strat_file_);

#        my $this_replaced_indicator_ = `grep -v -f $input_model_list_[$i] $original_model_file_` ; chomp($this_replaced_indicator_);
#        my $this_new_indicator_ = `grep -v -f $original_model_file_ $input_model_list_[$i]` ; chomp($this_new_indicator_);
#        my $this_replaced_indicator_index_ = `awk '/$this_new_indicator_/ {print FNR}' $input_model_list_[$i]` - 3;

        print $main_log_file_handle_ "optimizing $i : $this_input_strat_file_\n";
        my $this_output_model_file_ = $work_dir_."w_model_opt_".$i;
        #my  $this_output_model_file_ = "/home/".$USER."/modelling/fbmfswork/LFR_0/tmp_model_".$i;
        my $this_output_strat_file_ = $work_dir_."w_opt_".$i;
        my $debug_string_ = "D";
        if($debug_ == 0)
        {
            $debug_string_ = "N"
        }
        if($min_vol_check_ == 0)
        {
            $min_vol_ = -1;
        }
        if($max_ttc_check_ == 0)
        {
            $max_ttc_ = 1000000;
        }
        $exec_cmd_ = "";
        if($find_best_model_version_ == 0)
        {
            $exec_cmd_ = $MODELSCRIPTS_DIR."find_best_model_for_strategy_random_dates_single_indicator.pl $this_input_strat_file_ $this_output_model_file_ $last_trading_date_ $num_prev_days_ $change_fraction_ $batch_size_ $debug_string_ $cost_fn_ $use_fake_faster_data_";
        }
        else
        {
            $exec_cmd_ = $MODELSCRIPTS_DIR."find_best_model_for_strategy_var_pert_single_indicator.pl $original_strat_file_ $this_input_strat_file_ $this_output_model_file_ $last_trading_date_ $num_prev_days_ $change_fraction_ $max_number_iterations_ $search_algo_ $debug_string_ $cost_fn_ $min_vol_ $max_ttc_ $num_perturbations_ $retain_more_pert_ $use_fake_faster_data_";
        }
        print $main_log_file_handle_ "$exec_cmd_\n";
        my $output_lines_ = `$exec_cmd_`;
        print $main_log_file_handle_ "$output_lines_\n";
        my $training_result_ = `echo "$output_lines_" | grep training `;
        my @outsample_date_vec_ = ();
        
        GetOutSampleDates(\@outsample_date_vec_, $outsample_start_date_, $outsample_end_date_, $this_output_model_file_);
        print $main_log_file_handle_ join(' ', @outsample_date_vec_)."\n";
        
        $exec_cmd_ = "cat $this_input_strat_file_ | awk -vnmf=$this_output_model_file_ '{\$4 = nmf; print}' > $this_output_strat_file_";
        print $main_log_file_handle_ $exec_cmd_."\n";
        `$exec_cmd_`;
        #`cat $this_input_strat_file_ | awk '{\$4 = $this_output_model_file_; print}' > $this_output_strat_file_`;
        my @score_vec_ = ();
        my @vol_vec_ = ();
        my @ttc_vec_ = ();
        my @strat_list_ = ();
        push(@strat_list_, $this_output_strat_file_);
        push(@strat_list_, $original_strat_file_);
        my $stats_string_ = CalcScores( \@strat_list_, \@score_vec_, \@vol_vec_, \@ttc_vec_, \@outsample_date_vec_, $i );
        
        print $main_log_file_handle_ "Original on OutSample : $score_vec_[1] $vol_vec_[1] $ttc_vec_[1]\n";
        print $main_log_file_handle_ "Optimized with New Indicator on OutSample : $score_vec_[0] $vol_vec_[0] $ttc_vec_[0]\n";
        my $score_diff_ = $score_vec_[0] - $score_vec_[1];
        my $this_improvement_thresh_ = max($abs_outsample_improvement_thresh_, ($outsample_improvement_thresh_)*abs($score_vec_[1])); 
        my $vol_thresh_ = max(10,$outsample_vol_cuttoff_factor_*$vol_vec_[1]);
        my $ttc_thresh_ = $outsample_ttc_cuttoff_factor_*$ttc_vec_[1];
#        my $input_model_full_path_ = `awk '{print \$4}' $this_input_strat_file_`; chomp($input_model_full_path_);
        $exec_cmd_ = $MODELSCRIPTS_DIR."compare_opt_model.py $input_model_list_[$i] $this_output_model_file_";
        my $model_comp_string_ = `$exec_cmd_`;
	
        $exec_cmd_ = "tail -n2 $input_model_list_[$i] | head -n1 " ;
        print $main_log_file_handle_ $exec_cmd_."\n";
        my $this_new_indicator_ = `$exec_cmd_`;
        
        $this_mail_body_ = $this_mail_body_."================== strat : $i ====================\n";
        $this_mail_body_ = $this_mail_body_."$original_strat_file_\n";
        $this_mail_body_ = $this_mail_body_."Training Performance : SCORE VOL TTC\n";
        $this_mail_body_ = $this_mail_body_.$training_result_;
        $this_mail_body_ = $this_mail_body_."OutSample Performance : SCORE VOL TTC\n";
        $this_mail_body_ = $this_mail_body_."Original on OutSample : $score_vec_[1] $vol_vec_[1] $ttc_vec_[1]\n";
        $this_mail_body_ = $this_mail_body_."Optimized with New Indicator on OutSample : $score_vec_[0] $vol_vec_[0] $ttc_vec_[0]\n";
        $this_mail_body_ = $this_mail_body_."score_diff_ : $score_diff_ abs_score_thresh_ : $abs_outsample_improvement_thresh_ score_thresh_used_ : $this_improvement_thresh_ vol_thresh_ : $vol_thresh_ ttc_thresh_ : $ttc_thresh_\n\n";
        $this_mail_body_ = $this_mail_body_.$stats_string_;
        $this_mail_body_ = $this_mail_body_."STATISTICS PNL_AVG PNL_STD VOL_AVG PNL_SHARPE (PNL_AVG-0.33*PNL_STD) AVG_MIN_ADJ_PNL MED_AVG_TTC AVG_MAX_TTC PPC S B A AVG_MAXDD AVG_APOS PNL_CONSER_AVG PNL_MED_AVG AVG_MSGS\n\n";
        $this_mail_body_ = $this_mail_body_."New Added Indicator\n".$this_new_indicator_."\n";
#        $this_mail_body_ = $this_mail_body_."Indicator Replaced\n".$this_replaced_indicator_."\n";
        $this_mail_body_ = $this_mail_body_."Comparing Models\n".$model_comp_string_."\n";

        my $to_install_this_ = 1;
        if($score_diff_ <= $this_improvement_thresh_)   
        {
            $to_install_this_ = 0;
            $this_mail_body_ = $this_mail_body_."can't satisfy score_thresh_\n";
        }
        if($vol_vec_[0] <= $vol_thresh_)   
        {
            $to_install_this_ = 0;
            $this_mail_body_ = $this_mail_body_."can't satisfy vol_thresh_\n";
        }
        if($ttc_vec_[0] >= $ttc_thresh_)   
        {
            $to_install_this_ = 0;
            $this_mail_body_ = $this_mail_body_."can't satisfy ttc_thresh_\n";
        }
        
        if($to_install_this_ == 1)
        {
	    push(@potential_model_list_, $this_output_model_file_);
	    push(@potential_score_vec_, $score_vec_[0]);
	    $this_mail_body_ = $this_mail_body_."Satisfied different thresholds for installation\n";
	}
	
    }

    if($#potential_score_vec_ >= 0) 
    {
	my $t_best_index_ = GetIndexOfMaxValue ( \@potential_score_vec_ );
	my $this_output_model_file_ = $potential_model_list_[$t_best_index_];
	print $main_log_file_handle_ "installing optimization of $original_strat_file_\n";
	my $input_strat_dir_path_ = `dirname $original_strat_file_`; chomp($input_strat_dir_path_);
	my $input_strat_name_ = `basename $original_strat_file_`; chomp($input_strat_name_);
	my $input_model_dir_path_ = `dirname $original_model_file_`; chomp($input_model_dir_path_);
        my $input_model_name_ = `basename $original_model_file_`; chomp($input_model_name_);

	my $opt_model_full_path_ = $input_model_dir_path_."/w_f_m_".$run_string_."_".$input_model_name_;
	my $opt_strat_full_path_ = $input_strat_dir_path_."/w_f_".$run_string_."_".$input_strat_name_;

	if ( ! $mail_address_ )
	{ # this is probably a child run from the bootstrap script.
	    # keep filenames as short as possible.
	    $opt_model_full_path_ = $input_model_dir_path_."/Bf_".$input_model_name_;
	    $opt_strat_full_path_ = $input_strat_dir_path_."/Bf_".$input_strat_name_;
	}

	my $exec_cmd_ = "";
	
	if($install_strats_ == 1)
	{
	    `cp $this_output_model_file_  $opt_model_full_path_`;
	    $exec_cmd_ = "cat $original_strat_file_ | awk -vnmf=$opt_model_full_path_ '{\$4 = nmf; print}' > $opt_strat_full_path_";
	    print $main_log_file_handle_ $exec_cmd_."\n";
	    `$exec_cmd_`;
	}

	my $replaced_indicator_ = `grep -v -f $opt_model_full_path_ $original_model_file_` ; chomp($replaced_indicator_);
	my $new_indicator_ = `grep -v -f $original_model_file_ $opt_model_full_path_` ; chomp($new_indicator_);

	my $old_model_ = `cat $original_model_file_`;
	my $new_opt_model_ = `cat $opt_model_full_path_`;
	$this_mail_body_ = $this_mail_body_."================== Installation ====================\n";
#       $this_mail_body_ = $this_mail_body_."New Added Indicator in the installed model\n".$new_indicator_."\n";
#       $this_mail_body_ = $this_mail_body_."Indicator Replaced from the old model \n".$replaced_indicator_."\n";
	$this_mail_body_ = $this_mail_body_."Installed model with new Indicator : $opt_model_full_path_\n".$new_opt_model_."\n";
	$this_mail_body_ = $this_mail_body_."Original Model : $original_model_file_\n".$old_model_."\n";
	$this_mail_body_ = $this_mail_body_."Installed strategy : $opt_strat_full_path_\n\n";
	print $main_log_file_handle_ "Installed model with new Indicator : $opt_model_full_path_\n";
	print $main_log_file_handle_ "Installed strategy : $opt_strat_full_path_\n";
	$num_installed_ ++;

	if ( ! $mail_address_ )
	{ # print this information to stdout
	    print "OUTPUT INSTALLATION $opt_strat_full_path_\n";
	}
    }
    else
    {
	$this_mail_body_ = $this_mail_body_."not installing as thresholds not satisfied\n";
	if ( ! $mail_address_ )
	{ # print this information to stdout
	    print "OUTPUT NO_INSTALLATION $original_strat_file_\n";
	}
    }
    my $this_end_time_ = `date`;
    $this_mail_body_  = $this_mail_body_."\nend time for this strat : $this_end_time_\n";
    $mail_body_ = $mail_body_."\n".$this_mail_body_;
    SendMail($config_string_.$this_mail_body_)
}


sub GetOutSampleDates
{
    @_ == 4 or die "GetOutSampleDates called with !=4 args\n";
    my ( $outsample_date_vec_ref_, $t_start_date_, $t_end_date_, $model_file_ ) = @_;
    {
        $main_log_file_handle_->print  ("GetOutSampleDates $t_start_date_ $t_end_date_ $model_file_\n" );
    }
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
	print MAIL "Subject: replace_and_perturb_indicator ( $config_file_ ) $start_time_ $hostname_\n\n";
	## Mail Body
	print MAIL $t_mail_body_ ;
	
	close(MAIL);
	
	print $main_log_file_handle_ "Mail Sent to $mail_address_\n$t_mail_body_\n";
    }
    
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
        my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ $unique_sim_id_ $tradingdate_ $market_model_index_ 0 0.0 $use_fake_faster_data_ ADD_DBG_CODE -1 > $t_sim_res_filename_ 2>&1";
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

	    print $main_log_file_handle_ $exec_cmd_."\n";
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
    $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $cost_fn_ 0 INVALIDFILE 0";
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
