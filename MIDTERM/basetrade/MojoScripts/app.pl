use strict;
use warnings;
use Mojolicious::Lite;
use Mojo::JSON qw(decode_json encode_json);
use POSIX;
use List::Util qw[min max]; # max , min

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $WF_SCRIPTS_DIR=$HOME_DIR."/".$REPO."/walkforward";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
#my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/get_date_list.pl"; 
require "$GENPERLLIB_DIR/get_sample_features_pnls.pl";
require "$GENPERLLIB_DIR/strat_utils.pl";
require "$GENPERLLIB_DIR/stratstory_db_access_manager.pl";
require "$GENPERLLIB_DIR/global_results_methods.pl";
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/move_staged_strats_sanity_checks.pl";
require "$GENPERLLIB_DIR/pool_highlights.pl";
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl";
require "$GENPERLLIB_DIR/results_db_access_manager.pl";

my %api_param_named_arg_map_ = 
(
  #'ShortCode' => 'SHC' #example entry
);

#Get cutoff argument (format assumed: HV:0.1 => 10% of HV days; HV:10 =>  10 HV days)
sub GetArgValue
{
  #reading args
  my $args_ = shift;
  my @vals_ = split(/:/, $args_);
  if ( $#vals_ > 0 ) {
    return $vals_[1];
  }
  return -1;
}

post '/pulldates' => sub {
  my $req = shift;
  my $param_map_ref_ = $req->req->params->to_hash;
  #converting to internal keys, ignoring params with empty string as values
  my %arg_map_ = map { (exists($api_param_named_arg_map_{$_}) ? $api_param_named_arg_map_{$_} : $_) => $$param_map_ref_{$_} } grep { $$param_map_ref_{$_} ne "" } keys %$param_map_ref_;

  my $filter = $arg_map_{'FILTER'}; $filter = 'ALL' unless defined $filter ; 
  my $arg_val = GetArgValue ( $filter );

  my $dates_vec_ref = [];
  if ( $filter eq 'ALL' )
  {
    $dates_vec_ref = GetAllDatesForShc ( \%arg_map_ );
  }
  elsif ( index($filter, 'HV') == 0 )
  {
    if ( $arg_val > 0 ) {
      $arg_map_{'HV_DAYS_FRAC'} = $arg_val;
    }
    $dates_vec_ref = GetHVDatesForShc ( \%arg_map_ );
  }
  elsif ( index($filter, 'LV') == 0 )
  {
    $arg_map_{'LOW_VOL'} = 1;
    if ( $arg_val > 0 ) {
      $arg_map_{'HV_DAYS_FRAC'} = $arg_val;
    }
    $dates_vec_ref = GetHVDatesForShc ( \%arg_map_ );
  }
  elsif ( index($filter, 'WKD') == 0 )
  {
    if ( $arg_val > 0 ) {
      $arg_map_{'WKD'} = $arg_val;
    }
    $dates_vec_ref = GetSimilarDaysForShc ( \%arg_map_ );
  }
  elsif ( index($filter, 'ASX_EXPIRY') == 0 )
  {
    if ( $arg_val >= 0 ) {
      $arg_map_{'ASX_EXPIRY'} = $arg_val;
    }
    $dates_vec_ref = GetASXExpiryDays ( \%arg_map_ );
  }
  else
  {
    $arg_map_{'ECO_STR'} = $filter;
    $dates_vec_ref = GetEcoDates( \%arg_map_ );
  }

  $req->render(text => encode_json $dates_vec_ref);
};

