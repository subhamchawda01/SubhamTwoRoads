#!/usr/bin/perl

# \file ModelScripts/find_best_params_permute_for_strat.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
#
# SHORTCODE
# TIMEPERIOD
# BASEPX
# PARAMFILE_WITH_PERMUTATIONS
# TRADING_START_YYYYMMDD 
# TRADING_END_YYYYMMDD

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use List::Util qw(first); # for index

package ResultLine;
use Class::Struct;

# declare the struct
struct ( 'ResultLine', { pnl_ => '$', volume_ => '$', ttc_ => '$' } );

package main;

sub CheckVolTTC ;
sub PermuteBaseParam ;
sub CalcPnls ;
sub FillDateVecs ;
sub ReadParamFile ;
sub ReadStratFile ;
sub ReadConfigFile ;
sub GetBestIndex ;
sub CheckAggTrade ;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPA_WORK_DIR=$SPARE_HOME."FBPA/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "ankit" || $USER eq "anshul")
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to s

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_start_end_hhmm.pl"; # GetStratStartEndHHMM
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_base_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratBaseExcludingSets
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MAX_STRAT_FILES_IN_ONE_SIM = 40; # please work on optimizing this value

my $DEBUG = 0;
# start
my $USAGE="$0 config_file";


if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $config_file_ = $ARGV[0];

my $delete_intermediate_files_ = 1;

my %strat2param_ ;
my @model_filevec_ = ();
my @param_filevec_ = ();
my @strategy_filevec_ = ();
my %param_to_resultvec_;
my %param_to_cmedpnl_;
my %param_to_cmedvolume_;

my @intermediate_files_ = ();

my @training_date_vec_ = ();
my @outofsample_date_vec_ = ();

my @param_key_name_ = ();
my @orig_param_vec_ = ();
my $zeropos_place_index_ = "" ;
my $place_keep_diff_index_ = "" ;
my $increase_zeropos_diff_index_ = "" ;
my $zeropos_decrease_diff_index_ = "" ;
my $aggressive_index_ = "" ;
my $allowed_to_aggress_index_ = "" ;

my $base_model_file_ = "";
my $base_param_file_ = "";
my $base_start_time_ = "";
my $base_end_time_ = "";
my $base_prog_id_ = "";
my $base_pbat_dat_ = "";
my $base_shortcode_ = "";
my $base_model_start_text_ = "";
my $strat_pre_text_ = "";
my $strat_post_text_ = "";
my $work_dir_ = "/spare/local/$USER/FBP";

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $config_base_ = basename ( $config_file_ ); chomp ( $config_base_ );
$work_dir_ = $work_dir_."/".$config_base_."/".$unique_gsm_id_."/"; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $work_dir_."/".$config_base_."/".$unique_gsm_id_."/"; 
    }
    else
    {
	last;
    }
}

if ( -d $work_dir_ ) { `rm -rf $work_dir_`; }
`mkdir -p $work_dir_ `; #make_work_dir

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);
my $start_time_ =`date +%s`; chomp ( $start_time_ );

my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $local_params_dir_ = $work_dir_."/params_dir";

my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

my $input_strat_file_ = "" ;
my $strat_file_ = "";
my $orig_param_ = "";
my $min_vol_ = "";
my $max_vol_ = "";
my $min_agg_trade_ = "";
my $max_agg_trade_ = "";
my $max_ttc_ = "";
my $min_zeropos_place_ = "";
my $max_zeropos_place_ = "";
my $min_place_keep_diff_ = "";
my $max_place_keep_diff_ = "";
my $min_increase_zeropos_diff_ = "";
my $max_increase_zeropos_diff_ = "";
my $min_zeropos_decrease_diff_ = "";
my $max_zeropos_decrease_diff_ = "";
my $min_aggressive_ = "";
my $max_aggressive_ = "";
my $last_trading_date_ = "";
my $num_prev_days_ = "";
my $max_number_of_iterations_ = 10;
my $sort_algo_ = "kCNAPnlAverage";
my $training_period_fraction_ = 0.99;
my $training_days_type_ = "ALL";

ReadConfigFile ();

ReadStratFile( );

