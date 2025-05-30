#!/usr/bin/perl
use strict ;
use warnings ;
use feature "switch"; # for given, when
use List::Util qw/max min/; # for max
use POSIX;
use sigtrap qw(handler signal_handler normal-signals error-signals);
use File::Basename ;
use FileHandle;
use Mojolicious::Lite;
use Mojo::JSON qw(decode_json encode_json);

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $INSTALL_BIN = $HOME_DIR."/".$REPO."_install/bin";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage, GetStdev
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # GetActivePickstratConfigs, GetLastPickstratRecord
require "$GENPERLLIB_DIR/performance_risk_helper.pl"; # GetPnlForTimeperiods WriteTradesToOrsFile GetLossFromLogFile ParseOrsPnlFile GetSessionTag GetSession
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

my $SHORTCODE_TIMEPERIOD_FILE = "/spare/local/tradeinfo/riskinfo/shortcode_tags_risk.txt";
my $SHORTCODE_LIST = "/spare/local/tradeinfo/riskinfo/risk_stats_shclist";

if ( $#ARGV < 0 ) {
  print "USAGE: <script> <date(TODAY)>  [dump_json_] [<recompute (0:no_recompute, 1:recompute_on_null_values, 2:recompute_all)>]\n";
  exit(0);
}

my $current_date_ = GetIsoDateFromStrMin1 ( shift );
my $dump_stats_in_json_ = shift || 0;
my $recompute_ = shift || 0;
my $send_simula_link_ = 1;

my $current_time_ = `date`; chomp($current_time_);
print $current_time_.": Starting Script\n";

my $today_ = `date +%Y%m%d`; chomp( $today_ );
my $hhmm_ = `date +%H%M`; chomp ( $hhmm_ );
if ( $hhmm_ < 2230 && $current_date_ eq $today_) {
  $current_date_ = CalcPrevWorkingDateMult( $current_date_, 1 );
}

my @days_intv_ = (20, 60, "YTD");
my @days_intvs_num_ = grep { $_ !~ /YTD/ } @days_intv_;

my @risk_intervals_ = (20, 60, "YTD");
my $beta_ = 10;

#my $mail_address_ = "hrishav.agarwal\@tworoads.co.in";
my $mail_address_ = "nseall@tworoads.co.in";
my $risk_managers_mail_address_ = "puru.sarthy\@circulumvite.com, hrishav.agarwal\@tworoads.co.in";

my @invalid_confignames_vec_ = ( );

my %shc_to_grp_map_ = ( );
my %shcgrp_to_shc_map_ = ( );
my %shc_to_tperiod_map_ = ( );
my %shcgrp_to_exch_map_ = ( );
my %exchange_to_shclist_ = ( );

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

    if ( ! defined $shcgrp_to_exch_map_{ $shcgrp_ } ) {
      my $tshc_ = $grp_shc_[0];
      my $texch_ = `$HOME_DIR/basetrade_install/bin/get_contract_specs $tshc_ $current_date_ EXCHANGE | cut -d' ' -f2`;
      chomp( $texch_ );
      $shcgrp_to_exch_map_{ $shcgrp_ } = $texch_;
      if ( !exists $exchange_to_shclist_{ $texch_ } ) {
        @{ $exchange_to_shclist_{ $texch_ } } = ( );
      }
      push ( @{ $exchange_to_shclist_{ $texch_ } }, $shcgrp_ );
    }
    $shcgrp_to_shc_map_{ $shcgrp_ } = \@grp_shc_;

    foreach my $shc_ ( @grp_shc_ ) {
      if ( ! defined $shc_to_grp_map_{ $shc_ } ) {
        $shc_to_tperiod_map_{ $shc_ } = \@tperiod_vec_;
        $shc_to_grp_map_{ $shc_ } = $shcgrp_;
      } else {
        print "Warning: $shc_ of group $shcgrp_ also exists in group ".$shc_to_grp_map_{ $shc_ }."\n";
      }
    }
  }
}
close CFILE;

my %shc_tperiod_to_pickstat_config_ = ( );
my %shc_tperiod_to_live_ = ( );
LoadMaxLossQueryIds ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done Reading Pickstrat Configs\n";

my %shcgrp_to_trader_map_ = ( );
LoadProductToTraderMap ( \%exchange_to_shclist_, \%shcgrp_to_exch_map_, \%shcgrp_to_trader_map_ );
foreach my $shcgrp_ ( keys %shcgrp_to_trader_map_ ) {
  print join("\n", map { $_." ".$shcgrp_to_trader_map_{$shcgrp_}{$_} } keys %{$shcgrp_to_trader_map_{$shcgrp_}})."\n";
}

my $yoy_numdays_;
if ( FindItemFromVec ( "YTD", @days_intv_ ) ) {
  $yoy_numdays_ = 0;
  my $last_date_ = $current_date_;
  my $curr_yyyy_ = substr($current_date_,0,4);

  while ( substr($last_date_,0,4) eq $curr_yyyy_ ) {
    $last_date_ = CalcPrevWorkingDateMult( $last_date_, 1 );
    $yoy_numdays_++;
  }
}

my $longest_intv_buf_ = int( 1.5 * max( @days_intvs_num_ ) );
if ( $longest_intv_buf_ < $yoy_numdays_ ) { $longest_intv_buf_ = $yoy_numdays_; }

my @days_vec_ = ( );

my $last_date_ = $current_date_;
foreach my $i ( 1..$longest_intv_buf_ ) {
  push( @days_vec_, $last_date_ );
  $last_date_ = CalcPrevWorkingDateMult( $last_date_, 1 ); 
}
print join(' ', @days_vec_)."\n";

my %shc_tperiod_to_day_to_pnl_map_ = ( );
my %exch_to_session_to_day_to_pnl_map_ = ( );

