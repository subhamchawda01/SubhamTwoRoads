#!/usr/bin/perl

# \file ModelScripts/find_optimal_risk_allocation_across_configs.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
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
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $BASETRADE_BIN_DIR = $LIVE_BIN_DIR;
my $WF_DB_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward/wf_db_utils";
my $WF_SCRIPTS = $HOME_DIR."/".$REPO."/walkforward";

# start
my $USAGE="$0 STRAT_LIST TOTAL_RISK DATE [SKIP_DATE_FILE=INVALIDFILE]";
if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/strat_utils.pl"; # CheckIfRegimeParam
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats, PnlSamplesGetStats, PnlSamplesGetStatsLong
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; 
require "$GENPERLLIB_DIR/config_utils.pl"; # IsValidConfig
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName

my $cfg_list_ = $ARGV[0];
my $total_risk_ = $ARGV[1];
my $date_ = $ARGV[2];

my $skip_dates_file_ = "INVALIDFILE";
if ( $#ARGV > 2 ) { $skip_dates_file_ = $ARGV[3]; }

my $risk_granularity_ = 500;
if ( $#ARGV > 3 ) { $risk_granularity_ = $ARGV[4]; }

# interval-splits at 2m, 5m, 10m, 20m 
my @intv_vec_ = (40, 150, 350); #(40, 100, 200, 400);
my %intv_to_wt_ = ( 40 => 0.45, 150 => 0.3, 350 => 0.25 ); # ( 40 => 0.3, 100 => 0.2, 200 => 0.2, 400 => 0.2 )

# Fetch Dates Vector for all the intervals
# Also, fetch all_dates vector
my @all_dates_ = ( );
my %intv_to_dates_ = ( );
my %date_to_intv_ = ( );
GetDatesVec ( );

my %cfg_to_shc_ = ( );
my %cfg_to_orig_uts_ = ( );
FetchConfigDetails ( );

my @sorted_cfg_vec_ = sort keys %cfg_to_shc_;

# Fetch sample-pnls, stats for the configs
my %cfg_to_pnls_ = ( );
my %cfg_to_stats_ = ( );
FetchPnlAndStats ( );

# granularity: the min UTS-delta for each config for permutations on the UTS-vector of the combination
# granularity is ONLY USED when we opt for permutations based optimization (instead of the closed-form sharpe-maximization closed form solution)
my %cfg_to_omlperuts_ = ( );
my %cfg_to_granularity_ = ( );
ComputeConfigsGranularities ( );

print "AvgPNL DailyDD OverDaysDD RiskPerUTS ConfigName\n";
foreach my $cfg_ ( @sorted_cfg_vec_ ) {
  my ($wpnl_, $wdd_, $wpnldd_) = FindPerformance ( $cfg_to_pnls_{$cfg_} );
  my $overdays_dd_ = GetOverDaysDD ( $cfg_to_pnls_{$cfg_} );

  print sprintf("%5d %8d %8d %7d\t\t%s\n", $wpnl_, $wdd_, $overdays_dd_, $cfg_to_omlperuts_{$cfg_}, $cfg_);
}

# Finds the optimal UTS-vector
# Two options:
# 1. FindOptimalCombinationPermutations: finds, in a greedy approach, the UTS-vector for which pnlDD is maximized
# 2. FindOptimalCombinationSharpeMax: finds the weight vec that miximizes sharpe and then, reduces the weight-vec to UTS-vec
my %cfg_to_optimal_frac_uts_ = ( );
my %cfg_to_optimal_uts_ = ( );
my $metric_ = "PnlDD";
my ($wpnl_, $wdd_, $wpnldd_) = FindOptimalCombinationSharpeMax ( );

print "Frac UTS: ".join(" ", map { sprintf("%.2f", $cfg_to_optimal_frac_uts_{$_}) } @sorted_cfg_vec_)."\n";
print "Optimal UTS: ".join(" ", map { $cfg_to_optimal_uts_{$_} } @sorted_cfg_vec_)."\n";
#print "\n";
#print $cfg_to_optimal_uts_{$_}." ".$_."\n" foreach @sorted_cfg_vec_;
#print "\n";

# Generating the combined pnl-series and printing few stats on them
my %combined_pnl_series_ = ( );
GetCombinedPnlSeries ( \%cfg_to_optimal_uts_, \%combined_pnl_series_ );
my $overdays_dd_ = GetOverDaysDD ( \%combined_pnl_series_ );
print "Avg PNL $wpnl_\n";
print "Avg Daily DD: $wdd_\n";
print "OverDays DD: ".int($overdays_dd_)."\n";

# Get the list of dates for each interval and the list of all dates
sub GetDatesVec
{
  my %all_days_map_ = ( );
  foreach my $intv_ ( sort { $a <=> $b } @intv_vec_ ) {
      #print $intv_."\n";
    my $numdays_ = $intv_;
    my @dates_ = GetDatesFromNumDays ( "ALL", $date_, $numdays_, $skip_dates_file_ );
    @dates_ = sort grep { ! defined $all_days_map_{$_} } @dates_;

    $intv_to_dates_{ $intv_ } = \@dates_;
    $all_days_map_{$_} = 1 foreach @dates_;
    $date_to_intv_{$_} = $intv_ foreach @dates_;
  }
  @all_dates_ = sort keys %all_days_map_;
}

# Fetch the shortcode and original UTS for the configs
# Note: assumes that the UTS for the config is constant across days
sub FetchConfigDetails
{
  open CFGLIST, "< $cfg_list_" or PrintStacktraceAndDie ( "Could not open $cfg_list_ for reading\n" );
  my @cfg_vec_ = <CFGLIST>; chomp ( @cfg_vec_ );

  foreach my $config_ ( @cfg_vec_ ) {
    my $cmd = $WF_SCRIPTS."/print_strat_from_config.py -c $config_ -d $date_";
    my $stratline_ = `$cmd 2>/dev/null`; chomp ( $stratline_ );

    if ( defined $stratline_ and $stratline_ ne "" ) {
      my @swords_ = split(/\s+/, $stratline_);
      if ( $#swords_ >= 4 ) {
        $cfg_to_shc_{ $config_ } = $swords_[1];

        my $pfile_ = $swords_[4];
        $cmd = $LIVE_BIN_DIR."/get_UTS_for_a_day ".$swords_[1]." $pfile_ $date_ 2";
        $cfg_to_orig_uts_{ $config_ } = `$cmd`; chomp ( $cfg_to_orig_uts_{ $config_ } );
      }
    }
  }
}

# For each config, fetches pnl-samples
# and stats on the pnl-samples series (PNL_DD is esp. useful later)
sub FetchPnlAndStats
{
  foreach my $config_ ( @sorted_cfg_vec_ ) {
    FetchPnlSamplesStrats ( $cfg_to_shc_{ $config_ }, [ $config_ ], \@all_dates_, \%cfg_to_pnls_ );
    # print $_." " foreach values %{$cfg_to_pnls_{$config_}}; print "\n";

    my %stats_map_ = ( );
    PnlSamplesGetStats ( $cfg_to_pnls_{ $config_ }, \%stats_map_, \@all_dates_ );
    $cfg_to_stats_{ $config_ } = \%stats_map_;
  }
}

# computes the UTS-granularity for each config
# granularity: the min UTS-delta for each config for permutations on the UTS-vector of the combination
# granularity is ONLY USED when we opt for permutations based optimization (instead of the closed-form sharpe-maximization closed form solution)
sub ComputeConfigsGranularities
{
  print "\nRiskPerUTS UTSGranularity Config\n";
  foreach my $config_ ( @sorted_cfg_vec_ ) {
##  as risk, we can take MINPNL or DD_HIGHAVG of the pnlstats
    my $risk_per_uts_ = abs( $cfg_to_stats_{ $config_ }{ "MINPNL" } / $cfg_to_orig_uts_{ $config_ } );
    $cfg_to_omlperuts_{ $config_ } = int($risk_per_uts_);

##  granularity is the min no. of uts on which we will vary the UTS for permutation 
    $cfg_to_granularity_{ $config_ } = max(1, int(0.5 + $risk_granularity_ / $risk_per_uts_) );
  }
}

# Permutations based optimal-combination-find function
# Metric used: PNL_DD
sub FindOptimalCombinationPermutations
{
  print "\nFinding Optimal Combination\n";
  my $current_perf_ = 0;
  my %cfg_to_uts_ = map { $_ => 0 } @sorted_cfg_vec_;
  foreach my $cfg_ ( @sorted_cfg_vec_ ) {
    foreach my $unitrisk_idx_ ( 1 .. 4 ) {
      $cfg_to_uts_{ $cfg_ } += $cfg_to_granularity_{ $cfg_ };

      my %cfg_to_scaled_uts_ = ( );
      my $new_performance_ = ScaleUTSandFindPerformance ( $total_risk_, \%cfg_to_uts_, \%cfg_to_scaled_uts_ );

      if ( $new_performance_ <= $current_perf_ ) {
        $cfg_to_uts_{ $cfg_ } -= $cfg_to_granularity_{ $cfg_ };
        last;
      }
      print join(" ", map { $cfg_to_uts_{$_} } @sorted_cfg_vec_)." : ".$new_performance_."\n";
      $current_perf_ = $new_performance_;
    }
  }

  print "\nScaling UTS to meet the permitted Risk\n";
  %cfg_to_optimal_uts_ = ( );
  my $final_performance_ = ScaleUTSandFindPerformance ( $total_risk_, \%cfg_to_uts_, \%cfg_to_optimal_uts_, 1 );
  return $final_performance_;
}

# Sharpe-Max-closed-form based optimal-combination-find function
# Note: it calls optimize_portfolio_sharpemax.R for fetching the weights-vec
# optimize_portfolio_sharpemax.R, by default, works on daywise-pnl-series
# it has the option to work on sample-pnl-series as well
sub FindOptimalCombinationSharpeMax
{
  print "\nFinding Optimal Combination\n";

  my $cstempfile_ = GetCSTempFileName ( $HOME_DIR."/cstemp" );

  my %slot_to_pnl_vec_ = ( );

  foreach my $cfg_ ( @sorted_cfg_vec_ ) {
    $slot_to_pnl_vec_{ $_ } = undef foreach keys %{ $cfg_to_pnls_{ $cfg_ } };
  }

  foreach my $slot_ ( keys %slot_to_pnl_vec_ ) {
    my $date_ = substr $slot_, 0, 8;
    return if ! defined $date_to_intv_{ $date_ };

    my $pnl_vec_str_ = $slot_." ".$intv_to_wt_{$date_to_intv_{$date_}}." ";
    foreach my $cfg_ ( @sorted_cfg_vec_ ) {
      if ( defined $cfg_to_pnls_{ $cfg_ }{ $slot_ } ) {
        $pnl_vec_str_ .= $cfg_to_pnls_{ $cfg_ }{ $slot_ }." ";
      }
      else {
        $pnl_vec_str_ .= "NA ";
      }
    }
    $slot_to_pnl_vec_{ $slot_ } = $pnl_vec_str_;
  }

  open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
  print CSTF $slot_to_pnl_vec_{$_}."\n" foreach sort keys %slot_to_pnl_vec_;
  close CSTF;

  my $rcmd_ = "$MODELSCRIPTS_DIR/optimize_portfolio_sharpemax.R $cstempfile_";
  print $rcmd_."\n\n";
  my @outlines_ = `$rcmd_ 2>/dev/null`; chomp ( @outlines_ );

  my ($individ_sharpe_) = grep { $_ =~ /Individual sharpe/ } @outlines_;
  my ($combined_sharpe_) = grep { $_ =~ /Combined Sharpe/ } @outlines_;
  my ($weightline_) = grep { $_ =~ /Weights/ } @outlines_;
  my @weights_ = split(/\s+/, $weightline_);
  @weights_ = @weights_[ 1 .. $#weights_ ];

  if ( $#weights_ != $#sorted_cfg_vec_ ) {
    print "Error with the output of \"$rcmd_\"\n";
    return;
  }

  my %cfg_to_uts_ = ( );
  foreach my $idx ( 0 .. $#sorted_cfg_vec_ ) {
    $cfg_to_uts_{ $sorted_cfg_vec_[$idx] } = $weights_[$idx] * $cfg_to_orig_uts_{ $sorted_cfg_vec_[$idx] };
  }
  print $individ_sharpe_."\n";
  print $combined_sharpe_."\n";

  %cfg_to_optimal_frac_uts_ = ( );
  ScaleUTSandFindPerformance ( $total_risk_, \%cfg_to_uts_, \%cfg_to_optimal_frac_uts_, 0 );
  #print "Weights suggested: ".join(" ", map { sprintf("%.3f", $cfg_to_uts_{$_}) } @sorted_cfg_vec_)."\n"; 

  print "\nScaling UTS to meet the permitted Risk\n";
  %cfg_to_optimal_uts_ = ( );
  return ScaleUTSandFindPerformance ( $total_risk_, \%cfg_to_uts_, \%cfg_to_optimal_uts_, 1 );
}

# Scales the UTS-vector to reach close to the desired OverDaysDD
# Option to find integral UTS
# returns PNL_DD of the combined pnlseries
sub ScaleUTSandFindPerformance
{
  my $total_risk_ = shift;
  my $cfg_to_uts_ref_ = shift;
  my $cfg_to_scaled_uts_ref_ = shift;
  my $integral_uts_ = shift || 0;

  my %combined_pnl_series_ = ( );
  GetCombinedPnlSeries ( $cfg_to_uts_ref_, \%combined_pnl_series_ );

  my $overdays_dd_ = GetOverDaysDD ( \%combined_pnl_series_ );

  my $scale_ = $total_risk_ / $overdays_dd_;

  my %scaled_pnl_series_ = ( );
  
  if ( ! $integral_uts_ ) {
    %$cfg_to_scaled_uts_ref_ = map { $_ => $$cfg_to_uts_ref_{$_} * $scale_ } keys %$cfg_to_uts_ref_;
    %scaled_pnl_series_ = map { $_ => $combined_pnl_series_{$_} * $scale_ } keys %combined_pnl_series_;
  }
  else {
    my $total_pnlavg_ = 0;
    $total_pnlavg_ += ($$cfg_to_uts_ref_{$_} / $cfg_to_orig_uts_{$_}) * $cfg_to_stats_{$_}{"PNL"} foreach keys %$cfg_to_uts_ref_;

    my $total_pnlavg_scaled_ = $total_pnlavg_ * $scale_;

    print "Before Scaling: OverDaysDDPermitted: $total_risk_ CurrentOverDaysDD: $overdays_dd_ Scale: $scale_\n";

    my $current_pnl_ = 0;
    my $current_scaled_pnl_ = 0;
    foreach my $cfg_ ( keys %$cfg_to_uts_ref_ ) {
      if ( $total_pnlavg_ == $current_pnl_ ) {
        $$cfg_to_scaled_uts_ref_{ $cfg_ } = 0;
      }
      else {
        my $t_scale_ = ($total_pnlavg_scaled_ - $current_scaled_pnl_) / ($total_pnlavg_ - $current_pnl_); 

        $$cfg_to_scaled_uts_ref_{ $cfg_ } = int(0.5 + $$cfg_to_uts_ref_{$cfg_} * $t_scale_);

        $current_pnl_ += ($$cfg_to_uts_ref_{$cfg_} / $cfg_to_orig_uts_{$cfg_}) * $cfg_to_stats_{$cfg_}{"PNL"};
        $current_scaled_pnl_ += ($$cfg_to_scaled_uts_ref_{$cfg_} / $cfg_to_orig_uts_{$cfg_}) * $cfg_to_stats_{$cfg_}{"PNL"};
      }
    }
    GetCombinedPnlSeries ( $cfg_to_scaled_uts_ref_, \%scaled_pnl_series_ );
  }

  return FindPerformance ( \%scaled_pnl_series_ );
}

# combines pnl-series based on the given UTS_vec
sub GetCombinedPnlSeries
{
  my $cfg_to_uts_ref_ = shift;
  my $combined_pnl_series_ref_ = shift;

  foreach my $cfg_ ( keys %$cfg_to_uts_ref_ ) {
    my %t_pnl_series_ = ( );

    foreach my $slot_ ( keys %{ $cfg_to_pnls_{ $cfg_ } } ) {
      $t_pnl_series_{ $slot_ } = $cfg_to_pnls_{ $cfg_ }{ $slot_ } * ( $$cfg_to_uts_ref_{ $cfg_ } / $cfg_to_orig_uts_{ $cfg_ } );
    }

    CombinePnlSamples ( $combined_pnl_series_ref_, \%t_pnl_series_, $combined_pnl_series_ref_ );
   
    #print join(" ", map { $_.":".$t_pnl_series_{$_} } keys %t_pnl_series_ )."\n";
    #print join(" ", map { $_.":".$$combined_pnl_series_ref_{$_} } keys %$combined_pnl_series_ref_ )."\n";
  }
}

# returns PnlDD of the pnl-series
sub FindPerformance
{
  my $pnl_series_ref_ = shift;

  my $weighted_avgpnl_ = 0;
  my $weighted_avgdd_ = 0;

  foreach my $intv_ ( @intv_vec_ ) {
    my %t_pnl_stats_map_ = ( );
    PnlSamplesGetStats ( $pnl_series_ref_, \%t_pnl_stats_map_, $intv_to_dates_{ $intv_ } );
    $weighted_avgpnl_ += $intv_to_wt_{ $intv_ } * $t_pnl_stats_map_{"PNL"};
    $weighted_avgdd_ += $intv_to_wt_{ $intv_ } * $t_pnl_stats_map_{"DD_HIGHAVG"};
  }

  my $pnldd_ = $weighted_avgpnl_ / (1 + $weighted_avgdd_);
  return (int($weighted_avgpnl_), int($weighted_avgdd_), $pnldd_);
}

# returns Cumulative DD over-days of the pnl-series
sub GetOverDaysDD
{
  my $pnlsamples_series_ref_ = shift;

  my %dates2pnl_ = ( );
  foreach my $t_sample_ ( keys %$pnlsamples_series_ref_ ) {
    my ( $t_date_, $t_slot_ ) = split ( "_", $t_sample_ );
    if ( ! defined $dates2pnl_{ $t_date_ } ) {
      $dates2pnl_{ $t_date_ } = 0;
    }
    $dates2pnl_{ $t_date_ } += $$pnlsamples_series_ref_{ $t_sample_ };
  }

  my @pnl_series_ = map { $dates2pnl_{ $_ } } sort keys %dates2pnl_;

  my $cumulative_pnl_ = 0;
  my $max_pnl_ = 0;
  my $drawdown_ = 0;

  foreach my $tpnl_ ( @pnl_series_ ) {
    $cumulative_pnl_ += $tpnl_;

    $max_pnl_ = max( $max_pnl_, $cumulative_pnl_ );
    $drawdown_ = max( $drawdown_, ($max_pnl_ - $cumulative_pnl_) );
  }
  return $drawdown_;
}