FillDateVecs ( \@training_date_vec_, \@outofsample_date_vec_, $last_trading_date_, $num_prev_days_ );
if ( $#training_date_vec_ < 0 )
{
    print "No training dates\n";
    exit ( 0 );
}
if ( $#outofsample_date_vec_ < 0 )
{
    print "No outofsample dates\n";
}
if ( $DEBUG )
{
    printf ( "Working Directory: %s\n", $work_dir_ );
    $main_log_file_handle_->print ( "Training dates : @training_date_vec_ \n" );
    $main_log_file_handle_->print ( "OutofSample dates : @outofsample_date_vec_ \n" );
}


# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir ) ) { `mkdir -p $local_results_base_dir`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);


$strat_pre_text_ = "STRATEGYLINE ".$base_shortcode_." ".$base_pbat_dat_." ".$base_model_file_." ";
$strat_post_text_ = " ".$base_start_time_." ".$base_end_time_." ".$base_prog_id_;

ReadParamFile ();

my $vol_flag_ = 0;
my $ttc_flag_ = 0;
my $agg_trade_flag_ = 0;
my $orig_pnl_ = "" ;
my $orig_vol_ = "" ;
my $orig_ttc_ = "" ;
my @param_vec_vec_ = ();
my @base_param_vec_ = ();
my $iter_count_ = 0;
my $improvement_made_ = 1;
push(@param_vec_vec_, \@orig_param_vec_);

while ( $iter_count_ < $max_number_of_iterations_ && $improvement_made_ == 1)
{  
  if ( $vol_flag_==1 && $ttc_flag_==1 && $agg_trade_flag_==1)
  {
    last;
  }
  else
  { 
    $main_log_file_handle_->printf ( "Starting iteration %d\n", $iter_count_ ) ;
    my @stats_vec_ = ( );
    my @volume_vec_ = ( );
    my @ttc_vec_ = ( );
    my @agg_trade_vec_ = ( );
    my @index_vec_ = ( );
    my $best_index_ = "" ;
    CalcPnls (\@param_vec_vec_, \@stats_vec_, \@volume_vec_, \@ttc_vec_, \@agg_trade_vec_, $iter_count_, \@training_date_vec_);
    
    if ( $iter_count_ == 1 )
    {
        $orig_pnl_ = $stats_vec_[0];
        $orig_vol_ = $volume_vec_[0];
        $orig_ttc_ = $ttc_vec_[0];

        my $tref_ = $param_vec_vec_[0];
        @base_param_vec_ = @$tref_ ; # set to best
    }

    if ( $vol_flag_==1 && $ttc_flag_==1 && $agg_trade_flag_==0 )
    {
        $best_index_ = CheckAggTrade (\@param_vec_vec_, \@stats_vec_, \@agg_trade_vec_, \@index_vec_);
        $main_log_file_handle_->printf ( "Best index for agg_trade %d %d\n",$agg_trade_flag_, $best_index_ ) ;
        @base_param_vec_ = @{$param_vec_vec_[$best_index_]};
    } 
    else
    {
        $best_index_ = CheckVolTTC (\@param_vec_vec_, \@stats_vec_, \@volume_vec_, \@ttc_vec_, \@agg_trade_vec_, \@index_vec_);
        $main_log_file_handle_->printf ( "Best index for vol, ttc %d %d %d\n", $vol_flag_, $ttc_flag_, $best_index_ ) ;
        @base_param_vec_ = @{$param_vec_vec_[$best_index_]};
    }

    @param_vec_vec_ = PermuteBaseParam (\@base_param_vec_, \@volume_vec_, \@ttc_vec_, \@agg_trade_vec_, $best_index_, $iter_count_);
    $iter_count_++;
   
  }
}

WriteOutParam(\@base_param_vec_);

# end script
$main_log_file_handle_->close;
#
exit ( 0 );


sub PermuteBaseParam
{
    my ( $param_base_vec_ref_, $volume_vec_ref_, $ttc_vec_ref_, $agg_trade_vec_ref_, $best_index_, $iter_count_ ) = @_ ;
    
    my @param_vec_vec_ = ( );
    my @this_param_vec_ = @$param_base_vec_ref_ ;
    if ( $vol_flag_ == 1 && $ttc_flag_ == 1 && $agg_trade_flag_==1 )
    {
    #matched our criteria
        my @t_param_vec_ = @this_param_vec_ ;
        push ( @param_vec_vec_, \@t_param_vec_ );
    } elsif ( $vol_flag_ == 1 && $ttc_flag_ == 1 )
    {
    #permute aggressive param
        if ( $this_param_vec_[$allowed_to_aggress_index_] == 0 )
        {
            my @t_param_vec_ = @this_param_vec_ ;
            $t_param_vec_[$allowed_to_aggress_index_] = 1;
            push ( @param_vec_vec_, \@t_param_vec_ );
        } else {
           
            if ( $$agg_trade_vec_ref_[$best_index_] >= $min_agg_trade_ && $$agg_trade_vec_ref_[$best_index_] <= $max_agg_trade_ )
            {
                $agg_trade_flag_ = 1;
                my @t_param_vec_ = @this_param_vec_ ;
                push ( @param_vec_vec_, \@t_param_vec_ );
            } elsif ( $$agg_trade_vec_ref_[$best_index_] < $min_agg_trade_ )
            {
                if ( $this_param_vec_[$aggressive_index_] != $min_aggressive_ )
                {
                    my @t_param_vec_ = @this_param_vec_ ;
                    $t_param_vec_[$aggressive_index_] = $min_aggressive_ ;
                    $t_param_vec_[$allowed_to_aggress_index_] = 1;
                    push ( @param_vec_vec_, \@t_param_vec_ );
                }
            } else {   
                if ( $this_param_vec_[$aggressive_index_] != $max_aggressive_ )
                {
                    my @t_param_vec_ = @this_param_vec_ ;
                    $t_param_vec_[$aggressive_index_] = $max_aggressive_ ;
                    $t_param_vec_[$allowed_to_aggress_index_] = 1;
                    push ( @param_vec_vec_, \@t_param_vec_ );
                }
            }
        }           
    } elsif ( $vol_flag_ == 1 ) {  
    #permutations to decrease TTC
        if ( $this_param_vec_[$zeropos_decrease_diff_index_] != $max_zeropos_decrease_diff_ )
        {
            my @t_param_vec_ = @this_param_vec_ ;
            $t_param_vec_[$zeropos_decrease_diff_index_] = $max_zeropos_decrease_diff_ ;
            push ( @param_vec_vec_, \@t_param_vec_ );
        }
        if ($this_param_vec_[$place_keep_diff_index_] != $max_place_keep_diff_)
        {
            my @t_param_vec_ = @this_param_vec_ ;
            $t_param_vec_[$place_keep_diff_index_] = $max_place_keep_diff_ ;
            push ( @param_vec_vec_, \@t_param_vec_ );
        }
    } else {
    #volume criteria not met, permute all the params
        if ( $$volume_vec_ref_[$best_index_] < $min_vol_ )
        {
            if ($this_param_vec_[$zeropos_place_index_] != $min_zeropos_place_)
            {
                my @t_param_vec_ = @this_param_vec_ ;
                $t_param_vec_[$zeropos_place_index_] = $min_zeropos_place_ ;
                push ( @param_vec_vec_, \@t_param_vec_ );
            }
            if ($this_param_vec_[$place_keep_diff_index_] != $max_place_keep_diff_)
            {
                my @t_param_vec_ = @this_param_vec_ ;
                $t_param_vec_[$place_keep_diff_index_] = $max_place_keep_diff_ ;
                push ( @param_vec_vec_, \@t_param_vec_ );
            }
            if ($this_param_vec_[$increase_zeropos_diff_index_] != $min_increase_zeropos_diff_ )
            {
                my @t_param_vec_ = @this_param_vec_ ;
                $t_param_vec_[$increase_zeropos_diff_index_] = $min_increase_zeropos_diff_ ;
                push ( @param_vec_vec_, \@t_param_vec_ );
            }
            if ($this_param_vec_[$zeropos_decrease_diff_index_] != $max_zeropos_decrease_diff_ )
            {
                my @t_param_vec_ = @this_param_vec_ ;
                $t_param_vec_[$zeropos_decrease_diff_index_] = $max_zeropos_decrease_diff_ ;
                push ( @param_vec_vec_, \@t_param_vec_ );
            }           
        } else {
            if ($this_param_vec_[$zeropos_place_index_] != $max_zeropos_place_)
            {
                my @t_param_vec_ = @this_param_vec_ ;
                $t_param_vec_[$zeropos_place_index_] = $max_zeropos_place_ ;
                push ( @param_vec_vec_, \@t_param_vec_ );
            }
            if ($this_param_vec_[$place_keep_diff_index_] != $min_place_keep_diff_)
            {
                my @t_param_vec_ = @this_param_vec_ ;
                $t_param_vec_[$place_keep_diff_index_] = $min_place_keep_diff_ ;
                push ( @param_vec_vec_, \@t_param_vec_ );
            }
            if ($this_param_vec_[$increase_zeropos_diff_index_] != $max_increase_zeropos_diff_ )
            {
                my @t_param_vec_ = @this_param_vec_ ;
                $t_param_vec_[$increase_zeropos_diff_index_] = $max_increase_zeropos_diff_ ;
                push ( @param_vec_vec_, \@t_param_vec_ );
            }
            if ($this_param_vec_[$zeropos_decrease_diff_index_] != $min_zeropos_decrease_diff_ )
            {
                my @t_param_vec_ = @this_param_vec_ ;
                $t_param_vec_[$zeropos_decrease_diff_index_] = $min_zeropos_decrease_diff_ ;
                push ( @param_vec_vec_, \@t_param_vec_ );
            }   
        }    
    }

    if ( $#param_vec_vec_ < 0 )
    {
          $main_log_file_handle_->printf ( "No Permutation left to achieve the desired trading profile\n") ; 
          $improvement_made_ = 0 ;
          my @t_param_vec_ = @this_param_vec_ ;
          push ( @param_vec_vec_, \@t_param_vec_ );
    }
    return @param_vec_vec_ ;
}

sub CheckVolTTC
{
    my ( $param_vec_vec_ref_, $stats_vec_ref_, $volume_vec_ref_, $ttc_vec_ref_, $agg_trade_vec_ref_, $index_vec_ref_ ) = @_;
    my @score_vec_ = ();
    for(my $i=0; $i <= $#$param_vec_vec_ref_ ; $i++)
    {
        if($$volume_vec_ref_[$i] >= $min_vol_ && $$volume_vec_ref_[$i] <= $max_vol_ && $$ttc_vec_ref_[$i] <= $max_ttc_)
        {
            push(@$index_vec_ref_, $i);
            $vol_flag_ = 1;
            $ttc_flag_ = 1;
        }
    }
  
    if($#$index_vec_ref_<0)
    {
        for(my $i=0; $i <= $#$param_vec_vec_ref_ ; $i++)
        {
            if($$volume_vec_ref_[$i]>= $min_vol_ && $$volume_vec_ref_[$i]<=$max_vol_ )
            {
                push(@$index_vec_ref_, $i);
                $vol_flag_ = 1;
            }
        }
    }

    if($#$index_vec_ref_<0)
    {
        $main_log_file_handle_->printf ( "Scoring for vol and ttc\n" );
        for(my $i=0; $i <= $#$param_vec_vec_ref_ ; $i++)
        {
            my $x = abs($$volume_vec_ref_[$i]-$min_vol_);
            my $y = abs($$volume_vec_ref_[$i]-$max_vol_);
            $score_vec_[$i] = $x >= $y ? $y : $x ;
            $main_log_file_handle_->printf ( "score : %d\n", $score_vec_[$i] ) ;
            push (@$index_vec_ref_, $i);
        }         
    }
   
    my $t_best_index_ = $vol_flag_==1 ? GetBestIndex(\@$stats_vec_ref_, \@$index_vec_ref_) : GetBestIndex(\@score_vec_, \@$index_vec_ref_);
    return $t_best_index_ ;
}

sub CheckAggTrade
{
    my ( $param_vec_vec_ref_, $stats_vec_ref_, $agg_trade_vec_ref_, $index_vec_ref_ ) = @_;
    my @score_vec_ = ();
    for(my $i=0; $i <= $#$param_vec_vec_ref_ ; $i++)
    {
        if( $$agg_trade_vec_ref_[$i] >= $min_agg_trade_ && $$agg_trade_vec_ref_[$i] <= $max_agg_trade_ )
        {
            push(@$index_vec_ref_, $i);
            $agg_trade_flag_ = 1;
        }
    }
   
    if ($#$index_vec_ref_<0)
    {
        $main_log_file_handle_->printf ( "Scoring for agg_trade\n" );
        for(my $i=0; $i <= $#$param_vec_vec_ref_ ; $i++)
        {
            my $x = abs($$agg_trade_vec_ref_[$i]-$min_agg_trade_);
            my $y = abs($$agg_trade_vec_ref_[$i]-$max_agg_trade_);
            $score_vec_[$i] = $x >= $y ? $y : $x ;
            $main_log_file_handle_->printf ( "score : %d\n", $score_vec_[$i] ) ;
            push (@$index_vec_ref_, $i);
        }
    }
    my $t_best_index_ = $agg_trade_flag_== 1 ? GetBestIndex(\@$agg_trade_vec_ref_, \@$index_vec_ref_) : GetBestIndex(\@score_vec_, \@$index_vec_ref_);
    return $t_best_index_ ;
}

sub GetBestIndex
{
    my ( $score_vec_ref_, $index_vec_ref_ ) = @_;
    my @score_vec_temp_ = ( );
    for(my $i=0; $i<=$#$index_vec_ref_; $i++)
    {
        $score_vec_temp_[$i] = $$score_vec_ref_[$$index_vec_ref_[$i]];
    }
    my $best_index_ = GetIndexOfMaxValue ( \@score_vec_temp_ );
    $main_log_file_handle_->printf ( "GetBestIndex %d\n", $best_index_ ) ;
    $main_log_file_handle_->printf ( "GetBestIndex %d\n", $$index_vec_ref_[$best_index_] ) ;
    return $$index_vec_ref_[$best_index_];
}

	

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
            when("STRAT")
	    {
                $input_strat_file_ =  $thisline_words_[0];
                $strat_file_ = `find $HOME_DIR/modelling/strats/ -name $input_strat_file_`; chomp($strat_file_);
                
            }
            when("PARAM")
            {
                $orig_param_ =  $thisline_words_[0];
            }
            when("VOL_MIN_MAX")
            {
                $min_vol_ = $thisline_words_[0];
                $max_vol_ = $thisline_words_[1];
                print $main_log_file_handle_ "min_volume_ : $min_vol_\n";
                print $main_log_file_handle_ "max_volume_ : $max_vol_\n";
            }
            when("AGG_TRADE_MIN_MAX")
            {
                $min_agg_trade_ = $thisline_words_[0];
                $max_agg_trade_ = $thisline_words_[1];
                print $main_log_file_handle_ "min_agg_trade_ : $min_agg_trade_ , max_agg_trade_ : $max_agg_trade_\n";
            }
            when("MAX_TTC")
            {
                $max_ttc_ = $thisline_words_[0];
                print $main_log_file_handle_ "max_ttc_ : $max_ttc_\n";
            }
            when("ZEROPOS_PLACE_MIN_MAX")
            {
                $min_zeropos_place_ = $thisline_words_[0];
                $max_zeropos_place_ = $thisline_words_[1];
                print $main_log_file_handle_ "min_zeropos_place_ : $min_zeropos_place_ , max_zeropos_place_ : $max_zeropos_place_\n";
            }
            when("PLACE_KEEP_DIFF_MIN_MAX")
            {
                $min_place_keep_diff_ = $thisline_words_[0];
                $max_place_keep_diff_ = $thisline_words_[1];
                print $main_log_file_handle_ "min_place_keep_diff_ : $min_place_keep_diff_ , max_place_keep_diff_ : $max_place_keep_diff_\n";
            }
            when("INCREASE_ZEROPOS_DIFF_MIN_MAX")
            {
                $min_increase_zeropos_diff_ = $thisline_words_[0];
                $max_increase_zeropos_diff_ = $thisline_words_[1];
                print $main_log_file_handle_ "min_increase_zeropos_diff_ : $min_increase_zeropos_diff_ , max_increase_zeropos_diff_ : $max_increase_zeropos_diff_\n";
            }
            when("ZEROPOS_DECREASE_DIFF_MIN_MAX")
            {
                $min_zeropos_decrease_diff_ = $thisline_words_[0];
                $max_zeropos_decrease_diff_ = $thisline_words_[1];
                print $main_log_file_handle_ "min_zeropos_decrease_diff_ : $min_zeropos_decrease_diff_ , max_zeropos_decrease_diff_ : $max_zeropos_decrease_diff_\n";
            }
            when("AGGRESSIVE_MIN_MAX")
            {
                $min_aggressive_ = $thisline_words_[0];
                $max_aggressive_ = $thisline_words_[1];
                print $main_log_file_handle_ "min_aggressive_ : $min_aggressive_ , max_aggressive_ : $max_aggressive_\n";
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
            when("DEBUG")
            {
                if ( index ($thisline_words_[0] , "D" ) != -1 ) { $DEBUG = 1; print $main_log_file_handle_ "DEBUG MDOE\n"; }
            }
            when("SORT_ALGO")
            {
                $sort_algo_ = $thisline_words_[0];
            }
            when("MAX_NUMBER_ITERATIONS")
            {
                $max_number_of_iterations_ = $thisline_words_[0];
            }
            default
            {
            }
        }
    }
    close CONFIGFILEHANDLE;
}


sub ReadStratFile
{
    if ( $DEBUG )
    {
        printf ( "ReadStratFile\n" ) ;
    }

    open STRAT_FILE, "< $strat_file_" or PrintStacktraceAndDie ( "Could not open strategy file $strat_file_ for reading\n" );
    my $strat_line_ = <STRAT_FILE>;
    my @strat_words_ = split ' ', $strat_line_;

    $base_shortcode_ = $strat_words_[1];
    $base_pbat_dat_ = $strat_words_[2];
    $base_model_file_ = $strat_words_[3];
    $base_param_file_ = $strat_words_[4];
    $base_start_time_ = $strat_words_[5];
    $base_end_time_ = $strat_words_[6];
    $base_prog_id_ = $strat_words_[7];
    return;
}


sub ReadParamFile
{
    open PARAM_FILE_, "<", $orig_param_ or PrintStacktraceAndDie ( "Could not open file : $orig_param_\n" );

    while (my $line_ = <PARAM_FILE_>)
    {
        if (substr ($line_, 0, 1) eq '#')
        {
            next;
        }

        my @words_ = split (' ', $line_);

        if ($#words_ > 2)
        {
            printf "Ignoring Malformed line : %s\n", $line_;
            next;
        }
        my $param_name_val_ = $words_[0]." ".$words_[1];
        push (@param_key_name_, $param_name_val_);
        push (@orig_param_vec_, $words_[2]);
     }

     $zeropos_place_index_ = first { $param_key_name_[$_] eq 'PARAMVALUE ZEROPOS_PLACE' } 0..$#param_key_name_;
     print $main_log_file_handle_ "zeropos_place_index_ : $zeropos_place_index_ \n";
     $place_keep_diff_index_ = first { $param_key_name_[$_] eq 'PARAMVALUE PLACE_KEEP_DIFF' } 0..$#param_key_name_;
     print $main_log_file_handle_ "place_keep_diff_index_ : $place_keep_diff_index_ \n";
     $increase_zeropos_diff_index_ = first { $param_key_name_[$_] eq 'PARAMVALUE INCREASE_ZEROPOS_DIFF' } 0..$#param_key_name_;
     print $main_log_file_handle_ "increase_zeropos_diff_index_ : $increase_zeropos_diff_index_ \n";
     $zeropos_decrease_diff_index_ = first { $param_key_name_[$_] eq 'PARAMVALUE ZEROPOS_DECREASE_DIFF' } 0..$#param_key_name_;
     print $main_log_file_handle_ "zeropos_decrease_diff_index_ : $zeropos_decrease_diff_index_ \n";
     $aggressive_index_ = first { $param_key_name_[$_] eq 'PARAMVALUE AGGRESSIVE' } 0..$#param_key_name_;
     print $main_log_file_handle_ "aggressive_index_ : $aggressive_index_ \n";
     $allowed_to_aggress_index_ = first { $param_key_name_[$_] eq 'PARAMVALUE ALLOWED_TO_AGGRESS' } 0..$#param_key_name_;
     if ( defined $allowed_to_aggress_index_ ) 
     {
         print $main_log_file_handle_ "allowed_to_aggress_index_ : $allowed_to_aggress_index_ \n";
         $orig_param_vec_[$allowed_to_aggress_index_] = 0;
     }
     else
     { 
         print $main_log_file_handle_ "Not Allowed to aggress";
         $agg_trade_flag_ = 1; 
     }
     return;
}

sub FillDateVecs
{
    @_ == 4 or die "FillDateVec called with !=4 args\n";
    my ( $training_date_vec_ref_, $outsample_date_vec_ref_, $t_last_trading_date_, $t_num_days_ ) = @_;
    if ( $DEBUG )
    {
        $main_log_file_handle_->print  ("FillDateVec $t_last_trading_date_ $t_num_days_\n" );
    }


    if($training_days_type_ eq "HV" || $training_days_type_ eq "BD" || $training_days_type_ eq "VBD")
    {
        my @special_day_vec_ = ();
        if($training_days_type_ eq "HV")
        {
            GetHighVolumeDaysForShortcode ( $base_shortcode_, \@special_day_vec_ );
        }
        if($training_days_type_ eq "BD")
        {
            GetBadDaysForShortcode ( $base_shortcode_, \@special_day_vec_ );
        }
        if($training_days_type_ eq "VBD")
        {
            GetVeryBadDaysForShortcode ( $base_shortcode_, \@special_day_vec_ );
        }
        my $tradingdate_ = $t_last_trading_date_;
        for ( my $t_day_index_ = 0; $t_day_index_ < $t_num_days_; $t_day_index_ ++ )
        {
            if ( SkipWeirdDate ( $tradingdate_ ) ||
                 ( NoDataDateForShortcode ( $tradingdate_ , $base_shortcode_ ) ) ||
                 ( IsDateHoliday ( $tradingdate_ ) || ( ( $base_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $base_shortcode_ ) ) ) ) )
            {
                $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
                $t_day_index_ --;
                next;
            }
            if ( grep {$_ eq $tradingdate_} @special_day_vec_ )
            {
                push ( @$training_date_vec_ref_, $tradingdate_ );
            }
            $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
        }
        if($#$training_date_vec_ref_ >= min(15, $t_num_days_/2))    #20 days for training are fine, if smaller span is given, then even less days are ok
        {
            return;
        }
        else
        {
            $main_log_file_handle_->print("can't find enough ".$training_days_type_." days. Training in normal days\n");
            print "very few( ".($#$training_date_vec_ref_ + 1)." )".$training_days_type_." days for training.\n";
            exit(0);
        }
    }

    @$training_date_vec_ref_ = ();
    my $tradingdate_ = $t_last_trading_date_;
    for ( my $t_day_index_ = 0; $t_day_index_ < $t_num_days_; $t_day_index_ ++ )
    {
        if ( SkipWeirdDate ( $tradingdate_ ) ||
             ( NoDataDateForShortcode ( $tradingdate_ , $base_shortcode_ ) ) ||
             ( IsDateHoliday ( $tradingdate_ ) || ( ( $base_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $base_shortcode_ ) ) ) ) )
        {
            $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
            $t_day_index_ --;
            next;
        }
        if ( rand( ) <= $training_period_fraction_ )
        {
            push ( @$training_date_vec_ref_, $tradingdate_ );
        }
        else
        {
            push ( @$outsample_date_vec_ref_, $tradingdate_ );
        }

        $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }
}

sub WriteOutParam
{
    if ( $DEBUG )
    {
        $main_log_file_handle_->print ( "WriteOutParam\n" );
    }
    if ($vol_flag_ == 0 || $ttc_flag_ == 0 || $agg_trade_flag_ == 0 )
    {
        $main_log_file_handle_->print ( "No Param with desired trading profile\n" );
        $main_log_file_handle_->print ( "Vol Flag: $vol_flag_ , TTC Flag: $ttc_flag_ , Agg Trade: $agg_trade_flag_\n" );
    }
    else
    {
        my ( $tref_ ) = @_;
        my $orig_param_base_ = `basename $orig_param_` ;
        my $t_param_filename_ = $work_dir_."/"."opt_".$orig_param_base_;
        my $t_output_ = "";
        for ( my $j = 0; $j <= $#param_key_name_ ; $j ++)
        {
            $t_output_ = $t_output_.$param_key_name_[$j]." ".$$tref_[$j]."\n";
        }    
        open OUTPARAM, "> $t_param_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_param_filename_ for writing\n" );
        print OUTPARAM $t_output_;
        close OUTPARAM;
    }
}


sub CalcPnls
{
    if ( $DEBUG )
    {
        $main_log_file_handle_->print ( "CalcPnls\n" );
    }
    my ( $param_vec_vec_ref_, $stats_vec_ref_, $volume_vec_ref_, $ttc_vec_ref_, $agg_trade_vec_ref_, $iter_count_, $date_vec_ref_ ) = @_;
    my @date_vec_= ();
#    my %models_list_map_ = ( );
    my %params_list_map_ = ( );
    my $t_strat_filename_ = $work_dir_."/"."tmp_strat_".$iter_count_;
#    my $t_models_list_ = $work_dir_."/"."tmp_models_list_".$iter_count_;
    my $t_params_list_ = $work_dir_."/"."tmp_params_list_".$iter_count_;
    my $local_results_base_dir = $work_dir_."/local_results_base_dir".$iter_count_;
    open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
#    open OUTMODELLIST, "> $t_models_list_" or PrintStacktraceAndDie ( "Could not open output_models_list_filename_ $t_models_list_ for writing\n" );
    open OUTPARAMLIST, "> $t_params_list_" or PrintStacktraceAndDie ( "Could not open output_models_list_filename_ $t_params_list_ for writing\n" );
    for ( my $i = 0; $i <= $#$param_vec_vec_ref_ ; $i ++)
    {
        my $t_param_filename_ = $work_dir_."/"."tmp_param_".$iter_count_."_".$i;
#        my $t_strat_text_ = $strat_pre_text_.$t_param_filename_.$strat_post_text_."\n"; # TODO not changing progid ?
        my $t_output_ = "";
        my $tref_ = $$param_vec_vec_ref_[$i];

        for ( my $j = 0; $j <= $#param_key_name_ ; $j ++)
        {
            $t_output_ = $t_output_.$param_key_name_[$j]." ".$$tref_[$j]."\n";
        }


        open OUTPARAM, "> $t_param_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_param_filename_ for writing\n" );
        print OUTPARAM $t_output_;
        close OUTPARAM;
        my $t_strat_text_ = $strat_pre_text_.$t_param_filename_.$strat_post_text_.$i."\n";
        print OUTSTRAT $t_strat_text_;
        print OUTPARAMLIST "tmp_param_".$iter_count_."_".$i."\n";
        $params_list_map_{"tmp_param_".$iter_count_."_".$i}=$i;
    }
    close OUTSTRAT;
    close OUTPARAMLIST;

    my @nonzero_tradingdate_vec_ = ( );
    my $start_date_ = "";
    my $end_date_ = "";

    for ( my $t_day_index_ = 0; $t_day_index_ <= $#$date_vec_ref_ ; $t_day_index_ ++ )
    {
	my $tradingdate_ = $$date_vec_ref_[$t_day_index_];
	
	my $sim_strat_cerr_file_ = $t_strat_filename_."_cerr";
	my $market_model_index_ = GetMarketModelForShortcode ( $base_shortcode_ );
	my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $t_strat_filename_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1 2>$sim_strat_cerr_file_"; # using hyper optimistic market_model_index, added nologs argument
	my $t_sim_res_filename_ = $work_dir_."/"."sim_res_".$iter_count_."_".$tradingdate_;
	if ( -e $t_sim_res_filename_ ) { `rm -f $t_sim_res_filename_`; }
	#$main_log_file_handle_->print ( "$exec_cmd_ | grep SIMRESULT > $t_sim_res_filename_\n" );
	my @sim_strategy_output_lines_=();
	my %unique_id_to_pnlstats_map_ =();
	`$exec_cmd_ > $t_sim_res_filename_ `; 
	#@sim_strategy_output_lines_=`$exec_cmd`;
	if ( ExistsWithSize ( $t_sim_res_filename_ ) )
	{	
	    push (@nonzero_tradingdate_vec_, $tradingdate_);
	    @sim_strategy_output_lines_=`cat $t_sim_res_filename_`;
	}
	my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
	#print "trades file name ".$this_tradesfilename_."\n";
	if ( ExistsWithSize ( $this_tradesfilename_ ) )
	{
	    my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_";
	    # print $main_log_file_handle_ "$exec_cmd\n";
	    my @pnlstats_output_lines_ = `$exec_cmd`;
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
	}
	my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_.".txt" ;
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
	    my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database_3.pl $t_strat_filename_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir/$base_shortcode_"; # TODO init $local_results_base_dir
	    #print $main_log_file_handle_ "$exec_cmd\n";
	    my $this_local_results_database_file_ = `$exec_cmd`;
	    #push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
	    if ($end_date_ eq ""){  $end_date_ = $tradingdate_; }
	    $start_date_ = $tradingdate_;
	}

	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
    }  
    my $statistics_result_file_ = $work_dir_."/"."stats_res_file_".$iter_count_;
    my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_params_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $sort_algo_ > $statistics_result_file_";
    #print $main_log_file_handle_ "$exec_cmd\n";
    my $results_ = `$exec_cmd`;
    my @count_instances_ = ();
    for ( my $i = 0; $i <= $#$param_vec_vec_ref_ ; $i ++)
    {
	push ( @$stats_vec_ref_ , 0 );
	push ( @$volume_vec_ref_ , 0 );
	push ( @$ttc_vec_ref_ , 0 );
        push ( @$agg_trade_vec_ref_ , 0 );
	push ( @count_instances_, 0 );
    }
    
    if ( $DEBUG ) { $main_log_file_handle_->printf ( "Num days %s results = %d\n%s\n", ( 1 + $#nonzero_tradingdate_vec_ ), join ( ' ', @nonzero_tradingdate_vec_ ) ) ; }
    if ( $#nonzero_tradingdate_vec_ >= 0 )
    { # at least 1 day
	if ( ExistsWithSize ($statistics_result_file_) )
	{
	    #if ( $DEBUG ) { $main_log_file_handle_->printf ( "reading result file %s\n", $statistics_result_file_ ) ; }
	    open(FILE, $statistics_result_file_) or die ("Unable to open statistics result file $statistics_result_file_");
	    my @t_stats_output_lines = <FILE>;
	    close(FILE);
	    for (my $j = 0; $j <= $#t_stats_output_lines ; $j ++ )
	    {
		my @t_stat_rwords_ = split ( ' ', $t_stats_output_lines[$j]);
		my $index = $params_list_map_{$t_stat_rwords_[1]};
		$$stats_vec_ref_[$index]=$t_stat_rwords_[-1];
		$$volume_vec_ref_[$index]=$t_stat_rwords_[4];
		$$ttc_vec_ref_[$index]=$t_stat_rwords_[9];
                $$agg_trade_vec_ref_[$index]=$t_stat_rwords_[14];
	    }
	}
    }
}    