post '/getdailyfeaturespnls' => sub {
  my $req = shift;
  my $arg_json_data_ = $req->param('json');

#  my $json = Mojo::JSON->new;
#  my $arg_map_ref_ = $json->decode($arg_json_data_);
#  my $err  = $json->error;
#  if ($err) { return $req->render(json => {result => "0", message => "$err"}); }

  my $arg_map_ref_ = decode_json $arg_json_data_;

  my @features_vec_ = map { [ split(" ", $_) ] } @{ $$arg_map_ref_{ "FEATURES" } };
  my @strats_vec_ = map { [ split(" ", $_) ] } @{ $$arg_map_ref_{ "STRATS" } };
  my @dates_vec_ = @{ $$arg_map_ref_{ "DATES" } };
  my $start_hhmm_ = $$arg_map_ref_{ "START_HHMM" };
  my $end_hhmm_ = $$arg_map_ref_{ "END_HHMM" };

  my %feature_values_ = ( );
  GetDailyFeatureAndPnl( \@dates_vec_, \@features_vec_, \@strats_vec_, \%feature_values_, $start_hhmm_, $end_hhmm_ );

  $req->render(text => encode_json \%feature_values_);
};

post '/getsimrealvalues' => sub {
  my $req = shift;
  my $configid = $req->param('configid');
  my $run_date = $req->param('date');
  my %strat_trades = ();
  my $simreal_cmd = "export KEEP_TRADES_FILES=1; /home/dvctrader/basetrade/scripts/run_accurate_sim_real.pl $run_date $configid | grep ' SIM ' | awk '{print \$4}'";
  my $sim_id_ = `$simreal_cmd`;
  chomp($sim_id_);
  my $temp_trade_file_ = "/spare/local/logs/tradelogs/trades.$run_date.$sim_id_"."_tmp";
  my $sim_temp_trade_file_ = "/spare/local/logs/tradelogs/trades.$run_date.$sim_id_"."_simtmp";
  open my $handle, '<', $temp_trade_file_;
  chomp(my @real_trades = <$handle>);
  close $handle;
  open $handle, '<', $sim_temp_trade_file_;
  chomp(my @sim_trades = <$handle>);
  close $handle;
  $strat_trades{"real_trades"} = \@real_trades;
  $strat_trades{"sim_trades"} = \@sim_trades;
  $strat_trades{"configid"} = $configid;
  $req->render(text => encode_json \%strat_trades);
};

get '/getportfoliocorrs' => sub {
  my $req = shift;

  my $json_dir_ = "/media/shared/ephemeral0/globalMacro/correlation_analysis/jsons";
  my %combined_map_ = ( );
  my %last_update_map_ = ( );
  my %json_files_ = ( );

  my @predictive_json_files_ = `ls $json_dir_/predictive_json_* -1 2>/dev/null`;
  my @contemporaneous_json_files_ = `ls $json_dir_/contemporaneous_json_* -1 2>/dev/null`;
  $json_files_{"predictive"} = \@predictive_json_files_;
  $json_files_{"contemporaneous" } = \@contemporaneous_json_files_;

  foreach my $key_ ( keys %json_files_ ) {
    foreach my $fname_ ( @{ $json_files_{$key_} } ) { 
      print $fname_."\n";
      my @json_str_vec_ = `cat $fname_ 2>/dev/null`; chomp ( @json_str_vec_ );
      my $t_map_ref_ = decode_json $json_str_vec_[0];
      my $last_update_ = undef;
      foreach my $tkey_ ( keys %$t_map_ref_ ) {
        my $session_ = $$t_map_ref_{$tkey_}{"Session"};
        my $shortcode_ = $$t_map_ref_{$tkey_}{"Shortcode"};
        my $sessionkey_ = $session_."_".$key_;

        if ( defined $$t_map_ref_{$tkey_}{"Last-Update"} ) {
          if ( ! defined $last_update_map_{$sessionkey_}{$shortcode_} ) {
            $last_update_map_{$sessionkey_}{$shortcode_} = $$t_map_ref_{$tkey_}{"Last-Update"};
          }
          delete $$t_map_ref_{$tkey_}{"Last-Update"};
        }
        push ( @{$combined_map_{$sessionkey_}{$shortcode_}}, $$t_map_ref_{$tkey_} );
      }
    }
  }

  my @t_combined_map_ = ( );
  foreach my $tkey_ ( keys %combined_map_ ) {
    my %tmap_ = ( );
    $tmap_{ "session" } = $tkey_;
    my @sessvec_ = ( );
    foreach my $tshc_ ( keys %{$combined_map_{$tkey_}} ) {
      my %ttmap_ = ( );
      $ttmap_{ "shortcode" } = $tshc_;
      $ttmap_{ "values" } = $combined_map_{ $tkey_ }{ $tshc_ };
      if ( defined $last_update_map_{ $tkey_ }{ $tshc_ } ) {
        $ttmap_{ "last-update" } = $last_update_map_{ $tkey_ }{ $tshc_ };
      }
      push ( @sessvec_, \%ttmap_ );
    }
    $tmap_{ "values" } = \@sessvec_;
    push ( @t_combined_map_, \%tmap_ );
  }

  $req->render(text => encode_json \@t_combined_map_);
};

