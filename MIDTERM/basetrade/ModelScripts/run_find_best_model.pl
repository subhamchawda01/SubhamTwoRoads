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
my $MODELLING_FBMFS_DIR_=$HOME_DIR."/modelling/fbmfswork/";

#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; #GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllPIDSTerminated

sub ReadConfigFile;
sub RunFindBestModel;
sub GetOutSampleDates;
sub SendMail;


my $install_staged_strats_ = 1;
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
my $insample_improvement_weight_ = 0;
my $max_outsample_improvement_thresh_ = 10000;
my $abs_outsample_improvement_thresh_ = 20;
my $input_strat_list_file_ = "";
my $install_strats_ = 1;
my $outsample_ttc_cuttoff_factor_ = 1.2;
my $outsample_vol_cuttoff_factor_ = 0.8;
my $training_days_type_ = "ALL";
my $eco_date_time_file_ = "INVALIDFILE";
my $training_day_inclusion_prob_ = 0.7;
my $use_fake_faster_data_ = rand(1) > 0.5 ? 1 : 0;  #using delays randomly, to set usage use param USE_FAKE_FASTER_DATA 

my @input_strat_list_ = ();
my $fbmfs_dir_ = $SPARE_HOME."PerturbedModelTests";
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
my $work_dir_ = $fbmfs_dir_."/".$config_base_."/".$unique_gsm_id_."/"; 

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
{
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    $work_dir_ = $fbmfs_dir_."/".$config_base_."/".$unique_gsm_id_."/";
}
else
{
    last;
}
}

`mkdir -p $work_dir_ `;
print "work_dir_ : $work_dir_\n";
my $install_dir_ = $work_dir_;
my $main_log_file_ = $work_dir_."main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "$0 Could not open log file : $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);
$mail_body_ = $mail_body_."logfile : $main_log_file_\n";

ReadConfigFile();

RunFindBestModel();

SendMail($config_string_.$mail_body_);

$main_log_file_handle_->close;
exit (0);

