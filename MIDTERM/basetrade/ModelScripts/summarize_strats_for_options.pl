#!/usr/bin/perl

# \file ModelScripts/choose_strats_for_day.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
#my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $GLOBALRESULTSDBDIR="DB";

require "$GENPERLLIB_DIR/get_choose_strats_config.pl"; # GetChooseStratsConfig
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/option_strat_utils.pl"; # GetListOfShcFromUnderlying

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );

sub MakeStratVecFromDirectory
{
  my ($top_directory_) = @_;
  $top_directory_ = File::Spec->rel2abs ( $top_directory_ );
  my @strat_vec_ = ();
  if ( -d $top_directory_ )
  {
      if (opendir my $dh, $top_directory_)
      {
	  my @t_list_=();
	  while ( my $t_item_ = readdir $dh)
	  {
	      push @t_list_, $t_item_;
	  }
	  closedir $dh;
	  
	  for my $dir_item_ (@t_list_)
	  {
	      # Unix file system considerations.
	      next if $dir_item_ eq '.' || $dir_item_ eq '..';
	      
	      push @strat_vec_, $dir_item_  if -f "$top_directory_/$dir_item_";
	      push @strat_vec_, MakeStratVecFromDir ("$top_directory_/$dir_item_") if -d "$top_directory_/$dir_item_";
	  }
      }
  }
  @strat_vec_;
}

sub GetResultsForShortcode
{
  my $strats_vec_ref_ = shift;
  my $shc_ = shift;
  my $results_dir_ = shift;
  my $trading_start_yyyymmdd_ = shift;
  my $trading_end_yyyymmdd_ = shift;
  my $skip_dates_file_ = shift; 
  my $sort_algo_ = shift; 
  my $max_loss_per_uts_ = shift; 
  my $specific_dates_file_ = shift; 
  my $shorter_output_ = shift;
  my $stats_mode_ = shift;

  my $suffix = "_".$shc_;

  if( ($shc_ eq "IDXStrats") || ($shc_ eq "STKStrats"))
  {
    $suffix = "";  
  }

  if($stats_mode_ == 1)
  {
    $suffix = $suffix."_DELTA";
  }
  
  if($stats_mode_ == 2)
  {
    $suffix = $suffix."_VEGA";
  }


  my $temp_stratlist_filename_ = $SPARE_HOME."stratlist_".$unique_gsm_id_;

  open TSF, "> $temp_stratlist_filename_" or PrintStacktraceAndDie ( "Could not open $temp_stratlist_filename_ for writing\n" );
  foreach my $full_strat_filename_ ( @$strats_vec_ref_ )
  {
    # for each file 
    # add the shortcode to the end of filename 
    # write this to temp file
    my $strat_name_ = basename($full_strat_filename_);
    $strat_name_  = $strat_name_.$suffix;
    print TSF $strat_name_."\n";
  }
  close TSF;

  my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $shc_ $temp_stratlist_filename_ $results_dir_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ $skip_dates_file_ $sort_algo_ $max_loss_per_uts_ $specific_dates_file_ $shorter_output_";
  my @ssr_output_ = `$exec_cmd`;
  chomp ( @ssr_output_ );
  `rm -f $temp_stratlist_filename_`;

   return @ssr_output_ ; 
}

# start 
my $USAGE="$0 shortcode/ALL strat_dir results_dir start_date end_date [skip_dates_file or INVALIDFILE] [sortalgo=kCNAPnlAdjAverage] [MaxLossPerUts(0 no_maxloss|-ve param.maxloss)=0] [specific_dates_file or specific_dates_with_weights_file_ or INVALIDFILE] [shorter_output=1] [all_shc_=0] [skip_days_before_expiry_=0] [stats_mode_ = 0/1/2]";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $strat_dir_ = $ARGV[1];
my $results_dir_ = $ARGV[2];
my $trading_start_yyyymmdd_ = $ARGV[3];
my $trading_end_yyyymmdd_ = $ARGV[4];

