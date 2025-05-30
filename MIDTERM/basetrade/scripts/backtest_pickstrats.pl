#!/usr/bin/perl
use strict;
use warnings;
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use List::MoreUtils qw/ uniq /;
use Math::Complex; # sqrt
use FileHandle;
use POSIX;
use sigtrap qw(handler signal_handler normal-signals error-signals);

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV{'HOME'};
my $REPO = "basetrade";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $PICKSTRATS_DIR = "/spare/local/pickstrats_logs";
my $PICKSTRAT_SIMOUT_DIR = $PICKSTRATS_DIR."/simout_logs";
my $PICKSTRATS_RESULT_STAT_DIR = $PICKSTRATS_DIR."/result_stats";
my $PICKSTRATS_SCRIPT_LOG_DIR = $PICKSTRATS_DIR."/pick_strats_script_logs";
my $PICKSTRATS_BACKTEST_DIR = $PICKSTRATS_DIR."/backtest_logs";
my $PICKSTRAT_TEMP_DIR = $PICKSTRATS_DIR."/temp_dir";
#my $GLOBAL_RESULTS_DIR = $HOME_DIR."/ec2_globalresults";
my $GLOBAL_RESULTS_DIR = "DB"; #changed the primary file location of the globalresults from FS to DB

if ( ! -e $PICKSTRATS_DIR ) { `mkdir $PICKSTRATS_DIR`; }
if ( ! -e $PICKSTRAT_SIMOUT_DIR ) { `mkdir $PICKSTRAT_SIMOUT_DIR`; }
if ( ! -e $PICKSTRATS_RESULT_STAT_DIR ) { `mkdir $PICKSTRATS_RESULT_STAT_DIR`; }
if ( ! -e $PICKSTRAT_TEMP_DIR ) { `mkdir $PICKSTRAT_TEMP_DIR`; }

require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_query_type_for_id_date.pl"; # GetQueryTypeForIdDate
require "$GENPERLLIB_DIR/get_unix_time_from_utc.pl"; #GetUnixtimeFromUTC
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/array_ops.pl";

my $mail_address_ = "nseall@tworoads.co.in";
#my $mail_address_ = "hrishav.agarwal\@tworoads.co.in";
#print $mail_address_."\n";

my $usage_ = "$0 DATE SHORTCODE TIMEOFDAY [ CONFIG-FILE ]";

if ( $#ARGV < 2 ) {
  print $usage_."\n";
  exit ( 0 );
}

my $end_date_ = $ARGV [ 0 ]; chomp ( $end_date_ );
print $end_date_."\n";
my $shortcode_ = $ARGV [ 1 ]; chomp ( $shortcode_ );
my $timeofday_ = $ARGV [ 2 ]; chomp ( $timeofday_ );
my $querystartid_ = -1;
my $queryendid_ = -1;
my $config_file_ = "INVALID";
if ( $#ARGV > 2 ) {
  $config_file_ = $ARGV [ 3 ];
}
my @intv_days_ = (10,30,90,150);
my $max_intv_ = max @intv_days_;

my $pickstrats_queryids_file_ = $PICKSTRATS_DIR."/pickstrats_queryids";
my $pickstrats_script_file_ = $MODELSCRIPTS_DIR."/pick_strats_and_install.pl";
print $pickstrats_queryids_file_."\n";

my @temp_files_ = ( );

# Read pickstrats_queryids_file_ and store the queryids in a hash table
open QUERYIDS_FILE_HANDLE, "< $pickstrats_queryids_file_" or PrintStacktraceAndDie ( "Could not open $pickstrats_queryids_file_\n" );
my @lines_ = <QUERYIDS_FILE_HANDLE>;
close QUERYIDS_FILE_HANDLE;
foreach my $line_ ( @lines_ ) {
  chomp ( $line_ );
  if ( index ( $line_ , "#" ) == 0 ) {
    next;
  }
  my @line_words_ = split ( " " , $line_ );
  if ( $#line_words_ < 4 ) {
    next;
  }
  my $this_shc_ = $line_words_ [ 0 ];
  my $this_tod_ = $line_words_ [ 1 ];
  if ( $this_shc_ eq $shortcode_ && $this_tod_ eq $timeofday_ ) {
    $querystartid_ = $line_words_ [ 2 ];
    $queryendid_   = $line_words_ [ 3 ];
    if ( $config_file_ eq "INVALID" ) { $config_file_   = $line_words_ [ 4 ]; }
  }
}