post '/getsamplefeaturespnls' => sub {
  my $req = shift;
  my $arg_json_data_ = $req->param('json');

#  my $json = Mojo::JSON->new;
#  my $arg_map_ref_ = $json->decode($arg_json_data_);
#  my $err  = $json->error;
#  if ($err) { return $req->render(json => {result => "0", message => "$err"}); }

  my $arg_map_ref_ = decode_json $arg_json_data_;

  my @features_vec_ = map { [ split(" ", $_) ] } @{ $$arg_map_ref_{ "FEATURES" } };
  my @strats_vec_ = map { [ split(" ", $_) ] } @{ $$arg_map_ref_{ "STRATS" } };
  my @dates_vec_ = @{ $$arg_map_ref_{ "DATES" } };
  my $start_hhmm_ = $$arg_map_ref_{ "START_HHMM" };
  my $end_hhmm_ = $$arg_map_ref_{ "END_HHMM" };
  my %slot_count_map_ = ( );

  my %feature_values_ = ( );
  GetSampleFeatureAndPnl( \@dates_vec_, \@features_vec_, \@strats_vec_, \%feature_values_, $start_hhmm_, $end_hhmm_, \%slot_count_map_ );

  $req->render(text => encode_json \%feature_values_);
};

post '/get_real_configs_for_product_date' => sub {
  my $req = shift;
  my $shc_ = $req->param('SHC');
  my $tperiod_ = $req->param('TP');
  my $date_ = $req->param('DATE');

  my $last_active_date_ = CalcPrevWorkingDateMult ( $date_, 5 );
  my $configname_ = $shc_.".".$tperiod_.".txt";
  my ( $year, $month, $day ) = BreakDateYYYYMMDD ( $date_ );
  
  my $logfile_prefix_ = "/NAS1/logs/QueryLogs/$year/$month/$day/log.$date_.";
  my $tradefile_prefix_ = "/NAS1/logs/QueryTrades/$year/$month/$day/trades.$date_.";

  my ($configid_, $start_qid_, $end_qid_) = GetPickstratConfigId( $configname_ );
  print "$configid_, $start_qid_, $end_qid_\n";

  my %real_configs_ = ( );
  my %qid_to_pnls_ = ( );
  my @cname_to_qid_real_pnls_ = ( );
  if ( defined $configid_ ) {
    my ( $date_, undef, $nqueries_, $global_maxloss_, $sum_maxlosses_, $computed_maxloss_) = GetLastPickstratRecord ( $configid_, $last_active_date_ );

    for my $qid_ ( $start_qid_ .. $end_qid_ ) {
      if ( -s $logfile_prefix_.$qid_.".gz" ) {
        my $logfile_ = $logfile_prefix_.$qid_.".gz";
        my @qid_configs_ = `zgrep STRATEGYLINE $logfile_ | awk '{print \$8, \$NF}'`;
        chomp ( @qid_configs_ );
        print $logfile_."\n".join("\n", @qid_configs_)."\n";

        foreach my $t_qid_config_ ( @qid_configs_ ) {
          my @qc_toks_ = split ( /\s+/, $t_qid_config_ );
          if ( $#qc_toks_ >= 1 ) {
            $real_configs_{ $qc_toks_[1] } = $qc_toks_[0];
          }
        }

        if ( -s $tradefile_prefix_.$qid_ ) {
          my $tradefile_ = $tradefile_prefix_.$qid_;
          my @tradelines_ = `cat $tradefile_ 2>/dev/null`; chomp ( @tradelines_ );

          foreach my $tline_ ( @tradelines_ ) {
            my @twords_ = split ( /\s+/, $tline_ );

            if ( $#twords_ > 15 ) {
              my $qid_ = (split(/\./, $twords_[2]))[1];
              $qid_to_pnls_{ $qid_ } = $twords_[8];
            }
          }
        }
      }
    }

    foreach my $cname_ ( keys %real_configs_ ) {
      push ( @cname_to_qid_real_pnls_, [ $cname_, "query_id", $real_configs_{$cname_} ] );
      push ( @cname_to_qid_real_pnls_, [ $cname_, "pnl", $qid_to_pnls_{$real_configs_{$cname_}} ] );
    }
  }

  return $req->render(text => encode_json \@cname_to_qid_real_pnls_);
};