GetPnlForTimeperiods( \%shc_tperiod_to_pickstat_config_, \@days_vec_, \%shc_tperiod_to_day_to_pnl_map_, undef, $recompute_, 1 );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done GetPnlForTimeperiods:\n";

my %shcgrp_tperiod_to_day_to_pnl_map_ = ( );

my %shcgrp_tperiod_to_count_map_ = ( );
my %shcgrp_tperiod_to_interval_map_ = ( );
my %shcgrp_tperiod_to_sharpe_map_ = ( );
my %shcgrp_tperiod_to_maxloss_map_ = ( );
my %shcgrp_tperiod_to_risk_map_ =  ( );
my %shcgrp_tperiod_to_pickstrat_risk_ = ( );
my %shcgrp_tperiod_to_drawdown_map_ = ( );
my %shcgrp_tperiod_to_live_ = ( );

my %shc_tperiod_to_count_map_ = ( );
my %shc_tperiod_to_interval_map_ = ( );
my %shc_tperiod_to_sharpe_map_ = ( );
my %shc_tperiod_to_maxloss_map_ = ( );
my %shc_tperiod_to_risk_map_ =  ( );
my %shc_tperiod_to_drawdown_map_ = ( );

my %exch_to_sess_to_interval_map_ = ( );
my %exch_to_sess_to_sharpe_map_ = ( );
my %exch_to_sess_to_maxloss_map_ = ( );
my %exch_to_sess_to_risk_map_ = ( );

ComputeProductMetrics ( );
ComputeProductRisks ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done ComputeProductMetrics\n";

ComputeExchMetrics ( );
ComputeExchRisks ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done ComputeExchMetrics\n";

if ( $dump_stats_in_json_ ) {
  my $json_file_ = "/media/shared/ephemeral16/RiskStats.js";
  DumpStatsInJson ( $json_file_ );
  $current_time_ = `date`; chomp($current_time_);
  print $current_time_.": Done DumpStatsInJson to $json_file_\n";
}

#DumpStatsInMailBody ( 0 );
#$current_time_ = `date`; chomp($current_time_);
#print $current_time_.": Done DumpStatsInMailBody to STDOUT\n";

DumpStatsInMailBody ( 1 );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done DumpStatsInMailBody to MAIL\n";

