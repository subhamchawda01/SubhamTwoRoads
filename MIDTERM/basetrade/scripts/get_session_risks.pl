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

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage, GetStdev
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # FetchPnlSamplesStrats GetPnlSamplesCorrelation
require "$GENPERLLIB_DIR/performance_risk_helper.pl"; # GetPnlForTimeperiods WriteTradesToOrsFile GetLossFromLogFile ParseOrsPnlFile GetSessionTag GetSession

my $SHORTCODE_TIMEPERIOD_FILE = "/spare/local/tradeinfo/riskinfo/shortcode_tags_risk.txt";
my $SHORTCODE_LIST = "/spare/local/tradeinfo/riskinfo/risk_stats_shclist";

if ( $#ARGV < 0 ) {
  print "USAGE: <script> <dummy> [<date>] [<recompute?>]";
  exit(0);
}

my $argsession_ = shift;
my $current_date_ = shift || `date +%Y%m%d`; 
my $recompute_ = shift || 0;
chomp ( $current_date_ );

my $current_time_ = `date`; chomp($current_time_);
print $current_time_.": Starting Script\n";

my $today_ = `date +%Y%m%d`; chomp ( $today_ );
my $hhmm_ = `date +%H%M`; chomp ( $hhmm_ );
if ( $current_date_ eq $today_ && $hhmm_ < 2230 ) {
  $current_date_ = CalcPrevWorkingDateMult( $current_date_, 1 );
}

my $numdays_ = 50;
my $beta_ = 10;

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
      my $texch_ = `$HOME_DIR/basetrade_install/bin/get_contract_specs $tshc_ $current_date_ EXCHANGE | cut -d' ' -f2`;
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
        # print "Warning: $shc_ of group $shcgrp_ also exists in group ".$shc_to_grp_map_{ $shc_ }."\n";
      }
    }
  }
}
close CFILE;

my %shc_tperiod_to_pickstat_config_ = ( );
my %shc_tperiod_to_pool_vec_ = ( );

ReadPickstratConfigs ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done GetPnlForTimeperiods:\n";

my @days_vec_ = ( );

my $last_date_ = $current_date_;
foreach my $i ( 1..$numdays_ ) {
  push( @days_vec_, $last_date_ );
  $last_date_ = CalcPrevWorkingDateMult( $last_date_, 1 ); 
}
# print join(' ', @days_vec_)."\n";

my %shc_tperiod_to_day_to_pnl_map_ = ( );
my %exch_to_session_to_day_to_pnl_map_ = ( );

GetPnlForTimeperiods( \%shc_tperiod_to_pickstat_config_, \@days_vec_, \%shc_tperiod_to_day_to_pnl_map_, \%shc_tperiod_to_date_to_loss_ );
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

DumpProductsStats ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done DumpProductsStats\n";

DumpExchStats ( );
$current_time_ = `date`; chomp($current_time_);
print $current_time_.": Done DumpExchStats\n";

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

sub DumpProductsStats
{
  foreach my $shc_tperiod_ ( keys %shc_tperiod_to_day_to_pnl_map_ ) {
    my ($shc_, $timeperiod_) = split(' ', $shc_tperiod_);
    my $shcgrp_tperiod_ = $shc_to_grp_map_{ $shc_ }." ".$timeperiod_;

    foreach my $day_ ( keys %{ $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ } } ) {
      if ( ! exists $shc_tperiod_to_date_to_loss_{ $shc_tperiod_ }{ $day_ } ) {
	      # print "Loss for ".$shc_tperiod_.", day:".$day_." could not be fetched\n";
	      next;
      }

      my $pnl_ = $shc_tperiod_to_day_to_pnl_map_{ $shc_tperiod_ }{ $day_ };

      if ( ! exists $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ } ) {
        $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ } = 0;
      }
      $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ } += $pnl_;

      if ( ! exists $shcgrp_tperiod_to_date_to_loss_{ $shcgrp_tperiod_ }{ $day_ } ) {
       $shcgrp_tperiod_to_date_to_loss_{ $shcgrp_tperiod_ }{ $day_ } = 0;
      }
      $shcgrp_tperiod_to_date_to_loss_{ $shcgrp_tperiod_ }{ $day_ } += $shc_tperiod_to_date_to_loss_{ $shc_tperiod_ }{ $day_ };
    }

    if ( exists $shc_tperiod_to_pickstat_config_{ $shc_tperiod_ }{ "RISK" } ) { 
      if ( ! exists $shcgrp_tperiod_to_pickstrat_risk_{ $shcgrp_tperiod_ } ) {
        $shcgrp_tperiod_to_pickstrat_risk_{ $shcgrp_tperiod_ } = 0;
      }
      $shcgrp_tperiod_to_pickstrat_risk_{ $shcgrp_tperiod_ } += $shc_tperiod_to_pickstat_config_{ $shc_tperiod_ }{ "RISK" };
    }
  }

  foreach my $shcgrp_tperiod_ ( keys %shcgrp_tperiod_to_day_to_pnl_map_ ) {
    my ($shcgrp_, $timeperiod_) = split(' ', $shcgrp_tperiod_);

    foreach my $day_ ( keys %{ $shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ } } ) {
      print $shcgrp_tperiod_." ".$day_." ".$shcgrp_tperiod_to_day_to_pnl_map_{ $shcgrp_tperiod_ }{ $day_ }." ".$shcgrp_tperiod_to_date_to_loss_{ $shcgrp_tperiod_ }{ $day_ }."\n";
    }
  }
}

sub DumpExchStats
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

  foreach my $exch_ ( keys %exch_to_session_to_day_to_pnl_map_ ) {
    foreach my $session_ ( keys %{ $exch_to_session_to_day_to_pnl_map_{ $exch_ } } ) {
      foreach my $day_ ( keys %{ $exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ } } ) {
        print $exch_." ".$session_." ".$day_." ".$exch_to_session_to_day_to_pnl_map_{ $exch_ }{ $session_ }{ $day_ }."\n";
      }
    }
  }
}