post '/get_simreal_for_configid_date' => sub {
  my $req = shift;
  my $qid_ = $req->param('QueryID');
  my $date_ = $req->param('DATE');

  my $SIMREALEXEC = $SCRIPTS_DIR."/run_accurate_sim_real.pl";
  my @outlines_ = `export NO_PLOT_SIMREAL=1; $SIMREALEXEC $date_ $qid_ 2>/dev/null`;

  my ($real_fname_, $sim_fname_);
  foreach my $oline_ ( @outlines_ ) {
    my @otokens_ = split(/\s+/, $oline_);

    if ( $oline_ =~ /^REAL_TRADE_FILE/ && $#otokens_ >= 1 ) {
      $real_fname_ = $otokens_[1];
    }
    elsif ( $oline_ =~ /^SIM_TRADE_FILE/ && $#otokens_ >= 1 ) {
      $sim_fname_ = $otokens_[1];
    }
  }

  my @real_lines_ = `awk '{print \$1,\$9}' $real_fname_ 2>/dev/null`;
  my @sim_lines_ = `awk '{print \$1,\$9}' $sim_fname_ 2>/dev/null`;
  chomp ( @real_lines_ ); chomp ( @sim_lines_ );

  my %simreal_lines_ = ();
  $simreal_lines_{ "REAL" } = \@real_lines_;
  $simreal_lines_{ "SIM" } = \@sim_lines_;
      
  return $req->render(text => encode_json \%simreal_lines_);
};

post '/getstratstory' => sub {
  my $req = shift;
  my $arg_json_data_ = $req->param('json');

  my $arg_map_ref_ = decode_json $arg_json_data_;
  my $shortcode_ = $$arg_map_ref_{ "SHORTCODE" };
  my $stratname_ = $$arg_map_ref_{ "STRAT" };

  my %story_metrics_ = ( );
  FetchSingleStory( $stratname_, \%story_metrics_ );

  if ( ! defined $story_metrics_{ "SHARPE" } ) {
    my $stratstory_script_ = $HOME_DIR."/".$REPO."_install/WKoDii/feature_to_DB.pl";
    my $generation_cmd_ = "$stratstory_script_ $shortcode_ INVALIDFILE $stratname_";
    print $generation_cmd_."\n";
    `$generation_cmd_ 1>/dev/null 2>&1`;

    FetchSingleStory( $stratname_, \%story_metrics_ );
  }

  my %correlated_strats_ = ( );
  $story_metrics_{ "SIMILAR_STRATS" } = ( );
  FetchCorrelatedStrats ( $stratname_, \%{ $story_metrics_{ "SIMILAR_STRATS" } } );
  $req->render(text => encode_json \%story_metrics_);
};