sub LoadMaxLossQueryIds
{
  my $last_active_date_ = CalcPrevWorkingDateMult ( $current_date_, 40 );
  my %confignames_to_details_ = ( );

  GetActivePickstratConfigs ( $last_active_date_, \%confignames_to_details_ );

  my @valid_confignames_vec_ = ( );
  foreach my $shc_ ( keys %shc_to_tperiod_map_ ) {
    foreach my $tperiod_ ( @{ $shc_to_tperiod_map_{ $shc_ } } ) {
      push ( @valid_confignames_vec_, $shc_.".".$tperiod_.".txt" );
    }
  }

  my %valid_confignames_map_ = map { $_ => 1 } @valid_confignames_vec_;
  @invalid_confignames_vec_ = grep { ! exists $valid_confignames_map_{ $_ } && $_ !~ /NSE_/ && $_ !~ /EBT/ } keys %confignames_to_details_;

  foreach my $shc_ ( keys %shc_to_tperiod_map_ ) {
    foreach my $tperiod_ ( @{ $shc_to_tperiod_map_{ $shc_ } } ) {
      my $t_key_ = $shc_." ".$tperiod_;
      my $config_basename_ = $shc_.".".$tperiod_.".txt";

      my ($config_id_, $date_, $start_id_, $end_id_);
      my ($nqueries_, $global_maxloss_, $sum_maxlosses_, $computed_maxloss_);

      if ( exists $confignames_to_details_{ $config_basename_ } ) {
        ($config_id_, $date_, $start_id_, $end_id_) = @{ $confignames_to_details_{ $config_basename_ } };
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

      my @query_id_vec_ = $start_id_..$end_id_;
      $shc_tperiod_to_live_{ $t_key_ } = IsRunningProduct ( $shc_, \@query_id_vec_, $current_date_ );
    }
  }
}

sub ReadPickstratConfigs
{
  foreach my $shc_ ( keys %shc_to_tperiod_map_ ) {
    foreach my $tperiod_ ( @{ $shc_to_tperiod_map_{ $shc_ } } ) {
      my $t_key_ = $shc_." ".$tperiod_;
      my $tconfig_basename_ = $shc_.".".$tperiod_.".txt";
      my $tconfig_ = `ls $MODELING_BASE_DIR/pick_strats_config/\*/$tconfig_basename_ 2>/dev/null | head -1`;
      chomp( $tconfig_ );

      if ( $tconfig_ ne "" ) {
        my $query_id_start_ = `$SCRIPTS_DIR/get_config_field.pl $tconfig_ PROD_QUERY_START_ID | grep -v '^#' | head -1 | awk '{print \$1}'`; 
        chomp( $query_id_start_ );

        if ( $query_id_start_ ne "" ) {
          $shc_tperiod_to_pickstat_config_{ $t_key_ }{ "NAME" } = $tconfig_;
          $shc_tperiod_to_pickstat_config_{ $t_key_ }{ "START_ID" } = $query_id_start_;

          my $query_id_end_ = `$SCRIPTS_DIR/get_config_field.pl $tconfig_ PROD_QUERY_STOP_ID | grep -v '^#' | head -1 | awk '{print \$1}'`;
          chomp ( $query_id_end_ );
          $shc_tperiod_to_pickstat_config_{ $t_key_ }{ "END_ID" } = ( $query_id_end_ eq "" ) ? ( $query_id_start_ + 8 ): $query_id_end_;

          print $t_key_." $tconfig_ $query_id_start_ $query_id_end_\n";

          my $trisk_ = `$SCRIPTS_DIR/get_config_field.pl $tconfig_ GLOBAL_MAX_LOSS | grep -v '^#' | head -1 | awk '{print \$1}'`; 
          chomp ( $trisk_ );
          if ( $trisk_ ne "" ) {
            $shc_tperiod_to_pickstat_config_{ $t_key_ }{ "RISK" } = $trisk_;
          } else {
            print "Warning: No GLOBAL_MAX_LOSS mentioned for config $tconfig_\n"; 
          }
        }
        else {
          print "Warning: No PROD_QUERY_START_ID mentioned for config $tconfig_\n";
        }
      }
      else {
        print "Warning: No pickstrat found for $t_key_\n";
      }
    }
  }
}
  
sub ComputeProductMetrics
{
  foreach my $shc_tperiod_ ( keys %shc_tperiod_to_day_to_pnl_map_ ) {
    my ($shc_, $timeperiod_) = split(' ', $shc_tperiod_);
    my $shcgrp_tperiod_ = $shc_to_grp_map_{ $shc_ }." ".$timeperiod_;

    foreach my $day_ ( keys %{ $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ } } ) {
      my $pnl_ = $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ };

      if ( ! exists $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ } ) {
        $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ } = 0;
      }
      $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ } += $pnl_;
    }

    if ( exists $shc_tperiod_to_pickstat_config_{ $shc_tperiod_ }{ "RISK" } ) { 
      if ( ! exists $shcgrp_tperiod_to_pickstrat_risk_{ $shcgrp_tperiod_ } ) {
        $shcgrp_tperiod_to_pickstrat_risk_{ $shcgrp_tperiod_ } = 0;
      }
      $shcgrp_tperiod_to_pickstrat_risk_{ $shcgrp_tperiod_ } += $shc_tperiod_to_pickstat_config_{ $shc_tperiod_ }{ "RISK" };
    }

    if ( defined $shc_tperiod_to_live_{ $shc_tperiod_ } && $shc_tperiod_to_live_{ $shc_tperiod_ } == 1 ) {
      $shcgrp_tperiod_to_live_{ $shcgrp_tperiod_ } = 1;
    }
  }

  foreach my $intv_ ( @days_intvs_num_ ) {
    foreach my $shc_tperiod_ ( keys %shcgrp_tperiod_to_day_to_pnl_map_ ) {
      my @pnl_series_ = ( );
      my $count_ = 0;
      my $total_count_ = 0;
      my $max_pnl_ = 0;
      my $sum_pnl_ = 0;
      foreach my $day_ ( @days_vec_ ) {
        if ( exists $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ } ) {
          push ( @pnl_series_, $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ } );
          $sum_pnl_ += $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ };
          $max_pnl_ = max( $max_pnl_, $sum_pnl_ );

          $count_++;
          last if ( $count_ >= $intv_ );
        }
        $total_count_++;
        last if ( $total_count_ > 1.5 * $intv_ );
      }
      if ( $count_ < 0.5 * $intv_ ) {
        print "Skipping $shc_tperiod_ for $intv_\n";
        next;
      }

      my $pnl_avg_ = GetAverage(\@pnl_series_) ;
      my $pnl_sd_ = GetStdev(\@pnl_series_);
      $shcgrp_tperiod_to_count_map_{ $shc_tperiod_ }{ $intv_ } = $count_ ;
      $shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ } = $pnl_avg_ ;
      $shcgrp_tperiod_to_sharpe_map_{ $shc_tperiod_ }{ $intv_ } = 0;
      $shcgrp_tperiod_to_sharpe_map_{ $shc_tperiod_ }{ $intv_ } = $pnl_avg_ / $pnl_sd_ if $pnl_sd_ > 0;
      $shcgrp_tperiod_to_maxloss_map_{ $shc_tperiod_ }{ $intv_ } = min(0, min(@pnl_series_) ) ;
      if ( $intv_ == max ( @days_intvs_num_ ) ) {
        $shcgrp_tperiod_to_drawdown_map_{ $shc_tperiod_ } = $max_pnl_ - $sum_pnl_;
      }

      my ($shcgrp_, $tperiod_) = split(' ', $shc_tperiod_);

