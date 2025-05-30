#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use List::Util qw/max min/; # for max
use POSIX;
use sigtrap qw(handler signal_handler normal-signals error-signals);
use File::Basename ;
use FileHandle;


my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $INSTALL_BIN = $HOME_DIR."/".$REPO."_install/bin";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $MODELING_STRATS_DIR = $MODELING_BASE_DIR."/strats";
my $GLOBALRESULTSDBDIR = "$HOME_DIR/ec2_globalresults";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage, GetStdev
require "$GENPERLLIB_DIR/pnl_samples_fetch.pl"; # FetchPnlSamplesStrats, FetchPnlDaysStrats, PnlSamplesGetStats, PnlSamplesGetStatsLong
require "$GENPERLLIB_DIR/performance_risk_helper.pl"; # GetPnlForTimeperiods WriteTradesToOrsFile GetLossFromLogFile ParseOrsPnlFile GetSessionTag GetSession

my $SHORTCODE_TIMEPERIOD_FILE = "/spare/local/tradeinfo/riskinfo/shortcode_tags_risk.txt";
my $SHORTCODE_LIST = "/spare/local/tradeinfo/riskinfo/risk_stats_shclist";

if ( $#ARGV < 0 ) {
  print "USAGE: <script> <session> [<date>] [<recompute?>]\n";
  exit(0);
}

my $argsession_ = shift;
my $current_date_ = shift || `date +%Y%m%d`; 
my $recompute_ = shift || 0;
chomp ( $current_date_ );

my $current_time_ = `date`; chomp($current_time_);
print $current_time_.": Starting Script\n";

my $hhmm_ = `date +%H%M`; chomp ( $hhmm_ );
if ( $hhmm_ < 2230 ) {
  $current_date_ = CalcPrevWorkingDateMult( $current_date_, 1 );
}

my @days_intv_ = (5, 20, 60);
my @risk_intervals_ = (20,60);
my $beta_ = 10;

#my $mail_address_ = "hrishav.agarwal\@tworoads.co.in";
my $mail_address_ = "nseall@tworoads.co.in";

my %shc_to_grp_map_ = ( );
my %shc_to_tperiod_map_ = ( );
my %shcgrp_to_exch_map_ = ( );
my %exchange_to_shclist_ = ( );
my %shc_tperiod_to_date_to_loss_ = ( );
my %shcgrp_tperiod_to_date_to_loss_ = ( );

my @exch_vec_ = ("EUREX", "LIFFE", "ICE" , "TMX", "CME", "BMF", "HKEX", "OSE", "RTS", "MICEX", "CFE", "NSE", "ASX");

open CFILE, "< $SHORTCODE_LIST" or PrintStacktraceAndDie ( "Could not open config file $SHORTCODE_LIST" );
my @shc_vec_ = <CFILE>; chomp( @shc_vec_ );
close CFILE;

open CFILE, "< $SHORTCODE_TIMEPERIOD_FILE" or PrintStacktraceAndDie ( "Could not open config file $SHORTCODE_TIMEPERIOD_FILE" );