my $skip_dates_file_ = "IF";
my $sort_algo_  ="kCNAPnlAdjAverage";
my $max_loss_per_uts_  = 0;
my $specific_dates_file_ = "IF";
my $shorter_output_ = 0;
my $all_shc_ = 0;
my $skip_days_before_expiry_ = 0;
my $stats_mode_ = 0; #0 for normal stats , 1 for delta stats and 2 for vega stats  

if ( $#ARGV > 4 )
{
  $skip_dates_file_ = $ARGV[5];
}

if ( $#ARGV > 5 )
{
  $sort_algo_ = $ARGV[6];
}

if ( $#ARGV > 6 )
{
  $max_loss_per_uts_ = $ARGV[7];
}

if ( $#ARGV > 7 )
{
  $specific_dates_file_ = $ARGV[8];
}

if ( $#ARGV > 8 )
{
  $shorter_output_ = $ARGV[9];
}

if ( $#ARGV > 9 )
{
  $all_shc_ = int($ARGV[10] > 0) + 0;
}

if ( $#ARGV > 10 )
{
  $skip_days_before_expiry_ = $ARGV[11];
}

if ( $#ARGV > 11 )
{
  $stats_mode_ = $ARGV[12];
}

my @all_strats_in_dir_ = ();

if (-f $strat_dir_)
{
  open my $handle, '<', $strat_dir_;
  chomp(@all_strats_in_dir_ = <$handle>);
  close $handle;
}
else 
{
  @all_strats_in_dir_ = MakeStratVecFromDirectory($strat_dir_);
}

if($skip_days_before_expiry_ != 0)
{

  my $segment_shc_ = "NSE_NIFTY_FUT0"; 

  if ((index($shortcode_, "USDINR") != -1) || (index($shortcode_, "GIND10YR") != -1)) {
    $segment_shc_ = "NSE_USDINR_FUT0";
  }  

  my $temp_skip_filename_ = $SPARE_HOME."skiplist_".$unique_gsm_id_;
  open SKIPFILEHANDLE, "> $temp_skip_filename_" or PrintStacktraceAndDie ( "$0 could not open skip for writing\n" );

  my $exec_cmd_ = "$LIVE_BIN_DIR/option_details $segment_shc_ $trading_start_yyyymmdd_";
  my $ssr_output_ = `$exec_cmd_`;
  chomp ( $ssr_output_ );
  my $curr_expiry_ =  $ssr_output_;

  $exec_cmd_ = "$LIVE_BIN_DIR/option_details $segment_shc_ $trading_end_yyyymmdd_";
  $ssr_output_ = `$exec_cmd_`;
  chomp ( $ssr_output_ );
  my $last_expiry_ = $ssr_output_;

  $skip_days_before_expiry_ = min(20,$skip_days_before_expiry_);

  while($curr_expiry_ <= $last_expiry_)
  {
    $exec_cmd_ = "$SCRIPTS_DIR/get_list_of_dates_for_shortcode.pl $segment_shc_ $curr_expiry_ $skip_days_before_expiry_";
    $ssr_output_ =`$exec_cmd_`;
    chomp ( $ssr_output_ );
    $ssr_output_ =~ s/\s+/\n/g;
    print SKIPFILEHANDLE $ssr_output_."\n";
      
    $exec_cmd_ = "$LIVE_BIN_DIR/calc_next_week_day $curr_expiry_";
    my $next_date_ =`$exec_cmd_`;
    chomp ( $next_date_ );
        
    last if $next_date_ > $trading_end_yyyymmdd_;
    $exec_cmd_ =  "$LIVE_BIN_DIR/option_details $segment_shc_ $next_date_";
    $ssr_output_ =`$exec_cmd_`;
    chomp ( $ssr_output_ );
    $curr_expiry_ =  $ssr_output_;   
  }

  $skip_dates_file_ = $temp_skip_filename_;
  
}

if(!$all_shc_) {

  my @ssr_output_  = GetResultsForShortcode(\@all_strats_in_dir_,$shortcode_,$results_dir_,$trading_start_yyyymmdd_,$trading_end_yyyymmdd_,$skip_dates_file_,$sort_algo_,$max_loss_per_uts_,$specific_dates_file_,$shorter_output_,$stats_mode_);

  print join ( "\n", @ssr_output_ )."\n\n";

  exit ( 0 );
}
else { 

  print "SHORTCODE PNL STDEV VOL SHARPE PNL_CONSERVATIVE PNL_MEDIAN TTC TTC_VOL PNLMINADJ PPC NBL BL A I DD DDPNL ABS MSG G2PR PNL/LOSS MIN OTL OPEN UTS PPT\n\n";

  my @ssr_output_  = GetResultsForShortcode(\@all_strats_in_dir_,$shortcode_,$results_dir_,$trading_start_yyyymmdd_,$trading_end_yyyymmdd_,$skip_dates_file_,$sort_algo_,$max_loss_per_uts_,$specific_dates_file_,1,$stats_mode_);

  my %shc_to_strat_results_map_ = ();

  # Assumption is all stratfile has the same shortcodes list as top ranked strat
  if($#ssr_output_ < 0)
  {
    exit(0);
  }

  my @top_strat_result_words_ = split(' ',$ssr_output_[0]);
  my $top_strat_name_ = $top_strat_result_words_[1];
  my $string_to_remove_  = "_".$shortcode_."\$";
  $top_strat_name_ =~ s/$string_to_remove_//g; 

  my $exec_cmd_ = "find $strat_dir_ -name $top_strat_name_"; 
  my $full_strat_path_ = `$exec_cmd_`;

  my @list_shc_ = ();
  if( ($shortcode_ eq "IDXStrats") || ($shortcode_ eq "STKStrats"))
  {
    @list_shc_  = GetListOfUnderlying($full_strat_path_);
  }
  else {
    @list_shc_  = GetListOfShcFromUnderlying($full_strat_path_,$shortcode_);
    push(@list_shc_,"NSE_".$shortcode_."_FUT0");
  }

  foreach my $shc_ ( @list_shc_ )
  {
    my @shc_output_  = GetResultsForShortcode(\@all_strats_in_dir_,$shc_,$results_dir_,$trading_start_yyyymmdd_,$trading_end_yyyymmdd_,$skip_dates_file_,$sort_algo_,$max_loss_per_uts_,$specific_dates_file_,1,$stats_mode_);
    foreach my $shc_output_line_ (@shc_output_)
    {
      my @strat_result_words_ = split(' ',$shc_output_line_);
      next if $#strat_result_words_ < 0;   
      my $strat_name_ = $strat_result_words_[1];
      $string_to_remove_  = "_".$shc_."\$";
      $strat_name_ =~ s/$string_to_remove_//g; 
      my $results_ = join(' ',@strat_result_words_[2 .. $#strat_result_words_]);
      $shc_to_strat_results_map_{$shc_}{$strat_name_} = $results_;
    }
  }

  foreach my $shc_output_line_ (@ssr_output_)
  {
    my @strat_result_words_ = split(' ',$shc_output_line_);
    next if $#strat_result_words_ < 0;   
    my $strat_name_ = $strat_result_words_[1];
    $string_to_remove_  = "_".$shortcode_."\$";
    $strat_name_ =~ s/$string_to_remove_//g; 
    print "STRATEGYFILEBASE ".$strat_name_."\n";
    foreach my $shc_ ( @list_shc_ )
    {
      if(exists $shc_to_strat_results_map_{$shc_}{$strat_name_})
      {
        print $shc_." ".$shc_to_strat_results_map_{$shc_}{$strat_name_}."\n";
      }
    }
    print "TOTAL ".join(' ',@strat_result_words_[2 .. $#strat_result_words_])."\n";
    print "\n";
  }

  print "\n\n";

  if($skip_days_before_expiry_ != 0)
  {
    `rm -r $skip_dates_file_`;
  }

  exit(0);
}