# if the group has more than 1 product
      if ( $#{ $shcgrp_to_shc_map_{ $shcgrp_ } } > 0 ) {
        foreach my $shc_ ( @{ $shcgrp_to_shc_map_{ $shcgrp_ } } ) {
          my $tshc_tperiod_ = $shc_." ".$tperiod_;
          if ( ! exists $shcgrp_tperiod_to_interval_map_{ $tshc_tperiod_ } ) { 
            my @tpnl_series_ = ( );
            my $tcount_ = 0;
            my $ttotal_count_ = 0;
            my $tmax_pnl_ = 0;
            my $tsum_pnl_ = 0;
            foreach my $day_ ( @days_vec_ ) {
              if ( exists $shc_tperiod_to_day_to_pnl_map_{ $tshc_tperiod_ }{ $day_ } ) {
                push ( @tpnl_series_, $shc_tperiod_to_day_to_pnl_map_{ $tshc_tperiod_ }{ $day_ } );
                $tsum_pnl_ += $shc_tperiod_to_day_to_pnl_map_{ $tshc_tperiod_ }{ $day_ };
                $tmax_pnl_ = max( $tmax_pnl_, $tsum_pnl_ );
                
                $tcount_++;
                last if ( $tcount_ >= $intv_ );
              }
              $ttotal_count_++;
              last if ( $ttotal_count_ > 1.5 * $intv_ );
            }
            if ( $tcount_ < 0.5 * $intv_ ) {
              print "Skipping $tshc_tperiod_ for $intv_\n";
              next;
            }

            my $tpnl_avg_ = GetAverage(\@tpnl_series_) ;
            my $tpnl_sd_ = GetStdev(\@tpnl_series_);
            $shc_tperiod_to_count_map_{ $tshc_tperiod_ }{ $intv_ } = $tcount_ ;
            $shc_tperiod_to_interval_map_{ $tshc_tperiod_ }{ $intv_ } = $tpnl_avg_ ;
            $shc_tperiod_to_sharpe_map_{ $tshc_tperiod_ }{ $intv_ } = 0;
            $shc_tperiod_to_sharpe_map_{ $tshc_tperiod_ }{ $intv_ } = ($tpnl_avg_ / $tpnl_sd_) if $tpnl_sd_ > 0;
            $shc_tperiod_to_maxloss_map_{ $tshc_tperiod_ }{ $intv_ } = min(0, min(@tpnl_series_) ) ;
            if ( $intv_ == max ( @days_intvs_num_ ) ) {
              $shc_tperiod_to_drawdown_map_{ $tshc_tperiod_ } = $tmax_pnl_ - $tsum_pnl_;
            }
          }
        }
      }
    }
  }


  if ( defined $yoy_numdays_ ) {
    my $intv_ = "YTD";
    my $curr_yyyy_ = substr($current_date_,0,4);

    foreach my $shc_tperiod_ ( keys %shcgrp_tperiod_to_day_to_pnl_map_ ) {
      my @pnl_series_ = ( );
      my $count_ = 0;
      foreach my $day_ ( @days_vec_ ) {
        last if ( substr($day_,0,4) ne $curr_yyyy_ );
        if ( exists $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ } ) {
          push ( @pnl_series_, $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ } );
          
          $count_++;
        }
      }
      if ( $count_ < 0.5 * $yoy_numdays_ ) {
        print "Skipping $shc_tperiod_ for YTD_\n";
        next;
      }

      my $pnl_avg_ = GetAverage(\@pnl_series_);
      my $pnl_sd_ = GetStdev(\@pnl_series_);
      $shcgrp_tperiod_to_count_map_{ $shc_tperiod_ }{ $intv_ } = $count_ ;
      $shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ } = $pnl_avg_ ;
      $shcgrp_tperiod_to_sharpe_map_{ $shc_tperiod_ }{ $intv_ } = 0;
      $shcgrp_tperiod_to_sharpe_map_{ $shc_tperiod_ }{ $intv_ } = $pnl_avg_ / $pnl_sd_ if $pnl_sd_ > 0;
      $shcgrp_tperiod_to_maxloss_map_{ $shc_tperiod_ }{ $intv_ } = min(0, min(@pnl_series_) ) ;

      my ($shcgrp_, $tperiod_) = split(' ', $shc_tperiod_);

# if the group has more than 1 product
      if ( $#{ $shcgrp_to_shc_map_{ $shcgrp_ } } > 0 ) {
        foreach my $shc_ ( @{ $shcgrp_to_shc_map_{ $shcgrp_ } } ) {
          my $tshc_tperiod_ = $shc_." ".$tperiod_;
          my @tpnl_series_ = ( );
          my $tcount_ = 0;
          foreach my $day_ ( @days_vec_ ) {
            last if ( substr($day_,0,4) ne $curr_yyyy_ );
            if ( exists $shc_tperiod_to_day_to_pnl_map_{ $tshc_tperiod_ }{ $day_ } ) {
              push ( @tpnl_series_, $shc_tperiod_to_day_to_pnl_map_{ $tshc_tperiod_ }{ $day_ } );

              $tcount_++;
            }
          }
          if ( $tcount_ < 0.5 * $yoy_numdays_ ) {
            print "Skipping $tshc_tperiod_ for YTD_\n";
            next;
          }

          my $tpnl_avg_ = GetAverage(\@tpnl_series_);
          my $tpnl_sd_ = GetStdev(\@tpnl_series_);
          $shc_tperiod_to_count_map_{ $tshc_tperiod_ }{ $intv_ } = $tcount_ ;
          $shc_tperiod_to_interval_map_{ $tshc_tperiod_ }{ $intv_ } = $tpnl_avg_ ;
          $shc_tperiod_to_sharpe_map_{ $tshc_tperiod_ }{ $intv_ } = 0;
          $shc_tperiod_to_sharpe_map_{ $tshc_tperiod_ }{ $intv_ } = $tpnl_avg_ / $tpnl_sd_ if $tpnl_sd_ > 0;
          $shc_tperiod_to_maxloss_map_{ $tshc_tperiod_ }{ $intv_ } = min(0, min(@tpnl_series_) ) ;
        }
      }
    }
  }
}

sub ComputeProductRisks
{
  foreach my $shc_tperiod_ ( keys %shcgrp_tperiod_to_interval_map_ ) {
    my $trisk_ = GetRiskFromPnlSeriesCapped ( $shc_tperiod_, $shcgrp_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }, \@days_vec_ );
    if ( $trisk_ > 0 ) {
      $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ } = $trisk_;
    }
  }
  foreach my $shc_tperiod_ ( keys %shc_tperiod_to_interval_map_ ) {
    my $trisk_ = GetRiskFromPnlSeriesCapped ( $shc_tperiod_, $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }, \@days_vec_ );
    if ( $trisk_ > 0 ) {
      $shc_tperiod_to_risk_map_{ $shc_tperiod_ } = $trisk_;
    }
  }
}