foreach my $cline_ ( <CFILE> ) {
  my @tokens_ = split(" ", $cline_ );
  if ( $#tokens_ > 0 ) {
    my $shcgrp_ = $tokens_[0];
    my @tperiod_vec_ = @tokens_[1..$#tokens_];
    my @grp_shc_;
    if ( $shcgrp_ =~ /^DI1/ || $shcgrp_ =~ /^SP_/ ) {
      @grp_shc_ = grep { /^$shcgrp_/ } @shc_vec_;
    }
    else {
      @grp_shc_ = grep { /^$shcgrp_[_0-9]*$/ } @shc_vec_;
    }
    if ( $#grp_shc_ < 0 ) {
      push ( @grp_shc_, $shcgrp_ );
    }

    if ( !exists $shcgrp_to_exch_map_{ $shcgrp_ } ) {
      my $tshc_ = $grp_shc_[0];
      my $texch_ = `$HOME_DIR/basetrade_install/bin/get_contract_specs $tshc_ $current_date_ EXCHANGE 2>/dev/null | cut -d' ' -f2`;
      chomp( $texch_ );
      $shcgrp_to_exch_map_{ $shcgrp_ } = $texch_;
      if ( !exists $exchange_to_shclist_{ $texch_ } ) {
        @{ $exchange_to_shclist_{ $texch_ } } = ( );
      }
      push ( @{ $exchange_to_shclist_{ $texch_ } }, $shcgrp_ );
    }

    foreach my $shc_ ( @grp_shc_ ) {
      if ( ! exists $shc_to_grp_map_{ $shc_ } ) {
        $shc_to_tperiod_map_{ $shc_ } = \@tperiod_vec_;
        $shc_to_grp_map_{ $shc_ } = $shcgrp_;
      } else {
        print "Warning: $shc_ of group $shcgrp_ also exists in group ".$shc_to_grp_map_{ $shc_ }."\n";
      }
    }
  }
}
close CFILE;
@shc_vec_ = keys %shc_to_tperiod_map_;

my %shc_tperiod_to_pickstat_config_ = ( );
my %shc_tperiod_to_pool_vec_ = ( );

LoadMaxLossQueryIds ( );
ReadPoolFolders ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done Reading Pickstrat Configs\n";

my $longest_intv_buf_ = int( 1.5 * max( @days_intv_ ) );

my @days_vec_ = ( );

my $last_date_ = $current_date_;
foreach my $i ( 1..$longest_intv_buf_ ) {
  push( @days_vec_, $last_date_ );
  $last_date_ = CalcPrevWorkingDateMult( $last_date_, 1 ); 
}
print join(' ', @days_vec_)."\n";

my %shc_tperiod_to_day_to_pnl_map_ = ( );
my %exch_to_session_to_day_to_pnl_map_ = ( );

GetPnlForTimeperiods( \%shc_tperiod_to_pickstat_config_, \@days_vec_, \%shc_tperiod_to_day_to_pnl_map_, \%shc_tperiod_to_date_to_loss_, 0, 1 );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done GetPnlForTimeperiods:\n";

my %shcgrp_tperiod_to_day_to_pnl_map_ = ( );
my %shcgrp_tperiod_to_count_map_ = ( );
my %shcgrp_tperiod_to_interval_map_ = ( );
my %shcgrp_tperiod_to_pnl_per1kloss_map_ = ( );
my %shcgrp_tperiod_to_sharpe_map_ = ( );
my %shcgrp_tperiod_to_maxloss_map_ = ( );
my %shcgrp_tperiod_to_risk_map_ =  ( );
my %shcgrp_tperiod_to_pickstrat_risk_ = ( );

my %exch_to_sess_to_interval_map_ = ( );
my %exch_to_sess_to_sharpe_map_ = ( );
my %exch_to_sess_to_maxloss_map_ = ( );

my %shcgrp_tperiod_to_pool_avg_pnl_ = ( );
my %shcgrp_tperiod_to_pool_minpnl_ = ( );
my %shcgrp_tperiod_to_pool_sharpe_ = ( );

ComputeProductMetrics ( );
ComputeProductRisks ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done ComputeProductMetrics\n";

ComputeExchMetrics ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done ComputeExchMetrics\n";

GetPoolStatistics ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done GetPoolStatistics\n";

DumpStatsInMailBody ( 0 );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done DumpStatsInMailBody to STDOUT\n";

DumpStatsInMailBody ( 1 );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done DumpStatsInMailBody to MAIL\n";

sub LoadMaxLossQueryIds
{
  my $last_active_date_ = CalcPrevWorkingDateMult ( $current_date_, 10 );
  my %confignames_to_details_ = ( );

  GetActivePickstratConfigs ( $last_active_date_, \%confignames_to_details_ );

  my @valid_confignames_vec_ = ( );
  foreach my $shc_ ( keys %shc_to_tperiod_map_ ) {
    foreach my $tperiod_ ( @{ $shc_to_tperiod_map_{ $shc_ } } ) {
      push ( @valid_confignames_vec_, $shc_.".".$tperiod_.".txt" );
      if ( $shc_ eq "XT_0" || $shc_ eq "YT_0" ) {
        push ( @valid_confignames_vec_, $shc_."_expiry.".$tperiod_.".txt" );
      }
    }
  }

  my %valid_confignames_map_ = map { $_ => 1 } @valid_confignames_vec_;
  my @invalid_confignames_vec_ = grep { ! exists $valid_confignames_map_{ $_ } && $_ !~ /NSE_/ } keys %confignames_to_details_;

  foreach my $shc_ ( keys %shc_to_tperiod_map_ ) {
    foreach my $tperiod_ ( @{ $shc_to_tperiod_map_{ $shc_ } } ) {
      my $t_key_ = $shc_." ".$tperiod_;
      my $config_basename_ = $shc_.".".$tperiod_.".txt";
      my $config_basename_exp_ = $shc_."_expiry.".$tperiod_.".txt";

      my ($config_id_, $date_, $start_id_, $end_id_);
      my ($nqueries_, $global_maxloss_, $sum_maxlosses_, $computed_maxloss_);

      if ( exists $confignames_to_details_{ $config_basename_ } ) {
        ($config_id_, $date_, $start_id_, $end_id_) = @{ $confignames_to_details_{ $config_basename_ } };
      }

      if ( ($shc_ eq "XT_0" || $shc_ eq "YT_0") && exists $confignames_to_details_{ $config_basename_exp_ } ) {
        my ($tconfig_id_, $tdate_, $tstart_id_, $tend_id_) = @{ $confignames_to_details_{ $config_basename_exp_ } };
        if ( ! defined $date_ || $tdate_ > $date_ ) {
          print "Considering the Config $config_basename_exp_ with the same Query_start_id as that of Config $config_basename_\n";
          ($config_id_, $date_, $start_id_, $end_id_) = ($tconfig_id_, $tdate_, $tstart_id_, $tend_id_);
        }
      }
      
      ($date_, undef, $nqueries_, $global_maxloss_, $sum_maxlosses_, $computed_maxloss_) = GetLastPickstratRecord ($config_id_, $last_active_date_);
      
      if ( ! defined $date_ ) { 
        print "Warning: No Installation from config $config_basename_ recently\n";
        next; 
      }
      $end_id_ = ($start_id_ + 8) if ( ! defined $end_id_ || $end_id_ eq "" || $end_id_ < $start_id_ );

#      print "$t_key_: $config_basename_, $start_id_, $end_id_, ".min ( $global_maxloss_, $sum_maxlosses_ )."\n";

      $shc_tperiod_to_pickstat_config_{ $t_key_ }{ "NAME" } = $config_basename_;
      $shc_tperiod_to_pickstat_config_{ $t_key_ }{ "START_ID" } = $start_id_;
      $shc_tperiod_to_pickstat_config_{ $t_key_ }{ "END_ID" } = $end_id_;
      $shc_tperiod_to_pickstat_config_{ $t_key_ }{ "RISK" } = min ( $global_maxloss_, $sum_maxlosses_ );
    }
  }
}

sub ReadPoolFolders
{
  foreach my $shc_tp_ ( keys %shc_tperiod_to_pickstat_config_ ) {
    my ($shc_, $tperiod_) = split(' ', $shc_tp_);
    my $tconfig_basename_ = $shc_tperiod_to_pickstat_config_{ $shc_tp_ }{ "NAME" };
    my $tconfig_ = `ls $MODELING_BASE_DIR/pick_strats_config/\*/$tconfig_basename_ 2>/dev/null | head -1`;
    chomp( $tconfig_ );

    if ( $tconfig_ ne "" ) {
      my @pool_folders_ = `$SCRIPTS_DIR/get_config_field.pl $tconfig_ FOLDER_TIME_PERIODS | grep -v '^#'`;
      chomp( @pool_folders_ );
      $shc_tperiod_to_pool_vec_{ $shc_tp_ } = \@pool_folders_ if $#pool_folders_ >= 0;
    }
  }
}

sub ComputeProductMetrics
{
  foreach my $shc_tperiod_ ( keys %shc_tperiod_to_day_to_pnl_map_ ) {
    my ($shc_, $timeperiod_) = split(' ', $shc_tperiod_);
    my $shcgrp_tperiod_ = $shc_to_grp_map_{ $shc_ }." ".$timeperiod_;


    foreach my $day_ ( reverse sort keys %{ $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ } } ) {
      my $pnl_ = $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ };

      if ( ! exists $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ } ) {
        $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ } = 0;
      }
      $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ } += $pnl_;

      if ( defined $shc_tperiod_to_date_to_loss_{ $shc_tperiod_ }{ $day_ } ) {
        if ( ! defined $shcgrp_tperiod_to_date_to_loss_{ $shcgrp_tperiod_ }{ $day_ } ) {
          $shcgrp_tperiod_to_date_to_loss_{ $shcgrp_tperiod_ }{ $day_ } = 0;
        }
        $shcgrp_tperiod_to_date_to_loss_{ $shcgrp_tperiod_ }{ $day_ } += $shc_tperiod_to_date_to_loss_{ $shc_tperiod_ }{ $day_ };
      } else {
        print "Loss for ".$shc_tperiod_.", day:".$day_." could not be fetched\n";
      }
    }

    if ( ! defined $shcgrp_tperiod_to_pickstrat_risk_{ $shcgrp_tperiod_ } ) {
      $shcgrp_tperiod_to_pickstrat_risk_{ $shcgrp_tperiod_ } = 0;
    }
    if ( defined $shc_tperiod_to_pickstat_config_{ $shc_tperiod_ }{ "RISK" } ) { 
      $shcgrp_tperiod_to_pickstrat_risk_{ $shcgrp_tperiod_ } += $shc_tperiod_to_pickstat_config_{ $shc_tperiod_ }{ "RISK" };
    }
  }

  foreach my $shcgrp_tperiod_ ( keys %shcgrp_tperiod_to_day_to_pnl_map_ ) {
    my ($shcgrp_, $timeperiod_) = split(' ', $shcgrp_tperiod_);

    foreach my $day_ ( reverse sort keys %{ $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ } } ) {
      print $shcgrp_tperiod_." ".$day_." ".$shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ }." ".$shcgrp_tperiod_to_date_to_loss_{ $shcgrp_tperiod_ }{ $day_ }."\n";
    }
  }

  foreach my $intv_ ( @days_intv_ ) {
    foreach my $shc_tperiod_ ( keys %shcgrp_tperiod_to_day_to_pnl_map_ ) {
      my @pnl_series_ = ( );
      my @pnl_per_1kloss_series_ = ( );
      my $count_ = 0;
      my $total_count_ = 0;
      foreach my $day_ ( @days_vec_ ) {
        if ( exists $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ } ) {
          push ( @pnl_series_, $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ } );

          my $pnl_1kloss_ = 0;
          if ( $shcgrp_tperiod_to_date_to_loss_{ $shc_tperiod_ }{ $day_ } != 0 ) {
            $pnl_1kloss_ = $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ } * 1000.0 / $shcgrp_tperiod_to_date_to_loss_{ $shc_tperiod_ }{ $day_ };
          }
          push ( @pnl_per_1kloss_series_, $pnl_1kloss_ );

          $count_++;
          if ( $count_ >= $intv_ ) { last; }
        }
        $total_count_++;
        if ( $total_count_ > 1.5 * $intv_ ) { last; }
      }
      if ( $count_ < 0.5 * $intv_ ) {
        print "Skipping $shc_tperiod_ for $intv_\n";
        next;
      }

      my $pnl_avg_ = GetAverage(\@pnl_series_) ;
      my $pnl_1kloss_avg_ = GetAverage ( \@pnl_per_1kloss_series_ );

      $shcgrp_tperiod_to_count_map_{ $shc_tperiod_ }{ $intv_ } = $count_ ;
      $shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ } = int( 0.5 + $pnl_avg_ );

      $shcgrp_tperiod_to_pnl_per1kloss_map_{ $shc_tperiod_ }{ $intv_ } = int( 0.5 + $pnl_1kloss_avg_ );
      $shcgrp_tperiod_to_sharpe_map_{ $shc_tperiod_ }{ $intv_ } = $pnl_1kloss_avg_ / GetStdev(\@pnl_per_1kloss_series_) ;
      $shcgrp_tperiod_to_maxloss_map_{ $shc_tperiod_ }{ $intv_ } = int( 0.5 + min(0, min(@pnl_series_) ) );

      print $shc_tperiod_." ".$intv_." ".$shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ }." ".$shcgrp_tperiod_to_pnl_per1kloss_map_{ $shc_tperiod_ }{ $intv_ }." ".$shcgrp_tperiod_to_sharpe_map_{ $shc_tperiod_ }{ $intv_ }."\n";
    }
  }
}

