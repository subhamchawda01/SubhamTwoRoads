#!/usr/bin/perl

# \file ModelScripts/find_best_model_for_strategy_var_pert.pl
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
use feature "switch";
use FileHandle;
use POSIX;
use List::Util qw/max min/; # for max
use File::Basename;
use Term::ANSIColor; 

#use Data::Dumper;
my $SAVE_TRADELOG_FILE=0;
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/get_spare_local_dir_for_aws.pl"; # GetSpareLocalDir

my $SPARE_LOCAL="/spare/local/";
my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{
  $SPARE_LOCAL = GetSpareLocalDir();
}
my $TRADELOG_DIR="/spare/local/logs/tradelogs/";
if ( $USER ne "dvctrader" &&
    $USER ne "sghosh" ) 
{ 
  $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/array_ops.pl"; # GetSum
require "$GENPERLLIB_DIR/get_weighted_sum.pl"; # GetWeightedSum
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl";
require "$GENPERLLIB_DIR/get_bad_days_for_shortcode.pl"; # GetBadDaysForShortcode
require "$GENPERLLIB_DIR/get_very_bad_days_for_shortcode.pl"; # GetVeryBadDaysForShortcode
require "$GENPERLLIB_DIR/get_high_volume_days_for_shortcode.pl"; # GetHighVolumeDaysForShortcode
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllPIDSTerminated



#sub declarations
sub ReadStratFile;
sub FillDateVecs;
sub ReadModelFile;
sub GenerateWeightVecs;
sub CalcPnls;
sub GetBestIndex ;
sub WriteOutModel ;
sub UpdateChangeFractionVec ;
sub CompareVectors;

my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

my $USAGE="$0 input_strat_file output_model last_trading_date num_prev_days change_fraction max_number_iterations [algo=gradalpha3] [D] [sortalgo] [min_volume] [max_ttc] [num_perturbations=2] [retain_more_pert(0|1)=0] [training_period_fraction_=0.7] [training_days_type_(ALL|HV|BD|VBD|ECO)] [eco_date_time_file_=INVALIDFILE] [use_fake_faster_data_]";
if ( $#ARGV < 5 ) { print $USAGE."\n"; exit ( 0 ); }

my $DEBUG = 0;
my $training_period_fraction_ = 0.7;
my $def_allowance_factor_ = 1.2; #if input ttc volume checks are inconsistent with base weights then these weights are applied to new ttc volume factors obtained


my $strat_file_ = $ARGV [ 0 ];
my $output_model_filename_ = $ARGV[1];
my $last_trading_date_ = GetIsoDateFromStrMin1 ( $ARGV[2] ) ;
my $num_days_ = max ( 1, int($ARGV [ 3 ]) );
my $change_ = $ARGV [ 4 ];
my $max_number_of_iterations_ = max ( 1, int($ARGV [ 5 ]) );
my $calc_weights_algo_ = "gradalpha3"; 
if ( $#ARGV >= 6 ) { $calc_weights_algo_ = $ARGV[6]; } # BEST / GRAD

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
my $prod_index_ = 1 ;
my $work_dir_ = $SPARE_LOCAL."$USER/PerturbedModelTests/"; 

my $num_perturbations_ = 2;
my $retain_more_pert_ = 0;
my $training_days_type_ = "ALL";

if ( $#ARGV >= 7 )
{
  if ( index ( $ARGV [ 7 ] , "D" ) != -1 ) { $DEBUG = 1; print "DEBUG MODE\n"; }
}
my $sort_algo_ = "kCNAPnlAverage";
if ( $#ARGV >= 8 )
{
  $sort_algo_ = $ARGV[8];
}
my $use_ttc_bound_ = 0;
my $use_volume_bound_ = 0;
my $volume_min_train_ = 0;
my $ttc_max_train_ = 0;
if ( $#ARGV >=9 )
{
  $use_volume_bound_=1;
  $volume_min_train_=int($ARGV[9]);
}
if ( $#ARGV >=10 )
{
  $use_ttc_bound_=1;
  $ttc_max_train_=int($ARGV[10]);
}

if ( $#ARGV >=11 )
{
  $num_perturbations_=int($ARGV[11]);
}

if ( $#ARGV >=12 )
{
  $retain_more_pert_ = int($ARGV[12]);
}

if ( $#ARGV >=13 )
{
  my $t_training_period_fraction_ = ($ARGV[13])*1;
  if($t_training_period_fraction_>0 && $t_training_period_fraction_<=1)
  {
    $training_period_fraction_ = $t_training_period_fraction_;
  }
}

if( $#ARGV >= 14)
{
  $training_days_type_ = $ARGV[14];
}

my $eco_date_time_file_ = "";
my %eco_date_time_map_;
if( $training_days_type_ eq "ECO")
{
  if($#ARGV >= 15)
  {
    $eco_date_time_file_ = $ARGV[15];
  }
  if(!ExistsWithSize($eco_date_time_file_))
  {
    $training_days_type_ = "ALL";
  }
}

my $use_fake_faster_data_ = rand(1) > 0.5 ? 1 : 0;  #using delays randomly, to set usage use param USE_FAKE_FASTER_DATA 
if( $#ARGV >= 16)
{
  $use_fake_faster_data_ = $ARGV[16];
}

if ( $#ARGV >= 17 )
{
    $prod_index_ = $ARGV[17];
}

my @training_date_vec_ = ();
my @outofsample_date_vec_ = ();
my @sign_vec_ = ();
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my @indicator_list_ = ();

ReadStratFile( );

$work_dir_ = $SPARE_LOCAL."$USER/PerturbedModelTests/"; 
$work_dir_ = $work_dir_."/".$base_shortcode_."/".$unique_gsm_id_;

for ( my $i = 0 ; $i < 30 ; $i ++ )
{
  if ( -d $work_dir_ )
  {
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
    $work_dir_ = $SPARE_LOCAL."$USER/PerturbedModelTests/"."/".$base_shortcode_."/".$unique_gsm_id_;
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
$main_log_file_handle_->print ( "Optimizing $strat_file_ starting at $start_time_\n" );

FillDateVecs ( \@training_date_vec_, \@outofsample_date_vec_, $last_trading_date_, $num_days_ );
if ( $#training_date_vec_ < 0 )
{
  print "No training dates\n";
  exit ( 0 );
}
if ( $#outofsample_date_vec_ < 0 )
{
  print "No outofsample dates\n";
  exit ( 0 );
}
if ( $DEBUG ) 
{ 
  printf ( "Working Directory: %s\n", $work_dir_ ); 
  print ( "Sort algo is $sort_algo_\n" ) ;
  print ( "Calc Weights algo is $calc_weights_algo_\n" ) ;
  $main_log_file_handle_->print ( "Sort algo is $sort_algo_\n" ) ;
  $main_log_file_handle_->print ( "Calc Weights algo is $calc_weights_algo_\n" ) ;
  $main_log_file_handle_->print ( "Training period size: $#training_date_vec_ outsample size :  $#outofsample_date_vec_\n" );
  $main_log_file_handle_->print ( "Training dates : @training_date_vec_ \n" );
  $main_log_file_handle_->print ( "OutofSample dates : @outofsample_date_vec_ \n" ); 
}

my @best_indicator_weight_vec_ = ( );
my $model_type_ = "SIGLR";
my @siglr_alpha_vec_ = ( );
ReadModelFile( );
if ( $DEBUG )
{
  printf ("Sign vector:%s \n", join(' ', @sign_vec_ ) );
}

my @change_fraction_vec_ = ();
for ( my $i = 0; $i <= $#indicator_list_ ; $i ++ )
{
  push ( @change_fraction_vec_, $change_ ) ;
}
my @orig_indicator_weight_vec_ = @best_indicator_weight_vec_ ;

$main_log_file_handle_->printf ( " Date Vectors sizes TR : %d \t OS:%d\n", $#training_date_vec_, $#outofsample_date_vec_ ) ;
my $orig_pnl_ = 0;
my $orig_vol_ = 0;
my $orig_ttc_ = 0;

my $best_seen_pnl_ = 0;
my $best_seen_vol_ = 0;
my $best_seen_ttc_ = 0;
my @best_seen_indicator_weight_vec_ = (); # to store the max pnl seen so far ... due to nonlinearity

my $improvement_made_ = 1;
my $iter_count_ = 1;
my $num_change_factions_significant_ = 1;
my @extra_perturbations_ = ();

#find weight limits
my @indicator_abs_min_weights_ = ();
my @indicator_abs_max_weights_ = ();
CalcWeightLimits( @best_indicator_weight_vec_ );

while ( ( $iter_count_ < $max_number_of_iterations_ ) &&
       ( $improvement_made_ == 1 ) )
{
  $main_log_file_handle_->printf ( "Starting iteration %d\n", $iter_count_ ) ;
  my @indicator_weight_vec_vec_ = GenerateWeightVecs(\@best_indicator_weight_vec_, $iter_count_);
  @indicator_weight_vec_vec_ = (@indicator_weight_vec_vec_, @extra_perturbations_);
  @extra_perturbations_ = ();

  my @stats_vec_ = ( );
  my @volume_vec_ = ( );
  my @ttc_vec_ = ( );
  my @ttc_vol_check_ = ( );
  CalcPnls (\@indicator_weight_vec_vec_, \@stats_vec_, \@volume_vec_, \@ttc_vec_, $iter_count_, \@ttc_vol_check_, "train", \@training_date_vec_);

  if ( $iter_count_ == 1 )
  {
    $orig_pnl_ = $stats_vec_[0];
    $orig_vol_ = $volume_vec_[0];
    $orig_ttc_ = $ttc_vec_[0];

    $best_seen_pnl_ = $stats_vec_[0];
    $best_seen_vol_ = $volume_vec_[0];
    $best_seen_ttc_ = $ttc_vec_[0];

    my $tref_ = $indicator_weight_vec_vec_[0];
    @best_seen_indicator_weight_vec_ = @$tref_ ; # set to best
  }

  if ( 1 ) # $DEBUG ) 
  {
    $main_log_file_handle_->printf ( "No\tStats\tVolume\tTTC\n" );
    for ( my $i = 0 ; $i <= $#stats_vec_ ; $i ++ )
    {
      $main_log_file_handle_->printf ( "%d\t%.2f\t%d\t%d\n", $i, $stats_vec_[$i], $volume_vec_[$i], $ttc_vec_[$i] ) ;
    }
  }
  my $should_check_=1;

  my $best_index_ = GetBestIndex ( \@stats_vec_, \@volume_vec_, \@ttc_vol_check_, 1 );
  $main_log_file_handle_->printf ("Best index is %d\n", $best_index_);

# check if we had taken a bad step last time - best index < 0 means everything failed ttc or volume check
  if ( $best_seen_pnl_ >= $stats_vec_[$best_index_] || $best_index_ < 0 )
  { # last time we had seen a best that is better than this time
    if($best_index_ < 0)    
    {
      $main_log_file_handle_->printf ("Reset weights. No model could pass ttc or volume check\n");
    }
    else
    {
      $main_log_file_handle_->printf ("Reset weights. We had taken a bad step current best pnl %d < old best pnl %d\n", $stats_vec_[$best_index_], $best_seen_pnl_) ;
    }
    my $tref_ = $indicator_weight_vec_vec_[0];
    @best_indicator_weight_vec_ = @best_seen_indicator_weight_vec_ ;

# make all changes half # $change_ = $change_/2.0;
    for ( my $cfidx_ = 0 ; $cfidx_ <= $#change_fraction_vec_ ; $cfidx_ ++ )
    {
      $change_fraction_vec_[$cfidx_] /= 2.0 ; 
    }
    $iter_count_++;

    if ( -e $work_dir_."/exit_file" )
    { last; }
    next; # continue hoping our alpha was too high
  }
  else
  {
    $best_seen_pnl_ = $stats_vec_[$best_index_];
    $best_seen_vol_ = $volume_vec_[$best_index_];
    $best_seen_ttc_ = $ttc_vec_[$best_index_];

    my $tref_ = $indicator_weight_vec_vec_[$best_index_];
    @best_seen_indicator_weight_vec_ = @$tref_ ; # set to best
  }
#    my $vec_1 = $indicator_weight_vec_vec_[$best_index_];
#    my $vec_2 = $indicator_weight_vec_vec_[0];

#for instances where the 0th index is different from best ( i.e. apart from best1 )
# even if index is 0 and improvement is made we should not exit
  if ( ($best_index_ > 0) && ( $best_index_ <= $#indicator_weight_vec_vec_ ) )
  {  # if best index isn't 0
    $improvement_made_ = 1;

    if ( -e $work_dir_."/exit_file" )
    { last; }

    @best_indicator_weight_vec_ = CalcNextWeights ( \@indicator_weight_vec_vec_, \@stats_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $best_index_, $calc_weights_algo_, 0 );
    if($retain_more_pert_ == 1)
    {
      my $extra_pert_algo_ = $calc_weights_algo_;
      if(index($calc_weights_algo_, "grad") == 1)    
      {
# grad_type 0 is already there, add bestalpha10 to extra perturbations
        my @t_extra_pert_vec_ = CalcNextWeights ( \@indicator_weight_vec_vec_, \@stats_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $best_index_, "bestalpha10", 0 );
        push(@extra_perturbations_, \@t_extra_pert_vec_);        
      }
      else
      {
#we can't use the same search algo, need a grad algo
        $extra_pert_algo_ = "gradalpha3";
        my @t_extra_pert_vec_ = CalcNextWeights ( \@indicator_weight_vec_vec_, \@stats_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $best_index_, $extra_pert_algo_, 0 );
        push(@extra_perturbations_, \@t_extra_pert_vec_);
      }
      my @t_extra_pert_vec_ = CalcNextWeights ( \@indicator_weight_vec_vec_, \@stats_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $best_index_, $extra_pert_algo_, 1 );
      push(@extra_perturbations_, \@t_extra_pert_vec_);
      @t_extra_pert_vec_ = CalcNextWeights ( \@indicator_weight_vec_vec_, \@stats_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $best_index_, $extra_pert_algo_, 2 );
      push(@extra_perturbations_, \@t_extra_pert_vec_);
    }


    $main_log_file_handle_->printf ( "oldpnl_volume_ttc %d %d %d maxpnl_volume_ttc %d %d %d bestpnl_volume_ttc %d %d %d\n", int($stats_vec_[0]), int($volume_vec_[0]), int($ttc_vec_[0]), int($stats_vec_[$best_index_]), int($volume_vec_[$best_index_]), int($ttc_vec_[$best_index_]), int($best_seen_pnl_), int($best_seen_vol_), int($best_seen_ttc_)) ;
    if ( $DEBUG ) { $main_log_file_handle_->printf ( "new weights: %s\n", join(' ',@best_indicator_weight_vec_)) ;  }
    $iter_count_ ++;
  }
  else
  {
    if ( -e $work_dir_."/exit_file" )
    { last; }

    $improvement_made_ = 0; # only breaking on weight insignificance
        $main_log_file_handle_->printf ( "No improvement made at iteration %d\n", $iter_count_ ) ;
    next;
  }

  my $significance_thresh_ = 0.10 ;
  $num_change_factions_significant_ = UpdateChangeFractionVec ( \@stats_vec_, \@volume_vec_, ( 1 + $#orig_indicator_weight_vec_ ), $significance_thresh_, \@indicator_weight_vec_vec_ );
  if ( $num_change_factions_significant_ <= 0 )
  { # break out of loop if nothing significant
    $main_log_file_handle_->printf ( "Breaking out of loop since num of significant (%f) change fractions = 0\n", $significance_thresh_ );
    last;
  }    
}

# Print orig weight and final weight
printf ( "OrigOnTraining %d %d %d\n", $orig_pnl_, $orig_vol_, $orig_ttc_ );
$main_log_file_handle_->printf ( "OrigOnTraining %d %d %d\n", $orig_pnl_, $orig_vol_, $orig_ttc_ );
printf ( "BestOnTraining %d %d %d\n", $best_seen_pnl_, $best_seen_vol_, $best_seen_ttc_ );
$main_log_file_handle_->printf ( "BestOnTraining %d %d %d\n", $best_seen_pnl_, $best_seen_vol_, $best_seen_ttc_ );

WriteOutModel ( \@best_seen_indicator_weight_vec_ );
my @stats_vec_crossval_ = ( );
my @volume_vec_crossval_ = ( );
my @ttc_vec_crossval_ = ( );
my @ttc_vol_check_crossval_ = ( );
my @indicator_weight_vec_vec_crossval_ = ( );
push (@indicator_weight_vec_vec_crossval_, \@orig_indicator_weight_vec_);
push (@indicator_weight_vec_vec_crossval_, \@best_seen_indicator_weight_vec_);
my $stats_string_ = CalcPnls (\@indicator_weight_vec_vec_crossval_, \@stats_vec_crossval_, \@volume_vec_crossval_, \@ttc_vec_crossval_, $iter_count_, \@ttc_vol_check_crossval_, "outofsample", \@outofsample_date_vec_);
printf ("OrigOnOutofsample %d %d %d\n",$stats_vec_crossval_[0], $volume_vec_crossval_[0], $ttc_vec_crossval_[0]);
$main_log_file_handle_->printf ("OrigOnOutofsample %d %d %d\n",$stats_vec_crossval_[0], $volume_vec_crossval_[0], $ttc_vec_crossval_[0]);
printf ("BestOnOutofsample %d %d %d\n",$stats_vec_crossval_[1], $volume_vec_crossval_[1], $ttc_vec_crossval_[1]);
$main_log_file_handle_->printf ("BestOnOutofsample %d %d %d\n",$stats_vec_crossval_[1], $volume_vec_crossval_[1], $ttc_vec_crossval_[1]);
printf ("OutsamplePerformaceSummary\n%s\n", $stats_string_);
$main_log_file_handle_->printf ("OutsamplePerformaceSummary\n%s\n", $stats_string_);

my $end_time_ =`date +%s`; chomp ( $end_time_ );
$main_log_file_handle_->print ( "Wrote optimized $output_model_filename_ at end time $end_time_\n" );
$main_log_file_handle_->close;

exit (0);

# Read the strategy file to get the model file, param file and other parameters
sub ReadStratFile
{
  if ( $DEBUG ) 
  { 
    print ( "ReadStratFile\n" ) ; 
  }

  open STRAT_FILE, "< $strat_file_" or PrintStacktraceAndDie ( "Could not open strategy file $strat_file_ for reading\n" );
  my $strat_line_ = <STRAT_FILE>;
  my @strat_words_ = split ' ', $strat_line_;
  my $im_strat_ = $strat_words_[1];
  open IM_STRAT_FILE, "< $im_strat_ " or PrintStackTraceAndDie ( " COuld not open im strategy $im_strat_ for reading\n" );
  my @im_strat_words_ = <IM_STRAT_FILE> ;
  my $index_ = 0 ;
  
  while ( $index_ <= $#im_strat_words_ )
  {
      print  " $index_ $prod_index_ $im_strat_words_[$index_ ] ";
      if ( $index_ == 0 )
      {
          my @line_words_ = split ( " ", $im_strat_words_[$index_] ) ; chomp ( @line_words_ ) ;
          $base_shortcode_ = $line_words_[1];
          $base_pbat_dat_ = $line_words_[2];
          $base_start_time_ = $line_words_[4];
          $base_end_time_= $line_words_[5];
          $base_prog_id_ = $line_words_[6];
          print "LINE0 $base_shortcode_ $base_pbat_dat_ $base_start_time_ $base_end_time_ $base_prog_id_";
      }
      
      if ( $index_ == $prod_index_ ) 
      {
          my @line_words_ = split ( " ", $im_strat_words_[$index_] ) ;
          $strat_pre_text_ = $strat_pre_text_.$line_words_[0]." ".$line_words_[1];
          $strat_post_text_ = $line_words_[3]."\n";
          $base_model_file_ = $line_words_[2];
          $base_param_file_ = $line_words_[3];
          print "MODELPARAM: $base_model_file_\n\n $base_param_file_ \n";
      }
      elsif ( $index_ < $prod_index_ ) 
      {
          $strat_pre_text_ = $strat_pre_text_.$im_strat_words_[$index_];
      }
      else
      {
          $strat_post_text_ = $strat_post_text_.$im_strat_words_[$index_];
      }
      $index_++;
  }
  
  return;
}

# Read the strategy file to get the model file, param file and other parameters
sub ReadModelFile
{
#please make sure the model file has scores with '#', right now there is no check for that
  if ( $DEBUG )
  {
    $main_log_file_handle_->print ( "ReadModelFile\n" );
  }

  print ( "ModelFile : $base_model_file_\n" ) ;
  $main_log_file_handle_->print ( "ModelFile : $base_model_file_\n" ) ;

  open MODEL_FILE, "< $base_model_file_" or PrintStacktraceAndDie ( "Could not open model file $base_model_file_ for reading\n" );

  my $indicator_start_reaced_ = 0;
  while ( my $model_line_ = <MODEL_FILE> )
  {
    chomp($model_line_);

    my @model_words_ = split ' ', $model_line_;

    if( not $indicator_start_reaced_ )
    {
      $base_model_start_text_ = $base_model_start_text_.$model_line_."\n";
      if($model_words_[0] eq "INDICATORSTART")
      {
        $indicator_start_reaced_ = 1;
      }
    }

    if ($model_words_[0] eq "MODELMATH")
    {
      $model_type_ = $model_words_[1];
    }

    if($model_words_[0] eq "INDICATOR")
    {
      my @i_words_ = @model_words_;
      shift(@i_words_); shift(@i_words_);
#	pop(@i_words_);pop(@i_words_);
      my $t_indicator_name_ = join(' ',@i_words_);

      push( @indicator_list_, $t_indicator_name_);

      if ( $model_type_ eq "SIGLR" )
      {
        my @t_siglr_words_ = split ':', $model_words_[1];
        push( @siglr_alpha_vec_, $t_siglr_words_[0] );
        push( @best_indicator_weight_vec_, $t_siglr_words_[1] );
        if ( ( $t_siglr_words_[0] + 0 ) * ( $t_siglr_words_[1] + 0 ) >= 0 ) { push ( @sign_vec_, 1 ); }
        else { push (@sign_vec_, -1); }
      }
      else
      {
        push( @best_indicator_weight_vec_, $model_words_[1] + 0 );
        if ( $model_words_[1] + 0 >=0 ) { push (@sign_vec_ , 1 ); }
        else { push (@sign_vec_, -1); }
      }

    }
  }
  return;
}

sub WriteOutModel
{
  if ( $DEBUG )
  {
    $main_log_file_handle_->print ( "WriteOutModel\n" );
  }
  my ( $tref_ ) = @_;

  my $indicator_index_ = 0;
  open OUTMODEL, "> $output_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $output_model_filename_ for writing\n" );
  open MODEL_FILE, "< $base_model_file_" or PrintStacktraceAndDie ( "Could not open model file $base_model_file_ for reading\n" );
  while ( my $model_line_ = <MODEL_FILE> )
  {
    chomp($model_line_);
    my @model_words_ = split ' ', $model_line_;

    if ( ( $#model_words_ >= 2 ) && 
        ( $model_words_[0] eq "INDICATOR" ) )
    {
      $model_words_[1] = ( $$tref_[$indicator_index_] + 0 );
      if ( $model_type_ eq "SIGLR" )
      {
        $model_words_[1] = join ( ':', $siglr_alpha_vec_[$indicator_index_], $model_words_[1] );
      }
      printf OUTMODEL "%s\n", join ( ' ', @model_words_ ) ;
      $indicator_index_ ++;
    }
    else
    {
      printf OUTMODEL "%s\n", $model_line_ ;
    }
  }

  close OUTMODEL;
}

sub FillDateVecs
{
  @_ == 4 or die "FillDateVec called with !=4 args\n";
  my ( $training_date_vec_ref_, $outsample_date_vec_ref_, $t_last_trading_date_, $t_num_days_ ) = @_;
  if ( $DEBUG )
  {
    $main_log_file_handle_->print  ("FillDateVec $t_last_trading_date_ $t_num_days_\n" );
  }

  if($training_days_type_ eq "ECO")
  {
    open ECO_DATE_TIME_FILE, "< $eco_date_time_file_" or PrintStacktraceAndDie("can't open eco_date_time_file_ $eco_date_time_file_ for reading.\n");
    while (my $line_ = <ECO_DATE_TIME_FILE>)
    {
      chomp($line_);
      my @words_ = split ' ', $line_;
      if($#words_ >= 2)
      {
        push ( @$training_date_vec_ref_, $words_[0]);
        push ( @$outsample_date_vec_ref_, $words_[0]);	#for ECO days, outsample is same as insample, juts to see performance in mail....outsample don't make much sense in case of ECO
            $eco_date_time_map_{$words_[0]} = [ ( $words_[1], $words_[2] ) ];
      }
    }
    close ECO_DATE_TIME_FILE;
    return;
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
#with probability $training_period_fraction_ push in training set else in outsample set
      if ( grep {$_ eq $tradingdate_} @special_day_vec_ )
      {
        if ( rand( ) <= $training_period_fraction_ )
        {
          push ( @$training_date_vec_ref_, $tradingdate_ ); 
        }
        else
        {
          push ( @$outsample_date_vec_ref_, $tradingdate_ );
        }            
      }        
      $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }
    if($#$training_date_vec_ref_ >= min(15, $t_num_days_/2))    #20 days for training are fine, if smaller span is given, then even less days are ok
    {
      if ( $training_period_fraction_ >= 1 )
      {
#degenrating this to every day being insample as well as outsample for comparision
        @$outsample_date_vec_ref_ = @$training_date_vec_ref_;
      }  
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
#with probability $training_period_fraction_ push in training set else in outsample set
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

  if ( $training_period_fraction_ >= 1 )     
  {        
#degenrating this to every day being insample as well as outsample for comparision
    @$outsample_date_vec_ref_ = @$training_date_vec_ref_;
  }
}

sub GenerateWeightVecs
{
  my ( $indicator_to_weight_base_vec_ref_, $iter_count_ ) = @_;

  my @indicator_weight_vec_vec_ = ();

  my @this_indicator_weight_vec_ = @$indicator_to_weight_base_vec_ref_ ;
  push ( @indicator_weight_vec_vec_, \@this_indicator_weight_vec_ );
  $main_log_file_handle_->printf ( "GenerateWeightVecs from \n%s\n", join(' ',@this_indicator_weight_vec_ ) ) ;

  my $max_pert_factor_ = int($num_perturbations_/2);
  for (my $j = 0; $j <= $#indicator_list_; $j++)
  {
    if ( $this_indicator_weight_vec_ [$j] != 0 )
    {
# my $zero_included_ = 0;
      my $max_included_ = 0;
      my $min_included_ = 0;
      for(my $k = -1*$max_pert_factor_; $k<=$max_pert_factor_; $k++)
      {
        if($k==0)    {next;}
        {
          my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
          $t_indicator_weight_vec_[$j] = ( 1 + $k*$change_fraction_vec_[$j] ) * $this_indicator_weight_vec_ [$j];
          if ( (abs( $t_indicator_weight_vec_[$j] ) < $indicator_abs_min_weights_[$j] ) || ($sign_vec_[$j] * $t_indicator_weight_vec_[$j] <= 0))
          {
            if($min_included_ == 1) {next;}
            $t_indicator_weight_vec_[$j] = $sign_vec_[$j] * $indicator_abs_min_weights_[$j];
            $min_included_ = 1;
          }
          if ( abs( $t_indicator_weight_vec_[$j] ) >  $indicator_abs_max_weights_[$j] ) 
          {
            if($max_included_ == 1) {next;}
            $t_indicator_weight_vec_[$j] = $sign_vec_[$j] * $indicator_abs_max_weights_[$j];
            $max_included_ = 1;
          }
# if ( $sign_vec_[$j] * $t_indicator_weight_vec_[$j] <= 0 )
# {
# if($zero_included_ == 1)    {next;}
# $t_indicator_weight_vec_[$j] = 0;
# $zero_included_ = 1;
# }
          {
            push ( @indicator_weight_vec_vec_, \@t_indicator_weight_vec_ );
            if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
          }
        }
      }
# if(($num_perturbations_%2 == 1) && ($zero_included_ == 0))
# {
# my @t_indicator_weight_vec_ = @this_indicator_weight_vec_ ;
# $t_indicator_weight_vec_[$j] = 0;
# push ( @indicator_weight_vec_vec_, \@t_indicator_weight_vec_ );
# if ( $DEBUG ) { $main_log_file_handle_->printf ( "%s\n", join(' ',@t_indicator_weight_vec_ ) ) } ;
# }
    }
  }
  return @indicator_weight_vec_vec_ ;
}

sub CalcPnls
{
  if ( $DEBUG )
  {
    $main_log_file_handle_->print ( "CalcPnls\n" );
  }
  my ( $indicator_weight_vec_vec_ref_, $stats_vec_ref_, $volume_vec_ref_, $ttc_vec_ref_, $iter_count_, $ttc_volume_check_ref_, $var_, $date_vec_ref_ ) = @_;
  my @date_vec_= ();
  my %models_list_map_ = ( );
  my $t_strat_filename_ = $work_dir_."/"."tmp_strat_".$iter_count_."_".$var_;
  
  my $t_models_list_ = $work_dir_."/"."tmp_models_list_".$iter_count_."_".$var_;
  my $local_results_base_dir = $work_dir_."/local_results_base_dir".$iter_count_."_".$var_;
  open OUTSTRAT, "> $t_strat_filename_" or PrintStacktraceAndDie ( "Could not open output_strategy_filename_ $t_strat_filename_ for writing\n" );
  open OUTMODELLIST, "> $t_models_list_" or PrintStacktraceAndDie ( "Could not open output_models_list_filename_ $t_models_list_ for writing\n" );
  for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
  {
    my $t_model_filename_base_ = "tmp_model_".$iter_count_."_".$i."_".$var_;
    my $t_im_strat_filename_ = $work_dir_."/"."tmp_im_strat_".$iter_count_."_".$i."_".$var_;
    if($var_ eq "outofsample")
    {
      if($i==0)
      {
        $t_model_filename_base_ = "original_model"; #assuming 1st weight vec is original in outsample call
      }
      if($i==1)
      {
        $t_model_filename_base_ = "best_model"; #assuming 2nd weight vec is best in outsample call
      }
    }
    my $t_model_filename_ = $work_dir_."/".$t_model_filename_base_;
    my $t_im_strat_text_ = $strat_pre_text_.$t_model_filename_.$strat_post_text_."\n"; # TODO not changing progid ?
    my $t_output_ = $base_model_start_text_;
    my $tref_ = $$indicator_weight_vec_vec_ref_[$i];

    for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
    {
      if ( $model_type_ eq "SIGLR" )
      {
        $t_output_ = $t_output_."INDICATOR ".$siglr_alpha_vec_[$j].":".$$tref_[$j]." ".$indicator_list_[$j]."\n";
      }
      else
      {
        $t_output_ = $t_output_."INDICATOR ".$$tref_[$j]." ".$indicator_list_[$j]."\n";
      }
    }

    $t_output_ = $t_output_."INDICATOREND\n";

    open OUTMODEL, "> $t_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_ for writing\n" );
    print OUTMODEL $t_output_;
    close OUTMODEL;
    $t_im_strat_text_ = $strat_pre_text_." ".$t_model_filename_." ".$strat_post_text_."\n";
    
    open OUTIMSTRAT, "> $t_im_strat_filename_" or PrintStackTraceAndDie( "Could not open output strategy_filename_ $t_strat_filename_ for writing\n" );
    print OUTIMSTRAT $t_im_strat_text_;
    close OUTIMSTRAT;
    print OUTSTRAT "STRUCTURED_STRATEGYLINE $t_im_strat_filename_ $base_prog_id_\n";
    print OUTMODELLIST $t_model_filename_base_."\n";
    $models_list_map_{$t_model_filename_base_}=$i;
  }
  close OUTSTRAT;
  close OUTMODELLIST;

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

    my $t_sim_res_filename_ = $work_dir_."/"."sim_res_".$iter_count_."_".$tradingdate_."_".$var_;
    push ( @temp_strategy_output_file_list_ , $t_sim_res_filename_ );
    `> $t_sim_res_filename_`;

    my $this_strat_filename_ = $t_strat_filename_;
    if($training_days_type_ eq "ECO")
    {
#changing timings as per the event
      $this_strat_filename_ = $t_strat_filename_."_".$tradingdate_;
      my $start_time_ = $eco_date_time_map_{$tradingdate_}[0];
      my $end_time_ = $eco_date_time_map_{$tradingdate_}[1];
      `awk -vst=$start_time_ -vet=$end_time_ '{\$6=st; \$7=et; print \$_}' $t_strat_filename_ >  $this_strat_filename_`;
    }

    my $market_model_index_ = GetMarketModelForShortcode ( $base_shortcode_ );
    my $exec_cmd_ = "$LIVE_BIN_DIR/sim_strategy SIM $this_strat_filename_ $unique_sim_id_ $tradingdate_ $market_model_index_ 0 0.0 $use_fake_faster_data_ ADD_DBG_CODE -1 > $t_sim_res_filename_ 2>/dev/null";
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

      my $pid_ = fork();
      die "unable to fork $!" unless defined($pid_);
      if ( !$pid_ )
      {
        #child process has pid 0
        exec($exec_cmd_);
      }

      #back to parent process
      print $main_log_file_handle_ "PID of last sim_strategy is $pid_\n";
      $command_index_ ++;
      sleep ( 1 );
    }

    my $t_pid_ = 9999;
    while ( $t_pid_ > 0 )
    { # there are still some sim-strats which haven't output SIMRESULT lines
      $t_pid_ = wait();
      print $main_log_file_handle_ "PID of completed process: $t_pid_\n";
    }
  }

  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
  {
#print "iter_count_ ".$iter_count_." command_index_ ".$command_index_."\n";

    my $t_sim_res_filename_ = $temp_strategy_output_file_list_ [ $command_index_ ];
    my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];
    my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];

    my @sim_strategy_output_lines_=();
    my %unique_id_to_pnlstats_map_ =();

    if ( ExistsWithSize ( $t_sim_res_filename_ ) )
    {	
      push (@nonzero_tradingdate_vec_, $tradingdate_);
      @sim_strategy_output_lines_=`cat $t_sim_res_filename_`;
    }
    my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_sim_id_);
    my $this_logfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int($unique_sim_id_);
#print "trades file name ".$this_tradesfilename_."\n";
    if ( ExistsWithSize ( $this_tradesfilename_ ) )
    {
      my $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_stir_2.pl $this_tradesfilename_";
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
      `rm -f $this_tradesfilename_`;
      `rm -f $this_logfilename_`;
    }
    my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_."_".$var_.".txt" ;
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
              $unique_id_to_pnlstats_map_{$unique_sim_id_} = "0 0 0 0 0 0 0 0 0 0 0 0 0";
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
      if ($start_date_ eq ""){  $start_date_ = $tradingdate_; }
      if($tradingdate_ > $end_date_){ $end_date_ = $tradingdate_; }
      if($tradingdate_ < $start_date_){ $start_date_ = $tradingdate_; }
    }
  }

  my $statistics_result_file_ = $work_dir_."/"."stats_res_file_".$iter_count_."_".$var_;
  my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $sort_algo_ > $statistics_result_file_";
#print $main_log_file_handle_ "$exec_cmd\n";
  my $results_ = `$exec_cmd`;
  $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $sort_algo_ 0 INVALIDFILE 0";
  my $stats_string_ = `$exec_cmd`;
  my @count_instances_ = ();
  for ( my $i = 0; $i <= $#$indicator_weight_vec_vec_ref_ ; $i ++)
  {
    push ( @$stats_vec_ref_ , 0 );
    push ( @$volume_vec_ref_ , 0 );
    push ( @$ttc_vec_ref_ , 0 );
    push ( @count_instances_, 0 );
    push ( @$ttc_volume_check_ref_, 1 );
  }

  if ( $DEBUG ) { $main_log_file_handle_->printf ( "Num days %s results = %d\n%s\n", $var_,( 1 + $#nonzero_tradingdate_vec_ ), join ( ' ', @nonzero_tradingdate_vec_ ) ) ; }
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
        my $index = $models_list_map_{$t_stat_rwords_[1]};
        $$stats_vec_ref_[$index]=$t_stat_rwords_[-1];
        $$volume_vec_ref_[$index]=$t_stat_rwords_[4];
        $$ttc_vec_ref_[$index]=$t_stat_rwords_[9];
      }
      for (my $j = 0; $j <= $#t_stats_output_lines ; $j ++ )
      {
        if ( $j==0 && $iter_count_== 1 )
        {
          if ($var_ eq "train")
          {
            if ( $use_ttc_bound_== 1 && $ttc_max_train_<$$ttc_vec_ref_[$j]) { $ttc_max_train_=$$ttc_vec_ref_[$j] * $def_allowance_factor_; }
            if ( $use_volume_bound_==1 && $volume_min_train_>$$volume_vec_ref_[$j]) { $volume_min_train_=$$volume_vec_ref_[$j] / $def_allowance_factor_; }                      
            if ( $DEBUG ) { $main_log_file_handle_->printf ("Final volume and ttc bounds for %s are %d %d\n",$var_, $volume_min_train_, $ttc_max_train_); }
          }
        }
        if ( $use_ttc_bound_== 1 && $var_ eq "train" )
        {
          if ( $$ttc_vec_ref_[$j] >= $ttc_max_train_ )
          {
            $$ttc_volume_check_ref_[$j] = 0;
          }
        }
        if ( $use_volume_bound_== 1 && $$ttc_volume_check_ref_[$j]== 1 && $var_ eq "train")
        {
          if ( $$volume_vec_ref_[$j] <= $volume_min_train_ )
          {
            $$ttc_volume_check_ref_[$j] = 0;
          }
        }
      }

    }
  }
  return $stats_string_;

}

sub GetBestIndex
{
  my $BIG_NEG_NUMBER = -10000000;
  my ( $pnl_vec_ref_, $volume_vec_ref_, $ttc_volume_check_ref_, $should_check_ ) = @_;
  my $best_index_ = -1;
  my @sorting_value_vec_ = ();
  for ( my $i = 0 ; $i <= $#$pnl_vec_ref_ ; $i ++ )
  {
    push ( @sorting_value_vec_, $$pnl_vec_ref_[$i] );
#printf( "index %d check_pass %d\n", $i, $$ttc_volume_check_ref_[$i] );
  }
  my $max_val_ = $BIG_NEG_NUMBER;
  for ( my $i = 0 ; $i <= $#sorting_value_vec_ ; $i ++ )
  {
    if ( ( $sorting_value_vec_[$i] > $max_val_ ) &&
        (( $$ttc_volume_check_ref_[$i]==1 && $should_check_==1 ) || $should_check_==0) )
    {
      $max_val_ = $sorting_value_vec_[$i];
      $best_index_ = $i;
    }
  }
  return $best_index_;
}

sub UpdateChangeFractionVec
{
  my ( $pnl_vec_ref_, $volume_vec_ref_, $num_indicators_, $significance_thresh_, $indicator_weight_vec_vec_ ) = @_;

  my $t_num_change_factions_significant_ = 0;

# sanity checks
#~ if ( ( $#$pnl_vec_ref_ != $#$volume_vec_ref_ ) ||
#~ ( $#$pnl_vec_ref_ < ( $num_perturbations_ * $num_indicators_ ) ) )
#~ {
#~ return ;
#~ }

  my $base_score_ = $$pnl_vec_ref_[0];
  my $base_model_ = $$indicator_weight_vec_vec_[0];
  for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
  {
    my $not_improved_over_base_ = 1;
    my $max_change_over_base_ = -1;
    my @pert_score_vec_ = ();
    for(my $j = 1; $j <= $#$indicator_weight_vec_vec_; $j++)
    {
      my $this_model_ = $$indicator_weight_vec_vec_[$j];
      if($$this_model_[$i] == $$base_model_[$i])  {next;}
      my $pert_score_ = $$pnl_vec_ref_[$j];
      push(@pert_score_vec_, $pert_score_);
      $not_improved_over_base_ &= ($base_score_ >= $pert_score_-1);
      $max_change_over_base_ = max($max_change_over_base_, abs ( $base_score_ - $pert_score_ ));
    }
    if ( $not_improved_over_base_ )
    {
      if ( $DEBUG ) { $main_log_file_handle_->printf ( "Halving CF ( %d ) = %f Scores : %d %s\n", (1+$i),$change_fraction_vec_[$i], $base_score_, join(' ', @pert_score_vec_) ); }
      $change_fraction_vec_[$i] /= 2.0;
    }
    if ( ( $change_fraction_vec_[$i] > $significance_thresh_ ) &&
        ( $max_change_over_base_ >= ( 0.05 * abs ( $base_score_ ) ) ) ) # at least 5 percent PNL difference
    {
      if ( $DEBUG ) { $main_log_file_handle_->printf ( "CF ( %d ) = %f still significant. Scores : %d %s\n", (1+$i), $change_fraction_vec_[$i], $base_score_, join(' ', @pert_score_vec_) ); }
      $t_num_change_factions_significant_ ++;
    }
  }
  return $t_num_change_factions_significant_;
}

sub CalcNextWeights
{
  my @t_best_indicator_weight_vec_ = ();
  my ( $indicator_weight_vec_vec_ref_, $pnl_vec_ref_, $volume_vec_ref_, $num_indicators_, $best_index_, $calc_weights_algo_, $grad_type_ ) = @_;
# $grad_type_ - used only with grad algos
# 0 - towards max model for that indicator
# 1 - towards only +ve models for that indicator
# 2 - towards both +ve & -ve models for that indicator
  my $alpha_ = 0.5;

  if ( ( $calc_weights_algo_ eq "grad10" ) ||
      ( $calc_weights_algo_ eq "grad5" ) ||
      ( $calc_weights_algo_ eq "grad2" ) ||
      ( $calc_weights_algo_ eq "grad1" ) ||
      ( $calc_weights_algo_ eq "grad0.5" ) ||
      ( $calc_weights_algo_ eq "grad0.25" ) ||
      ( $calc_weights_algo_ eq "gradalpha3" ) ||
      ( $calc_weights_algo_ eq "gradalpha10" ) )
  {
    if ( $calc_weights_algo_ eq "grad10" ) 
    { $alpha_ = 10; }
    if ( $calc_weights_algo_ eq "grad5" ) 
    { $alpha_ = 5; }
    if ( $calc_weights_algo_ eq "grad2" ) 
    { $alpha_ = 2; }
    if ( $calc_weights_algo_ eq "grad1" ) 
    { $alpha_ = 1; }
    if ( $calc_weights_algo_ eq "grad0.5" ) 
    { $alpha_ = 0.5; }
    if ( $calc_weights_algo_ eq "grad0.25" ) 
    { $alpha_ = 0.25; }
    if ( $calc_weights_algo_ eq "gradalpha3" ) 
    { $alpha_ = ( ( $iter_count_ + 3 ) / ( $iter_count_ ) ) ; }
    if ( $calc_weights_algo_ eq "gradalpha10" ) 
    { $alpha_ = ( ( $iter_count_ + 10 ) / ( $iter_count_ ) ) ; }

    if ( $DEBUG ) 
    { 
      $main_log_file_handle_->print ( "$calc_weights_algo_ $alpha_ $iter_count_ $grad_type_\n" );
    }

# sanity checks
    if ( $#$pnl_vec_ref_ != $#$indicator_weight_vec_vec_ref_ )
    {
      my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
      @t_best_indicator_weight_vec_ = @$tref_ ; # set to best
          return @t_best_indicator_weight_vec_ ;
    }

    my @delta_vec = ();
    for ( my $i = 0 ; $i <= $#$pnl_vec_ref_ ; $i ++ ) 
    {
      push ( @delta_vec, ( $$pnl_vec_ref_[$i] - $$pnl_vec_ref_[0] ) ) ;
    }

    my $base_model = $$indicator_weight_vec_vec_ref_[0];	
    @t_best_indicator_weight_vec_ = @$base_model ;
    my $is_changed = 0;
    if($grad_type_ == 0)
    {
      for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
      {
        my $indicator_weight_diff = 0;
        my $delta_max = 0;
        for ( my $j = 1 ; $j <= $#$indicator_weight_vec_vec_ref_ ; $j ++ )
        {
          my $this_model = $$indicator_weight_vec_vec_ref_[$j];
          if ( ( $delta_vec[$j] > $delta_max ) && ( $$this_model[$i] != $$base_model[$i] ) ) 
          {
# if this score was best so far and the weight of this indicator was different than base
            $indicator_weight_diff = ( $$this_model[$i] - $$base_model[$i] ) ;
            $delta_max = $delta_vec[$j] ;
          }
        }
        if ( $delta_max > 0 ) 
        {
          $t_best_indicator_weight_vec_[$i] += $alpha_ * $indicator_weight_diff ;
        }
      }
    }

    if($grad_type_ == 1)
    {
      for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
      {
        my $sum_weight_diff = 0;
        my $sum_weight = 0;
        for ( my $j = 1 ; $j <= $#$indicator_weight_vec_vec_ref_ ; $j ++ )
        {
          my $this_model = $$indicator_weight_vec_vec_ref_[$j];
          if ( ( $delta_vec[$j] > 0 ) && ( $$this_model[$i] != $$base_model[$i] ) ) 
          {
# if this score was best so far and the weight of this indicator was different than base
            $sum_weight_diff += $delta_vec[$j] * ( $$this_model[$i] - $$base_model[$i] ) ;
            $sum_weight += $delta_vec[$j] ;
          }
        }
        if ( $sum_weight > 0 ) 
        {
          $t_best_indicator_weight_vec_[$i] += $alpha_ * ($sum_weight_diff/$sum_weight) ;
        }
      }
    }

    if($grad_type_ == 2)
    {
      for ( my $i = 0 ; $i < $num_indicators_; $i ++ )
      {
        my $sum_weight_diff = 0;
        my $sum_weight = 0;
        for ( my $j = 1 ; $j <= $#$indicator_weight_vec_vec_ref_ ; $j ++ )
        {
          my $this_model = $$indicator_weight_vec_vec_ref_[$j];
          if ( $$this_model[$i] != $$base_model[$i] ) 
          {
# if this score was best so far and the weight of this indicator was different than base
            $sum_weight_diff += $delta_vec[$j] * ( $$this_model[$i] - $$base_model[$i] ) ;
            $sum_weight += $delta_vec[$j] ;
          }
        }
        if ( $sum_weight > 0 ) 
        {
          $t_best_indicator_weight_vec_[$i] += $alpha_ * ($sum_weight_diff/$sum_weight) ;
        }
      }
    }

    if ( CompareVectors(\@t_best_indicator_weight_vec_, \@$base_model) == 0 )
    {   # somehow no change : all gradients are <= 0 .. should not happen since best_index_ is not 0, but anyway
      my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
      @t_best_indicator_weight_vec_ = @$tref_ ; # set it to the best vector and return
          $main_log_file_handle_->print ( "Somehow no change : Returning best model\n" );
    }
  }
  if ( ( $calc_weights_algo_ eq "best10" ) ||
      ( $calc_weights_algo_ eq "best5" ) ||
      ( $calc_weights_algo_ eq "best2" ) ||
      ( $calc_weights_algo_ eq "best1" ) ||
      ( $calc_weights_algo_ eq "best0.5" ) ||
      ( $calc_weights_algo_ eq "best0.25" ) ||
      ( $calc_weights_algo_ eq "bestalpha10" ) )
  {
    if ( $calc_weights_algo_ eq "best10" ) 
    { $alpha_ = 10; }
    if ( $calc_weights_algo_ eq "best5" ) 
    { $alpha_ = 5; }
    if ( $calc_weights_algo_ eq "best2" ) 
    { $alpha_ = 2; }
    if ( $calc_weights_algo_ eq "best1" ) 
    { $alpha_ = 1; }
    if ( $calc_weights_algo_ eq "best0.5" ) 
    { $alpha_ = 0.5; }
    if ( $calc_weights_algo_ eq "best0.25" ) 
    { $alpha_ = 0.25; }
    if ( $calc_weights_algo_ eq "bestalpha10" ) 
    { $alpha_ = ( 10 / ( $iter_count_ ) ) ; }

    if ( $DEBUG ) 
    { 
      $main_log_file_handle_->print ( "$calc_weights_algo_ $alpha_ $iter_count_\n" ) ;
    }

    my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
    @t_best_indicator_weight_vec_ = @$tref_ ; # linear combination of first and best

        GetWeightedSum ( \@t_best_indicator_weight_vec_, $$indicator_weight_vec_vec_ref_[$best_index_], $$indicator_weight_vec_vec_ref_[0], $alpha_ ) ;

    if ( $DEBUG ) 
    { 
      $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
      $main_log_file_handle_->printf ( "A weights: %s\n", join(' ',@$tref_) ) ; 
      $tref_ = $$indicator_weight_vec_vec_ref_[0];
      $main_log_file_handle_->printf ( "B weights: %s\n", join(' ',@$tref_) ) ; 
      $main_log_file_handle_->printf ( "Final weights: %s\n", join(' ',@t_best_indicator_weight_vec_) ) ; 
    }

  }


  if ( $#t_best_indicator_weight_vec_ < 0 )
  { # default = best1
    my $tref_ = $$indicator_weight_vec_vec_ref_[$best_index_];
    @t_best_indicator_weight_vec_ = @$tref_ ; # set it to the best vector and return
  }

  if( $DEBUG )
  {
    $main_log_file_handle_->printf( "Pre Sanity check, weight vec: %s \n", join(' ',@t_best_indicator_weight_vec_) );
  }

#sanity check sign of weights
  for ( my $i = 0, my $j = 0 ; $i < $num_indicators_; $i ++ )
  {
#check for min & max violations
    if( ( abs ( $t_best_indicator_weight_vec_[$i] ) < $indicator_abs_min_weights_[$i] ) ||
       ( $t_best_indicator_weight_vec_[$i] * $sign_vec_[$i] < 0 ) )
    {
      $t_best_indicator_weight_vec_[$i] = $sign_vec_[$i] * $indicator_abs_min_weights_[$i]; 
    }
    if( abs ( $t_best_indicator_weight_vec_[$i] ) > $indicator_abs_max_weights_[$i] )
    {
      $t_best_indicator_weight_vec_[$i] = $sign_vec_[$i] * $indicator_abs_max_weights_[$i];
    }
  }
  if( $DEBUG )
  {
    $main_log_file_handle_->printf( "Post Sanity check, weight vec: %s \n", join(' ',@t_best_indicator_weight_vec_) );
  }

  return @t_best_indicator_weight_vec_ ;
}

sub CalcWeightLimits 
{
  my ( @def_indicator_weight_vec_ ) = @_;
  my @var_factors_to_test_ = ( 0.1, 0.25, 0.5, 0.75, 1.25, 1.5, 2.0, 4.0, 8.0 );
  my $t_model_filename_ = $work_dir_."/"."tmp_model_calcweightlimits";
  if ( $DEBUG )
  {
    $main_log_file_handle_->printf ( "In CalcWeightLimits " );
  }
  my $t_output_ = $base_model_start_text_;
  for ( my $j = 0; $j <= $#indicator_list_ ; $j ++)
  {
    if ( $model_type_ eq "SIGLR" )
    {
      $t_output_ = $t_output_."INDICATOR ".$siglr_alpha_vec_[$j].":".$def_indicator_weight_vec_[$j]." ".$indicator_list_[$j]."\n";
    }
    else
    {
      $t_output_ = $t_output_."INDICATOR ".$def_indicator_weight_vec_[$j]." ".$indicator_list_[$j]."\n";
    }
  }

  $t_output_ = $t_output_."INDICATOREND\n";

  open OUTMODEL, "> $t_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_ for writing\n" );
  print OUTMODEL $t_output_;
  close OUTMODEL;
  my $exec_cmd_ ="$MODELSCRIPTS_DIR/get_stdev_model.pl  $t_model_filename_ $training_date_vec_[0] $training_date_vec_[0] $base_start_time_ $base_end_time_ | head -n1";
  my $result_ = `$exec_cmd_`;
  chomp $result_;
  my @tokens = split( ' ', $result_ );
  my $def_stdev_ = $tokens[0];
  my $DEF_THRESH_INDEX = 100000;

  for ( my $j = 0; $j <= $#indicator_list_ ; $j++ )
  {
    my $min_index_ = -1;
    my $max_index_ = $DEF_THRESH_INDEX;

#find minimum 
    my $curr_stdev_ratio_ = 0 ;
    for ( my $k = 0; $k <= $#var_factors_to_test_ ; $k++ )
    {
#guaranteed to terminate for current set of weights ( having elements < 1 & > 1 )
      if ( $var_factors_to_test_[ $k ] > 1 || $curr_stdev_ratio_ > 0.5 )
      {
        if( $min_index_ > -1 )
        {
          push( @indicator_abs_min_weights_, abs( $var_factors_to_test_[ $min_index_ ] * $def_indicator_weight_vec_[ $j ] ) );		    
        }
        else
        {
          push( @indicator_abs_min_weights_, abs( 0.1 * $def_indicator_weight_vec_[ $j ] ) );
        }
        last;		
      }

#create a model; get stdev and update curr_stdev_ratio
      open OUTMODEL, "> $t_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_ for writing\n" );
      $t_output_ = $base_model_start_text_;
      for ( my $l = 0; $l <= $#indicator_list_ ; $l ++)
      {
        if ( $model_type_ eq "SIGLR" )
        {
          if ( $l != $j )
          {
            $t_output_ = $t_output_."INDICATOR ".$siglr_alpha_vec_[$l].":".$def_indicator_weight_vec_[$l]." ".$indicator_list_[$l]."\n";
          }
          else
          {
            $t_output_ = $t_output_."INDICATOR ".$siglr_alpha_vec_[$l].":".( $def_indicator_weight_vec_[$l] * $var_factors_to_test_[$k])." ".$indicator_list_[$l]."\n";			
          }
        }
        else
        {
          if ( $l != $j )
          {
            $t_output_ = $t_output_."INDICATOR ".$def_indicator_weight_vec_[$l]." ".$indicator_list_[$l]."\n";
          }
          else
          {
            $t_output_ = $t_output_."INDICATOR ".( $def_indicator_weight_vec_[$l] * $var_factors_to_test_[$k] )." ".$indicator_list_[$l]."\n";
          }
        }
      }

      $t_output_ = $t_output_."INDICATOREND\n";
      print OUTMODEL $t_output_;
      close OUTMODEL;
      $exec_cmd_ ="$MODELSCRIPTS_DIR/get_stdev_model.pl  $t_model_filename_ $training_date_vec_[0] $training_date_vec_[0] $base_start_time_ $base_end_time_ | head -n1";
      $result_ = `$exec_cmd_`;
      chomp $result_;
      @tokens = split( ' ', $result_ );
      $curr_stdev_ratio_ = $tokens[0]/$def_stdev_;

      if( $curr_stdev_ratio_ < 0.5 )
      {
        $min_index_ = $k;
      }

      if ( $DEBUG )
      {
        $main_log_file_handle_->printf ( "Variable:%d weight:%f ratio:%f \n", $j, $var_factors_to_test_[$k], $curr_stdev_ratio_ );
      }
    }

    if ( $DEBUG )
    {
      $main_log_file_handle_->printf ( "Min weight for %d variable set as %f \n", $j, $indicator_abs_min_weights_[$j] );
    }

#find maximum value
    $curr_stdev_ratio_ = 100 ;
    for ( my $k = $#var_factors_to_test_ ; $k >= 0; $k-- )
    {
#guaranteed to terminate for current set of weights ( having elements < 1 & > 1 )
      if ( $var_factors_to_test_[ $k ] < 1 || $curr_stdev_ratio_ < 2.0 )
      {
        if( $max_index_ < $DEF_THRESH_INDEX )
        {
          push( @indicator_abs_max_weights_, abs( $var_factors_to_test_[ $max_index_ ] * $def_indicator_weight_vec_[ $j ] ) );		    
        }
        else
        {
          push( @indicator_abs_max_weights_, abs( 8.0 * $def_indicator_weight_vec_[ $j ] ) );
        }
        last;		
      }

#create a model; get stdev and update curr_stdev_ratio
      open OUTMODEL, "> $t_model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $t_model_filename_ for writing\n" );
      $t_output_ = $base_model_start_text_;
      for ( my $l = 0; $l <= $#indicator_list_ ; $l ++)
      {
        if ( $model_type_ eq "SIGLR" )
        {
          if ( $l != $j )
          {
            $t_output_ = $t_output_."INDICATOR ".$siglr_alpha_vec_[$l].":".$def_indicator_weight_vec_[$l]." ".$indicator_list_[$l]."\n";
          }
          else
          {
            $t_output_ = $t_output_."INDICATOR ".$siglr_alpha_vec_[$l].":".( $def_indicator_weight_vec_[$l] * $var_factors_to_test_[$k])." ".$indicator_list_[$l]."\n";			
          }
        }
        else
        {
          if ( $l != $j )
          {
            $t_output_ = $t_output_."INDICATOR ".$def_indicator_weight_vec_[$l]." ".$indicator_list_[$l]."\n";
          }
          else
          {
            $t_output_ = $t_output_."INDICATOR ".( $def_indicator_weight_vec_[$l] * $var_factors_to_test_[$k] )." ".$indicator_list_[$l]."\n";
          }
        }
      }

      $t_output_ = $t_output_."INDICATOREND\n";
      print OUTMODEL $t_output_;
      close OUTMODEL;
      my $exec_cmd_ ="$MODELSCRIPTS_DIR/get_stdev_model.pl  $t_model_filename_ $training_date_vec_[0] $training_date_vec_[0] $base_start_time_ $base_end_time_ | head -n1";
      my $result_ = `$exec_cmd_`;
      chomp $result_;
      my @tokens = split( ' ', $result_ );
      $curr_stdev_ratio_ = $tokens[0]/$def_stdev_;
      if( $curr_stdev_ratio_ > 2 )
      {
        $max_index_ = $k;
      }	    
      if ( $DEBUG )
      {
        $main_log_file_handle_->printf ( "Variable:%d weight:%f ratio:%f \n", $j, $var_factors_to_test_[$k], $curr_stdev_ratio_ );
      }
    }

    if ( $DEBUG )
    {
      $main_log_file_handle_->printf ( "Max weight for %d variable set as %f \n", $j, $indicator_abs_max_weights_[$j] );
    }
  }    
}

sub CompareVectors
{
  my ( $vector_1_ref_, $vector_2_ref_ ) = @_;
  if ( $#$vector_1_ref_ != $#$vector_2_ref_ ) { return 1; }
  for ( my $i =0; $i <= $#$vector_1_ref_; $i ++ ) 
  { 
    if ( $$vector_1_ref_[$i] != $$vector_2_ref_[$i] ) 
    { return 1; } 
  }
  return 0;
}