sub ComputeExchRisks
{
  my @INTV = (20, 60);
  foreach my $exch_ ( keys %exch_to_sess_to_interval_map_ ) {
    foreach my $session_ ( keys %{ $exch_to_sess_to_interval_map_{ $exch_ } } ) {
      my $exchs_map_ = $exch_to_sess_to_interval_map_{ $exch_ }{ $session_ };
      my $tpnl_ = max ( @$exchs_map_{ @INTV } );
      next if ! defined $tpnl_;

      if ( $tpnl_ > 10000 ) {
        $exch_to_sess_to_risk_map_{ $exch_ }{ $session_ } = 4 * $tpnl_;
      }
      elsif ( $tpnl_ > 1000 ) {
        $exch_to_sess_to_risk_map_{ $exch_ }{ $session_ } = 6 * $tpnl_;
      }
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

    foreach my $day_ ( keys %{ $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ } } ) {
      my $pnl_ = $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ };

      if ( ! exists $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } ) {
        $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } = 0;
      }

      $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } += $pnl_;
    }
  }

  foreach my $intv_ ( @days_intvs_num_ ) {
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
        my $pnl_sd_ = GetStdev(\@pnl_series_);
        $exch_to_sess_to_interval_map_{ $exch_ }{ $session_ }{ $intv_ } = $pnl_avg_;
        $exch_to_sess_to_sharpe_map_{ $exch_ }{ $session_ }{ $intv_ } = 0;
        $exch_to_sess_to_sharpe_map_{ $exch_ }{ $session_ }{ $intv_ } = $pnl_avg_ / $pnl_sd_ if $pnl_sd_ > 0;
        $exch_to_sess_to_maxloss_map_{ $exch_ }{ $session_ }{ $intv_ } = min(0, min(@pnl_series_) );
      }
    }
  }
  if ( defined $yoy_numdays_ ) {
    my $intv_ = "YTD";
    my $curr_yyyy_ = substr($current_date_,0,4);

    foreach my $exch_ ( keys %exch_to_session_to_day_to_pnl_map_ ) {
      foreach my $session_ ( keys %{ $exch_to_session_to_day_to_pnl_map_{ $exch_ } } ) {
        my @pnl_series_ = ( );
        my $count_ = 0;
        foreach my $day_ ( @days_vec_ ) {
          if ( substr($day_,0,4) ne $curr_yyyy_ ) {
            last;
          }
          if ( exists $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } ) {
            push ( @pnl_series_, $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ } );
            $count_++;
          }
        }
        if ( $count_ < 0.5 * $yoy_numdays_ ) {
          print "Skipping $exch_ $session_ for YTD_\n";
          next;
        }

        my $pnl_avg_ = GetAverage(\@pnl_series_) ;
        my $pnl_sd_ = GetStdev(\@pnl_series_);
        $exch_to_sess_to_interval_map_{ $exch_ }{ $session_ }{ $intv_ } = $pnl_avg_;
        $exch_to_sess_to_sharpe_map_{ $exch_ }{ $session_ }{ $intv_ } = 0;
        $exch_to_sess_to_sharpe_map_{ $exch_ }{ $session_ }{ $intv_ } = $pnl_avg_ / $pnl_sd_ if $pnl_sd_ > 0;
        $exch_to_sess_to_maxloss_map_{ $exch_ }{ $session_ }{ $intv_ } = min(0, min(@pnl_series_) );
      }
    }
  }
}