sub ComputeProductRisks
{
  foreach my $shc_tperiod_ ( keys %shcgrp_tperiod_to_day_to_pnl_map_ ) {
    my $trisk_ = GetRiskFromPnlSeries ( $shc_tperiod_, $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }, \@days_vec_ );
    if ( $trisk_ > 0 ) {
      $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ } = $trisk_;
    }
  }
}

sub ComputeExchMetrics
{
  %exch_to_session_to_day_to_pnl_map_ = ( );
  foreach my $shc_tperiod_ ( keys %shc_tperiod_to_day_to_pnl_map_ ) {
    my ($shc_, $timeperiod_) = split(' ', $shc_tperiod_);
    my $shcgrp_ = $shc_to_grp_map_{ $shc_ };
    my $exch_ = $shcgrp_to_exch_map_ { $shcgrp_ };
    my $session_ = GetSessionTag( $timeperiod_, $shcgrp_ );

    foreach my $day_ ( reverse sort keys %{ $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ } } ) {
      my $pnl_ = $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ };

      if ( ! exists $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } ) {
        $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } = 0;
      }

      $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } += $pnl_;
    }
  }

  foreach my $exch_ ( keys %exch_to_session_to_day_to_pnl_map_ ) {
    foreach my $session_ ( keys %{ $exch_to_session_to_day_to_pnl_map_{ $exch_ } } ) {
      foreach my $day_ ( reverse sort keys %{ $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ } } ) {
        print $exch_." ".$session_." ".$day_." ".$exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ }."\n";
      }
    }
  }

  foreach my $intv_ ( @days_intv_ ) {
    foreach my $exch_ ( keys %exch_to_session_to_day_to_pnl_map_ ) {
      foreach my $session_ ( keys %{ $exch_to_session_to_day_to_pnl_map_{ $exch_ } } ) {
        my @pnl_series_ = ( );
        my $count_ = 0;
        my $total_count_ = 0;
        foreach my $day_ ( @days_vec_ ) {
          if ( exists $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } ) {
            push ( @pnl_series_, $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } );
            $count_++;

            if ( $count_ >= $intv_ ) {
              last;
            }
          }
          $total_count_++;
          if ( $total_count_ > 1.5 * $intv_ ) {
            last;
          }
        }
        if ( $count_ < 0.5 * $intv_ ) {
          print "Skipping $exch_ $session_ for $intv_\n";
          next;
        }

        my $pnl_avg_ = GetAverage(\@pnl_series_) ;
        $exch_to_sess_to_interval_map_{ $exch_ }{ $session_ }{ $intv_ } = $pnl_avg_;
        $exch_to_sess_to_sharpe_map_{ $exch_ }{ $session_ }{ $intv_ } = $pnl_avg_ / GetStdev(\@pnl_series_);
        $exch_to_sess_to_maxloss_map_{ $exch_ }{ $session_ }{ $intv_ } = min(0, min(@pnl_series_) );

        print $exch_." ".$session_." ".$intv_." ".$exch_to_sess_to_interval_map_{ $exch_ }{ $session_ }{ $intv_ }." ".$exch_to_sess_to_sharpe_map_{ $exch_ }{ $session_ }{ $intv_ }."\n";
      }
    }
  }
}