post '/getconsistencymetrics' => sub {
  my $req = shift;
  my $arg_json_data_ = $req->param('json');

  my $arg_map_ref_ = decode_json $arg_json_data_;
  my $shortcode_ = $$arg_map_ref_{ "SHORTCODE" };
  my $stratname_ = $$arg_map_ref_{ "STRAT" };

  my @lines_to_print_ = ( );
  my ($avg_pnl_act_, $drawdown_, $avg_pnl_metric_, $riskadj_drawdown_, $sharpe_5days_) = GetRiskAdjPnlAndDD( $shortcode_, $stratname_, \@lines_to_print_);

  my $pnl_drawdown_ = undef;
  $pnl_drawdown_ = $avg_pnl_act_ / $drawdown_ if defined $drawdown_ && $drawdown_ != 0;

  my $risk_adj_pnl_ratio_ = undef;
  $risk_adj_pnl_ratio_ = $avg_pnl_metric_ / $avg_pnl_act_ if defined $avg_pnl_act_ && $avg_pnl_act_ != 0;

  my %consistency_metrics_ = ( );
  $consistency_metrics_{ "Drawdown" } = $drawdown_;
  $consistency_metrics_{ "PNL_by_DD" } = $pnl_drawdown_;
  $consistency_metrics_{ "RiskAdjPNL" } = $avg_pnl_metric_;
  $consistency_metrics_{ "RiskAdjPNL_by_AvgPNL" } = $risk_adj_pnl_ratio_;
  $consistency_metrics_{ "5DAY_SHARPE" } = $sharpe_5days_;
  $req->render(text => encode_json \%consistency_metrics_); 
};

get '/filternames' => sub {
  my $req = shift;
  my @filters_vec = ( 'HV', 'LV', 'WKD', 'ASX_EXPIRY:1' );
  push ( @filters_vec, @ {GetAllEcoTags()} ); 
  $req->render(text => encode_json \@filters_vec);
};

post '/poolregimeperformance' => sub {
  my $req = shift;
  my $shc_ = $req->param("SHORTCODE");
  my $timeperiod_ = $req->param("TIMEPERIOD");

## REGIMEPERFORMANCE
  my %pool_regime_performance_ = ();
  RegimePoolPerformanceDays($shc_, $timeperiod_, \%pool_regime_performance_);

  $req->render(text => encode_json \%pool_regime_performance_);
};

post '/poolsize' => sub {
  my $req = shift;
  my $shc_ = $req->param("SHORTCODE");
  my $timeperiod_ = $req->param("TIMEPERIOD");
  my @poolsize_vec_ = ( );

## POOLSIZE
  my $cmd = "$WF_SCRIPTS_DIR/get_pool_configs.py -m POOL -shc $shc_ -tp $timeperiod_ 2>&1";
  my @pool_strats_ = `$cmd 2>/dev/null`; chomp ( @pool_strats_ );
  my $t_poolsize_ = scalar @pool_strats_;
  push ( @poolsize_vec_, ["POOLSIZE", $t_poolsize_] );
## UNAPPROVED_STRATS: implemented in simula

  $req->render(text => encode_json \@poolsize_vec_);
};

post '/approve_strat' => sub {
  my $req = shift;
  my $cname = $req->param('cname');
  ApprovePoolConfig($cname);
  my %strat_props = ();
# $strat_props{"cname"} = $cname;
  $strat_props{"response"} = "0";
  $req->render(text => encode_json \%strat_props);
};

post '/risk_statistics' => sub {
  my $req = shift;
  my $json;
  {
    local $/; #Enable 'slurp' mode
    open my $fh, "<", "/media/shared/ephemeral16/RiskStats.js";
    $json = <$fh>;
    close $fh;
  }
  my $data = decode_json($json);
  $req->render(text => $json);
};

post '/risk_statistics_date' => sub {
  my $req = shift;
  my $last_updated_time_ = (stat("/media/shared/ephemeral16/RiskStats.js"))[9];
  $req->render(text => $last_updated_time_);
};

post '/risk_allocations' => sub {
  my $req = shift;
  my $json;
  {
    local $/; #Enable 'slurp' mode
    open my $fh, "<", "/media/shared/ephemeral16/RiskAlloc.js";
    $json = <$fh>;
    close $fh;
  }
  my $data = decode_json($json);
  $req->render(text => $json);
};