sub DumpStatsInJson {
  my $json_fname_ = shift;

  my @stats_map_ = ( );

  my %session_to_shc_to_tp_ = ( );
  foreach my $shc_tperiod_ ( keys %shcgrp_tperiod_to_risk_map_ ) {
    my ($shc_, $timeperiod_) = split(' ', $shc_tperiod_);
    my $session_ = GetSessionTag ( $timeperiod_, $shc_ );

    if ( ! exists $session_to_shc_to_tp_{ $session_ }{ $shc_ } ) {
      @{ $session_to_shc_to_tp_{ $session_ }{ $shc_ } } = ( );
    }
    push ( @{ $session_to_shc_to_tp_{ $session_ }{ $shc_ } }, $timeperiod_ );
  }

  foreach my $session_ ( sort keys %session_to_shc_to_tp_ ) {
    my @session_stats_ = ( );
    foreach my $texch_ ( keys %exchange_to_shclist_ ) {
      foreach my $shc_ ( sort @{ $exchange_to_shclist_{ $texch_ } } ) {
        next if ( ! exists $session_to_shc_to_tp_{ $session_ }{ $shc_ } );

        my @tp_vec_ = grep { defined $shcgrp_tperiod_to_risk_map_{$shc_." ".$_} 
          && defined $shcgrp_tperiod_to_pickstrat_risk_{$shc_." ".$_} } @{ $session_to_shc_to_tp_{ $session_ }{ $shc_ } };

        foreach my $timeperiod_ ( @tp_vec_ ) {
          my $shc_tperiod_ = $shc_." ".$timeperiod_;
          my %shc_sess_stats_ = ( );
          $shc_sess_stats_{ "shortcode" } = $shc_;
          $shc_sess_stats_{ "timeperiod" } = $timeperiod_;

          $shc_sess_stats_{ "trader" } = "--";
          if ( defined $shcgrp_to_trader_map_{$shc_}{ $session_ } ) {
            $shc_sess_stats_{ "trader" } = $shcgrp_to_trader_map_{$shc_}{ $session_ };
          } elsif ( defined $shcgrp_to_trader_map_{$shc_}{ "ALL" } ) {
            $shc_sess_stats_{ "trader" } = $shcgrp_to_trader_map_{$shc_}{ "ALL" };
          }

          foreach my $intv_ ( @days_intv_ ) {
            my $intv_numdays_ = $intv_ eq "YTD" ? $yoy_numdays_ : $intv_;
            if ( exists $shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ } ) {
              my $intv_avg_pnl_ = int( 0.5 + $shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ } );
              if ( $shcgrp_tperiod_to_count_map_{ $shc_tperiod_ }{ $intv_ } < 0.8 * $intv_numdays_ ) { 
                $intv_avg_pnl_ .= " (".$shcgrp_tperiod_to_count_map_{ $shc_tperiod_ }{ $intv_ }.")";
              }
              $shc_sess_stats_{ "INTV: ".$intv_ }{ "PNL_AVG" } = $intv_avg_pnl_;
              $shc_sess_stats_{ "INTV: ".$intv_ }{ "SHARPE" } = $shcgrp_tperiod_to_sharpe_map_{ $shc_tperiod_ }{ $intv_ };
            }
          }
          $shc_sess_stats_{ "DD/RISK" } = 0;
          if ( defined $shcgrp_tperiod_to_drawdown_map_{ $shc_tperiod_ }
              && defined $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ }
              && $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ } > 0 ) {
            $shc_sess_stats_{ "DD/RISK" } = $shcgrp_tperiod_to_drawdown_map_{ $shc_tperiod_ } / $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ };
          }
          $shc_sess_stats_{ "SUGG_RISK" } = $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ };
          $shc_sess_stats_{ "PICKS_RISK" } = $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ };
          $shc_sess_stats_{ "RISK_RATIO" } = $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ } / $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ };
          $shc_sess_stats_{ "IS_LIVE" } = 0;
          if ( defined $shcgrp_tperiod_to_live_{ $shc_tperiod_ } && $shcgrp_tperiod_to_live_{ $shc_tperiod_ } == 1 ) {
            $shc_sess_stats_{ "IS_LIVE" } = 1;
          }

          if ( $#{ $shcgrp_to_shc_map_{ $shc_ } } > 0 ) {
            print "Group Stats for shc: $shc_\n";
            foreach my $tshc_ ( @{ $shcgrp_to_shc_map_{ $shc_ } } ) {
              my %tshc_sess_stats_ = ( );
              my $tshc_tperiod_ = $tshc_." ".$timeperiod_;
              next if ( ! defined $shc_tperiod_to_risk_map_{$tshc_tperiod_} || ! defined $shc_tperiod_to_pickstat_config_{ $tshc_tperiod_ }{ "RISK" } );

              $tshc_sess_stats_{ "shortcode" } = $tshc_;
              $tshc_sess_stats_{ "timeperiod" } = $timeperiod_;
              $tshc_sess_stats_{ "trader" } = $shc_sess_stats_{ "trader" };

              foreach my $intv_ ( @days_intv_ ) {
                my $intv_numdays_ = $intv_ eq "YTD" ? $yoy_numdays_ : $intv_;
                if ( exists $shc_tperiod_to_interval_map_{ $tshc_tperiod_ }{ $intv_ } ) {
                  my $intv_avg_pnl_ = int( 0.5 + $shc_tperiod_to_interval_map_{ $tshc_tperiod_ }{ $intv_ } );
                  if ( $shc_tperiod_to_count_map_{ $tshc_tperiod_ }{ $intv_ } < 0.8 * $intv_numdays_ ) { 
                    $intv_avg_pnl_ .= " (".$shc_tperiod_to_count_map_{ $tshc_tperiod_ }{ $intv_ }.")";
                  }
                  $tshc_sess_stats_{ "INTV: ".$intv_ }{ "PNL_AVG" } = $intv_avg_pnl_;
                  $tshc_sess_stats_{ "INTV: ".$intv_ }{ "SHARPE" } = $shc_tperiod_to_sharpe_map_{ $tshc_tperiod_ }{ $intv_ };
                }
              }
              $tshc_sess_stats_{ "DD/RISK" } = 0;
              if ( $shc_tperiod_to_pickstat_config_{ $tshc_tperiod_ }{ "RISK" } > 0 ) {
                $tshc_sess_stats_{ "DD/RISK" } = $shc_tperiod_to_drawdown_map_{ $tshc_tperiod_ } / $shc_tperiod_to_pickstat_config_{ $tshc_tperiod_ }{ "RISK" };
              }
              $tshc_sess_stats_{ "SUGG_RISK" } = $shc_tperiod_to_risk_map_{ $tshc_tperiod_ };
              $tshc_sess_stats_{ "PICKS_RISK" } = $shc_tperiod_to_pickstat_config_{ $tshc_tperiod_ }{ "RISK" };
              $tshc_sess_stats_{ "RISK_RATIO" } = $shc_tperiod_to_pickstat_config_{ $tshc_tperiod_ }{ "RISK" } / $shc_tperiod_to_risk_map_{ $tshc_tperiod_ };
              $tshc_sess_stats_{ "IS_LIVE" } = 0;
              if ( defined $shc_tperiod_to_live_{ $tshc_tperiod_ } && $shc_tperiod_to_live_{ $tshc_tperiod_ } == 1 ) {
                $tshc_sess_stats_{ "IS_LIVE" } = 1;
              }

              push ( @{ $shc_sess_stats_{ "groups" } }, \%tshc_sess_stats_ );
            }
          }
          push ( @session_stats_, \%shc_sess_stats_ );
        }
      }
    }
    my %session_map_ = ( );
    $session_map_{ "session" } = $session_;
    $session_map_{ "values" } = \@session_stats_; 
    push ( @stats_map_, \%session_map_ );
  }

  my $stats_json_string_ = encode_json \@stats_map_;

  open JSONHANDLE, "> $json_fname_" or PrintStacktraceAndDie ( "Could not open $json_fname_ for writing" );
  print JSONHANDLE $stats_json_string_;
  close JSONHANDLE;
}


sub DumpStatsInMailBody {
  my $sendmail_ = shift;

  print "SendMail: ".$sendmail_."\n";
  my $mailhandle_;
  if ( $sendmail_ ) {
    open ( $mailhandle_ , "|/usr/sbin/sendmail -t" );

    print $mailhandle_ "To: $mail_address_\n";
    print $mailhandle_ "From: $mail_address_\n";
    printf $mailhandle_ "Subject: RISK STATISTICS %s\n", $current_date_;
    print $mailhandle_ "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n";
  }
  else {
    $mailhandle_ = *STDOUT;
  }

  print $mailhandle_ "<html><body>\n";

  if ( $send_simula_link_ ) {
      print $mailhandle_ "<p>Simula Link for Product Risk Statistics:<br>";
      print $mailhandle_ "http://52.0.55.252:2080/risk_statistics<br></p>";
  }
  else {
    PrintProductsRiskTable ( $mailhandle_ );
  }
  
  print $mailhandle_ "<br><br>";
  PrintExchangeRiskTable ( $mailhandle_ );
  
  print $mailhandle_ "<br><br>";
  my $risk_managers_mail_body_ .= "Unexpected Confignames installed recently: <br>\n".join("<br>\n", @invalid_confignames_vec_)."\n";
  print $mailhandle_ "<p>$risk_managers_mail_body_</p>";

  print $mailhandle_ "</body></html>\n";

  close ( $mailhandle_ );
}

