#!/usr/bin/perl

# \file ModelScripts/prune_strats_from_pool.pl
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

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl";
require "$GENPERLLIB_DIR/pool_utils.pl";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";
my $TEMP_DIR="/spare/local/temp";
my $GLOBALRESULTSDBDIR = "DB";

sub PruneStrats;
sub LoadConfigFile;
sub SimilarityPruning;
sub PerformancePruning;
sub FilterStratsCutoff;
sub SendReportMail;

my $USAGE="$0 shortcode timeperiod config-file [STAGED(default: NORMAL)]";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = shift;
my $timeperiod_string_ = shift;
my $config_file_ = shift;
my $staged_str_ = shift || "NORMAL";
my $staged_ = ( $staged_str_ eq "STAGED" ) ? 1 : 0;

my @timeperiods_vec_ = ( );
if ( $timeperiod_string_ eq "ALL" ) {
  @timeperiods_vec_ = GetPoolsForShortcode ( $shortcode_ );
}
else {
  push ( @timeperiods_vec_, $timeperiod_string_ );
}

my @intervals_ = ( );
my $numstrats_to_keep_each_intv_ = 30;
my $numstrats_noprune_each_intv_ = 10;

my $lookback_days_ = 200;
my $pool_similarity_thresh_ = 0.85;
my $sort_algo_ = "kCNAPnlSharpeAverage";
my $to_prune_ = 0;
my $email_address_;

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );

my $PRUNE_PERF_SCRIPT="$HOME_DIR/basetrade/scripts/prune_strats_for_day_from_periods.pl";
my $PRUNE_SIMILAR_SCRIPT="$HOME_DIR/basetrade/scripts/prune_similar_strats.pl";

my $OML_lookback_days_ = 80;
my $OML_hit_ratio_ = 0.1;
my $OML_number_top_loss_ = 10;

my ( $volume_cutoff_, $ttc_cutoff_, $sharpe_cutoff_, $pnl_maxloss_cutoff_ );
my $skip_days_file_ = "INVALIDFILE";
my ( $msg_count_cutoff_, $minpnl_cutoff_ );

my $similarity_based_pruning_ = 1;
my $performance_based_pruning_ = 1;
my @strats_to_prune_ = ( );
my %strats_vol_cutoff_ = ( );
my %strats_ttc_cutoff_ = ( );
my %strats_sharpe_cutoff_ = ( );
my %strats_pnl_maxloss_cutoff_ = ( );
my %strats_minpnl_cutoff_ = ( );
my %strats_msgcount_cutoff_ = ( );
my %strats_performance_prunes_ = ( );
my %strats_similarity_prunes_ = ( );
my @cutoff_filtered_strats_ = ( );
my %strats_allowed_to_prune_ = ( );
my $timeperiod_ = ( );
my $mailstr_ = "";

foreach my $tp_ ( @timeperiods_vec_ )
{
  $timeperiod_ = $tp_;
  print "\nLooking for Timeperiod: ".$timeperiod_."\n";

  @strats_to_prune_ = ( );
  %strats_vol_cutoff_ = ( );
  %strats_ttc_cutoff_ = ( );
  %strats_sharpe_cutoff_ = ( );
  %strats_pnl_maxloss_cutoff_ = ( );
  %strats_minpnl_cutoff_ = ( );
  %strats_msgcount_cutoff_ = ( );
  %strats_performance_prunes_ = ( );
  %strats_similarity_prunes_ = ( );
  @cutoff_filtered_strats_ = ( );
  %strats_allowed_to_prune_ = ( );
  $mailstr_ = "";

  LoadConfigFile ( $config_file_ );

  FindNoPruneStrats ( \%strats_allowed_to_prune_ );

  my @strats_recently_installed_ = ( );
  ReadPickstratConfigs( \@strats_recently_installed_ );
#print join("\n", @strats_recently_installed_)."\n";
  delete @strats_allowed_to_prune_{ @strats_recently_installed_ };

  FilterStratsCutoff ( );

# If Staged and the pool size is very large, the SimilarityPruning is likely to very slow, so PerformancePruning first
# Else first SimilarityPruning followed by PerformancePruning

  if ( $staged_ && $#cutoff_filtered_strats_ > 100 ) {
    if ( $performance_based_pruning_ ) { PerformancePruning ( ); }
    if ( $similarity_based_pruning_ ) { SimilarityPruning ( ); }
  }
  else {
    if ( $similarity_based_pruning_ ) { SimilarityPruning ( ); }
    if ( $performance_based_pruning_ ) { PerformancePruning ( ); }
  }

  LogandMail ( );
}