post '/risk_allocations_date' => sub {
  my $req = shift;
  my $last_updated_time_ = (stat("/media/shared/ephemeral16/RiskAlloc.js"))[9];
  $req->render(text => $last_updated_time_);
};

post '/set_strat_description' => sub {
  my $req = shift;
  my $cname = $req->param('cname');
  my $description = $req->param('description');
  SetStratDescription($cname, $description);
  $req->render(text => $description);
};

post '/strat_details' => sub {
  my $req = shift;
  my $cname = $req->param('cname');
  my %strat_props = ();
  if ( IsValidConfig($cname) ) {
    my $json_string_ = GetConfigJson($cname);
    my $config_props_ref_ = decode_json($json_string_);
    %strat_props = %$config_props_ref_;
    my ($type_, $strat_approved_ ) = GetConfigTypeApproved($cname);
    $strat_props{"type"} = $type_;
    $strat_props{"approved"} = $strat_approved_;
    my $date = $req->param('date');
    if ( defined $date && $date ne "" ) {
      my $cmd_ = "$WF_SCRIPTS_DIR/print_strat_from_config.py -c $cname -d $date";
      my $stratline_ = `$cmd_ 2>/dev/null | head -1`; chomp ( $stratline_ );
      my @swords_ = split(/\s+/, $stratline_);
      open my $handle, '<', $swords_[3];
      chomp(my @model_file_lines = <$handle>);
      close $handle;
      open $handle, '<', $swords_[4];
      chomp(my @param_file_lines = <$handle>);
      close $handle;
      $strat_props{"model_lines"} = \@model_file_lines;
      $strat_props{"param_lines"} = \@param_file_lines;
    } 
  }
  else {
    my $strat_valid = GetStratPropertiesVec($cname, \%strat_props);
    open my $handle, '<', $strat_props{"modelfilename"};
    chomp(my @model_file_lines = <$handle>);
    close $handle;
    open $handle, '<', $strat_props{"paramfilename"};
    chomp(my @param_file_lines = <$handle>);
    close $handle;
    $strat_props{"model_lines"} = \@model_file_lines;
    $strat_props{"param_lines"} = \@param_file_lines;
    $strat_props{"cname"} = $cname;
  }
  $req->render(text => encode_json \%strat_props);
};

post '/keep_staged_strat_prelim' => sub {
  my $req = shift;
  my $cname = $req->param('cname');
  my $shortcode = $req->param('shortcode');

  my $cmd = "$WF_SCRIPTS_DIR/get_pool_configs.py -m CONFIG -c $cname 2>&1";
  my @pool_strats_ = `$cmd 2>/dev/null`; chomp ( @pool_strats_ );
  my $poolsize_ = scalar @pool_strats_;

  $cmd = "$WF_SCRIPTS_DIR/process_config.py -c $cname -m VIEW";
  my @stratprops_ = `$cmd 2>/dev/null`; chomp ( $cmd );
  my $pooltag = (split(/\s+/, (grep {$_ =~ /POOLTAG/} @stratprops_)[0]))[1];
  my $expect0vol = (split(/\s+/, (grep {$_ =~ /EXPECT_0VOL/} @stratprops_)[0]))[1];
  $pooltag = "" if ! defined $pooltag;
  $expect0vol = 0 if ! defined $expect0vol;

  my $cstemp_file_ = GetCSTempFileName ( "/spare/local/temp/" );
  open FHANDLE, "> $cstemp_file_" or PrintStacktrace ( "Could not open $cstemp_file_ for writing" );
  print FHANDLE join("\n", @pool_strats_)."\n";
  close FHANDLE;

  my @intv_ = (60,120,200);
  my %metrics_ = ( );
  my $perf_comment_ = "";
  my $perf_check_ = 1;

  foreach my $numdays_ ( @intv_ ) {
    my %t_metrics_ = ( );
    $perf_comment_ = GetPerformanceDetails ( $shortcode, $cname, $cstemp_file_, $numdays_, \%t_metrics_ );
    $metrics_{ $numdays_ } = \%t_metrics_;
    $metrics_{ $numdays_." comment" } = $perf_comment_;

    if ( defined $perf_comment_ && $perf_comment_ ne "" ) {
      $perf_check_ &= ( $t_metrics_{ "STRAT_SHARPE" } > $t_metrics_{ "POOL_SHARPE" } 
          && $t_metrics_{ "STRAT_PNLSQRTDD" } > $t_metrics_{ "POOL_PNLSQRTDD" } );
    }
  }
  `rm -f $cstemp_file_ 2>/dev/null`;

  my ($rescaled_volume_, $prod_volume_) = GetVolumeCheck ( $shortcode, $cname );

  my %similar_strats_ = ( );
  GetSimilarStrat ( $cname, \%similar_strats_ );

  my %return_story_ = ( );
  $return_story_{ "POOLSIZE" } = $poolsize_;
  $return_story_{ "METRIC" } = \%metrics_;
  $return_story_{ "PROD_VOLUME" } = $prod_volume_;
  $return_story_{ "RESCALED_20L1SZ_VOLUME" } = $rescaled_volume_;
  $return_story_{ "SIMILAR_STRATS" } = \%similar_strats_;
  $return_story_{ "POOLTAG" } = $pooltag;
  $return_story_{ "EXPECT_0VOL" } = $expect0vol;


  $return_story_{ "poolsize_check" } = ($poolsize_ < 30) ? 1 : 0;
  $return_story_{ "perf_check" } = $perf_check_;
  $return_story_{ "volume_check" } = ($rescaled_volume_ > 0.05 * $prod_volume_) ? 1 : 0;
  $return_story_{ "similar_strats_check" } = (! %similar_strats_) ? 1 : 0;

  $req->render(text => encode_json \%return_story_);
};