sub PrintExchangeRiskTable {
  my $mailhandle_ = shift;

  my @bg_cols_ = ("#bbefff","#99ecff","#bbefff");
  my $risk_bg_col_ = "#c7ebc7";
  my $pnl_col_ = "#2b00b3";
  my $empty_col_ = "#ffff99";
  my $empty_pnl_col_ = "#bfbfbb";

  print $mailhandle_ "<table border = \"2\">" ;
  printf $mailhandle_ "<tr><td align=center colspan=\"%d\"><font font-weight = \"bold\" size = \"2\" color=darkblue>EXCHANGES</font></td></tr>", (scalar @days_intv_)*3 + 2;

  printf $mailhandle_ "<tr>\n";
  foreach my $elem_str_ ( qw(EXCHANGE SESSION) ) {
    printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $elem_str_;
  }
  foreach my $elem_str_ ( @days_intv_ ) {
    printf $mailhandle_ "<td align=center colspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", "INTV: ".$elem_str_;
  }
  printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", "STOPLOSS_LIMIT";
  printf $mailhandle_ "</tr>\n";

  foreach my $intv_ ( @days_intv_ ) {
    foreach my $elem_str_ ( qw( PNL_AVG SHARPE ) ) {
      printf $mailhandle_ "<td align=center><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $elem_str_;
    }
  }
  printf $mailhandle_ "</tr>";


  foreach my $exch_ ( keys %exch_to_session_to_day_to_pnl_map_ ) {
    my $session_idx_ = 0;
    my @sess_vec_ = keys %{ $exch_to_session_to_day_to_pnl_map_{ $exch_ } };
    foreach my $session_ ( @sess_vec_ ) {
      print $mailhandle_ "<tr>";
      if ( $session_idx_ == 0 ) {
        printf $mailhandle_ "<td align=center rowspan=\"%d\">%s</td>", (scalar @sess_vec_), $exch_;
      }
      printf $mailhandle_ "<td align=center>%s</td>", $session_;

      foreach my $intv_idx_ ( 0..$#days_intv_ ) {
        my $intv_ = $days_intv_[ $intv_idx_ ];
        my $bg_col_ = $bg_cols_[ min($#bg_cols_, $intv_idx_) ];
        
        if ( defined $exch_to_sess_to_interval_map_{ $exch_ }{ $session_ }{ $intv_ } ) {
          my $intv_avg_pnl_ = int( 0.5 + $exch_to_sess_to_interval_map_{ $exch_ }{ $session_ }{ $intv_ } );
          my $intv_sharpe_ = $exch_to_sess_to_sharpe_map_{ $exch_ }{ $session_ }{ $intv_ };
          my $intv_min_pnl_ = int( 0.5 + $exch_to_sess_to_maxloss_map_{ $exch_ }{ $session_ }{ $intv_ } );

          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>", $bg_col_, $pnl_col_, $intv_avg_pnl_;
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%.2f</td>", $bg_col_, $pnl_col_, $intv_sharpe_;
        }
        else {
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
        }
      }
      if ( defined $exch_to_sess_to_risk_map_{ $exch_ }{ $session_ } ) {
        printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>", $risk_bg_col_, $pnl_col_, $exch_to_sess_to_risk_map_{ $exch_ }{ $session_ };
      }
      else {
        printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">NA</td>", $risk_bg_col_, $empty_pnl_col_;
      }
      print $mailhandle_ "</tr>\n";
      $session_idx_++;
    }
  }
  print $mailhandle_ "</table>";
}

sub PrintProductsRiskTable {
  my $mailhandle_ = shift;

  my @bg_cols_ = ("#bbefff","#99ecff","#bbefff"); #("#d2f6de", "#cbece8");
  my $pnl_col_ = "#2b00b3";
  my $empty_col_ = "#ffff99";

  my @col_headers_ = qw( SHORTCODE_GRP TIMEPERIOD TRADER );
  my $risk_bg_col_ = "#c7ebc7";
  my $risk_col_high_ = "#8b0000";
  my $risk_col_low_ = "#006600";
  my $risk_col_mid_ = "#00008b";
  
  my $ncols_ = (scalar @col_headers_) + (scalar @days_intv_)*2 + 4;
  my @intv_subheaders_ = qw( PNL_AVG SHARPE );

  my %session_to_shc_to_tp_ = ( );
  foreach my $shc_tperiod_ ( keys %shcgrp_tperiod_to_risk_map_ ) {
    my ($shc_, $timeperiod_) = split(' ', $shc_tperiod_);
    my $session_ = GetSessionTag ( $timeperiod_, $shc_ );

    if ( ! exists $session_to_shc_to_tp_{ $session_ }{ $shc_ } ) {
      @{ $session_to_shc_to_tp_{ $session_ }{ $shc_ } } = ( );
    }
    push ( @{ $session_to_shc_to_tp_{ $session_ }{ $shc_ } }, $timeperiod_ );
  }


  print $mailhandle_ "<table border = \"2\">\n<tr>";
  foreach my $elem_str_ ( @col_headers_ ) {
    printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $elem_str_;
  }
  foreach my $elem_str_ ( @days_intv_ ) {
    printf $mailhandle_ "<td align=center colspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", "INTV: ".$elem_str_;
  }
  printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", "DD/RISK";
  printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", "SUGG_RISK";
  printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", "PICKS_RISK";
  printf $mailhandle_ "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", "RISK_RATIO";
  printf $mailhandle_ "</tr>\n";
  
  printf $mailhandle_ "<tr>";
  foreach my $intv_ ( @days_intv_ ) {
    foreach my $elem_str_ ( @intv_subheaders_ ) {
      printf $mailhandle_ "<td align=center><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $elem_str_;
    }
  }
  printf $mailhandle_ "</tr>";

  foreach my $session_ ( sort keys %session_to_shc_to_tp_ ) {
    printf $mailhandle_ "<tr>";
    printf $mailhandle_ "<td align=center colspan=\"%d\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $ncols_, "SESSION: ".$session_;
    printf $mailhandle_ "</tr>";

    foreach my $texch_ ( keys %exchange_to_shclist_ ) {
      foreach my $shc_ ( sort @{ $exchange_to_shclist_{ $texch_ } } ) {
        if ( ! exists $session_to_shc_to_tp_{ $session_ }{ $shc_ } ) {
          next;
        }

        my @tp_vec_ = grep { exists $shcgrp_tperiod_to_risk_map_{$shc_." ".$_} 
        && exists $shcgrp_tperiod_to_pickstrat_risk_{$shc_." ".$_} } @{ $session_to_shc_to_tp_{ $session_ }{ $shc_ } };

        foreach my $timeperiod_ ( @tp_vec_ ) {
          my $trader_ = "--";
          if ( defined $shcgrp_to_trader_map_{$shc_}{ $session_ } ) {
            $trader_ = $shcgrp_to_trader_map_{$shc_}{ $session_ };
          } elsif ( defined $shcgrp_to_trader_map_{$shc_}{ "ALL" } ) {
            $trader_ = $shcgrp_to_trader_map_{$shc_}{ "ALL" };
          }
          my $shc_tperiod_ = $shc_." ".$timeperiod_;

          printf $mailhandle_ "<tr>";
          if ( $timeperiod_ eq $tp_vec_[0] ) {
            printf $mailhandle_ "<td align=center rowspan=\"%d\">%s</td>", (scalar @tp_vec_), $shc_;
          }
          printf $mailhandle_ "<td align=center>%s</td>", $timeperiod_;
          printf $mailhandle_ "<td align=center>%s</td>", $trader_;

          foreach my $intv_idx_ ( 0..$#days_intv_ ) {
            my $intv_ = $days_intv_[ $intv_idx_ ];
            my $bg_col_ = $bg_cols_[ min($#bg_cols_, $intv_idx_) ];

            if ( exists $shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ } ) {
              my $intv_avg_pnl_ = int( 0.5 + $shcgrp_tperiod_to_interval_map_{ $shc_tperiod_ }{ $intv_ } );
              my $intv_min_pnl_ = int( 0.5 + $shcgrp_tperiod_to_maxloss_map_{ $shc_tperiod_ }{ $intv_ } );
              my $pnl_days_str_ = "$intv_avg_pnl_";
              my $intv_numdays_ = $intv_ eq "YTD" ? $yoy_numdays_ : $intv_;
              if ( $shcgrp_tperiod_to_count_map_{ $shc_tperiod_ }{ $intv_ } < 0.8 * $intv_numdays_ ) { 
                $pnl_days_str_ .= " (".$shcgrp_tperiod_to_count_map_{ $shc_tperiod_ }{ $intv_ }.")";
              }
              printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%s</td>", $bg_col_, $pnl_col_, $pnl_days_str_;
              printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%.2f</td>", $bg_col_, $pnl_col_, $shcgrp_tperiod_to_sharpe_map_{ $shc_tperiod_ }{ $intv_ };
            }
            else {
              printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
              printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>", $empty_col_, $pnl_col_;
            }
          }

          my $ddrisk_ = "--";;
          if ( defined $shcgrp_tperiod_to_drawdown_map_{ $shc_tperiod_ }
              && defined $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ }
              && $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ } > 0 ) {
            $ddrisk_ = sprintf("%.2f",$shcgrp_tperiod_to_drawdown_map_{ $shc_tperiod_ } / $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ });
          }
          print "$shc_tperiod_ DD/RISK: $ddrisk_, DD:".$shcgrp_tperiod_to_drawdown_map_{ $shc_tperiod_ }."\n";
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%s</td>", $risk_bg_col_, $risk_col_mid_, $ddrisk_; 
          printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%d</td>", $risk_bg_col_, $risk_col_mid_, int( 0.5 + $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ } );

          if ( defined $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ } ) {
            my $ps_risk_col_ = $risk_col_mid_;
            if ( $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ } > 1.1 * $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ } ) {
              $ps_risk_col_ = $risk_col_high_;
            } elsif ( $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ } < 0.9 * $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ } ) {
              $ps_risk_col_ = $risk_col_low_;
            }

            printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%s</td>", $risk_bg_col_, $ps_risk_col_, int( 0.5 + $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ } );
            if ( $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ } > 0 ) {
              printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">%.2f</td>", $risk_bg_col_, $ps_risk_col_, ( $shcgrp_tperiod_to_pickstrat_risk_{ $shc_tperiod_ } / $shcgrp_tperiod_to_risk_map_{ $shc_tperiod_ } );
            } else {
              printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>",  $empty_col_, $risk_col_mid_;
            }
          } else {
            printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>",  $empty_col_, $risk_col_mid_;
            printf $mailhandle_ "<td align=center bgcolor=\"%s\"><font font-weight=\"bold\" color=\"%s\">--</td>",  $empty_col_, $risk_col_mid_;
          }
          printf $mailhandle_ "</tr>\n";
        }
      }
    }
  }
  printf $mailhandle_ "<tr><td colspan=\"%d\"></td></tr>", $ncols_;
  print $mailhandle_ "</table>";
}