sub ChooseMaxLoss
{
  my $pnl_series_ref_ = shift;
  my $hit_ratio_ = shift || 0.07;

  my %pnl_stats_ = ( );
  PnlSamplesGetStatsLong ( $pnl_series_ref_, \%pnl_stats_ );

  my @min_pnl_list_ = @{$pnl_stats_{ "MINPNL" }};
  my @final_pnl_list_ = @{$pnl_stats_{ "PNL" }};

  my @max_losses_to_try_ = sort { $a <=> $b } @min_pnl_list_;
  my $num_hits_ = max(1, int(0.5 + $hit_ratio_ * ($#max_losses_to_try_ + 1)));

  my $max_loss_ = $max_losses_to_try_ [ $num_hits_ - 1 ];
  return $max_loss_;

  my $sum_pnl_ = 0;
  foreach my $i ( 0..$#min_pnl_list_ ) {
    if( $min_pnl_list_[ $i ] > $max_loss_ ) {
      $sum_pnl_ += $final_pnl_list_[ $i ];
    }
    else{
      $sum_pnl_ += $max_loss_;
    }
  }
  my $avg_pnl_ = $sum_pnl_ / ($#min_pnl_list_+1);
}

sub GetPoolStatistics {
  my $numdays_ = max( @days_intv_, 80 );
  my $sort_algo_ = "kCNAPnlSharpeAverage";
  my $top_nstrats_ = 5;

  foreach my $shcgrp_tperiod_ ( keys %shcgrp_tperiod_to_risk_map_ ) {
    my ($shcgrp_, $timeperiod_) = split(' ', $shcgrp_tperiod_);
    my @shc_vec_ = grep { $shc_to_grp_map_{ $_ } eq $shcgrp_ } keys %shc_to_grp_map_;
    if ( $#shc_vec_ < 0 ) {
      @shc_vec_ = ( $shcgrp_ );
    }

    foreach my $shc_ ( @shc_vec_ ) {
      my $shc_tperiod_ = $shc_." ".$timeperiod_;
      if ( exists $shc_tperiod_to_pool_vec_{ $shc_tperiod_ } ) {
        my @pool_vec_ = @{ $shc_tperiod_to_pool_vec_{ $shc_tperiod_ } };
        print "Fetching Pool Statistics for $shc_tperiod_; pools: ".join(" ", @pool_vec_)."\n";

        my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";
        my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $shc_ $current_date_ $numdays_";
        my $exec_output_ = `$exec_cmd_`; chomp ( $exec_output_ );
        my @dates_vec_ = split ( ' ', $exec_output_ ); chomp ( @dates_vec_ );

        my @top_strats_ = ( );
        foreach my $pool_ ( @pool_vec_ ) {
          my @t_top_strats_ = ( );
          GetTopStrats ( $shc_, $pool_, $numdays_, $sort_algo_, $top_nstrats_, \@t_top_strats_ );
          push ( @top_strats_, @t_top_strats_ );
        }

        my %sample_pnls_strats_vec_;
        FetchPnlSamplesStrats ( $shc_, \@top_strats_, \@dates_vec_, \%sample_pnls_strats_vec_ );

        my %samples_map_ = ( );
        my $total_uts_ = 0;
        foreach my $tstrat_ ( @top_strats_ ) {
          if ( ! %samples_map_ ) {
            %samples_map_ = %{ $sample_pnls_strats_vec_{ $tstrat_ } };
          } else {
            CombinePnlSamples ( \%samples_map_, \%{ $sample_pnls_strats_vec_{ $tstrat_ } }, \%samples_map_ );
          }
          my $exec_cmd_ = $INSTALL_BIN."/get_UTS_for_a_day ".$shc_." ".$tstrat_." ".$current_date_;
          my $uts_ = `$exec_cmd_`; chomp ( $uts_ );
          $total_uts_ += $uts_;
        }

        my $max_loss_ = ChooseMaxLoss ( \%samples_map_ );
        $max_loss_ = abs( $max_loss_ );
        print "Maxloss for $shcgrp_tperiod_ is $max_loss_\n";

        foreach my $intv_ ( @days_intv_ ) {
          my $num_dates_ = min( $intv_-1, $#dates_vec_ );
          my @this_dates_vec_ = @dates_vec_[ 0..$num_dates_ ];

          my @intv_top_strats_ = ( );
          foreach my $pool_ ( @pool_vec_ ) {
            my @t_top_strats_ = ( );
            GetTopStrats ( $shc_, $pool_, $intv_, $sort_algo_, $top_nstrats_, \@t_top_strats_ );
            push ( @intv_top_strats_, @t_top_strats_ );
          }

          %sample_pnls_strats_vec_ = ( );
          FetchPnlSamplesStrats ( $shc_, \@intv_top_strats_, \@this_dates_vec_, \%sample_pnls_strats_vec_ );

          %samples_map_ = ( );
          foreach my $tstrat_ ( @intv_top_strats_ ) {
            if ( exists $sample_pnls_strats_vec_{ $tstrat_ } ) {
              if ( ! %samples_map_ ) {
                %samples_map_ = %{ $sample_pnls_strats_vec_{ $tstrat_ } };
              } else {
                CombinePnlSamples ( \%samples_map_, \%{ $sample_pnls_strats_vec_{ $tstrat_ } }, \%samples_map_ );
              }
            }
          }

          if ( %samples_map_ ) {
            my %pnl_stats_ = ( );
            PnlSamplesGetStats( \%samples_map_, \%pnl_stats_, \@this_dates_vec_);

            if ( $max_loss_ > 0 ) {
              $shcgrp_tperiod_to_pool_avg_pnl_{ $shcgrp_tperiod_ }{ $shc_ }{ $intv_ } = int( 0.5 + $pnl_stats_{ "PNL" } * 1000 / $max_loss_ );
              $shcgrp_tperiod_to_pool_minpnl_{ $shcgrp_tperiod_ }{ $shc_ }{ $intv_ } = int( 0.5 + $pnl_stats_{ "MINPNL" } * 1000 / $max_loss_ );
            }
            else {
              $shcgrp_tperiod_to_pool_avg_pnl_{ $shcgrp_tperiod_ }{ $shc_ }{ $intv_ } = int( 0.5 + $pnl_stats_{ "PNL" } );
              $shcgrp_tperiod_to_pool_minpnl_{ $shcgrp_tperiod_ }{ $shc_ }{ $intv_ } = int( 0.5 + $pnl_stats_{ "MINPNL" } );
            }
            $shcgrp_tperiod_to_pool_sharpe_{ $shcgrp_tperiod_ }{ $shc_ }{ $intv_ } = $pnl_stats_{ "DAILY_SHARPE" };
          }
        }
      }
    }
  }
}

sub GetTopStrats
{
  my $shortcode_ = shift;
  my $timeperiod_ = shift;
  my $numdays_ = shift;
  my $sort_algo_ = shift;
  my $topn_ = shift || 5;
  my $top_strats_ref_ = shift;

  my $start_date_ = CalcPrevWorkingDateMult ( $current_date_, $numdays_ );
  my $strats_dir_ = "$MODELING_STRATS_DIR/$shortcode_/$timeperiod_";

  my $exec_cmd_ = "$INSTALL_BIN/summarize_strategy_results $shortcode_ $strats_dir_ $GLOBALRESULTSDBDIR $start_date_ $current_date_ INVALIDFILE $sort_algo_ 0 IF 1";

  print $exec_cmd_."\n";
  my @ssr_output_ = `$exec_cmd_`;
  chomp( @ssr_output_ );

  if ( $#ssr_output_ < 0 ) { return; }

  my @strats_sorted_ = map { (split(' ', $_))[1] } @ssr_output_;
  my $nstrats_to_pick_ = min( $topn_, $#strats_sorted_+1 );

  @$top_strats_ref_ = @strats_sorted_[ 0..($nstrats_to_pick_-1) ];
}

sub DumpStatsInMailBody {
  my $sendmail_ = shift;

  my @col_headers_ = qw( SHORTCODE_GRP TIMEPERIOD );
  my @intv_subheaders_ = qw( NDAYS PNL_AVG PER1KLOSS SHARPE MIN_PNL );
#  my @bg_cols_real_ = ("#ecf8f7", "#cbece8", "#b5e3de", "#8fd6cd");
  my @bg_cols_real_ = ("#d2f6de", "#cbece8", "#d2f6de", "#cbece8");
  #my @bg_cols_pool_ = ("#eafbef", "#d2f6de", "#aaeec1", "#7fe6a1");
  my @bg_cols_pool_ = ("#d0e3f6", "#d0e3f6", "#d0e3f6", "#d0e3f6");
  my $pnl_col_ = "#2b00b3";
  my $empty_col_ = "#ffffbb";
  my $pool_tag_col_ = "#d0e3f6";
  
  my $risk_bg_col_ = "#aaeec1";
  my $risk_col_high_ = "#8b0000";
  my $risk_col_low_ = "#006600";
  my $risk_col_mid_ = "#00008b";

  my $ncols_per_intv_ = scalar @intv_subheaders_;

  my %session_to_shc_to_tp_ = ( );
  my %tperiod_to_session_ = ( );

  foreach my $shc_tperiod_ ( keys %shcgrp_tperiod_to_risk_map_ ) {
    my ($shc_, $timeperiod_) = split(' ', $shc_tperiod_);
    if ( ! exists $tperiod_to_session_{ $timeperiod_ } ) {
      $tperiod_to_session_{ $timeperiod_ } = GetSessionTag ( $timeperiod_, $shc_ );
    }
    my $session_ = $tperiod_to_session_{ $timeperiod_ };

    if ( ! exists $session_to_shc_to_tp_{ $session_ }{ $shc_ } ) {
      @{ $session_to_shc_to_tp_{ $session_ }{ $shc_ } } = ( );
    }
    push ( @{ $session_to_shc_to_tp_{ $session_ }{ $shc_ } }, $timeperiod_ );
  }

  print "SendMail: ".$sendmail_."\n";
  my $mailhandle_;
  if ( $sendmail_ ) {
    open ( $mailhandle_ , "|/usr/sbin/sendmail -t" );

    my $date_ = `date +%Y%m%d`; chomp ( $date_ );

    print $mailhandle_ "To: $mail_address_\n";
    print $mailhandle_ "From: $mail_address_\n";
    printf $mailhandle_ "Subject: %s Performance Summary %s\n", $argsession_, $date_;
    print $mailhandle_ "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n";
  }
  else {
    $mailhandle_ = *STDOUT;
  }

  print $mailhandle_ "<html><body>\n";

  print $mailhandle_ "<table border = \"2\"><tr>" ;

  foreach my $elem_str_ ( @col_headers_ ) {
    printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $elem_str_;
  }
  foreach my $elem_str_ ( @days_intv_ ) {
    printf $mailhandle_ "<td align=center colspan=\"%d\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $ncols_per_intv_, "INTV: ".$elem_str_;
  }
  printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", "SUGGESTED_RISK";
  printf $mailhandle_ "</tr>\n";
  my $ncols_ = (scalar @col_headers_) + (scalar @days_intv_)*$ncols_per_intv_ + 1;


  printf $mailhandle_ "<tr>";
  foreach my $intv_ ( @days_intv_ ) {
    foreach my $elem_str_ ( @intv_subheaders_ ) {
      printf $mailhandle_ "<td align=center><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $elem_str_;
    }
  }
  printf $mailhandle_ "</tr>";

  my $session_ = $argsession_;
  {
    printf $mailhandle_ "<tr>";
    printf $mailhandle_ "<td align=center colspan=\"%d\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $ncols_, "SESSION: ".$session_;
    printf $mailhandle_ "</tr>";

    foreach my $texch_ ( keys %exchange_to_shclist_ ) {
      foreach my $shc_ ( sort @{ $exchange_to_shclist_{ $texch_ } } ) {
        if ( ! exists $session_to_shc_to_tp_{ $session_ }{ $shc_ } ) {
          next;
        }

        my @tp_vec_ = @{ $session_to_shc_to_tp_{ $session_ }{ $shc_ } };

## Detemine which timeperiod to print the Pool Statistics for
## Currently, only print when the shcgrp_ in consideration is a single shortcode (NOT a group)

        my %tperiod_to_pool_ = ( );
        foreach my $timeperiod_ ( @tp_vec_ ) {
          my $shcgrp_tperiod_ = $shc_." ".$timeperiod_;
          if ( exists $shcgrp_tperiod_to_pool_avg_pnl_{ $shcgrp_tperiod_ } ) {
            my @comp_shc_vec_ = keys %{ $shcgrp_tperiod_to_pool_avg_pnl_{ $shcgrp_tperiod_ } };

            if ( $#comp_shc_vec_ == 0 ) {
              $tperiod_to_pool_{ $timeperiod_ } = $comp_shc_vec_[0];
            }
          }
        }
        my @tp_pool_vec_ = keys %tperiod_to_pool_;

        foreach my $timeperiod_ ( @tp_vec_ ) {
          my $shc_tperiod_ = $shc_." ".$timeperiod_;

          if ( exists $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ } ) {
            printf $mailhandle_ "<tr>";
            if ( $timeperiod_ eq $tp_vec_[0] ) {
              printf $mailhandle_ "<td align=center rowspan=\"%d\">%s</td>", (scalar @tp_vec_) + (scalar @tp_pool_vec_), $shc_;
            }
            my $t_nrows_ = 1;
            if ( exists $tperiod_to_pool_{ $timeperiod_ } ) { $t_nrows_++; }

            printf $mailhandle_ "<td align=center rowspan=\"%d\">%s</td>", $t_nrows_, $timeperiod_;

            foreach my $intv_idx_ ( 0..$#days_intv_ ) {
              my $intv_ = $days_intv_[ $intv_idx_ ];
              my $bg_col_ = $bg_cols_real_[ min($#bg_cols_real_, $intv_idx_) ];

              if ( exists $shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ } ) {
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>", $bg_col_, $pnl_col_, $shcgrp_tperiod_to_count_map_{ $shc_tperiod_ }{ $intv_ };
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>", $bg_col_, $pnl_col_, $shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ };

                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>", $bg_col_, $pnl_col_, $shcgrp_tperiod_to_pnl_per1kloss_map_{ $shc_tperiod_ }{ $intv_ };
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%.2f</td>", $bg_col_, $pnl_col_, $shcgrp_tperiod_to_sharpe_map_{ $shc_tperiod_ }{ $intv_ };
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>\n", $bg_col_, $pnl_col_, $shcgrp_tperiod_to_maxloss_map_{ $shc_tperiod_ }{ $intv_ };
              }
              else {
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>\n", $empty_col_, $pnl_col_;
              }
            }

            printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>", $risk_bg_col_, $risk_col_mid_, int( 0.5 + $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ } );

            printf $mailhandle_ "</tr>\n";

            if ( exists $tperiod_to_pool_{ $timeperiod_ } ) { 
              printf $mailhandle_ "<tr>";
              foreach my $intv_idx_ ( 0..$#days_intv_ ) {
                my $intv_ = $days_intv_[ $intv_idx_ ];
                my $bg_col_ = $bg_cols_pool_[ min($#bg_cols_pool_, $intv_idx_) ];

                printf $mailhandle_ "<td align=center colspan=\"2\" bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">POOL</td>", $pool_tag_col_, $pnl_col_;
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>", $bg_col_, $pnl_col_, $shcgrp_tperiod_to_pool_avg_pnl_{ $shc_tperiod_ }{ $tperiod_to_pool_{ $timeperiod_ } }{ $intv_ };
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%.2f</td>", $bg_col_, $pnl_col_, $shcgrp_tperiod_to_pool_sharpe_{ $shc_tperiod_ }{ $tperiod_to_pool_{ $timeperiod_ } }{ $intv_ };
                printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>\n", $bg_col_, $pnl_col_, $shcgrp_tperiod_to_pool_minpnl_{ $shc_tperiod_ }{ $tperiod_to_pool_{ $timeperiod_ } }{ $intv_ };
              }
              printf $mailhandle_ "</tr>\n";
            }
          }
        }
      }
    }
  }
  printf $mailhandle_ "<tr><td colspan=\"%d\"></td></tr>", $ncols_;
  print $mailhandle_ "</table>";

  print $mailhandle_ "<table border = \"2\"><tr>" ;

  printf $mailhandle_ "<tr><td align=center colspan=\"%d\"><font font-weight = \"bold\" size = \"2\" color=darkblue>EXCHANGES</font></td></tr>", (scalar @days_intv_)*3 + 2;

  printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>EXCHANGE</font></td>";
  
  foreach my $elem_str_ ( @days_intv_ ) {
    printf $mailhandle_ "<td align=center colspan=\"3\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", "INTV: ".$elem_str_;
  }
  printf $mailhandle_ "</tr>\n";

  foreach my $intv_ ( @days_intv_ ) {
    foreach my $elem_str_ ( qw( PNL_AVG SHARPE MIN_PNL ) ) {
      printf $mailhandle_ "<td align=center><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $elem_str_;
    }
  }
  printf $mailhandle_ "</tr>";

  foreach my $exch_ ( keys %exch_to_session_to_day_to_pnl_map_ ) {
    if ( exists $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $argsession_ } ) {
      my $session_ = $argsession_;
      print $mailhandle_ "<tr>";
      printf $mailhandle_ "<td align=center>%s</td>", $exch_;

      foreach my $intv_idx_ ( 0..$#days_intv_ ) {
        my $intv_ = $days_intv_[ $intv_idx_ ];
        my $bg_col_ = $bg_cols_real_[ min($#bg_cols_real_, $intv_idx_) ];
        
        if ( exists $exch_to_sess_to_interval_map_{ $exch_ }{ $session_ }{ $intv_ } ) {
          my $intv_avg_pnl_ = int( 0.5 + $exch_to_sess_to_interval_map_{ $exch_ }{ $session_ }{ $intv_ } );
          my $intv_sharpe_ = $exch_to_sess_to_sharpe_map_{ $exch_ }{ $session_ }{ $intv_ };
          my $intv_min_pnl_ = int( 0.5 + $exch_to_sess_to_maxloss_map_{ $exch_ }{ $session_ }{ $intv_ } );

          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>", $bg_col_, $pnl_col_, $intv_avg_pnl_;
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%.2f</td>", $bg_col_, $pnl_col_, $intv_sharpe_;
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>\n", $bg_col_, $pnl_col_, $intv_min_pnl_;
        }
        else {
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>\n", $empty_col_, $pnl_col_;
        }
      }
      print $mailhandle_ "</tr>\n";
    }
  }
  print $mailhandle_ "</body></html>\n";

  if ( $sendmail_ ) {
    close ( $mailhandle_ );
  }
}