sub FindNoPruneStrats
{
  my $strats_allowed_to_prune_ref_ = shift;

  my $perf_script_config_ = GetCSTempFileName( $SPARE_HOME."/prune_work/" );

  open PHANDLE, "> $perf_script_config_" or PrintStacktraceAndDie ( "Could not open file $perf_script_config_" );
  print PHANDLE "SHORTCODE TIME_PERIOD ".("DURATION " x scalar @intervals_)."TARGET_POOL_SIZE SORT_ALGO\n";
  print PHANDLE $shortcode_." ".$timeperiod_." ".join(" ", @intervals_)." ".$numstrats_noprune_each_intv_." ".$sort_algo_."\n";
  close PHANDLE;

  my $perf_cmd_ = "$PRUNE_PERF_SCRIPT $shortcode_ $timeperiod_ $perf_script_config_ 0 $staged_str_";
  print $perf_cmd_."\n";

  my @perf_out_ = `$perf_cmd_ 2>/dev/null`; chomp( @perf_out_ );

  @perf_out_ = grep { $_ =~ /^PRUNE/ } @perf_out_;
  my @t_strats_allowed_to_prune_ = map { (split)[1] } @perf_out_;
  %$strats_allowed_to_prune_ref_ = map { $_ => 1 } @t_strats_allowed_to_prune_;
}

sub PerformancePruning
{
  my $perf_script_config_ = GetCSTempFileName( $SPARE_HOME."/prune_work/" );

  open PHANDLE, "> $perf_script_config_" or PrintStacktraceAndDie ( "Could not open file $perf_script_config_" );
  print PHANDLE "SHORTCODE TIME_PERIOD ".("DURATION " x scalar @intervals_)."TARGET_POOL_SIZE SORT_ALGO\n";
  print PHANDLE $shortcode_." ".$timeperiod_." ".join(" ", @intervals_)." ".$numstrats_to_keep_each_intv_." ".$sort_algo_."\n";
  close PHANDLE;

  my $perf_cmd_ = "$PRUNE_PERF_SCRIPT $shortcode_ $timeperiod_ $perf_script_config_ 0 $staged_str_ $skip_days_file_";
  print $perf_cmd_."\n";

  my @perf_out_ = `$perf_cmd_ 2>/dev/null`; chomp( @perf_out_ );

  @perf_out_ = grep { $_ =~ /^PRUNE/ } @perf_out_;
  my @strats_to_prune_perf_ = map { (split)[1] } @perf_out_;
  @strats_to_prune_perf_ = grep { exists $strats_allowed_to_prune_{ $_ } } @strats_to_prune_perf_;
  %strats_performance_prunes_ = map { $_ => 1 } @strats_to_prune_perf_;

  PruneStrats ( \@strats_to_prune_perf_ );
  push ( @strats_to_prune_, @strats_to_prune_perf_ );
}

