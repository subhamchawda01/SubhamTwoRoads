#!/usr/bin/perl

# \file ModelScripts/prune_strats_for_day_from_periods.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes an instructionfilename :
#
# shortcode
# TIME_DURATION = [ EU_MORN_DAY | US_MORN | US_DAY | EU_MORN_DAY_US_DAY ]
# [ CONFIG_FILE ]
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' }; 
my $SPARE_HOME = "/spare/local/".$USER."/";

my $REPO = "basetrade";

my $CHOICEDIR = $HOME_DIR."/choices";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $GLOBALRESULTSDBDIR = "DB";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl";
require "$GENPERLLIB_DIR/pool_utils.pl";

sub LoadConfigFile;

sub GetResultsForDurations;

sub SaveDoNotPrune;

sub PickBadStrats;

sub PruneBadStrats;

sub GetScore;

# start 
my $USAGE="$0 shortcode timeperiod config-file [REMOVE=0] [NORMAL/STAGED] skip-days-file";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $config_file_ = $ARGV [ 2 ];

my $verbose_ = 1;

my $to_remove_ = 0;
if ( $#ARGV >= 3 ) { $to_remove_ = $ARGV [ 3 ]; }

my $staged_ = 0;
if ( $#ARGV >= 4 && $ARGV[4] eq "STAGED" ) { $staged_ = 1; }

my $skip_days_file_ = "INVALIDFILE";
if ( $#ARGV >= 5 ) { $skip_days_file_ = $ARGV[5]; }

my $pool_list_ = MakeStratList ( );

my @trading_end_yyyymmdds_ = ( );
my @durations_ = ( );
my @strats_sorted_ = ( );

my @all_strats_ = ( );

my @strats_to_keep_ = ( );
my @strats_to_prune_ = ( );

my $target_pool_size_ = -1;

my $sort_algo_ = "kCNASqDDAdjPnlSqrtVolume";

LoadConfigFile ( $config_file_ );

GetResultsForDurations ( );

SaveDoNotPrune ( );

PickBadStrats ( );

PruneBadStrats ( );

exit ( 0 );

sub LoadConfigFile
{
  my ( $t_config_file_ ) = @_;

  open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  my $is_reading_template_ = 0;
  my $num_durations_in_template_ = 0;

  foreach my $config_file_line_ ( @config_file_lines_ )
  {
    my @config_line_words_ = split ( ' ' , $config_file_line_ );

    if ( $#config_line_words_ > 0 )
    {
      if ( $config_line_words_ [ 0 ] eq "SHORTCODE" )
      { # SHORTCODE TIME_PERIOD           DURATION        DURATION        DURATION        TARGET_POOL_SIZE        SORT_ALGO
        $is_reading_template_ = 1;

        $num_durations_in_template_ = 0;
        for ( my $i = 2 ; $i <= $#config_line_words_ && $config_line_words_ [ $i ] eq "DURATION" ; $i ++ )
        {
          $num_durations_in_template_ ++;
        }
      }
      elsif ( $#config_line_words_ >= 1 &&
             $shortcode_ eq $config_line_words_ [ 0 ] &&
             $timeperiod_ eq $config_line_words_ [ 1 ] )
      { # BR_DOL_0  BRT_910-BRT_1700      TODAY-100 30    TODAY-30 30     TODAY-70 30     200                     kCNASqDDAdjPnlSqrtVolume
# This is the shortcode to prune
        if ( ! $is_reading_template_ )
        { # Check if we have a template for this line of specification.
          PrintStacktraceAndDie ( "No template to work off" );
        }

        print "Using config : ".$config_file_line_."\n";

        for ( my $t_duration_ = 0 ; $t_duration_ < $num_durations_in_template_ ; $t_duration_ ++ )
        {
	    push ( @trading_end_yyyymmdds_ , GetIsoDateFromStrMin1 ( $config_line_words_ [ 2 + 2 * $t_duration_ ] ) );
	    push ( @durations_ , $config_line_words_ [ 3 + 2 * $t_duration_ ] );
        }

        $target_pool_size_ = $config_line_words_ [ 2 + 2 * $num_durations_in_template_ ];
        $sort_algo_ = $config_line_words_ [ 3 + 2 * $num_durations_in_template_ ];

        $is_reading_template_ = 0;
      }
    }
    else
    {
      $is_reading_template_ = 0;
    }
  }

  if ( $#durations_ < 0 || $#trading_end_yyyymmdds_ < 0 )
  {
    PrintStacktraceAndDie ( "Nothing for ( $shortcode_ $timeperiod_ ) in $t_config_file_" );
  }

  return;
}

sub MakeStratList {
  my $staged_type_ = ($staged_) ? 'S' : 'N';
  my $pool_list_ = GetCSTempFileName ( "/spare/local/temp/" );

  my @configs_ = GetConfigsForPool ($shortcode_, $timeperiod_, $staged_type_);
  open CSTF, "> $pool_list_" or PrintStacktraceAndDie ( "Could not open $pool_list_ for writing\n" );
  print CSTF $_."\n" foreach @configs_;
  close CSTF;

  return $pool_list_;
}

sub GetResultsForDurations
{
  for ( my $i = 0 ; $i <= $#durations_ ; $i ++ )
  {
    my $this_trading_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_end_yyyymmdds_[ $i ], $durations_[ $i ] , $shortcode_ );
    
    my @strats_base_ = ();

    my $exec_cmd_ = "$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $pool_list_ $GLOBALRESULTSDBDIR $this_trading_start_yyyymmdd_ $trading_end_yyyymmdds_[$i] $skip_days_file_ $sort_algo_ 0 IF 0";
    print $exec_cmd_."\n";
    my @ssr_output_ = `$exec_cmd_`;
    chomp ( @ssr_output_ );
    
    my $curr_strat_ = "";
    my $numdates_ = 0;
    
    foreach my $ssr_line_ ( @ssr_output_ ) {
      my @ssr_words_ = split (' ', $ssr_line_ );
      chomp ( @ssr_words_ );

      if ( $#ssr_words_ < 0 ) { next; }

      if ( $curr_strat_ ne "" ) {
        if ( $ssr_words_[0] eq "STATISTICS" ) {
          if ( $numdates_ > 0.8 * $durations_[ $i ] ) {
#            print "Pushed ".$curr_strat_." for period ".$trading_end_yyyymmdds_[ $i ]."\n";
            push ( @strats_base_, $curr_strat_ );
          }
          $curr_strat_ = "";
          $numdates_ = 0;
        }
        elsif ( ValidDate( $ssr_words_[0] ) && !SkipWeirdDate( $ssr_words_[0] ) ) {
         $numdates_++;
        }
      }
      elsif ( $ssr_words_[0] eq "STRATEGYFILEBASE" ) {
        $curr_strat_ = $ssr_words_[1];
      }
    }
    if ( $i == 0 ) {
      @all_strats_ = @strats_base_;
    }
    else {
      @all_strats_ = grep { FindItemFromVec( $_, @strats_base_ ) } @all_strats_;
    }

    $strats_sorted_ [ $i ] = [ @strats_base_ ];
  }

  for ( my $i = 0 ; $i <= $#durations_ ; $i ++ )
  {
    my @filtered_strats_base_ = grep { FindItemFromVec( $_, @all_strats_ ) } @{ $strats_sorted_[ $i ] } ;
    $strats_sorted_[ $i ] = [ @filtered_strats_base_ ];
  }

  print "Size of pool with results in periods : ".@all_strats_."\n";
}

sub SaveDoNotPrune
{
  my $donotprune_filename_ = $CHOICEDIR."/DONOTPRUNE.".$shortcode_;
  if ( -e $donotprune_filename_ )
  {
    print "Not pruning strats mentioned in ".$donotprune_filename_."\n";
    open DNPFILEH, "< $donotprune_filename_" or PrintStacktraceAndDie ( "Could not open $donotprune_filename_\n" );
    while ( my $dnpline_ = <DNPFILEH> )
    {
      chomp $dnpline_ ;

      my @dnp_words_ = split ( ' ', $dnpline_ );
      if ( $#dnp_words_ >= 0 )
      {
        my $t_strategy_filename_base_ = basename ( $dnp_words_[0] ) ;
        push ( @strats_to_keep_, $t_strategy_filename_base_ ) ;
      }
    }

    close DNPFILEH;
  }
  print "After SaveDoNotPrune : ".@strats_to_keep_."\n";
}

sub PickBadStrats
{
# Pick the top $target_pool_size_ number of strats from each duration
# and add them to strats_to_keep_.
# Then for all strats not in strats_to_keep_ , move to strats_to_prune_.
  for ( my $i = 0 ; $i <= $#strats_sorted_ ; $i ++ )
  {
    my @sorted_strats_ = @{ $strats_sorted_[ $i ] };

    my $t_size_to_keep_ = min( $target_pool_size_ - 1 , $#sorted_strats_ );
    push ( @strats_to_keep_, @sorted_strats_[ 0 .. $t_size_to_keep_ ] );
  }
  
  @strats_to_keep_ = GetUniqueList( @strats_to_keep_ );

  print "Size of strats_to_keep_ : ".@strats_to_keep_."\n";

  @strats_to_prune_ = grep { ! FindItemFromVec ( $_, @strats_to_keep_ ) } @all_strats_;

  print "Size of strats_to_prune_ : ".@strats_to_prune_."\n";
}

sub PruneBadStrats
{
  if ( $to_remove_ ) {
    print "Pruning Strats :\n";
    foreach my $strat_ ( @strats_to_prune_ ) {
      print "PRUNE: ".$strat_."\n";
      my $exec_cmd_ = "$HOME_DIR/walkforward/process_config.py -c $strat_ -m PRUNE";
      `$exec_cmd_`;
    }
  }
}

