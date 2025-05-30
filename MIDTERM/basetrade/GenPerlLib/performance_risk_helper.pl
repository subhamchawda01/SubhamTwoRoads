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

my $INFRA_SCRIPTS_DIR = $HOME_DIR."/infracore_install/scripts";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage, GetStdev
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # GetActivePickstratConfigs, GetLastPickstratRecord
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; # GetDatesFromNumDays 

my $SHORTCODE_TIMEPERIOD_FILE = "/spare/local/tradeinfo/riskinfo/shortcode_tags_risk.txt";
my $SHORTCODE_LIST = "/spare/local/tradeinfo/riskinfo/risk_stats_shclist";
my $TRADERS_FILE = "/spare/local/tradeinfo/riskinfo/products_traders_map.txt";
my $SESSIONS_SUPERSESSIONS_MAP_FILE = "/spare/local/tradeinfo/riskinfo/sessions_supersessions_map";

my $WORK_DIR = "/media/ephemeral2/risk_computations";
#my $PNLS_DIR = "/media/shared/ephemeral16/risk_PNL_records";

LoadSessionMap ( );

sub GetRiskFromPnlSeries
{
  my ($product_, $day_to_pnl_map_ref_, $day_vec_rev_sorted_ref_) = @_;
  my $MINRISK = 2500;
  my @INTV = (20, 60);
  my $beta_ = 10;

  my @pnls_vec_ = ( );
  foreach my $intv_ ( @INTV ) {
    my @pnl_series_ = ( );
    my $count_ = 0;
    my $total_count_ = 0;
    foreach my $day_ ( @$day_vec_rev_sorted_ref_ )  {
      if ( exists $$day_to_pnl_map_ref_{ $day_ } ) {
        push ( @pnl_series_, $$day_to_pnl_map_ref_{ $day_ } );
        $count_++;
        last if ( $count_ >= $intv_ );
      }
      $total_count_++;
      last if ( $total_count_ > 1.5 * $intv_ );
    }
    if ( $count_ >= 0.5 * $intv_ ) {
      push ( @pnls_vec_, GetAverage(\@pnl_series_) );
    }
  }

  my $net_pnl_form_ = max ( @pnls_vec_ );
  if ( defined $net_pnl_form_ ) {
    return max ($MINRISK, $beta_ * $net_pnl_form_);
  } else {
    return 0;
  }
}

sub IsRunningProduct
{
  my ($shortcode_, $query_id_ref_, $date_) = @_;
  my $numdays_ = 3;

  my $is_running_ = 0;
  my @dates_ = GetDatesFromNumDays( $shortcode_, $date_, $numdays_ );

  foreach my $date_ ( @dates_ ) {
    my ( $year, $month, $day ) = BreakDateYYYYMMDD ( $date_ );
    my $tradefile_prefix_ = "/NAS1/logs/QueryTrades/$year/$month/$day/trades.$date_.";

    foreach my $query_id_ ( @$query_id_ref_ ) {
      my $trade_file_ = $tradefile_prefix_.$query_id_;
      if ( -f $trade_file_ ) {
        $is_running_ = 1;
        last;
      }
    }

    last if $is_running_;
  }

  return $is_running_;
}

sub GetRiskFromPnlSeriesCapped
{
  my ($product_, $day_to_pnl_map_ref_, $day_vec_rev_sorted_ref_) = @_;
  my $MINRISK = 2500;
  my @INTV = (20, 60);
  my $beta_ = 10;

  my @all_pnl_series_ = values %$day_to_pnl_map_ref_;
  my $pnl_sd_ = GetStdev(\@all_pnl_series_);
  my $pnl_mean_ = GetAverage(\@all_pnl_series_);
  my $lower_cap_ = $pnl_mean_ - 3 * $pnl_sd_;
  my $upper_cap_ = $pnl_mean_ + 6 * $pnl_sd_;

  my @pnls_vec_ = ( );
  foreach my $intv_ ( @INTV ) {
    my @pnl_series_ = ( );
    my $count_ = 0;
    my $total_count_ = 0;
    foreach my $day_ ( @$day_vec_rev_sorted_ref_ )  {
      if ( exists $$day_to_pnl_map_ref_{ $day_ } ) {
        my $this_pnl_ = min( $upper_cap_, max( $lower_cap_, $$day_to_pnl_map_ref_{ $day_ } ) );
        push ( @pnl_series_, $this_pnl_ );
        $count_++;
        last if ( $count_ >= $intv_ );
      }
      $total_count_++;
      last if ( $total_count_ > 1.5 * $intv_ );
    }
    if ( $count_ >= 0.5 * $intv_ ) {
      push ( @pnls_vec_, GetAverage(\@pnl_series_) );
    }
  }

  my $net_pnl_form_ = max ( @pnls_vec_ );
  if ( defined $net_pnl_form_ ) {
    return max ($MINRISK, $beta_ * $net_pnl_form_);
  } else {
    return 0;
  }
}