if ( $querystartid_ == -1 || 
    $queryendid_ == -1 ||
    $config_file_ eq "INVALID" ) {
  print "Either the pickstrat config file is empty OR no product in the config file matches the provided product\n";
  exit ( 0 );
}
print "$shortcode_ $timeofday_ $querystartid_ $queryendid_ $config_file_\n";

my %intv_start_days_ = ( );
my %intv_ndays_ = ( );

my @real_picked_strats_ = ( );
my @pickstrats_picked_strats_ = ( );
my $read_OML_ = 0;
my $OML_lookback_days_ = 0;
my $OML_hit_ratio_ = 0;
my $OML_number_top_loss_ = 0;
my $max_loss_per_unit_size_ = 0;
my $min_max_loss_per_unit_size_ = 0;
my $max_max_loss_per_unit_size_ = 0;

my %date_to_picked_sum_pnl_ = ( );
my %date_to_picked_avg_drawdown_ = ( );
my %date_to_picked_volume_ = ( );
my %date_to_real_sum_pnl_ = ( );
my %date_to_real_avg_drawdown_ = ( );
my %date_to_real_volume_ = ( );

my %intv_to_real_pnl_ = ( );
my %intv_to_real_drawdown_ = ( );
my %intv_to_real_volume_ = ( );
my %intv_to_picked_pnl_ = ( );
my %intv_to_picked_drawdown_ = ( );
my %intv_to_picked_volume_ = ( );

my $log_dir_ = $PICKSTRATS_BACKTEST_DIR."/".$shortcode_."_".$timeofday_;
if ( ! -e $log_dir_ ) { `mkdir -p $log_dir_`; }
my $config_basename_ = `basename $config_file_`; chomp($config_basename_);
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my $log_filepath_ = $log_dir_."/log_".$config_basename_."_".$unique_gsm_id_;
while ( -e $log_filepath_ ) {
  $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
  $log_filepath_ = $log_dir_."/log_".$config_basename_."_".$unique_gsm_id_;
}
open LOG_FILE_,  "> $log_filepath_" or PrintStacktraceAndDie ( "Could not open file $log_filepath_ for writing.\n" );

CompareForDates ( );

RemoveTempFiles ( );

print "Detailed Results file: ".$log_filepath_."\n";

close (LOG_FILE_);