post '/keep_staged_strat' => sub {
  my $req = shift;
  my $cname = $req->param('cname');
  my $shortcode = $req->param('shortcode');
  my $pooltag = $req->param('POOLTAG');
  my $expect0vol = $req->param('EXPECT_0VOL');

  my %strat_props = ();

  my $cmd = "$WF_SCRIPTS_DIR/process_config.py -c $cname -m MOVE_TO_POOL";
  $cmd .= " -pooltag \"$pooltag\"" if defined $pooltag;
  $cmd .= " -expect0vol $expect0vol" if defined $expect0vol;
  my @output = `$cmd 2>&1`;
  my $retcode = $?;
  chomp ( @output );
  
  if ( $retcode != 0 ) {
     $strat_props{"response"} = "1"; #1 - Failed to move
     $strat_props{"output"} = $output[0]; #Unable to Move!!
  }
  else {
    #Mark this strat simula-approved by default, as it was moved from Simula
    $strat_props{"response"} = "0"; #Moved to pool!!
    ApprovePoolConfig($cname);
  }

  $req->render(text => encode_json \%strat_props);
};

post '/keep_prod_strat' => sub {
  my $req = shift;
  my $cname = $req->param('cname');
  my %strat_props = ();
  $req->render(text => encode_json \%strat_props);
};

post '/prune_strat' => sub {
  my $req = shift;
  my $cname = $req->param('cname');
  my $shortcode = $req->param('shortcode');
  my %strat_props = ();
 
  my $cmd = "$WF_SCRIPTS_DIR/process_config.py -c $cname -m PRUNE 2>&1";
  my @output = `$cmd`;
  my $retcode = $?;
  chomp (@output);

  if ( $retcode != 0 ) {
     $strat_props{"response"} = "1"; #1 - Failed to prune
     $strat_props{"output"} = $output[0]; #Unable to Prune!!
  } else {
    $strat_props{"response"} = "0"; #0 - Pruned
  }

  $req->render(text => encode_json \%strat_props);
};

get '/reloadecotags' => sub {
  my $req = shift;
  LoadEcoTags();
  $req->render(text => encode_json GetEcoTagsMap());
};

get '/ecotagsmap' => sub {
  my $req = shift;
  $req->render(text => encode_json GetEcoTagsMap());
};

app->start;
