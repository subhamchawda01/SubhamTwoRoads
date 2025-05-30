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
my $PICKSTRATS_SCRIPT_LOG_DIR = $PICKSTRATS_DIR."/pick_strats_script_logs";
my $PICKSTRATS_BACKTEST_DIR = $PICKSTRATS_DIR."/backtest_logs";
my $PICKSTRAT_TEMP_DIR = $PICKSTRATS_DIR."/temp_dir";
#my $GLOBAL_RESULTS_DIR = $HOME_DIR."/ec2_globalresults";
my $GLOBAL_RESULTS_DIR = "DB"; #changed the primary file location of the globalresults from FS to DB

if ( ! -e $PICKSTRATS_DIR ) { `mkdir $PICKSTRATS_DIR`; }
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

my $usage_ = "$0 DATE SHORTCODE TIMEOFDAY [ LIST OF CONFIG-FILES ]";

if ( $#ARGV < 3 ) {
  print $usage_."\n";
  exit ( 0 );
}

my $end_date_ = $ARGV [ 0 ]; chomp ( $end_date_ );
print $end_date_."\n";
my $shortcode_ = $ARGV [ 1 ]; chomp ( $shortcode_ );
my $timeofday_ = $ARGV [ 2 ]; chomp ( $timeofday_ );
my $config_list_file_ = $ARGV [ 3 ]; chomp ( $config_list_file_ );

my @intv_days_ = (10,30,90,150);
my $max_intv_ = max @intv_days_;

my $pickstrats_script_file_ = $MODELSCRIPTS_DIR."/pick_strats_and_install.pl";

my @temp_files_ = ( );

my %intv_start_days_ = ( );
my %intv_ndays_ = ( );

my @config_names_ = ( );

my $read_OML_ = 0;
my $OML_lookback_days_ = 0;
my $OML_hit_ratio_ = 0;
my $OML_number_top_loss_ = 0;
my $max_loss_per_unit_size_ = 0;
my $min_max_loss_per_unit_size_ = 0;
my $max_max_loss_per_unit_size_ = 0;

my %date_to_config_to_sum_pnl_ = ( );
my %date_to_config_to_avg_drawdown_ = ( );
my %date_to_config_to_volume_ = ( );

my %intv_to_config_to_pnl_ = ( );
my %intv_to_config_to_drawdown_ = ( );
my %intv_to_config_to_volume_ = ( );

my %config_to_uts_ = ( );

my $log_dir_ = $PICKSTRATS_BACKTEST_DIR."/".$shortcode_."_".$timeofday_;
if ( ! -e $log_dir_ ) { `mkdir -p $log_dir_`; }

open CONFIG_LIST, "< $config_list_file_" or PrintStacktraceAndDie ( "Could not open file $config_list_file_ for writing.\n" );
@config_names_ = <CONFIG_LIST>; chomp( @config_names_ );
print join("\n", @config_names_)."\n";