sub CompareForDates {
  foreach my $t_intv_ ( @intv_days_ )  {
    $intv_start_days_ { $t_intv_ } = CalcPrevWorkingDateMult ( $end_date_, $t_intv_ );
    $intv_ndays_ { $t_intv_ } = 0;
  }

  my $start_date_ = $intv_start_days_ { $max_intv_ };
  my $t_date_ = $end_date_;
  while ( $t_date_ > $start_date_ ) {
    if ( SkipWeirdDate( $t_date_ ) ||
        IsDateHoliday( $t_date_ ) ||
        IsProductHoliday( $t_date_, $shortcode_ ) ||
        NoDataDateForShortcode ( $t_date_ , $shortcode_ ) ||
        !ValidDate( $t_date_ ) ) {
      print "No Data for date ".$t_date_." for shortcode ".$shortcode_."\n";
      $t_date_ = CalcPrevWorkingDateMult ( $t_date_, 1 );
      next;
    }

    CompareForDate ( $t_date_ );

    if ( ! exists $date_to_picked_sum_pnl_ { $t_date_ } ||
        ! exists $date_to_real_sum_pnl_ { $t_date_ } ) {
      print "REAL_PICKED or AUTOMATED_PICKED for $t_date_ does NOT EXIST\n";
      $t_date_ = CalcPrevWorkingDateMult ( $t_date_, 1 );
      next;
    }

    foreach my $t_intv_ ( @intv_days_ )  {
      if ( $t_date_ > $intv_start_days_ { $t_intv_ } ) {
        push ( @{$intv_to_real_pnl_ { $t_intv_ }}, $date_to_real_sum_pnl_ { $t_date_ } );
        push ( @{$intv_to_real_drawdown_ { $t_intv_ }}, $date_to_real_avg_drawdown_ { $t_date_ } );
        push ( @{$intv_to_real_volume_ { $t_intv_ }}, $date_to_real_volume_ { $t_date_ } );
        push ( @{$intv_to_picked_pnl_ { $t_intv_ }}, $date_to_picked_sum_pnl_ { $t_date_ } );
        push ( @{$intv_to_picked_drawdown_ { $t_intv_ }}, $date_to_picked_avg_drawdown_ { $t_date_ } );
        push ( @{$intv_to_picked_volume_ { $t_intv_ }}, $date_to_picked_volume_ { $t_date_ } );

        $intv_ndays_ { $t_intv_ } += 1;
      }
    }
    $t_date_ = CalcPrevWorkingDateMult ( $t_date_, 1 );
  }

  foreach my $t_intv_ ( @intv_days_ )  {
    my $ndays_ = scalar ( @{$intv_to_real_pnl_ { $t_intv_ }} );
    if ( $ndays_ > 0 ) {
      my $sum_pnl_ = GetSum ( \@{$intv_to_real_pnl_ { $t_intv_ }} );
      my $avg_pnl_ = $sum_pnl_ / $ndays_;
      my $std_pnl_ = GetStdev( \@{$intv_to_real_pnl_ { $t_intv_ }} );

      my $sum_vol_ = GetSum ( \@{$intv_to_real_volume_ { $t_intv_ }} );
      my $avg_vol_ = $sum_vol_ / $ndays_;
      my $std_vol_ = GetStdev( \@{$intv_to_real_volume_ { $t_intv_ }} );

      my $pnl_dd_ = GetAverage( \@{$intv_to_real_drawdown_ { $t_intv_ }} );

      print "INTV: ".$t_intv_.", REAL STATS:\n";
      print "PNL_SUM \t PNL_DD \t PNL_MEAN \t PNL_STDEV \t VOL_SUM \t VOL_MEAN \t VOL_STDEV\n" ;
      printf "%.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \n\n", $sum_pnl_, $pnl_dd_, $avg_pnl_, $std_pnl_, $sum_vol_, $avg_vol_, $std_vol_ ;

      print LOG_FILE_ "INTV: ".$t_intv_.", REAL STATS:\n";
      print LOG_FILE_ "PNL_SUM \t PNL_DD \t PNL_MEAN \t PNL_STDEV \t VOL_SUM \t VOL_MEAN \t VOL_STDEV\n" ;
      printf LOG_FILE_ "%.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \n\n", $sum_pnl_, $pnl_dd_, $avg_pnl_, $std_pnl_, $sum_vol_, $avg_vol_, $std_vol_ ;
      
      $sum_pnl_ = GetSum ( \@{$intv_to_picked_pnl_ { $t_intv_ }} );
      $avg_pnl_ = $sum_pnl_ / $ndays_;
      $std_pnl_ = GetStdev( \@{$intv_to_picked_pnl_ { $t_intv_ }} );

      $sum_vol_ = GetSum ( \@{$intv_to_picked_volume_ { $t_intv_ }} );
      $avg_vol_ = $sum_vol_ / $ndays_;
      $std_vol_ = GetStdev( \@{$intv_to_picked_volume_ { $t_intv_ }} );

      $pnl_dd_ = GetAverage( \@{$intv_to_picked_drawdown_ { $t_intv_ }} );

      print "INTV: ".$t_intv_.", PICKED STATS:\n";
      print "PNL_SUM \t PNL_DD \t PNL_MEAN \t PNL_STDEV \t VOL_SUM \t VOL_MEAN \t VOL_STDEV\n" ;
      printf "%.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \n\n", $sum_pnl_, $pnl_dd_, $avg_pnl_, $std_pnl_, $sum_vol_, $avg_vol_, $std_vol_ ;

      print LOG_FILE_ "INTV: ".$t_intv_.", PICKED STATS:\n";
      print LOG_FILE_ "PNL_SUM \t PNL_DD \t PNL_MEAN \t PNL_STDEV \t VOL_SUM \t VOL_MEAN \t VOL_STDEV\n" ;
      printf LOG_FILE_ "%.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \n\n", $sum_pnl_, $pnl_dd_, $avg_pnl_, $std_pnl_, $sum_vol_, $avg_vol_, $std_vol_ ;
      
    }
  }
}