sub ReadConfigFile
{
    print $main_log_file_handle_ "ReadConfigFile $config_file_\n";
    open CONFIGFILEHANDLE, "+< $config_file_ " or PrintStacktraceAndDie ( "$0 Could not open config file : $config_file_ for reading\n" );
    
    my $current_instruction_="";
    my $current_instruction_set_ = 0;
    
    my @training_days_choices_ = ();
    my @eco_date_time_file_choices_ = ();

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
		$find_best_model_version_ = 1;	#disabling support for version 0 as not being used and updated
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
			if ( not IsStratOptimizable ( $strat_ ) )
			{
			    print $main_log_file_handle_ "Ignoring strategy $strat_ as it is non-linear and non-siglr \n";
			}
			else
			{
			    push ( @input_strat_list_ , $strat_ );
			}
		    }
		    else
		    {
			my $strat_full_path_ = `find $HOME_DIR/modelling/strats/ -name $strat_`; chomp($strat_full_path_);
			if( ExistsWithSize ($strat_full_path_) )
			{ 
			    if ( not IsStratOptimizable ( $strat_full_path_ ) )
			    {
				print $main_log_file_handle_ "Ignoring strategy $strat_full_path_ as it is non-linear and non-siglr \n";
			    }
			    else
			    {
				push ( @input_strat_list_ , $strat_full_path_ );
			    }
			}
		    }
                }
                close STRATFILEHANDLE;

            }
            when("OUTSAMPLE_IMPROVEMENT_THRESH")
            {
                $outsample_improvement_thresh_ = $thisline_words_[0];
            }
            when("MAX_OUTSAMPLE_IMPROVEMENT_THRESH")
            {
                $max_outsample_improvement_thresh_ = $thisline_words_[0];
            }
            when("INSAMPLE_IMPROVEMENT_WEIGHT")
            {
                $insample_improvement_weight_ = min(1, max(0, $thisline_words_[0]));  #b/w 0 to 1
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
                if($install_strats_ == 0)
                {
                    if($#thisline_words_ >=1)
                        {
                            $install_dir_ = $thisline_words_[1];
                        }
                        `mkdir -p $install_dir_`;
                }
            }
            when("OUTSAMPLE_TTC_CUTTOFF_FACTOR")
            {
                $outsample_ttc_cuttoff_factor_ = $thisline_words_[0];
            }
            when("OUTSAMPLE_VOL_CUTTOFF_FACTOR")
            {
                $outsample_vol_cuttoff_factor_ = $thisline_words_[0];
            }
            when("TRAINING_DAYS_TYPE")
            {
                my $training_days_type_t_ = $thisline_words_[0];
                if($training_days_type_t_ eq "ECO")
                {
                    if($#thisline_words_>=1)
                    {
                        my $eco_date_time_file_t_ = $thisline_words_[1];
                        if (!ExistsWithSize($eco_date_time_file_t_) )
                        {
                        	push ( @eco_date_time_file_choices_, $eco_date_time_file_t_);
                        	push ( @training_days_choices_, $training_days_type_t_ );
                        }
                    }
                } else {
                	push ( @training_days_choices_, $training_days_type_t_ );
                }
            }
            when ("USE_FAKE_FASTER_DATA")           
            {            
              $use_fake_faster_data_ = $thisline_words_[0]>0 ? 1 : 0;
            }
            when("TRAINING_DAY_INCLUSION_PROB")
            {
                $training_day_inclusion_prob_ = $thisline_words_[0];

            }
            default
            {
            }
        }
    }
    
    if ( $#eco_date_time_file_choices_ >= 0 )
    {
    	my $t_random_index_ = int ( rand ( $#eco_date_time_file_choices_ + 1));
    	$eco_date_time_file_ = $eco_date_time_file_choices_[$t_random_index_];
    }
    if ( $#training_days_choices_ >= 0 )
    {
    	my @uniq_training_days_choices_ = do { my %seen; grep { !$seen{$_}++ } @training_days_choices_ };
    	my $t_random_index_ = int ( rand ( $#uniq_training_days_choices_ + 1));
    	$training_days_type_ = $uniq_training_days_choices_ [ $t_random_index_ ];
    }
    
    my $fbm_version_string_ = $find_best_model_version_?"v1:find_best_model_for_strategy_var_pert.pl":"v0:find_best_model_for_strategy_random_dates.pl";
    $config_string_ = $config_string_."FIND_BEST_MODEL_VERSION : $fbm_version_string_\n";
    $config_string_ = $config_string_."COST_FN : $cost_fn_\n";
    
    close CONFIGFILEHANDLE;
}


sub RunFindBestModel
{
    my $start_training_date_ = CalcPrevWorkingDateMult($last_trading_date_, $num_prev_days_);
    my $run_string_ = "";
    if($training_days_type_ eq "HV")
    {
        $run_string_ = $run_string_."hv_";
    }
    if($training_days_type_ eq "BD")
    {
        $run_string_ = $run_string_."bd_";
    }
    if($training_days_type_ eq "VBD")
    {
        $run_string_ = $run_string_."vbd_";
    }
    
        
    $run_string_ = $run_string_.$num_prev_days_."_".$last_trading_date_."_F".$use_fake_faster_data_."_".substr($unique_gsm_id_, length($unique_gsm_id_) - 5, 5); #used for naming only, shortening uid for length issues
    
    my $num_installed_ = 0;
    for (my $i=0; $i<=$#input_strat_list_; $i++)
    {
        my $this_start_time_ = `date`;
    	my $this_mail_body_ = "start time for this strat : $this_start_time_\n-----------------------------------\nlogfile : $main_log_file_\n";
        my $this_input_strat_file_ = $input_strat_list_[$i];
	my $base_shortcode_ = `awk '{print \$2}' $this_input_strat_file_`; chomp($base_shortcode_);
	my $compare_dir_installed_ = $MODELLING_FBMFS_DIR_.$base_shortcode_."/installed/";
	my $compare_dir_uninstalled_ = $MODELLING_FBMFS_DIR_.$base_shortcode_."/uninstalled/";
        `mkdir -p $compare_dir_installed_`;
	`mkdir -p $compare_dir_uninstalled_`; 
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
        my $exec_cmd_ = "";
        if($find_best_model_version_ == 0)
        {
            $exec_cmd_ = $MODELSCRIPTS_DIR."find_best_model_for_strategy_random_dates.pl $this_input_strat_file_ $this_output_model_file_ $last_trading_date_ $num_prev_days_ $change_fraction_ $batch_size_ $debug_string_ $cost_fn_ $use_fake_faster_data_ ";
        }
        else
        {
            $exec_cmd_ = $MODELSCRIPTS_DIR."find_best_model_for_strategy_var_pert.pl $this_input_strat_file_ $this_output_model_file_ $last_trading_date_ $num_prev_days_ $change_fraction_ $max_number_iterations_ $search_algo_ $debug_string_ $cost_fn_ $min_vol_ $max_ttc_ $num_perturbations_ $retain_more_pert_ $training_day_inclusion_prob_ $training_days_type_ $eco_date_time_file_ $use_fake_faster_data_";
        }
        print $main_log_file_handle_ "$exec_cmd_\n";
        my $output_lines_ = `$exec_cmd_`;
        print $main_log_file_handle_ "$output_lines_\n";
        if(! ExistsWithSize($this_output_model_file_))
        {
            next;
        }
        my $training_result_ = "";

        my $insample_orig_score_ = 0;
        my $insample_orig_vol_ = 0;
        my $insample_orig_ttc_ = 0;
        my $insample_best_score_ = 0;
        my $insample_best_vol_ = 0;
        my $insample_best_ttc_ = 0;

        my $orig_score_ = 0;
        my $orig_vol_ = 0;
        my $orig_ttc_ = 0;
        my $best_score_ = 0;
        my $best_vol_ = 0;
        my $best_ttc_ = 0;

        my $stats_string_ = "";
        my $reading_summary_ = 0;
        my @output_lines_split_ = split '\n', $output_lines_;
        
        foreach (@output_lines_split_)
        {
            my @words_ = split ' ', $_;
            if($reading_summary_)
            {
                $stats_string_ = $stats_string_.$_."\n";
            }
            elsif($words_[0] eq "OrigOnTraining")
            {
                $training_result_ = $training_result_.$_."\n";
                $insample_orig_score_ = int($words_[1]);
                $insample_orig_vol_ = int($words_[2]);
                $insample_orig_ttc_ = int($words_[3]);                
            }
            elsif($words_[0] eq "BestOnTraining")
            {
                $training_result_ = $training_result_.$_."\n";
                $insample_best_score_ = int($words_[1]);
                $insample_best_vol_ = int($words_[2]);
                $insample_best_ttc_ = int($words_[3]);                
            }
            elsif($words_[0] eq "OrigOnOutofsample" && $#words_ >= 3)
            {
                $orig_score_ = int($words_[1]);
                $orig_vol_ = int($words_[2]);
                $orig_ttc_ = int($words_[3]);                
            }
            elsif($words_[0] eq "BestOnOutofsample" && $#words_ >= 3)
            {
                $best_score_ = int($words_[1]);
                $best_vol_ = int($words_[2]);
                $best_ttc_ = int($words_[3]);                
            }
            elsif($words_[0] eq "OutsamplePerformaceSummary")
            {
                $reading_summary_ = 1;
            }
        }
        my @score_vec_ = ($best_score_, $orig_score_);
        my @vol_vec_ = ($best_vol_, $orig_vol_);
        my @ttc_vec_ = ($best_ttc_, $orig_ttc_);

        my @combined_score_vec_ = ( $insample_improvement_weight_*$insample_best_score_ + (1 - $insample_improvement_weight_)*$best_score_, $insample_improvement_weight_*$insample_orig_score_ + (1 - $insample_improvement_weight_)*$orig_score_);
        my @combined_vol_vec_ = ($insample_improvement_weight_*$insample_best_vol_ + (1 - $insample_improvement_weight_)*$best_vol_, $insample_improvement_weight_*$insample_orig_vol_ + (1 - $insample_improvement_weight_)*$orig_vol_);
        my @combined_ttc_vec_ = ($insample_improvement_weight_*$insample_best_ttc_ + (1 - $insample_improvement_weight_)*$best_ttc_, $insample_improvement_weight_*$insample_orig_ttc_ + (1 - $insample_improvement_weight_)*$orig_ttc_);


        
        $exec_cmd_ = "cat $this_input_strat_file_ | awk -vnmf=$this_output_model_file_ '{\$4 = nmf; print}' > $this_output_strat_file_";
        #print $main_log_file_handle_ $exec_cmd_."\n";
        `$exec_cmd_`;
        
        print $main_log_file_handle_ "Original on OutSample : $score_vec_[1] $vol_vec_[1] $ttc_vec_[1]\n";
        print $main_log_file_handle_ "Optimized on OutSample : $score_vec_[0] $vol_vec_[0] $ttc_vec_[0]\n";
        my $score_diff_ = $combined_score_vec_[0] - $combined_score_vec_[1];
        my $this_improvement_thresh_ = max($abs_outsample_improvement_thresh_, ($outsample_improvement_thresh_)*abs($combined_score_vec_[1])); 
        my $vol_thresh_ = max(10,$outsample_vol_cuttoff_factor_*$combined_vol_vec_[1]);
        my $ttc_thresh_ = $outsample_ttc_cuttoff_factor_*$combined_ttc_vec_[1];
        my $input_model_full_path_ = `awk '{print \$4}' $this_input_strat_file_`; chomp($input_model_full_path_);
        $exec_cmd_ = $MODELSCRIPTS_DIR."compare_opt_model.py $input_model_full_path_ $this_output_model_file_";
        my $model_comp_string_ = `$exec_cmd_`;
        
        $this_mail_body_ = $this_mail_body_."================== strat : $i ====================\n";
        $this_mail_body_ = $this_mail_body_."$this_input_strat_file_\n";
        $this_mail_body_ = $this_mail_body_."Training Performance : SCORE VOL TTC\n";
        $this_mail_body_ = $this_mail_body_.$training_result_;
        $this_mail_body_ = $this_mail_body_."OutSample Performance : SCORE VOL TTC\n";
        $this_mail_body_ = $this_mail_body_."Original on OutSample : $score_vec_[1] $vol_vec_[1] $ttc_vec_[1]\n";
        $this_mail_body_ = $this_mail_body_."Optimized on OutSample : $score_vec_[0] $vol_vec_[0] $ttc_vec_[0]\n";
        $this_mail_body_ = $this_mail_body_."score_diff_ : $score_diff_ abs_score_thresh_ : $abs_outsample_improvement_thresh_ score_thresh_used_ : $this_improvement_thresh_ vol_thresh_ : $vol_thresh_ ttc_thresh_ : $ttc_thresh_\n\n";
        $this_mail_body_ = $this_mail_body_.$stats_string_;
        $this_mail_body_ = $this_mail_body_."STATISTICS PNL_AVG PNL_STD VOL_AVG PNL_SHARPE (PNL_AVG-0.33*PNL_STD) AVG_MIN_ADJ_PNL MED_AVG_TTC AVG_MAX_TTC PPC S B A AVG_MAXDD AVG_APOS PNL_CONSER_AVG PNL_MED_AVG AVG_MSGS\n\n";
        $this_mail_body_ = $this_mail_body_."Coparing Models\n".$model_comp_string_."\n";

        my $to_install_this_ = 1;
        if($score_diff_ <= $this_improvement_thresh_)   
        {
            $to_install_this_ = 0;
            $this_mail_body_ = $this_mail_body_."can't satisfy score_thresh_\n";
        }
        if($combined_vol_vec_[0] <= $vol_thresh_)   
        {
          if($score_diff_ > ($max_outsample_improvement_thresh_)*abs($combined_score_vec_[1]))
          {
            $this_mail_body_ = $this_mail_body_."can't satisfy vol_thresh_. Still installing because of huge score improvement\n";
          }
          else
          {
            $to_install_this_ = 0;
            $this_mail_body_ = $this_mail_body_."can't satisfy vol_thresh_\n";
          }
        }
        if($combined_ttc_vec_[0] >= $ttc_thresh_)   
        {

          if( $score_diff_ > ($max_outsample_improvement_thresh_)*abs($combined_score_vec_[1]))
          {
            $this_mail_body_ = $this_mail_body_."can't satisfy ttc_thresh_. Still installing because of huge score improvement\n";
          }
          else
          {
            $to_install_this_ = 0;
            $this_mail_body_ = $this_mail_body_."can't satisfy ttc_thresh_\n";
          }
        }

        my $comp_file_ = "";
	my $mail_file_ = "";

        if($to_install_this_ == 1 ) #TODO:think of a better way to handle degenrate cases of score values
        {
            print $main_log_file_handle_ "installing optimization of $this_input_strat_file_\n";
            my $input_strat_dir_path_ = `dirname $this_input_strat_file_`; chomp($input_strat_dir_path_);
            my $input_strat_name_ = `basename $this_input_strat_file_`; chomp($input_strat_name_);
            my $input_model_dir_path_ = `dirname $input_model_full_path_`; chomp($input_model_dir_path_);
            my $input_model_name_ = `basename $input_model_full_path_`; chomp($input_model_name_);
            
            if($install_strats_ == 0)
	    {
 		$input_model_dir_path_ = $install_dir_;
                $input_strat_dir_path_ = $install_dir_;
            }

            if($install_staged_strats_ == 1)
            {
              my $staged_strat_dir_path_ = $input_strat_dir_path_;
              $staged_strat_dir_path_ =~ s/modelling\/strats/modelling\/staged_strats/;
              if( !( -d $staged_strat_dir_path_ ) ) { `mkdir -p $staged_strat_dir_path_`; }
              $input_strat_dir_path_ = $staged_strat_dir_path_;
            }

            my $opt_model_full_path_ = $input_model_dir_path_."/w_m_o_".$run_string_."_".$input_model_name_;
            my $opt_strat_full_path_ = $input_strat_dir_path_."/w_o_".$run_string_."_".$input_strat_name_;
            while(-e $opt_model_full_path_ || -e $opt_strat_full_path_)
            {
              $run_string_ = $run_string_.int(rand(9));
              $opt_model_full_path_ = $input_model_dir_path_."/w_m_o_".$run_string_."_".$input_model_name_;
              $opt_strat_full_path_ = $input_strat_dir_path_."/w_o_".$run_string_."_".$input_strat_name_;
            }
            
            #if($install_strats_ == 1)
            {
		`cp $this_output_model_file_  $opt_model_full_path_`;
		$exec_cmd_ = "cat $this_input_strat_file_ | awk -vnmf=$opt_model_full_path_ '{\$4 = nmf; print}' > $this_output_strat_file_";
		`$exec_cmd_`;
		`cp $this_output_strat_file_  $opt_strat_full_path_`;
            }
            $this_mail_body_ = $this_mail_body_."installed model : $opt_model_full_path_\n";
            $this_mail_body_ = $this_mail_body_."installed strategy : $opt_strat_full_path_\n\n";
            print $main_log_file_handle_ "installed model : $opt_model_full_path_\n";
            print $main_log_file_handle_ "installed strategy : $opt_strat_full_path_\n";
	    $comp_file_ = $compare_dir_installed_."comp_".$run_string_."_".$input_strat_name_;
	    $mail_file_ = $compare_dir_installed_."mail_".$run_string_."_".$input_strat_name_;
            $num_installed_ ++;

	    if ( ! $mail_address_ )
	    { # print this information to stdout
		print "OUTPUT INSTALLATION $opt_strat_full_path_\n";
	    }
        }
        else
        {
            print $main_log_file_handle_ "copying optimization of $this_input_strat_file_\n";
            my $input_strat_name_ = `basename $this_input_strat_file_`; chomp($input_strat_name_);
	    my $input_model_name_ = `basename $input_model_full_path_`; chomp($input_model_name_);
            my $opt_model_full_path_ = $compare_dir_uninstalled_."w_m_o_un_".$run_string_."_".$input_model_name_;
            my $opt_strat_full_path_ = $compare_dir_uninstalled_."/w_o_un_".$run_string_."_".$input_strat_name_;
	    while(-e $opt_model_full_path_ || -e $opt_strat_full_path_)
            {
              $run_string_ = $run_string_.int(rand(9));
              $opt_model_full_path_ = $compare_dir_uninstalled_."w_m_o_un_".$run_string_."_".$input_model_name_;
              $opt_strat_full_path_ = $compare_dir_uninstalled_."/w_o_un_".$run_string_."_".$input_strat_name_;
            }
            if ( $install_strats_ == 1 )
	    {
		`cp $this_output_model_file_  $opt_model_full_path_`;
		$exec_cmd_ = "cat $this_input_strat_file_ | awk -vnmf=$opt_model_full_path_ '{\$4 = nmf; print}' > $this_output_strat_file_";
		`$exec_cmd_`;
		`cp $this_output_strat_file_  $opt_strat_full_path_`;
	    }
            $this_mail_body_ = $this_mail_body_."uninstalled model : $opt_model_full_path_\n";
            $this_mail_body_ = $this_mail_body_."uninstalled strategy : $opt_strat_full_path_\n\n";
            print $main_log_file_handle_ "uninstalled model : $opt_model_full_path_\n";
            print $main_log_file_handle_ "uninstalled strategy : $opt_strat_full_path_\n";

            $comp_file_ = $compare_dir_uninstalled_."comp_".$run_string_."_".$input_strat_name_;
            $mail_file_ = $compare_dir_uninstalled_."mail_".$run_string_."_".$input_strat_name_;
            $this_mail_body_ = $this_mail_body_."not installing as thresholds not satisfied\n";

	    if ( ! $mail_address_ )
	    { # print this information to stdout
		print "OUTPUT NO_INSTALLATION $this_input_strat_file_\n";
	    }
        }
	if($mail_address_)
	{
		open COMPFILE, "> $comp_file_ " or PrintStacktraceAndDie ( "$0 Could not open comp_file_ : $comp_file_ for writing\n" );
		print COMPFILE $model_comp_string_;
		close COMPFILE;
	}
	
	my $this_end_time_ = `date`;
	$this_mail_body_  = $this_mail_body_."\nend time for this strat : $this_end_time_\n";
        $mail_body_ = $mail_body_."\n".$this_mail_body_;
	SendMail($config_string_.$this_mail_body_);
	if($mail_address_)
	{	
		open MAILFILE, "> $mail_file_ " or PrintStacktraceAndDie ( "$0 Could not open mail_file_ : $mail_file_ for writing\n" );
		print MAILFILE $config_string_.$this_mail_body_;
		close MAILFILE;
	}
    }
    $mail_body_ = "Total installed strats : $num_installed_($install_strats_)\n".$mail_body_;
    my $end_time_ = `date`;
    $mail_body_ = $mail_body_."\nend time : $end_time_\n-------------------------------------------------------------------------\n";
}


sub GetOutSampleDates
{
    @_ == 4 or die "GetOutSampleDates called with !=4 args\n";
    my ( $outsample_date_vec_ref_, $t_start_date_, $t_end_date_, $model_file_ ) = @_;
    #{
    #$main_log_file_handle_->print  ("GetOutSampleDates $t_start_date_ $t_end_date_ $model_file_\n" );
    #}
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
	print MAIL "Subject: runfbmfs ( $config_file_ ) $start_time_ $hostname_\n\n";
	## Mail Body
	print MAIL $t_mail_body_ ;
	
	close(MAIL);
	
	print $main_log_file_handle_ "Mail Sent to $mail_address_\n\n$t_mail_body_\n";
    }
    
}

sub IsStratOptimizable
{
  my ( $strategy_filename_ ) = @_;  
  my $model_filename_ = GetModelFromStrat ( $strategy_filename_ );
  my $modelmath_ = `awk '{if(\$1==\"MODELMATH\"){print \$2;}}' $model_filename_`; chomp($modelmath_);
  return ( $modelmath_ eq "LINEAR" || $modelmath_ eq "SIGLR" ) 
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