my $config_list_basename_ = `basename $config_list_file_`; chomp($config_list_basename_);
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my $log_filepath_ = $log_dir_."/log_".$config_list_basename_."_".$unique_gsm_id_;
while ( -e $log_filepath_ ) {
  $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
  $log_filepath_ = $log_dir_."/log_".$config_list_basename_."_".$unique_gsm_id_;
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
      print "No Data for DATE ".$t_date_." for shortcode ".$shortcode_."\n";
      $t_date_ = CalcPrevWorkingDateMult ( $t_date_, 1 );
      next;
    }

    print LOG_FILE_ "\nDATE $t_date_, PRODUCT $shortcode_ $timeofday_ \n\n";

    my $all_results_exist_ = 1;
    foreach my $t_config_ ( @config_names_ ) {
      CompareForDate ( $t_date_, $t_config_ );
      if ( ! exists $date_to_config_to_sum_pnl_ { $t_config_ }{ $t_date_ } ) {
        print "WARNING: CONFIG ".$t_config_." for DATE ".$t_date_." Could not work..\n";
        $all_results_exist_ = 0;
      }
    }

    if ( $all_results_exist_ == 0 ) {
      print "Due to incomplete results.. Skipping for DATE ".$t_date_."\n";
      $t_date_ = CalcPrevWorkingDateMult ( $t_date_, 1 );
      next;
    }

    foreach my $t_intv_ ( @intv_days_ )  {
      if ( $t_date_ > $intv_start_days_ { $t_intv_ } ) {
        foreach my $t_config_ ( @config_names_ ) {
        push ( @{$intv_to_config_to_pnl_ { $t_intv_ }{ $t_config_ }}, $date_to_config_to_sum_pnl_ { $t_config_ }{ $t_date_ } );
        push ( @{$intv_to_config_to_drawdown_ { $t_intv_ }{ $t_config_ }}, $date_to_config_to_avg_drawdown_ { $t_config_ }{ $t_date_ } );
        push ( @{$intv_to_config_to_volume_ { $t_intv_ }{ $t_config_ }}, $date_to_config_to_volume_ { $t_config_ }{ $t_date_ } );
        }

        $intv_ndays_ { $t_intv_ } += 1;
      }
    }
    $t_date_ = CalcPrevWorkingDateMult ( $t_date_, 1 );
  }

  foreach my $t_intv_ ( @intv_days_ )  {
    foreach my $t_config_ ( @config_names_ ) {
      my $sum_pnl_ = GetSum ( \@{$intv_to_config_to_pnl_ { $t_intv_ }{ $t_config_ }} );
      my $avg_pnl_ = GetAverage ( \@{$intv_to_config_to_pnl_ { $t_intv_ }{ $t_config_ }} );
      my $std_pnl_ = GetStdev( \@{$intv_to_config_to_pnl_ { $t_intv_ }{ $t_config_ }} );

      my $sum_vol_ = GetSum ( \@{$intv_to_config_to_volume_ { $t_intv_ }{ $t_config_ }} );
      my $avg_vol_ = GetAverage ( \@{$intv_to_config_to_volume_ { $t_intv_ }{ $t_config_ }} );
      my $std_vol_ = GetStdev( \@{$intv_to_config_to_volume_ { $t_intv_ }{ $t_config_ }} );

      my $pnl_dd_ = GetAverage( \@{$intv_to_config_to_drawdown_ { $t_intv_ }{ $t_config_ }} );

      print "INTV: ".$t_intv_.", STATS for CONFIG: ".$t_config_."\n";
      print "PNL_SUM \t PNL_DD \t PNL_MEAN \t PNL_STDEV \t VOL_SUM \t VOL_MEAN \t VOL_STDEV\n" ;
      printf "%.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \n\n", $sum_pnl_, $pnl_dd_, $avg_pnl_, $std_pnl_, $sum_vol_, $avg_vol_, $std_vol_ ;

      print LOG_FILE_ "INTV: ".$t_intv_.", STATS for CONFIG: ".$t_config_."\n";
      print LOG_FILE_ "PNL_SUM \t PNL_DD \t PNL_MEAN \t PNL_STDEV \t VOL_SUM \t VOL_MEAN \t VOL_STDEV\n" ;
      printf LOG_FILE_ "%.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \t %.4f \n\n", $sum_pnl_, $pnl_dd_, $avg_pnl_, $std_pnl_, $sum_vol_, $avg_vol_, $std_vol_ ;
    }
  }
}

