#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use Fcntl qw (:flock);
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

package ResultLine;
use Class::Struct;

# declare the struct
struct ( 'ResultLine', { pnl_ => '$', volume_ => '$', ttc_ => '$' } );
#
package main;
#
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/";
my $FBPA_WORK_DIR=$SPARE_HOME."FBPA/";
#
my $REPO="basetrade";
#
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats";

sub RunSimulationOnCandidate;
sub ComputeStatistics;

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; #GetMarketModelForShortcode
require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; #CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; #GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort#
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId ,#AllOutputFilesPopulated
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace ,#PrintStacktraceAndDie

# Exec dependencies
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my @ADDITIONAL_EXEC_PATHS=();
# Execs to be used here
my $SIM_STRATEGY_EXEC = SearchExec ( "sim_strategy", @ADDITIONAL_EXEC_PATHS ) ;

if ( ! $SIM_STRATEGY_EXEC ) {
  print "Did not find sim_strategy anywhere in search paths\n";
  exit(0);
} else {
#  print "found $SIM_STRATEGY_EXEC\n";
}
# End

my $MAX_STRAT_FILES_IN_ONE_SIM = 40; # please work on optimizing this value
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

if ( $#ARGV != 3 ) { print "USAGE shortcode_ trading_start_yymmdd_ trading_end_yymmdd_ strategy_filename_\n"; exit(0);}

my $shortcode_ = $ARGV[0];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[1] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $strat_filename_ = $ARGV[3];

$FBPA_WORK_DIR = $FBPA_WORK_DIR.$shortcode_."/".$trading_start_yyyymmdd_."-".$trading_end_yyyymmdd_."/";

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_;
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
  if ( -d $work_dir_ )
  {
    print STDERR "Surprising but this dir exists\n";
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    $work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_;
  }
  else
  {
    last;
  }
}


if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }

RunSimulationOnCandidatesAndSummarise ( );

exit ( 0 );