sub CompareForDate {
  my $t_date_ = shift;

  ( my $yyyy_ , my $mm_ , my $dd_ ) = BreakDateYYYYMMDD ( $t_date_ );
  my $query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;

  @real_picked_strats_ = ( );
  @pickstrats_picked_strats_ = ( );
  my %query_real_picked_strats_ = ( );

  $read_OML_ = 0;
  $OML_lookback_days_ = 0;
  $OML_hit_ratio_ = 0;
  $OML_number_top_loss_ = 0;
  $max_loss_per_unit_size_ = 0;
  $min_max_loss_per_unit_size_ = 0;
  $max_max_loss_per_unit_size_ = 0;

# Get real picked query names
  for ( my $queryid_  = $querystartid_; $queryid_ <= $queryendid_; $queryid_++ ) {
    my $query_log_file_ = $query_log_dir_."/log.".$t_date_.".".$queryid_.".gz";
    if ( ! -e $query_log_file_ ) {
      next;
    }
    my $query_log_file_local_ = $PICKSTRAT_SIMOUT_DIR."/log.".$t_date_.".".$queryid_;
    `cp $query_log_file_ $query_log_file_local_.gz`;
    if ( -e $query_log_file_local_ ) {
      `rm $query_log_file_local_`;
    }
    `gunzip $query_log_file_local_.gz`;
    push ( @temp_files_, $query_log_file_local_ );
    my @query_file_name_ = `head -n1 $query_log_file_local_`; chomp ( @query_file_name_ );
    my @grep_lines_ = `grep STRATEGYLINE $query_log_file_local_`; chomp ( @grep_lines_ );

    foreach my $grep_line_  ( @grep_lines_ ) {
      my @grep_line_words_ = split ( ' ', $grep_line_ );
      my $t_strat_name_ = $grep_line_words_[8];
      my $t_query_id_ = $grep_line_words_[7];
      $query_real_picked_strats_ { $t_query_id_ } = $t_strat_name_;
    }
  }
  foreach my $query_id_ ( keys %query_real_picked_strats_ ) {
    push ( @real_picked_strats_ , $query_real_picked_strats_ { $query_id_ } );
  }

  if ( $#real_picked_strats_ < 0 ) {
# If in real, no strat was installed, then no comparison need to be done
    return;
  }

# Change DATE, NUM_STRATS_TO_INSTALL, INSTALL_PICKED_STRATS, EMAIL in the config
  my $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$t_config_." DATE";
  my @date_output_ = `$exec_cmd_`; chomp ( @date_output_ );

  $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." NUM_STRATS_TO_INSTALL";
  my @num_strats_to_install_output_ = `$exec_cmd_`; chomp ( @num_strats_to_install_output_ );

  $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$t_config_." INSTALL_PICKED_STRATS";
  my @install_picked_strats_output_ = `$exec_cmd_`; chomp ( @install_picked_strats_output_ );

  $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." TOTAL_SIZE_TO_RUN";
  my @total_size_to_install_output_ = `$exec_cmd_`; chomp ( @total_size_to_install_output_ );

  $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$t_config_." EMAIL";
  my @email_output_ = `$exec_cmd_`; chomp ( @email_output_ );

  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_." DATE ".$t_date_; `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." NUM_STRATS_TO_INSTALL ".( $#real_picked_strats_ + 1 ); `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_." INSTALL_PICKED_STRATS ".( 0 ); `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." TOTAL_SIZE_TO_RUN ".( $#real_picked_strats_ + 1 ); `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_." EMAIL #"; `$exec_cmd_`;

# Finally run pickstrats
  $exec_cmd_ = $pickstrats_script_file_." ".$shortcode_." ".$timeofday_." ".$config_file_." 1 1";
#  print $exec_cmd_."\n";
  @exec_output_ = `$exec_cmd_`;

  my $script_log_name_ = $PICKSTRATS_SCRIPT_LOG_DIR."/log.".$t_date_.".".$shortcode_."_".$timeofday_;
  open SCRIPT_LOG_HANDLE, "> $script_log_name_" or PrintStacktraceAndDie ( "Could not open $script_log_name_\n" );
  print SCRIPT_LOG_HANDLE join("", @exec_output_)."\n";
  close ( SCRIPT_LOG_HANDLE );

  chomp ( @exec_output_ );
  for (my $idx_ = 0; $idx_ <= $#exec_output_; $idx_++) {
    if ( index ( $exec_output_ [ $idx_ ] , "PICKED:" ) != -1 ) {
      push ( @pickstrats_picked_strats_ , substr ( $exec_output_ [ $idx_ ] , 8 ) );
    }
  }

  for (my $idx_ = 0; $idx_ <= $#exec_output_; $idx_++) {
    my @tokens_ = split (' ', $exec_output_ [ $idx_ ] ); chomp ( @tokens_ );
    if ( $#tokens_ < 0 ) { next; }
    if ( index ($tokens_[0], "OML_Setting" ) != -1 && $#tokens_ >= 3 ) {
      $OML_lookback_days_ = $tokens_[1];
      $OML_hit_ratio_ = $tokens_[2];
      $OML_number_top_loss_ = $tokens_[3];
      $read_OML_ = 1;
#      print "reading OML_Setting..\n";
    }
    if ( index ($tokens_[0], "MAX_MAX_LOSS") != -1 && $#tokens_ >= 1 ) {
      $max_max_loss_per_unit_size_ = $tokens_[1];
    }
    if ( $tokens_[0] eq "MIN_MAX_LOSS" && $#tokens_ >= 1 ) {
      $min_max_loss_per_unit_size_ = $tokens_[1];
    }
    if ( $tokens_[0] eq "MAX_LOSS" && $#tokens_ >= 1 ) {
      $max_loss_per_unit_size_ = $tokens_[1];
    }
  }
  if ( ( ! $read_OML_ ) && ( ! $max_loss_per_unit_size_ ) ) {
    print "No OML Setting or MAX_LOSS received for date $t_date_\n";
    return ;
  }

# Set the original things back in
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." DATE ".join(' ', map { qq/"$_"/ } @date_output_); `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." NUM_STRATS_TO_INSTALL ".join(' ', map { qq/"$_"/ } @num_strats_to_install_output_); `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." INSTALL_PICKED_STRATS ".join(' ', map { qq/"$_"/ } @install_picked_strats_output_);  `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." TOTAL_SIZE_TO_RUN ".join(' ', map { qq/"$_"/ } @total_size_to_install_output_);  `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." EMAIL ".join(' ', map { qq/"$_"/ } @email_output_);  `$exec_cmd_`;
  
  ComputeNextDaySimResultsLog ( $t_date_ );
}

sub ComputeNextDaySimResultsLog {
  my $t_date_ = shift;

  my $log_body_ = "";
  $log_body_ = $log_body_."DATE $t_date_, PRODUCT $shortcode_ $timeofday_ \n\n";

  my $temp_strategy_list_file_ = $PICKSTRAT_TEMP_DIR."/temp_strategy_list_file_.".$shortcode_."_".$timeofday_;
  push ( @temp_files_, $temp_strategy_list_file_ );
  open TEMP_FILE_HANDLE, "> $temp_strategy_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );
  for (my $idx_ = 0; $idx_ <= $#real_picked_strats_; $idx_++) {
    print TEMP_FILE_HANDLE $real_picked_strats_ [ $idx_ ]."\n";
  }
  for (my $idx_ = 0; $idx_ <= $#pickstrats_picked_strats_; $idx_++) {
    print TEMP_FILE_HANDLE $pickstrats_picked_strats_ [ $idx_ ]."\n";
  }
  close ( TEMP_FILE_HANDLE );
  my $exec_cmd_ = $BIN_DIR."/summarize_strategy_results $shortcode_ $temp_strategy_list_file_ $GLOBAL_RESULTS_DIR $t_date_ $t_date_";
#  print $exec_cmd_."\n";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );
#print join('\n', @exec_output_)."\n";
  my %strat_name_to_pnl_ = ( );
  my %strat_name_to_volume_ = ( );
  my %strat_name_to_drawdown_ = ( );
  foreach my $output_line_ ( @exec_output_ ) {
    my @output_line_words_ = split ( ' ' , $output_line_ );
    my $max_loss_per_uts_ = ComputeMaxLossPerUTS ( $output_line_words_ [ 1 ], $t_date_ );
    $exec_cmd_ = $BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$output_line_words_[1]." ".$t_date_;
    my $unit_trade_size_ = `$exec_cmd_`; chomp ( $unit_trade_size_ );
    my $max_loss_ = -1 * $max_loss_per_uts_ * $unit_trade_size_;
    my $min_pnl_ = $output_line_words_ [ 22 ];

    my $strat_pnl_ = $output_line_words_ [ 2 ];
    if ( $min_pnl_ < $max_loss_ ) {
      $strat_pnl_ = $max_loss_;
      print "Strat ".$output_line_words_ [ 1 ]." hit MAX_LOSS: ".$max_loss_."..\n";
    }

#    print "PNL / MIN_PNL for ".$output_line_words_ [ 1 ]." : ".$output_line_words_ [ 2 ]." / ".$output_line_words_ [ 22 ]."\n";
#    print "MAX_LOSS for ".$output_line_words_ [ 1 ]." : ".$max_loss_."..\n";

    $strat_name_to_pnl_ { $output_line_words_ [ 1 ] } = $strat_pnl_ ;
    $strat_name_to_volume_ { $output_line_words_ [ 1 ] } = $output_line_words_ [ 4 ];
    $strat_name_to_drawdown_ { $output_line_words_ [ 1 ] } = $output_line_words_ [ 16 ];
  }

  my $real_sum_pnl_ = 0;
  my $real_avg_drawdown_ = 0;
  my $real_avg_volume_ = 0;
  my $picked_sum_pnl_ = 0;
  my $picked_avg_drawdown_ = 0;
  my $picked_avg_volume_ = 0;

  if ( $#real_picked_strats_ >= 0 ) {
    for (my $idx_ = 0; $idx_ <= $#real_picked_strats_; $idx_++) {
#    print  $real_picked_strats_ [ $idx_ ] . "\n";
      if ( exists $strat_name_to_pnl_ { $real_picked_strats_ [ $idx_ ] } ) {
        $real_sum_pnl_ += $strat_name_to_pnl_ { $real_picked_strats_ [ $idx_ ] };
        $real_avg_drawdown_ += $strat_name_to_drawdown_ { $real_picked_strats_ [ $idx_ ] };
        $real_avg_volume_ += $strat_name_to_volume_ { $real_picked_strats_ [ $idx_ ] };
      }
      else {
        print "Results for strat ".$real_picked_strats_ [ $idx_ ]." does NOT exist for date ".$t_date_."\n";
        splice @real_picked_strats_, $idx_, 1;
        $idx_--;
      }
    }
    if ( $#real_picked_strats_ >= 0 ) {
      $real_avg_drawdown_ = $real_avg_drawdown_ / ( $#real_picked_strats_+1 );
      $real_avg_volume_ = $real_avg_volume_ / ( $#real_picked_strats_+1 );
    }
  }
  else {
    print "NOTE: No Real strats for ".$shortcode_."_".$timeofday_."\n\n";
    return;
  }

  if ( $#pickstrats_picked_strats_ >= 0) {
    for (my $idx_ = 0; $idx_ <= $#pickstrats_picked_strats_; $idx_++) {
      if ( exists $strat_name_to_pnl_ { $pickstrats_picked_strats_ [ $idx_ ] } ) {
        $picked_sum_pnl_ += $strat_name_to_pnl_ { $pickstrats_picked_strats_ [ $idx_ ] };
        $picked_avg_drawdown_ += $strat_name_to_drawdown_ { $pickstrats_picked_strats_ [ $idx_ ] };
        $picked_avg_volume_ += $strat_name_to_volume_ { $pickstrats_picked_strats_ [ $idx_ ] };
      }
      else {
        print "Results for strat ".$pickstrats_picked_strats_ [ $idx_ ]." does NOT exist for date ".$t_date_."\n";
        splice @pickstrats_picked_strats_, $idx_, 1;
        $idx_--;
      }
    }
    if ( $#pickstrats_picked_strats_ >= 0 ) {
      $picked_avg_drawdown_ = $picked_avg_drawdown_ / ( $#pickstrats_picked_strats_+1 );
      $picked_avg_volume_ = $picked_avg_volume_ / ( $#pickstrats_picked_strats_+1 );
    }
  }
  else {
    print "NOTE: No Picked strats for ".$shortcode_."_".$timeofday_."\n\n";
    return;
  }

  $date_to_picked_sum_pnl_ { $t_date_ } = $picked_sum_pnl_;
  $date_to_picked_avg_drawdown_ { $t_date_ } = $picked_avg_drawdown_;
  $date_to_picked_volume_ { $t_date_ } = $picked_avg_volume_;
  $date_to_real_sum_pnl_ { $t_date_ } = $real_sum_pnl_;
  $date_to_real_avg_drawdown_ { $t_date_ } = $real_avg_drawdown_;
  $date_to_real_volume_ { $t_date_ } = $real_avg_volume_;


  $log_body_ = $log_body_."REAL_PICKED ".$real_sum_pnl_." ".$real_avg_drawdown_."\n";
  for (my $idx_ = 0; $idx_ <= $#real_picked_strats_; $idx_++) {
    $log_body_ = $log_body_.$strat_name_to_pnl_ { $real_picked_strats_ [ $idx_ ] }." ".$strat_name_to_volume_ { $real_picked_strats_ [ $idx_ ] }." ".$strat_name_to_drawdown_ { $real_picked_strats_ [ $idx_ ] }." ".$real_picked_strats_ [ $idx_ ]."\n";
  }
  $log_body_ = $log_body_."\n\n";

  $log_body_ = $log_body_."AUTOMATED_PICKED ".$picked_sum_pnl_." ".$picked_avg_drawdown_."\n";
  for (my $idx_ = 0; $idx_ <= $#pickstrats_picked_strats_; $idx_++) {
    $log_body_ = $log_body_.$strat_name_to_pnl_ { $pickstrats_picked_strats_ [ $idx_ ] }." ".$strat_name_to_volume_ { $pickstrats_picked_strats_ [ $idx_ ] }." ".$strat_name_to_drawdown_ { $pickstrats_picked_strats_ [ $idx_ ] }." ".$pickstrats_picked_strats_ [ $idx_ ]."\n";
  }
  $log_body_ = $log_body_."\n";

  print LOG_FILE_ $log_body_;
}

sub ComputeMaxLossPerUTS { 
  my $strat_name_ = shift;
  my $t_date_ = shift;
  my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl";

  my $optimal_max_loss_ = -1;
  my $max_loss_ = -1;


  if ( ( ! $read_OML_ ) && $max_loss_per_unit_size_ ) {
    $max_loss_ = $max_loss_per_unit_size_;
  }
  else {
    my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shortcode_ $timeofday_ $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ $strat_name_ $t_date_ ";
    my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );

# Consider the first result within range and not hitting max-loss too often
    foreach my $max_loss_line_ ( @exec_output_ )
    {
      if ( index ( $max_loss_line_ , "=>" ) >= 0 || index ( $max_loss_line_ , "MAX_LOSS" ) >= 0 )
      {
        next;
      }

      my @max_loss_words_ = split ( ' ' , $max_loss_line_ );
      if ( $#max_loss_words_ >= 2 )
      {
        my $max_loss_ = $max_loss_words_ [ 0 ];
        my $num_max_loss_hits_ = $max_loss_words_ [ 2 ];
        if ( $num_max_loss_hits_ <= $OML_lookback_days_ * $OML_hit_ratio_ ) # fist max_loss_line may not follow the hit_ratio constraint in some cases, adding a sanity check here 
        {
# $strat_name_to_optimal_max_loss_ { $picked_strat_ } = max ( min ( $max_loss_ , $max_max_loss_per_unit_size_ ) , $min_max_loss_per_unit_size_ );
          $optimal_max_loss_ = $max_loss_ ;
          last;
        }
      }
    }

    if ( $optimal_max_loss_ == -1 )
    {
      if ( $max_loss_per_unit_size_ ) { 
        $optimal_max_loss_ = $max_loss_per_unit_size_;
      }
      elsif ( $max_max_loss_per_unit_size_ ) {
        $optimal_max_loss_ = $min_max_loss_per_unit_size_;
      }
      else {
        print "WARNING: MAX_LOSS could not computed for ".$strat_name_."..\n";
        return $optimal_max_loss_;
      }
    }

    if ( $max_max_loss_per_unit_size_ && $optimal_max_loss_ > $max_max_loss_per_unit_size_ ) {
      $optimal_max_loss_ = $max_max_loss_per_unit_size_;
    }
    elsif ( $min_max_loss_per_unit_size_ && $optimal_max_loss_ < $min_max_loss_per_unit_size_ ) {
      $optimal_max_loss_ = $min_max_loss_per_unit_size_;
    }

    $max_loss_ = $optimal_max_loss_;
  }

  return $max_loss_;
}

sub RemoveTempFiles {
 foreach my $temp_file_ ( @temp_files_ ) {
  if ( -e $temp_file_ ) {
   `rm -f $temp_file_`;
  }
 }
}



