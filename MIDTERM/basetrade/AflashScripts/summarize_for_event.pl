#!/usr/bin/perl

# \file ModelScripts/setprodqueries.pl
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
use POSIX qw/ceil/;
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $AFLASHSCRIPTS_DIR = $HOME_DIR."/".$REPO."/AflashScripts";
my $AFLASH_TRADEINFO_DIR = "/spare/local/tradeinfo/Alphaflash";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $TEMP_DIR = "/spare/local/temp";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/get_unique_list.pl"; #GetUniqueList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage

my $USAGE="$0 config-file start-date end-date results_dir";

if ( $#ARGV < 1 ) { 
  print $USAGE."\n";
  exit(0);
}

my $config_file_ = $ARGV[0];
my $start_date_ = $ARGV[1];
my $end_date_ = $ARGV[2];
my $results_dir_ = $ARGV[3];

if ( ! -d $results_dir_ ) {
  print "Error: Results-dir $results_dir_ does not exist.. Exiting\n";
  exit(0);
}

my $ev_id_;
my @dateslist_ = ( );
my $date_ = $end_date_;
my @shc_list_ = ( );

LoadConfigFile ( );

LoadDates ( );

SummarizeStrats ( );

sub LoadConfigFile
{
  print "Loading Config File ...\n";

  open ( CONFIG_FILE , "<" , $config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  my $current_shc_ = "";
  foreach my $cline_ ( @config_file_lines_ )
  {
    next if ( $cline_ =~ /^#/ );  
    
    my @t_words_ = split ( ' ' , $cline_ );

    if ( $#t_words_ < 0 ) {
      $current_shc_ = "";
      next;
    }

    next if ( $#t_words_ < 1 );

    my $param_ = $t_words_[0];

    if ( $current_shc_ eq "" ) {
      if ( $param_ eq "SHORTCODE" ) {
        $current_shc_ = $t_words_[1];
        push ( @shc_list_, $current_shc_ );
      }
      elsif ( $param_ eq "EVENT_ID" ) {
        $ev_id_ = $t_words_[1];
      }
    }
  }
}

sub LoadDates
{
  my $cmd_ = "grep \^$ev_id_ /spare/local/tradeinfo/Alphaflash/Estimates/estimates_201\[4-9\]\* | cut -d\: -f1 | cut -d\_ -f2";
  my @dates_vec_ = `$cmd_`; chomp ( @dates_vec_ );
  @dateslist_ = grep { $_ >= $start_date_ && $_ <= $end_date_ } @dates_vec_;
}

sub SummarizeStrats
{
  my %strats_to_shc_ = map { "strat__".$_."_af_".$ev_id_ => $_ } @shc_list_;
  my %dates_map_ = map { $_ => 1 } @dateslist_;
  my %shc_to_dt_to_results_ = ( );

  my $summarize_cmd_ = $BIN_DIR."/summarize_local_results_dir_and_choose_by_algo kCNAGainPainRatio 4000 4000 -1 1 5000 100000 $results_dir_ 2>/dev/null";
  my @summarize_lines_ = `$summarize_cmd_`;
  chomp ( @summarize_lines_ );

  my $current_shc_ = "";
  foreach my $sline_ ( @summarize_lines_ ) {
    next if ( $sline_ eq "" || $sline_ =~ /STATISTICS/ );

    my @swords_ = split( /\s+/, $sline_ );

    if ( $swords_[0] =~ /STRATEGYFILEBASE/ ) {
      if ( defined $strats_to_shc_{ $swords_[1] } ) {
        $current_shc_ = $strats_to_shc_{ $swords_[1] };
      } else {
        $current_shc_ = "";
      }
    }
    elsif ( $current_shc_ ne "" ) {
      if ( defined $dates_map_{ $swords_[0] } ) {
        $shc_to_dt_to_results_{ $current_shc_ }{ $swords_[0] } = $sline_;
      }
    }
  }

  my @dt_vec_ = ( );
  my @pnl_vec_ = ( );
  my @minpnl_vec_ = ( );
  my @maxpnl_vec_ = ( );
  my @dd_vec_ = ( );

  foreach my $dt ( @dateslist_ ) {
    my ($pnl_, $minpnl_, $maxpnl_, $drawdown_, $count_) = (0,0,0,0,0);
    foreach my $shc_ ( @shc_list_ ) {
      if ( defined $shc_to_dt_to_results_{ $shc_ }{ $dt } ) {
        my @swords_ = split( /\s+/, $shc_to_dt_to_results_{ $shc_ }{ $dt } );
        $pnl_ += $swords_[1];
        $minpnl_ += $swords_[9];
        $maxpnl_ += $swords_[11];
        $drawdown_ += $swords_[13];
        $count_++;
      }
    }
    if ( $count_ > 0 ) {
      print "$dt $pnl_ min: $minpnl_ max: $maxpnl_ dd: $drawdown_\n";
      push ( @dt_vec_, $dt );
      push ( @pnl_vec_, $pnl_ );
      push ( @minpnl_vec_, $minpnl_ );
      push ( @maxpnl_vec_, $maxpnl_ );
      push ( @dd_vec_, $drawdown_ );
    }
  }

  my $numdays_ = scalar @dt_vec_;
  my $pnl_avg_ = GetAverage ( \@pnl_vec_ );
  my $pnl_sd_ = 0.01 + GetStdev ( \@pnl_vec_ );
  my $dd_highavg_ = GetMeanHighestQuartile ( \@dd_vec_ );

  my @minpnl_sorted_ = sort { $b <=> $a } @minpnl_vec_;
  my $percentile95_ = max(0, (0.95 * $numdays_) - 1 );
  my $min_pnl_ = $minpnl_sorted_[ $percentile95_ ];

  my $pnl_sharpe_ = $pnl_avg_ / $pnl_sd_;
  my $gain_pain_ = (1 + $pnl_avg_*$numdays_) / ( 1+ -1*GetSum ( \@minpnl_vec_ ) );
  my $pnl_dd_ = $pnl_avg_ / (1 + $dd_highavg_);

  printf "STATISTICS %.2f sharpe: %.2f sd: %.2f min: %d dd: %d gainpain: %.2f\n", $pnl_avg_, $pnl_sharpe_, $pnl_sd_, $min_pnl_, $dd_highavg_, $gain_pain_;
}