sub LoadProductToTraderMap 
{
  my $exchange_to_shclist_ref_ = shift;
  my $shcgrp_to_exch_map_ref_ = shift;
  my $shcgrp_to_trader_map_ref_ = shift;

  my %exchange_trader_map_ = ( ); 
  my %shortcode_trader_map_ = ( ); 

  open TRADERHANDLE, "< $TRADERS_FILE" or PrintStacktraceAndDie( "Could not open $TRADERS_FILE for reading" ); 
  my @tlines_ = <TRADERHANDLE>; chomp( @tlines_ ); 
  close TRADERHANDLE; 

  foreach my $line_ ( @tlines_ ) { 
    if ( $line_ =~ /^#/ ) { next; } 
    my @twords_ = split(' ', $line_); 

    if ( $twords_[0] eq "EXCHANGE" ) { 
      $exchange_trader_map_{ $twords_[1] }{ $twords_[2] } = $twords_[3]; 
    } 
    elsif ( $twords_[0] eq "PRODUCT" ) { 
      $shortcode_trader_map_{ $twords_[1] }{ $twords_[2] } = $twords_[3]; 
    } 
  } 

  foreach my $exch_ ( keys %exchange_trader_map_ ) { 
    if ( ! exists $$exchange_to_shclist_ref_{ $exch_ } ) { next; } 

    foreach my $tp_ ( keys %{ $exchange_trader_map_{ $exch_ } } ) { 
      my $session_ = ($tp_ eq "ALL") ? "ALL" : GetSessionTag( $tp_ ); 
      foreach my $shcgrp_ ( @{ $$exchange_to_shclist_ref_{ $exch_ } } ) { 
        $$shcgrp_to_trader_map_ref_{ $shcgrp_ }{ $session_ } = $exchange_trader_map_{ $exch_ }{ $tp_ }; 
      } 
    } 
  } 

  foreach my $shc_ ( keys %shortcode_trader_map_ ) { 
    if ( ! exists $shortcode_trader_map_{ $shc_ } ) { next; } 

    my @shcgrp_vec_ = ( ); 
    if ( $shc_ =~ /^DI1/ || $shc_ =~ /^SP_/ ) { 
      @shcgrp_vec_ = grep { $shc_ =~ /^$_/ } keys %$shcgrp_to_exch_map_ref_; 
    } else {
      @shcgrp_vec_ = grep { $shc_ =~ /^$_[_0-9]*$/ } keys %$shcgrp_to_exch_map_ref_;
    }

    foreach my $tp_ ( keys %{ $shortcode_trader_map_{ $shc_ } } ) {
      my $session_ = ($tp_ eq "ALL") ? "ALL" : GetSessionTag( $tp_ );

      foreach my $shcgrp_ ( @shcgrp_vec_ ) {
        $$shcgrp_to_trader_map_ref_{ $shcgrp_ }{ $session_ } = $shortcode_trader_map_{ $shc_ }{ $tp_ };
      }
    }
  }

  foreach my $shcgrp_ ( keys %$shcgrp_to_exch_map_ref_ ) {
    if ( exists $$shcgrp_to_trader_map_ref_{ $shcgrp_ }{ "ALL" } ) { next; }

    if ( exists $$shcgrp_to_trader_map_ref_{ $shcgrp_ } ) {
      my @values_ = values %{ $$shcgrp_to_trader_map_ref_{ $shcgrp_ } };
      $$shcgrp_to_trader_map_ref_{ $shcgrp_ }{ "ALL" } = $values_[0];
    }
  }
}

sub GetCurrentTime
{
  my $current_time_ = `date`; chomp($current_time_);
  return $current_time_;
}

sub LoadExpiryDaysForASX
{
  my $asx_tp_to_expiry_days_ref_ = shift;

  my $as_file_ = "/spare/local/tradeinfo/ASX_expiry_days_AS";
  my $eus_file_ = "/spare/local/tradeinfo/ASX_expiry_days_EUS";

  open INFILE, "< $as_file_" or PrintStacktrace ("WARN: Could not open ASX expiry $as_file_ for reading");
  @{ $$asx_tp_to_expiry_days_ref_{ "AS" } } = <INFILE>;
  chomp ( @{ $$asx_tp_to_expiry_days_ref_{ "AS" } } );
  close INFILE;
  
  open INFILE, "< $eus_file_" or PrintStacktrace ("WARN: Could not open ASX expiry $as_file_ for reading");
  @{ $$asx_tp_to_expiry_days_ref_{ "EU" } } = <INFILE>;
  chomp ( @{ $$asx_tp_to_expiry_days_ref_{ "EU" } } );
  @{ $$asx_tp_to_expiry_days_ref_{ "US" } } = @{ $$asx_tp_to_expiry_days_ref_{ "EU" } };
  close INFILE;
}

# Get combined pnl for the mentioned shortcodes vector for the list of days for each of their timeperiod
sub GetPnlForTimeperiods
{
  my $shc_tperiod_to_pickstat_ref_ = shift;
  my $days_vec_ref_ = shift;
  my $shc_tperiod_to_day_to_pnl_map_ref_ = shift;
  my $shc_tperiod_to_date_to_loss_ref_ = shift;
  my $recompute_ = shift || 0; # 0: no recompute, 1: recompute on just null or 0, 2: recompute everything
  my $verbose_ = shift || 0;
  my $update_ = $recompute_ > 0;


  my @ASX_EXPIRY_SHCS = ("XTE_0", "YTE_0");
  my %asx_tp_to_expiry_days_ = ( );
  LoadExpiryDaysForASX ( \%asx_tp_to_expiry_days_ );

  my %shc_to_exch_ = ( );
  my $cur_date_ = $$days_vec_ref_[0];
  foreach my $shc_tp_ ( keys %$shc_tperiod_to_pickstat_ref_ ) {
    my ($shc_, $tperiod_) = split(' ', $shc_tp_);
    if ( ! defined $shc_to_exch_{ $shc_ } ) {
      $shc_to_exch_{ $shc_ } = `$HOME_DIR/basetrade_install/bin/get_contract_specs $shc_ $cur_date_ EXCHANGE | cut -d' ' -f2`;
      chomp ( $shc_to_exch_{ $shc_ } );
    }
  }

  foreach my $date_ ( @$days_vec_ref_ ) 
  {
    if ( $verbose_ ) { print GetCurrentTime().": GetPnlForTimeperiods for date: ".$date_."\n"; }
    
    my %shc_tperiod_to_pnl_map_ = ( );
    my %shc_tperiod_to_vol_map_ = ( );

    if ( $recompute_ != 2 ) {
      FetchRealPnlsForDate ( $date_, \%shc_tperiod_to_pnl_map_, \%shc_tperiod_to_vol_map_ );
    }

    my %asx_expiry_prods_ = ( );
    foreach my $shc_tp_ ( keys %$shc_tperiod_to_pickstat_ref_ ) {
      my ($shc_, $tperiod_) = split(' ', $shc_tp_);

      if ( ( defined $shc_to_exch_{ $shc_ } && $shc_to_exch_{ $shc_ } eq "ASX" ) &&
           ( defined $asx_tp_to_expiry_days_{ $tperiod_ } && FindItemFromVec ( $date_, @{ $asx_tp_to_expiry_days_{ $tperiod_ } } ) ) ) {
        $asx_expiry_prods_{ $shc_tp_ } = 1;
      }
    }
    print "ASX Expiry Prods for $date_ ".join(" ", keys %asx_expiry_prods_)."\n";

    my @shc_tperiod_for_computation_ = ( );
    if ( $recompute_ == 1 ) {
      @shc_tperiod_for_computation_ = grep { ! defined $shc_tperiod_to_pnl_map_{ $_ } || 
                                              ($shc_tperiod_to_pnl_map_{ $_ } == 0 && $shc_tperiod_to_vol_map_{ $_ } == 0) } keys %$shc_tperiod_to_pickstat_ref_;
    }
    elsif ( $recompute_ == 2 ) {
      @shc_tperiod_for_computation_ = keys %$shc_tperiod_to_pickstat_ref_;
    }
    else {
      @shc_tperiod_for_computation_ = grep { ! exists $shc_tperiod_to_pnl_map_{ $_ } } keys %$shc_tperiod_to_pickstat_ref_;
    }

    if ( $#shc_tperiod_for_computation_ < 0 ) { next; }

    my %all_shc_map_ = ( );
    my %tperiod_to_shc_ = ( );
    my %shc_tperiod_to_queryids_map_ = ( );
    my %tperiod_to_queryids_map_ = ( );
    foreach my $shc_tp_ ( @shc_tperiod_for_computation_ ) {
      my ($shc_, $tperiod_) = split(' ', $shc_tp_);
      $all_shc_map_{ $shc_ } = 1;
      push ( @{ $tperiod_to_shc_{ $tperiod_ } }, $shc_ );

      my $st_id_ = $$shc_tperiod_to_pickstat_ref_{ $shc_tp_ }{ "START_ID" };
      my $ed_id_ = $$shc_tperiod_to_pickstat_ref_{ $shc_tp_ }{ "END_ID" };
      my @query_ids_ = $st_id_..$ed_id_;
      push ( @{ $tperiod_to_queryids_map_{ $tperiod_ } }, @query_ids_ );
      $shc_tperiod_to_queryids_map_{ $shc_tperiod_ } = \@query_ids_;
    } 
    if ( $verbose_ ) { print GetCurrentTime().": QueryIds Vec for all <shortcode,timeperiod> Built and already existing PNL numbers Read for date: ".$date_."\n"; }

    my %secname_to_shc_map_ = ( );
    my %shc_to_secname_map_ = ( );
# Building the shortcode to security_code mapping
    foreach my $shc_ ( keys %all_shc_map_ ) {
      my $t_secname_ = `$INSTALL_BIN/get_exchange_symbol $shc_ $date_ 2>/dev/null`; chomp ( $t_secname_ );
      if ( $t_secname_ ne "" ) {
        $shc_to_secname_map_{ $shc_ } = $t_secname_;
        if ( ! FindItemFromVec( $shc_, @ASX_EXPIRY_SHCS ) ) {
          $secname_to_shc_map_{ $t_secname_ } = $shc_;
        }
      }
    }
    if ( $verbose_ ) { print GetCurrentTime().": ShortcodeSecuritCode_mapping Built for date: ".$date_."\n"; }


# Log directory for current date
    my $ors_fprefix_ = $WORK_DIR."/".$date_."/";
    if ( ! -d $ors_fprefix_ ) { `mkdir -p $ors_fprefix_`; }

    my %timeperiod_to_fnames_map_ = ( );
    WriteTradesToOrsFile ( $date_, \%tperiod_to_queryids_map_, \%tperiod_to_shc_, \%secname_to_shc_map_, $ors_fprefix_, \%timeperiod_to_fnames_map_, $verbose_ );
    if ( $verbose_ ) { print GetCurrentTime().": TradesFile created Built for date: ".$date_."\n"; }

    if ( defined $shc_tperiod_to_date_to_loss_ref_ ) {
      GetLossFromLogFile ( $date_, \%shc_to_secname_map_, \%shc_tperiod_to_queryids_map_, $shc_tperiod_to_date_to_loss_ref_, $verbose_ );
      if ( $verbose_ ) { print GetCurrentTime().": Loss numbers fetched from logfiles for date: ".$date_."\n"; }
    }

    foreach my $tperiod_ ( keys %timeperiod_to_fnames_map_ ) {
      my $ors_trade_file_ = $timeperiod_to_fnames_map_{ $tperiod_ };
      if ( ! -f $ors_trade_file_ ) { 
        if ( $verbose_ ) { print "WARN: $ors_trade_file_ does not exist.. skipping $tperiod_ for date $date_\n"; } 
        next;
      }

      my %tsecname_to_shc_map_ = %secname_to_shc_map_;
      if ( FindItemFromVec ( $date_, @{ $asx_tp_to_expiry_days_{ $tperiod_ } } ) ) {
        foreach my $exp_shc_ ( @ASX_EXPIRY_SHCS ) {
          if ( defined $shc_to_secname_map_{ $exp_shc_ } ) {
            $tsecname_to_shc_map_{ $shc_to_secname_map_{ $exp_shc_ } } = $exp_shc_;
          }
        }
      }

      my $ors_temp_pnl_file_ = $ors_fprefix_."temp.pnls_".$tperiod_; 
      my $err_ors_pnl_file_ = $ors_fprefix_."err.pnls_".$tperiod_;

      if ( -f $ors_trade_file_ ) {
        my $ors_pnl_cmd_ = "perl $INFRA_SCRIPTS_DIR/see_ors_pnl.pl R $ors_trade_file_ $date_ 0 0 1 > $ors_temp_pnl_file_ 2>> $err_ors_pnl_file_";
        `$ors_pnl_cmd_`;
      }
      if ( $verbose_ ) { print GetCurrentTime().": PNLs computed from the trades_files for date: ".$date_."\n"; }

      my %shc_to_pnl_map_new_ = ( );
      my %shc_to_vol_map_new_ = ( );
      ParseOrsPnlFile ( $ors_temp_pnl_file_, \@{ $tperiod_to_shc_{ $tperiod_ } }, \%tsecname_to_shc_map_, \%shc_to_pnl_map_new_, \%shc_to_vol_map_new_, $verbose_ );
      print $_." ".$shc_to_pnl_map_new_{$_}."\n" foreach keys %shc_to_pnl_map_new_;

      AddPnlsToDB ( $date_, $tperiod_, \%shc_to_pnl_map_new_, \%shc_to_vol_map_new_, $update_);
      if ( $verbose_ ) { print GetCurrentTime().": new PNLs to DB: ".$date_."\n"; }

      foreach my $shc_ ( keys %shc_to_pnl_map_new_ ) {
        my $shc_tperiod_ = $shc_." ".$tperiod_;
        if ( $recompute_ > 0 || ! defined $shc_tperiod_to_pnl_map_{ $shc_tperiod_ } ) {
          $shc_tperiod_to_pnl_map_{ $shc_tperiod_ } = $shc_to_pnl_map_new_{ $shc_ };
        }
      }
    }

    foreach my $shc_tperiod_ ( keys %shc_tperiod_to_pnl_map_ ) {
      if ( defined $shc_tperiod_to_pnl_map_{ $shc_tperiod_ } 
          && exists $$shc_tperiod_to_pickstat_ref_{ $shc_tperiod_ }
          && ! defined $asx_expiry_prods_{ $shc_tperiod_ } ) {
        $$shc_tperiod_to_day_to_pnl_map_ref_{ $shc_tperiod_ }{ $date_ } = $shc_tperiod_to_pnl_map_{ $shc_tperiod_ };
      }
    }
  }
}

sub WriteTradesToOrsFile
{
  my $date_ = shift;
  my $tperiod_to_queryids_ref_ = shift;
  my $tperiod_to_shc_ref_ = shift;
  my $secname_to_shc_ref_ = shift;
  my $ors_fprefix_ = shift;
  my $timeperiod_to_fnames_map_ref_ = shift;
  my $verbose_ = shift || 0;

  my ( $year, $month, $day ) = BreakDateYYYYMMDD ( $date_ );

  my $tradefile_prefix_ = "/NAS1/logs/QueryTrades/$year/$month/$day/trades.$date_.";
  
  foreach my $tperiod_ ( keys %$tperiod_to_queryids_ref_ ) {
    if ( $#{ $$tperiod_to_queryids_ref_{ $tperiod_ } } < 0 ) {
      if ( $verbose_ ) { print "WriteTradesToOrsFile $date_: No QueryIds to fetch for $tperiod_\n"; }
      next;
    }

    my $ut_orsfname_ = $ors_fprefix_."_".$tperiod_;
    $$timeperiod_to_fnames_map_ref_{ $tperiod_ } = $ut_orsfname_;

    my $tradefile_all_str_ = $tradefile_prefix_."{".join(",", @{ $$tperiod_to_queryids_ref_{ $tperiod_ } })."}";
    if ( $#{ $$tperiod_to_queryids_ref_{ $tperiod_ } } == 0 ) {
      $tradefile_all_str_ = $tradefile_prefix_.${ $$tperiod_to_queryids_ref_{ $tperiod_ } }[0];
    } 

    my @trades_ = `file $tradefile_all_str_ 2>/dev/null | grep \"ASCII text\" | cut -d':' -f1 | xargs cat | sort -k1n` ; chomp( @trades_ );

    if ( scalar ( @trades_ ) < 1 ) {
      if ( $verbose_ ) { print "WriteTradesToOrsFile $date_: No Trades to compute PNLs for $tperiod_\n"; }
      next;
    }

    my @ors_lines_ = ( );
    ConvertTradesForORSRead( \@trades_, $secname_to_shc_ref_, $$tperiod_to_shc_ref_{ $tperiod_ }, \@ors_lines_ ); 

    if ( $#ors_lines_ >= 0 ) {
      open ORSFHANDLE, "> $ut_orsfname_" or PrintStacktraceAndDie( "Could not open $ut_orsfname_ for writing" );
      print ORSFHANDLE join("\n", @ors_lines_)."\n";
      close ORSFHANDLE;
    }
  }
  if ( $verbose_ ) { print GetCurrentTime().": WriteTradesToOrsFile $date_ done\n"; }
}

sub ConvertTradesForORSRead
{
  my $trades_ref_ = shift;
  my $secname_to_shc_ref_ = shift;
  my $shc_vec_ref_ = shift;
  my $output_lines_ref_ = shift;
  @$output_lines_ref_ = ( );

  my ($secname_, $query_id_, $buysell_, $tsize_, $tprice_, $saos_);
  my %secname_to_pos_map_ = ( ) ;
  my %secname_to_lastprice_map_ = ( ) ;

  foreach my $trade_line_ ( @$trades_ref_ ) {
    my @words_ = split (' ', $trade_line_);
    if ( $#words_ < 15 ) { next; }

    my $utime_ = $words_[0];
    my $numbered_secname_ = $words_[2];

    ($secname_, $query_id_) = split( /\./, $numbered_secname_ );
    $secname_ =~ tr/\~/ /;

    if ( ! exists $$secname_to_shc_ref_{ $secname_ } 
        || ! FindItemFromVec( $$secname_to_shc_ref_{ $secname_ }, @$shc_vec_ref_ ) ) {
      next;
    }
#    print "$secname_ $query_id_ $numbered_secname_\n";

    my $bidsz_ = $words_[10];
    my $asksz_ = $words_[14];

    if ( $bidsz_==0 || $asksz_==0 ) { next; }

    $buysell_ = ($words_[3] eq 'B') ? 0 : 1;
    $tsize_ = $words_[4];
    $tprice_ = $words_[5];

    if ( ! exists $secname_to_pos_map_{ $numbered_secname_ } ) {
      $secname_to_pos_map_{ $numbered_secname_ } = 0;
    }

    my $new_pos_calc_ = $secname_to_pos_map_{ $numbered_secname_ } + ( ($buysell_ == 0) ? $tsize_ : (-1*$tsize_) );

    if ( $new_pos_calc_ != $words_[6] && exists $secname_to_lastprice_map_{ $numbered_secname_ } ) {
      my $add_tsize_ = $words_[6] - $new_pos_calc_;
      my $add_tprice_ = $secname_to_lastprice_map_{ $numbered_secname_ };
      my $add_buysell_ = ( ($words_[6] - $new_pos_calc_) > 0 ) ? 0 : 1;
      $add_tsize_ = abs($add_tsize_);

      my @out_tokens_ = ($secname_, $add_buysell_, $add_tsize_, $add_tprice_, 0, 0, $utime_, 0);
      my $line_to_print_ = join(chr(01), @out_tokens_);
      push ( @$output_lines_ref_, $line_to_print_ );
    }
    my @out_tokens_ = ($secname_, $buysell_, $tsize_, $tprice_, 0, 0, $utime_, 0);
    my $line_to_print_ = join(chr(01), @out_tokens_);
    push ( @$output_lines_ref_, $line_to_print_ );

    $secname_to_lastprice_map_{ $numbered_secname_ } = $tprice_;
    $secname_to_pos_map_{ $numbered_secname_ } = $words_[6];
  }
}


sub GetLossFromLogFile
{
  my $date_ = shift;
  my $shc_to_secname_ref_ = shift;
  my $shc_tperiod_to_queryids_map_ref_ = shift;
  my $shc_tperiod_to_date_to_loss_ref_ = shift;
  my $verbose_ = shift;

  my ( $year, $month, $day ) = BreakDateYYYYMMDD ( $date_ );

  my $logfile_prefix_ = "/NAS1/logs/QueryLogs/$year/$month/$day/log.$date_.";
  my $tradefile_prefix_ = "/NAS1/logs/QueryTrades/$year/$month/$day/trades.$date_.";
  
  foreach my $shc_tperiod_ ( keys %$shc_tperiod_to_queryids_map_ref_ ) {
    my @query_ids_ = @{ $$shc_tperiod_to_queryids_map_ref_{ $shc_tperiod_ } };
    
    my ( $shc_, $tperiod_ ) = split(' ', $shc_tperiod_);

    my $tradefile_all_str_ = $tradefile_prefix_."{".join(",", @query_ids_ )."}";

    my @trades_ = `file $tradefile_all_str_ 2>/dev/null | grep \"ASCII text\" | cut -d':' -f1 | xargs cat | sort -k1n` ; chomp( @trades_ );

    if ( scalar ( @trades_ ) < 1 ) {
      next;
    }

    if ( ! exists $$shc_to_secname_ref_{ $shc_ } ) {
      next;
    }

    my %query_ids_to_trades_present_ = ( );
    foreach my $trade_line_ ( @trades_ ) {
      my @words_ = split (' ', $trade_line_);

      if ( $#words_ < 15 ) { next; }

      my $numbered_secname_ = $words_[2];
      my ($secname_, $query_id_) = split( /\./, $numbered_secname_ );
      $secname_ =~ tr/\~/ /;

      if ( ! defined $secname_ || ! exists $$shc_to_secname_ref_{ $shc_ } ) {
        print "Secname1:".$secname_." ";
        print "Secname2:".$$shc_to_secname_ref_{ $shc_ }."\n";
      }

      if ( $secname_ ne $$shc_to_secname_ref_{ $shc_ } ) {
        next;
      }

      $query_ids_to_trades_present_{ $query_id_ } = 1;
    }

    my %queryid_to_maxloss_ = ( );
    foreach my $query_id_ ( @query_ids_ ) {
      my $logfile_ = $logfile_prefix_."$query_id_.gz";

      my @mq_lines_ = `zgrep -w \"0.000000 max_loss_\\\|getflat_due_to_external_getflat_\" $logfile_ 2>/dev/null`;
      chomp ( @mq_lines_ );
      
      my @maxloss_lines_idx_ = grep { $mq_lines_[$_] =~ /max_loss_/ && $mq_lines_[$_+1] =~ /getflat_due_to_external_getflat_/  } 0..($#mq_lines_-1);

      foreach my $line_idx_ ( 0..$#maxloss_lines_idx_ ) {
        my @maxloss_words_ = split(' ', $mq_lines_[ $line_idx_ ]);
        my @queryid_words_ = split(' ', $mq_lines_[ $line_idx_+1 ]);

        if ( $#queryid_words_ < 2 || $#maxloss_words_ < 2 ) { next; }
        my $t_queryid_ = $queryid_words_[2];
        my $t_maxloss_ = $maxloss_words_[2];

        if ( ! exists $query_ids_to_trades_present_{ $t_queryid_ } ) {
          next;
        }

        $queryid_to_maxloss_{ $t_queryid_ } = $t_maxloss_;
      }
    }

    if ( %queryid_to_maxloss_ ) {
      $$shc_tperiod_to_date_to_loss_ref_{ $shc_tperiod_ }{ $date_ } = 0;

      foreach my $qid_ ( keys %queryid_to_maxloss_ ) {
        $$shc_tperiod_to_date_to_loss_ref_{ $shc_tperiod_ }{ $date_ } += abs($queryid_to_maxloss_{ $qid_ });
      }
    }
  }
}

sub ParseOrsPnlFile 
{
  my $ors_pnl_file_ = shift;
  my $shc_vec_ref_ = shift;
  my $sec_to_shc_map_ref_ = shift;
  my $shc_to_pnl_map_ref_ = shift;
  my $shc_to_vol_map_ref_ = shift;
  my $verbose_ = shift;

  if ( ! -f $ors_pnl_file_ ) {
    if ( $verbose_ ) { print "WARN: file $ors_pnl_file_ does not exist\n"; }
    return;
  }

  open ORSFHANDLE, "< $ors_pnl_file_" or PrintStacktraceAndDie( "Could not open $ors_pnl_file_ for reading." );
  my @pnl_lines_ = <ORSFHANDLE>;
  close ORSFHANDLE;

  foreach my $pnl_line_ ( @pnl_lines_ ) {
    my @pwords_ = split('\|', $pnl_line_); chomp ( @pwords_ );
    next if ( $#pwords_ <= 2 );

    my $exch_name_ = $pwords_[0];
    $exch_name_ =~ s/^\s+|\s+$//g;
    my $secname_ = $pwords_[1];
    $secname_ =~ s/^\s+|\s+$//g;

    my @pnl_words_ = split(':', $pwords_[2]);
    my $shc_pnl_ = $pnl_words_[1];
    $shc_pnl_ =~ s/^\s+|\s+$//g;

    my @vol_words_ = split(':', $pwords_[4]);
    my $shc_vol_ = $vol_words_[1];
    $shc_vol_ =~ s/^\s+|\s+$//g;

    if ( exists $$sec_to_shc_map_ref_{ $secname_ } 
        && FindItemFromVec( $$sec_to_shc_map_ref_{ $secname_ }, @$shc_vec_ref_ ) ) {
      my $shc_ = $$sec_to_shc_map_ref_{ $secname_ };
      $$shc_to_pnl_map_ref_{ $shc_ } = $shc_pnl_;
      $$shc_to_vol_map_ref_{ $shc_ } = $shc_vol_;
    }
  }
  foreach my $shc_ ( @$shc_vec_ref_ ) {
    $$shc_to_pnl_map_ref_{ $shc_ } = undef if ! exists $$shc_to_pnl_map_ref_{ $shc_ };
    $$shc_to_vol_map_ref_{ $shc_ } = undef if ! exists $$shc_to_vol_map_ref_{ $shc_ };
  }
  close ORSFHANDLE; 
}

sub LoadSessionMap {
  open SESSMAP, "< $SESSIONS_SUPERSESSIONS_MAP_FILE" or PrintStacktraceAndDie( "Could not open $SESSIONS_SUPERSESSIONS_MAP_FILE for reading." );
  my @sesslines_ = <SESSMAP>; chomp ( @sesslines_ );

  foreach my $sline_ ( @sesslines_ ) {
    my @tokens_ = split(" ", $sline_);
    if ( $#tokens_ < 0 ) { next; }

    my $ssession_ = $tokens_[0];

    foreach my $session_ ( @tokens_[0..$#tokens_] ) {
      my @ttokens_ = split(":", $session_);
      if ( $#ttokens_ == 1 ) {
        $session_to_supersession_specific_{ $ttokens_[0] }{ $ttokens_[1] } = $ssession_;
      } else {
        $session_to_supersession_{ $session_ } = $ssession_;
      }
    }
  }
}

sub GetSessionTag {
  my $tag_ = shift;
  my $shc_ = shift || "";

  my @specific_shcvec_ = grep { $shc_ =~ /$_/ && defined $session_to_supersession_specific_{ $_ }{ $tag_ } } keys %session_to_supersession_specific_;

  if ( $#specific_shcvec_ >= 0 ) {
    return $session_to_supersession_specific_{ $specific_shcvec_[0] }{ $tag_ };
  }

  if ( defined $session_to_supersession_{ $tag_ } ) {
    return $session_to_supersession_{ $tag_ };
  }

  return "UNDEF";
}

sub GetSession {
  my $timeperiod_ = shift;
  my ($start_time_, $end_time_) = split('-', $timeperiod_);
  my $start_utc_ = `$HOME_DIR/basetrade_install/bin/get_utc_hhmm_str $start_time_ 2>/dev/null`;
#  print $start_time_." ".$end_time_." ".$start_utc_."\n";

  if ( $start_utc_ eq "" ) {
    return "";
  }

  if ( $start_utc_ < 500 || $start_utc_ > 2130 ) {
    return "AS";
  }

  if ( $start_utc_ < 1100 ) {
    return "EU";
  }

  return "US";
}

1