sub CompareForDate {
  my $t_date_ = shift;
  my $t_config_ = shift;

  my @config_picked_strats_ = ( );

  $read_OML_ = 0;
  $OML_lookback_days_ = 0;
  $OML_hit_ratio_ = 0;
  $OML_number_top_loss_ = 0;
  $max_loss_per_unit_size_ = 0;
  $min_max_loss_per_unit_size_ = 0;
  $max_max_loss_per_unit_size_ = 0;

  my $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$t_config_." TOTAL_SIZE_TO_RUN";
  $config_to_uts_{ $t_config_ } = `$exec_cmd_`; chomp ( $config_to_uts_{ $t_config_ } ); 

# Change DATE, INSTALL_PICKED_STRATS, EMAIL in the config
  $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$t_config_." DATE";
  my @date_output_ = `$exec_cmd_`; chomp ( @date_output_ );

  $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$t_config_." INSTALL_PICKED_STRATS";
  my @install_picked_strats_output_ = `$exec_cmd_`; chomp ( @install_picked_strats_output_ );

  $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$t_config_." EMAIL";
  my @email_output_ = `$exec_cmd_`; chomp ( @email_output_ );

  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_." DATE ".$t_date_; `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_." INSTALL_PICKED_STRATS ".( 0 ); `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_." EMAIL #"; `$exec_cmd_`;

# Finally run pickstrats
  $exec_cmd_ = $pickstrats_script_file_." ".$shortcode_." ".$timeofday_." ".$t_config_." 1 1";
#  print $exec_cmd_."\n";
  my @exec_output_ = `$exec_cmd_`;

  my $script_log_name_ = $PICKSTRATS_SCRIPT_LOG_DIR."/log.".$t_date_.".".$shortcode_."_".$timeofday_;
  open SCRIPT_LOG_HANDLE, "> $script_log_name_" or PrintStacktraceAndDie ( "Could not open $script_log_name_\n" );
  print SCRIPT_LOG_HANDLE join("", @exec_output_)."\n";
  close ( SCRIPT_LOG_HANDLE );

  chomp ( @exec_output_ );
  for (my $idx_ = 0; $idx_ <= $#exec_output_; $idx_++) {
    if ( index ( $exec_output_ [ $idx_ ] , "PICKED:" ) != -1 ) {
      push ( @config_picked_strats_ , substr ( $exec_output_ [ $idx_ ] , 8 ) );
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
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_." DATE ".join(' ', map { qq/"$_"/ } @date_output_); `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_." INSTALL_PICKED_STRATS ".join(' ', map { qq/"$_"/ } @install_picked_strats_output_); `$exec_cmd_`;
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$t_config_." EMAIL ".join(' ', map { qq/"$_"/ } @email_output_); `$exec_cmd_`;
  
  ComputeNextDaySimResultsLog ( $t_date_, $t_config_, \@config_picked_strats_ );
}

sub ComputeNextDaySimResultsLog {
  my $t_date_ = shift;
  my $t_config_ = shift;
  my $config_picked_strats_ref_ = shift;
  my @config_picked_strats_ = @$config_picked_strats_ref_;

  my $log_body_ = "";

  my $t_config_basename_ = basename ( $t_config_ );
  my $temp_strategy_list_file_ = $PICKSTRAT_TEMP_DIR."/temp_strategy_list_file_".$t_config_basename_."_".$t_date_."_".$unique_gsm_id_;
  push ( @temp_files_, $temp_strategy_list_file_ );
  open TEMP_FILE_HANDLE, "> $temp_strategy_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );

  for (my $idx_ = 0; $idx_ <= $#config_picked_strats_; $idx_++) {
    print TEMP_FILE_HANDLE $config_picked_strats_ [ $idx_ ]."\n";
  }
  close ( TEMP_FILE_HANDLE );

  my $exec_cmd_ = $BIN_DIR."/summarize_strategy_results $shortcode_ $temp_strategy_list_file_ $GLOBAL_RESULTS_DIR $t_date_ $t_date_";
#  print $exec_cmd_."\n";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );
  
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

    $strat_name_to_pnl_ { $output_line_words_ [ 1 ] } = $strat_pnl_ / $unit_trade_size_ ;
    $strat_name_to_volume_ { $output_line_words_ [ 1 ] } = $output_line_words_ [ 4 ] / $unit_trade_size_;
    $strat_name_to_drawdown_ { $output_line_words_ [ 1 ] } = $output_line_words_ [ 16 ] / $unit_trade_size_;
  }

  my $picked_sum_pnl_ = 0;
  my $picked_avg_drawdown_ = 0;
  my $picked_avg_volume_ = 0;
  my $picked_sum_uts_ = 0;

  if ( $#config_picked_strats_ >= 0) {
    for (my $idx_ = 0; $idx_ <= $#config_picked_strats_; $idx_++) {
      if ( exists $strat_name_to_pnl_ { $config_picked_strats_ [ $idx_ ] } ) {
        $picked_sum_pnl_ += $strat_name_to_pnl_ { $config_picked_strats_ [ $idx_ ] };
        $picked_avg_drawdown_ += $strat_name_to_drawdown_ { $config_picked_strats_ [ $idx_ ] };
        $picked_avg_volume_ += $strat_name_to_volume_ { $config_picked_strats_ [ $idx_ ] };
        $picked_sum_uts_ += 1;
      }
      else {
        print "Results for strat ".$config_picked_strats_ [ $idx_ ]." does NOT exist for date ".$t_date_."\n";
        splice @config_picked_strats_, $idx_, 1;
        $idx_--;
      }
    }
    if ( $picked_sum_uts_ > 0 ) {
      my $uts_scale_factor_ = $config_to_uts_ { $t_config_ } / $picked_sum_uts_;
      $picked_avg_drawdown_ = $picked_avg_drawdown_ * $uts_scale_factor_;
      $picked_avg_volume_ =  $picked_avg_volume_ * $uts_scale_factor_;
      $picked_sum_pnl_ = $picked_sum_pnl_ * $uts_scale_factor_; 
      
      $log_body_ = $log_body_."\nCONFIG: ".$t_config_." ".$picked_sum_pnl_." ".$picked_avg_drawdown_."\n";
      for (my $idx_ = 0; $idx_ <= $#config_picked_strats_; $idx_++) {
        my $t_strat_ = $config_picked_strats_ [ $idx_ ];
        $log_body_ = $log_body_.(($strat_name_to_pnl_{ $t_strat_ })*$uts_scale_factor_)." ".
                                (($strat_name_to_volume_{ $t_strat_ })*$uts_scale_factor_)." ".
                                (($strat_name_to_drawdown_{ $t_strat_ })*$uts_scale_factor_)." ".$t_strat_."\n";
      }
      $log_body_ = $log_body_."\n";

      $date_to_config_to_sum_pnl_{ $t_config_ }{ $t_date_ } = $picked_sum_pnl_;
      $date_to_config_to_avg_drawdown_{ $t_config_ }{ $t_date_ } = $picked_avg_drawdown_;
      $date_to_config_to_volume_{ $t_config_ }{ $t_date_ } = $picked_avg_volume_;
    }
  }
  else {
    print "NOTE: No Picked strats for ".$shortcode_."_".$timeofday_."\n\n";
    return;
  }

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