sub SimilarityPruning
{
  my $similarity_cmd_ = "$PRUNE_SIMILAR_SCRIPT $shortcode_ $timeperiod_ TODAY-1 $lookback_days_ $pool_similarity_thresh_ $sort_algo_ 0 $staged_str_";
  print $similarity_cmd_."\n";

  my @similarity_out_ = `$similarity_cmd_ 2>/dev/null`; chomp( @similarity_out_ );

  @similarity_out_ = grep { $_ =~ /^PRUNE/ } @similarity_out_;
  foreach my $line_ ( @similarity_out_ ) {
    my @lwords_ = split(" ", $line_);
    if ( $#lwords_ >= 7 ) {
      $strats_similarity_prunes_{ $lwords_[1] } = $lwords_[5]." ".$lwords_[7];
    }
  }
  my @strats_to_prune_similarity_ = grep { exists $strats_allowed_to_prune_{ $_ } } keys %strats_similarity_prunes_; 

  PruneStrats ( \@strats_to_prune_similarity_ );
  push ( @strats_to_prune_, @strats_to_prune_similarity_ );
}

sub FilterStratsCutoff
{
  my $staged_type_ = ($staged_) ? 'S' : 'N';
  my $pool_list_ = GetCSTempFileName ( "/spare/local/temp/" );

  my @configs_ = GetConfigsForPool ($shortcode_, $timeperiod_, $staged_type_);
  open CSTF, "> $pool_list_" or PrintStacktraceAndDie ( "Could not open $pool_list_ for writing\n" );
  print CSTF $_."\n" foreach @configs_;
  close CSTF;

  my $intv_ = $lookback_days_;
  my $start_date_ = CalcPrevWorkingDateMult ( $yyyymmdd_, $intv_ );
  
  @cutoff_filtered_strats_ = ( );

  my $exec_cmd_ = "$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $pool_list_ $GLOBALRESULTSDBDIR $start_date_ $yyyymmdd_ $skip_days_file_ $sort_algo_ 0 IF 0";

  print $exec_cmd_."\n";
  my @ssr_output_ = `$exec_cmd_`;
  chomp ( @ssr_output_ );

  my $strat_ = "";
  my $numdates_ = 0;
  my @strats_to_prune_cutoffs_ = ( );

  foreach my $ssr_line_ ( @ssr_output_ ) {
    my @ssr_words_ = split (' ', $ssr_line_ );
    chomp ( @ssr_words_ );

    if ( $#ssr_words_ < 0 ) { next; }

    if ( $strat_ ne "" ) {
      if ( $ssr_words_[0] eq "STATISTICS" ) {
        if ( $numdates_ > 0.8 * $intv_ ) {

          my $t_volume_average_ = $ssr_words_[3];
          my $t_sharpe_ = $ssr_words_[4];
          my $t_normalized_ttc_ = $ssr_words_[8];
          my $t_pnl_maxloss_ = $ssr_words_[21];
          my $t_msg_count_ = $ssr_words_[19];
          my $t_min_pnl_ = $ssr_words_[22];

          if ( defined $volume_cutoff_ && $t_volume_average_ < $volume_cutoff_ ) {
            push ( @strats_to_prune_cutoffs_, $strat_ );
            $strats_vol_cutoff_{ $strat_ } = $t_volume_average_;
          }
          elsif ( defined $ttc_cutoff_ && $t_normalized_ttc_ > $ttc_cutoff_ ) {
            push ( @strats_to_prune_cutoffs_, $strat_ );
            $strats_ttc_cutoff_{ $strat_ } = $t_normalized_ttc_;
          }
          elsif ( defined $sharpe_cutoff_ && $t_sharpe_ < $sharpe_cutoff_ ) {
            push ( @strats_to_prune_cutoffs_, $strat_ );
            $strats_sharpe_cutoff_{ $strat_ } = $t_sharpe_;
          }
          elsif ( defined $pnl_maxloss_cutoff_ && $t_pnl_maxloss_ < $pnl_maxloss_cutoff_ ) {
            push ( @strats_to_prune_cutoffs_, $strat_ );
            $strats_pnl_maxloss_cutoff_{ $strat_ } = $t_pnl_maxloss_;
          }
          elsif ( defined $minpnl_cutoff_ && $t_min_pnl_ < $minpnl_cutoff_ ) {
            push ( @strats_to_prune_cutoffs_, $strat_ );
            $strats_minpnl_cutoff_{ $strat_ } = $t_min_pnl_;
          }
          elsif ( defined $msg_count_cutoff_ && $t_msg_count_ > $msg_count_cutoff_ ) {
            push ( @strats_to_prune_cutoffs_, $strat_ );
            $strats_msgcount_cutoff_{ $strat_ } = $t_msg_count_;
          }
          else {
            push ( @cutoff_filtered_strats_, $strat_ );
          }
        }
        else {
          push ( @cutoff_filtered_strats_, $strat_ );
        }
        $strat_ = "";
        $numdates_ = 0;
      }
      elsif ( ValidDate( $ssr_words_[0] ) && !SkipWeirdDate( $ssr_words_[0] ) ) {
        $numdates_++;
      }
    }
    elsif ( $ssr_words_[0] eq "STRATEGYFILEBASE" ) {
      $strat_ = $ssr_words_[1];
#      print "Starting for ".$strat_."\n";
    }
  }
  @strats_to_prune_cutoffs_ = grep { exists $strats_allowed_to_prune_{ $_ } } @strats_to_prune_cutoffs_;
  PruneStrats ( \@strats_to_prune_cutoffs_ );
}

sub LoadConfigFile
{
  print "Loading Config File ...\n";

  my $t_config_file_ = shift;

  open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  my $current_param_ = "";
  foreach my $config_file_lines_ ( @config_file_lines_ )
  {
    if ( $config_file_lines_ =~ /^#/ ) {  # not ignoring lines with # not at the beginning
      next;
    }
    my @t_words_ = split ( ' ' , $config_file_lines_ );

    if ( $#t_words_ < 0 ) {
      $current_param_ = "";
      next;
    }

    if ( ! $current_param_ ) {
      $current_param_ = $t_words_ [ 0 ];
      next;
    }
    else {
      given ( $current_param_ ) {
        when ( "INTERVALS" ) {
          if ( scalar @t_words_ < 2 ) {
            next;
          }
          my $last_date_ = GetIsoDateFromStrMin1 ( $t_words_[0] );
          my $numdays_ = $t_words_[1];
          push ( @intervals_, $last_date_." ".$numdays_ );
        }
        when ( "NUMSTRATS_TO_KEEP_EACH_INTV" ) {
          $numstrats_to_keep_each_intv_ = $t_words_[0];
        }
        when ( "NUMSTRATS_NOPRUNE_EACH_INTV" ) {
          $numstrats_noprune_each_intv_ = $t_words_[0];
        }
        when ( "LOOKBACK_DAYS" ) {
          $lookback_days_ = $t_words_[0];
        }
        when ( "POOL_SIMILARITY_THRESHOLD" ) {
          $pool_similarity_thresh_ = $t_words_[0];
        }
        when ( "VOLUME_CUTOFF" ) {
          $volume_cutoff_ = $t_words_[0];
        }
        when ( "TTC_CUTOFF" ) {
          $ttc_cutoff_ = $t_words_[0];
        }
        when ( "SHARPE_CUTOFF" ) {
          $sharpe_cutoff_ = $t_words_[0];
        }
        when ( "PNL_MAXLOSS_CUTOFF" ) {
          $pnl_maxloss_cutoff_ = $t_words_[0];
        }
        when ( "MIN_PNL_CUTOFF" ) {
          $minpnl_cutoff_ = $t_words_[0];
        }
        when ( "MSG_COUNT_CUTOFF" ) {
          $msg_count_cutoff_ = $t_words_[0];
        }
        when ( "SKIP_DAYS_FILE" ) {
          $skip_days_file_ = $t_words_[0];
        }
        when ( "TO_PRUNE" ) {
          $to_prune_ = $t_words_[0];
        }
        when ( "SORT_ALGO" ) {
          $sort_algo_ = $t_words_[0];
        }
        when ( "EMAIL_ADDRESS" ) {
          $email_address_ = $t_words_[0];
        }
        when ( "OPTIMAL_MAX_LOSS_SETTINGS" ) {
          $OML_lookback_days_ = max ( 30, $t_words_ [ 0 ] ) ;
          $OML_hit_ratio_ = min ( 0.2, $t_words_ [ 1 ] ) ; #greater than 20% is not sane
        }
        when ( "PERFORMANCE_BASED_PRUNE" ) {
          $performance_based_pruning_ =  ( $t_words_[0] ) ? 1 : 0;
        }
        when ( "SIMILARITY_BASED_PRUNE" ) {
          $similarity_based_pruning_ =  ( $t_words_[0] ) ? 1 : 0;
        }
      }
    }
  }
}

sub ComputeOptimalMaxLoss
{
  my $t_strat_ = shift;

  my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl";
  my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shortcode_ $timeperiod_ $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ $t_strat_ $yyyymmdd_ $skip_days_file_";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

  my ( $max_loss_, $avg_pnl_, $num_max_loss_hits_ );
  foreach my $max_loss_line_ ( @exec_output_ )
  {
    if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 ) {
      next;
    }

    my @max_loss_words_ = split ( ' ' , $max_loss_line_ );
    if ( $#max_loss_words_ >= 2 )
    {
      $max_loss_ = $max_loss_words_ [ 0 ];
      $avg_pnl_ = $max_loss_words_ [ 1 ];
      $num_max_loss_hits_ = $max_loss_words_ [ 2 ];

      last;
    }
  }

  if ( defined $avg_pnl_ && defined $max_loss_ ) {
    my $pnl_maxloss_ = $avg_pnl_ / $max_loss_;
    return $pnl_maxloss_;
  }
  else {
    return -1;
  }
}

sub PruneStrats
{
  my $strats_vec_ref_ = shift;
  my $logstr_ = "";

  foreach my $strat_ ( @$strats_vec_ref_ ) {
    $logstr_ .= "PRUNE: ".$strat_;

    if ( exists $strats_vol_cutoff_{ $strat_ } ) {
      $logstr_ .= "\n\tVolume: ".$strats_vol_cutoff_{ $strat_ };
    }

    if ( exists $strats_ttc_cutoff_{ $strat_ } ) {
      $logstr_ .= "\n\tNormalizedTTC: ".$strats_ttc_cutoff_{ $strat_ };
    }

    if ( exists $strats_sharpe_cutoff_{ $strat_ } ) {
      $logstr_ .= "\n\tSharpe: ".$strats_sharpe_cutoff_{ $strat_ };
    }

    if ( exists $strats_pnl_maxloss_cutoff_{ $strat_ } ) {
      $logstr_ .= "\n\tPnlMaxloss: ".$strats_pnl_maxloss_cutoff_{ $strat_ };
    }

    if ( exists $strats_minpnl_cutoff_{ $strat_ } ) {
      $logstr_ .= "\n\tMinPnl: ".$strats_minpnl_cutoff_{ $strat_ };
    }

    if ( exists $strats_msgcount_cutoff_{ $strat_ } ) {
      $logstr_ .= "\n\tMsgCount: ".$strats_msgcount_cutoff_{ $strat_ };
    }

    if ( exists $strats_performance_prunes_{ $strat_ } ) {
      $logstr_ .= "\n\tPerformance insufficient";
    }

    if ( exists $strats_similarity_prunes_{ $strat_ } ) {
      $logstr_ .= "\n\tSimilar_strategy: ".$strats_similarity_prunes_{ $strat_ };
    }
    $logstr_ .= "\n";  
    
    if ( $to_prune_ ) {
      my $prune_cmd_ = "$HOME_DIR/walkforward/process_config.py -c $strat_ -m PRUNE";
      `$prune_cmd_`;
    }
  }
  if ( $to_prune_ ) {
    print "Strats Pruned\n".$logstr_."\n";
  } else {
    print "Strats to be Pruned\n".$logstr_."\n";
  }

  if ( $logstr_ ne "" ) {
    $mailstr_ .= $logstr_."\n";
  }
}

sub LogandMail
{
  my $log_file_ = $MODELING_BASE_DIR."/prune_strats_log/".$shortcode_."_".$timeperiod_."_logs.txt";

  if ( ! $staged_ && $to_prune_ && $mailstr_ ne "" ) {
    open LOGFHANDLE, ">> $log_file_" or PrintStacktraceAndDie ( "Could not open $log_file_ for writing" );
    print LOGFHANDLE "\n\nStrats Pruned: rundate: $yyyymmdd_ :\n\n";
    print LOGFHANDLE $mailstr_."\n";
    close LOGFHANDLE;

    SendReportMail ( "Strats Pruned\n".$mailstr_."\n" );
  }
}

sub SendReportMail
{
  my $mailstr_ = shift;

  if ( $email_address_ && $#strats_to_prune_ >= 0 && $mailstr_ ne "" )
  {
    open ( MAIL , "|/usr/sbin/sendmail -t" );
    print MAIL "To: $email_address_\n";
    print MAIL "From: $email_address_\n";
    print MAIL "Subject: Prune Strats $shortcode_ $timeperiod_ ( $config_file_ )\n\n";
    print MAIL $mailstr_;
    close(MAIL);
  }
}

sub ReadPickstratConfigs
{
  my $strat_names_ref_ = shift;

  my @query_ids_vec_ = ( );

  my @configs_ = `ls $MODELING_BASE_DIR/pick_strats_config/\*/$shortcode_\.\*\.txt 2>/dev/null`;
  chomp( @configs_ );

  foreach my $tconfig_ ( @configs_ ) {
    if ( $tconfig_ ne "" ) {
      my $query_id_start_ = `$SCRIPTS_DIR/get_config_field.pl $tconfig_ PROD_QUERY_START_ID | grep -v '^#' | head -1 | awk '{print \$1}'`;
      chomp( $query_id_start_ );

      if ( $query_id_start_ eq "" ) {
        next;
      }

      my $query_id_end_ = `$SCRIPTS_DIR/get_config_field.pl $tconfig_ PROD_QUERY_STOP_ID | grep -v '^#' | head -1 | awk '{print \$1}'`;
      chomp ( $query_id_end_ );

      if ( $query_id_end_ eq "" ) { 
        $query_id_end_ = $query_id_start_ + 8;
      }

      push ( @query_ids_vec_, $query_id_start_..$query_id_end_ );
    }
  }

  my $tdate_ = $yyyymmdd_;
  my $numdays_ = 10;

  for ( my $iday_ = 0; $iday_ < $numdays_; $iday_++ ) {
    $tdate_ = CalcPrevWorkingDateMult ( $tdate_, 1 );
    
    ( my $yyyy_ , my $mm_ , my $dd_ ) = BreakDateYYYYMMDD ( $tdate_ );
    my $query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;

    foreach my $queryid_ ( @query_ids_vec_ ) {
      my $query_log_file_ = $query_log_dir_."/log.".$tdate_.".".$queryid_.".gz";
      if ( ! -e $query_log_file_ ) { next; }

      my @grep_lines_ = `zgrep STRATEGYLINE $query_log_file_`; chomp ( @grep_lines_ );

      foreach my $grep_line_  ( @grep_lines_ ) {
        my @grep_line_words_ = split ( ' ', $grep_line_ );
        push ( @$strat_names_ref_, $grep_line_words_[8] );
      }
    }
  }

  my %strat_names_hash_ = map { $_ => 1 } @$strat_names_ref_;
  @$strat_names_ref_ = keys %strat_names_hash_;
}

sub GetTimeperiodsForShortcode
{
  my $timperiods_vec_ = ( );

  my $top_directory_ = File::Spec->rel2abs ( $HOME_DIR."/modelling/strats/".$shortcode_ );
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

        if ( -d "$top_directory_/$dir_item_" ) {
          push ( @timeperiods_vec_, $dir_item_ );
        }
      }
    }
  }
  return @timeperiods_vec_;
}