sub RunSimulationOnCandidatesAndSummarise
{
  my @unique_sim_id_list_ = ( );
  my @independent_parallel_commands_ = ( );
  my @tradingdate_list_ = ( );
  my @strategy_output_file_list_ = ( );
  my @strategy_pnl_stats_file_list_ = ( );
  my @strategy_log_file_list_ = ( );

  my $tradingdate_ = $trading_end_yyyymmdd_;
  my $max_days_at_a_time_ = 2000;
  my $number_of_strats_ = `wc -l $strat_filename_`;

  for ( my $i = 0; $i < $max_days_at_a_time_ && $tradingdate_ >= $trading_start_yyyymmdd_ ; $i++ )
  {
    if ( SkipWeirdDate ( $tradingdate_ ) ||
        IsDateHoliday ( $tradingdate_ ) ||
        IsProductHoliday ( $tradingdate_ , $shortcode_ ) ||
        NoDataDate ( $tradingdate_ ) )
    {
      $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_ , 1 );
      next;
    }

    my $strat_output_file_ = $work_dir_."/strat_output_file_".$tradingdate_.".txt";  
    my $strat_pnl_stats_file_ = $work_dir_."/strat_pnl_stats_file_".$tradingdate_.".txt";  

    my $unique_sim_id_ = GetGlobalUniqueId ( ); # Get a concurrency safe id.
    my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
    my $this_log_filename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".$unique_sim_id_;

    my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
    my $exec_cmd_ = $SIM_STRATEGY_EXEC." SIM ".$strat_filename_." ".$unique_sim_id_." ".$tradingdate_." ".$market_model_index_." ADD_DBG_CODE -1 > ".$strat_output_file_." 2>/dev/null ; ".$MODELSCRIPTS_DIR."/get_pnl_stats_2.pl $this_trades_filename_ > $strat_pnl_stats_file_ 2>/dev/null; rm -f $this_trades_filename_; rm -f $this_log_filename_ ";

    push ( @unique_sim_id_list_ , $unique_sim_id_ );
    push ( @independent_parallel_commands_ , $exec_cmd_ );

    push ( @tradingdate_list_ ,$tradingdate_ );
    push ( @strategy_output_file_list_ , $strat_output_file_ );
    push ( @strategy_pnl_stats_file_list_ , $strat_pnl_stats_file_ );
    push ( @strategy_log_file_list_ , $this_log_filename_ );

    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_ , 1 );
  }

  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; )
  {
    my @output_files_to_poll_this_run_ = ( );
    my @logfiles_files_to_poll_this_run_ = ( );

    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ )
    {
      push ( @output_files_to_poll_this_run_ , $strategy_output_file_list_ [ $command_index_ ] );
      push ( @logfiles_files_to_poll_this_run_ ,$strategy_output_file_list_ [ $command_index_ ] );

      { # empty the output result file.
        my $exec_cmd_ = "> ".$strategy_output_file_list_ [ $command_index_ ];
        `$exec_cmd_`;
      }

      my $exec_cmd_ = $independent_parallel_commands_ [ $command_index_ ];
      my $temp_script_ = $work_dir_."/temp_script_".$command_index_;
      open SCRIPT, "> $temp_script_" or PrintStacktraceAndDie ( "Could not open $temp_script_ for writing\n" );
      print SCRIPT "$exec_cmd_\n";
      close ( SCRIPT );

      my $pid_ = fork();
      die "unable to fork $!" unless defined($pid_);
      if ( !$pid_ )
      {
        #child process has pid 0
        exec("sh $temp_script_");
      }
      #back to parent process
      $command_index_ ++;
      sleep ( 1 );
    }
    
    my $t_pid_ = 9999;
    while ( $t_pid_ > 0 )
    {
# there are still some child processes running, wait returns -1 when no child
# process is left
      $t_pid_ = wait();
    }
  }

  my %unique_id_to_sumpnl_stats_map_ = ();
  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
  {
    my %unique_id_to_pnlstats_map_ = ( );
    my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];
    my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];
    my $strategy_output_file_ = $strategy_output_file_list_ [ $command_index_ ];
    my $strategy_pnl_stats_file_ = $strategy_pnl_stats_file_list_[ $command_index_ ];
    my $exec_cmd_ = "cat ".$strategy_output_file_list_[ $command_index_ ];
    my @tradeinit_output_lines_ = `$exec_cmd_`;

    if ( ExistsWithSize ( $strategy_pnl_stats_file_ ) )
    {
      my $exec_cmd_ = "cat $strategy_pnl_stats_file_";
      my @pnlstats_output_lines_ = `$exec_cmd_`;

      for ( my $t_pnlstats_output_lines_index_ = 0; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
      {
        my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
        if( $#rwords_ >= 1 )
        {
          my $unique_strat_id_ = $rwords_[0];
          splice ( @rwords_, 0, 1 );
# remove
# the first word since it is unique_strat_id_
          $unique_id_to_pnlstats_map_{$unique_strat_id_} = join ( ' ', @rwords_ );
        }
      }
    }

    for ( my $t_tradeinit_output_lines_index_ = 0, my $psindex_ = 0;$t_tradeinit_output_lines_index_ <= $#tradeinit_output_lines_;$t_tradeinit_output_lines_index_ ++ )
    {
      if ( $tradeinit_output_lines_ [ $t_tradeinit_output_lines_index_ ] =~ /SIMRESULT/ )
      { # SIMRESULT pnl volume                                                                                                                                                                                               
        my @rwords_ = split ( ' ', $tradeinit_output_lines_[$t_tradeinit_output_lines_index_]);
        splice ( @rwords_, 0, 1 ); 
        my $remaining_simresult_line_ = join ( ' ',@rwords_ );
        if ( ( $rwords_[1] > 0 )  || ( ( $shortcode_ =~/BAX/) && ( $rwords_[1] >= 0 ) ) )
        {
          my $unique_strat_id_ = GetUniqueSimIdFromCatFile ( $strat_filename_, $psindex_ );
          if ( ! exists $unique_id_to_pnlstats_map_{$unique_strat_id_} )
          {
            $unique_id_to_pnlstats_map_{$unique_strat_id_} = "0 0 0 0 0 0 0 0 0 0 0";
          }
          $remaining_simresult_line_ = $remaining_simresult_line_." ".$unique_id_to_pnlstats_map_{$unique_strat_id_};
          if ( ! exists $unique_id_to_sumpnl_stats_map_{$unique_strat_id_} )
          {
            $unique_id_to_sumpnl_stats_map_{$unique_strat_id_} = ( );
            push(@{$unique_id_to_sumpnl_stats_map_{$unique_strat_id_}},$remaining_simresult_line_);
          }
          else
          {
            push(@{$unique_id_to_sumpnl_stats_map_{$unique_strat_id_}},$remaining_simresult_line_);           
          }
        }
        $psindex_++;
      }
    }
  } 
  
  foreach my $id ( keys %unique_id_to_sumpnl_stats_map_ )
  {
    my @array = @{$unique_id_to_sumpnl_stats_map_{$id}};
    print $id." ".ComputeStatistics(@array)."\n";
  }
  `rm -rf $work_dir_`; #lot of space is consumed
}

sub ComputeStatistics
{
  my @all_stats_ = @_;
  
  my @pnl_vec_ = ();
  my @vol_vec_ = ();
  my @supporting_order_filled_percent_vec_ = ();
  my @best_level_order_filled_percent_vec_ = ();
  my @aggressive_order_filled_percent_vec_ = ();
  my @improve_order_filled_percent_vec_ = ();
  my @average_abs_position_vec_ = ();
  my @median_time_to_close_trades_vec_ = ();
  my @average_time_to_close_trades_vec_ = ();
  my @median_closed_trade_pnls_vec_ = ();
  my @average_closed_trade_pnls_vec_ = ();
  my @stdev_closed_trade_pnls_vec_ = ();
  my @sharpe_closed_trade_pnls_vec_ = ();
  my @fracpos_closed_trade_pnls_vec_ = ();
  my @min_pnl_vec_ = ();
  my @max_pnl_vec_ = ();
  my @max_drawdown_vec_ = ();
  my @max_time_to_close_vec_ = ();
  my @msg_count_vec_ = ();
  my @volume_normalized_average_time_to_close_trades_vec_ = ();
  my @num_opentrade_hits_vec_ = ();
  my @abs_closing_position_vec_ = ();
  my @unit_trade_size_vec_ = ();
  my @ptrades_vec_ = ();
  my @ttrades_vec_ = ();

  foreach my $str (@all_stats_)
  {
    my @stat_vec_ = split(' ',$str);
    push ( @pnl_vec_,$stat_vec_[0] );
    push ( @vol_vec_,$stat_vec_[1] );
    push ( @supporting_order_filled_percent_vec_,$stat_vec_[2] );
    push ( @best_level_order_filled_percent_vec_,$stat_vec_[3] );
    push ( @aggressive_order_filled_percent_vec_,$stat_vec_[4] );
    push ( @improve_order_filled_percent_vec_,$stat_vec_[5] );
    push ( @average_abs_position_vec_,$stat_vec_[6] );
    push ( @median_time_to_close_trades_vec_,$stat_vec_[7] );
    push ( @average_time_to_close_trades_vec_,$stat_vec_[8] );
    push ( @median_closed_trade_pnls_vec_,$stat_vec_[9] );
    push ( @average_closed_trade_pnls_vec_,$stat_vec_[10] );
    push ( @stdev_closed_trade_pnls_vec_,$stat_vec_[11] );
    push ( @sharpe_closed_trade_pnls_vec_,$stat_vec_[12] );
    push ( @fracpos_closed_trade_pnls_vec_,$stat_vec_[13] );
    push ( @min_pnl_vec_,$stat_vec_[14] );
    push ( @max_pnl_vec_,$stat_vec_[15] );
    push ( @max_drawdown_vec_,$stat_vec_[16] );
    push ( @max_time_to_close_vec_,$stat_vec_[17] );
    push ( @msg_count_vec_,$stat_vec_[18] );
    push ( @volume_normalized_average_time_to_close_trades_vec_,$stat_vec_[19] );
    push ( @num_opentrade_hits_vec_,$stat_vec_[20] );
    push ( @abs_closing_position_vec_,$stat_vec_[21] );
    push ( @unit_trade_size_vec_,$stat_vec_[22] );
    push ( @ptrades_vec_,$stat_vec_[23] );
    push ( @ttrades_vec_,$stat_vec_[24] );
  }

  my $pnl_average_ = int (GetAverage ( \@pnl_vec_) );
  my $pnl_stdev_ = int ( GetStdev ( \@pnl_vec_ ) ) ;
  my $vol_average_ = int( GetAverage ( \@vol_vec_ ));
  my $pnl_sharpe_ = $pnl_average_ * 1.0 / $pnl_stdev_;
  my $pnl_adj_average_ = int( ($pnl_average_ - 0.33 * $pnl_stdev_));
  my $average_min_adjusted_pnl_ = GetAverage ( \@min_pnl_vec_ );
  my $median_average_time_to_close_trades_ = GetMedianConst ( \@average_time_to_close_trades_vec_ );
  my $median_volume_normalized_average_time_to_close_trades_ = GetMedianConst ( \@volume_normalized_average_time_to_close_trades_vec_ ) ;
  my $pnl_per_contract_ = $pnl_average_ * 1.0 / $vol_average_;
  my $supporting_order_filled_percent_ = int( GetAverage (\@supporting_order_filled_percent_vec_));
  my $best_order_filled_percent_ = int( GetAverage (\@best_level_order_filled_percent_vec_) );
  my $aggressive_order_filled_percent_ = int (GetAverage (\@aggressive_order_filled_percent_vec_));
  my $improve_order_filled_percent_ = int( GetAverage (\@improve_order_filled_percent_vec_));
  my $average_max_drawdown_ = int(GetAverage ( \@max_drawdown_vec_ ));
  my $average_abs_position_ = int(GetAverage ( \@average_abs_position_vec_ ));
  my $pnl_conservative_average_ = $pnl_average_;
  my $pnl_median_average_ = GetAverage ( \@median_closed_trade_pnls_vec_ );
  my $drawdown_adjusted_pnl_average_ = $pnl_average_ * 1.0 / GetMaxDrawdown (\@pnl_vec_ );
  my $average_message_count_ = int(GetAverage ( \@msg_count_vec_ ));
  my $gain_to_pain_ratio_ = GetGainToPainRatio ( \@pnl_vec_ );
  my $pnl_by_maxloss_ = $pnl_average_ * 1.0 / GetAverage ( \@min_pnl_vec_ );
  my $ninetyfive_ = int(GetMeanHighestQuartile ( \@pnl_vec_ ));
  my $average_num_opentrade_hits_ = int(GetAverage ( \@num_opentrade_hits_vec_));
  my $average_abs_closing_position_ = int(GetAverage (\@abs_closing_position_vec_ ));
  my $pnl_skewness_ = $pnl_average_;
  my $average_uts_ = int(GetAverage ( \@unit_trade_size_vec_ ));
  my $percent_positive_trades_ = int(GetAverage ( \@ptrades_vec_ ));

  return $pnl_average_." ".$pnl_stdev_." ".$vol_average_." ".$pnl_sharpe_." ".$pnl_adj_average_." ".$average_min_adjusted_pnl_." ".$median_average_time_to_close_trades_." ".$median_volume_normalized_average_time_to_close_trades_." ".$pnl_per_contract_." ".$supporting_order_filled_percent_." ".$best_order_filled_percent_." ".$aggressive_order_filled_percent_." ".$improve_order_filled_percent_." ".$average_max_drawdown_." ".$average_abs_position_ ." ".$pnl_conservative_average_ ." ".$pnl_median_average_." ".$drawdown_adjusted_pnl_average_." ".$average_message_count_." ".$gain_to_pain_ratio_." ".$pnl_by_maxloss_." ".$ninetyfive_." ".$average_num_opentrade_hits_." ".$average_abs_closing_position_." ".$pnl_skewness_." ".$average_uts_." ".$percent_positive_trades_;

}





