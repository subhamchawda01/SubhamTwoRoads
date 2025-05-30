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
my $PICKSTRAT_TEMP_DIR = $PICKSTRATS_DIR."/temp_dir";
my $GLOBAL_RESULTS_DIR = $HOME_DIR."/ec2_globalresults";

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

my $mail_address_ = "nseall@tworoads.co.in";
#my $mail_address_ = "hrishav.agarwal\@tworoads.co.in";
#print $mail_address_."\n";

my $usage_ = "compare_pickstrats.pl DATE [ SHORTCODE TIMEOFDAY ]";

if ( $#ARGV < 0 ) {
  print $usage_."\n";
  exit ( 0 );
}

#my $date_ = CalcPrevWorkingDateMult ( $ARGV [ 0 ] , 1 );
my $date_ = $ARGV [ 0 ]; chomp ( $date_ );
print $date_."\n";
my $given_shc_ = "INVALID";
my $given_tod_ = "INVALID";
if ( $#ARGV >= 2 ) {
  $given_shc_ = $ARGV [ 1 ]; chomp ( $given_shc_ );
  $given_tod_ = $ARGV [ 2 ]; chomp ( $given_tod_ );
}
( my $yyyy_ , my $mm_ , my $dd_ ) = BreakDateYYYYMMDD ( $date_ );
my $query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;
my $query_trades_dir_ = "/NAS1/logs/QueryTrades/".$yyyy_."/".$mm_."/".$dd_;
my $pickstrats_queryids_file_ = $PICKSTRATS_DIR."/pickstrats_queryids";
print $query_log_dir_."\n";
print $query_trades_dir_."\n";
print $pickstrats_queryids_file_."\n";
my %shortcode_timeofday_to_querystartid_ = ( );
my %shortcode_timeofday_to_queryendid_ = ( );
my %shortcode_timeofday_to_configfile_ = ( );

my %product_days_to_real_avgpnl_ = ( );
my %product_days_to_real_avgdrawdown_ = ( );
my %product_days_to_picked_avgpnl_ = ( );
my %product_days_to_picked_avgdrawdown_ = ( );
my %product_name_to_last_traded_date_ = ( );

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
  if ( ( $given_shc_ eq "INVALID" || $given_tod_ eq "INVALID" ) || ( $this_shc_ eq $given_shc_ && $this_tod_ eq $given_tod_ ) ) {
    $shortcode_timeofday_to_querystartid_ { $this_shc_." ".$this_tod_ } = $line_words_ [ 2 ];
    $shortcode_timeofday_to_queryendid_   { $this_shc_." ".$this_tod_ } = $line_words_ [ 3 ];
    $shortcode_timeofday_to_configfile_   { $this_shc_." ".$this_tod_ } = $line_words_ [ 4 ];
  }
}

if ( ! keys %shortcode_timeofday_to_querystartid_ ) {
  print "Either the pickstrat config file is empty OR no product in the config file matches the provided product\n";
  exit ( 0 );
}

my @real_picked_strats_ = ( );
my @pickstrats_picked_strats_ = ( );
my $mail_body_ = "";
my $log_body_ = "";
my $read_OML_ = 0;
my $OML_lookback_days_ = 0;
my $OML_hit_ratio_ = 0;
my $OML_number_top_loss_ = 0;
my $max_loss_per_unit_size_ = 0;
my $min_max_loss_per_unit_size_ = 0;
my $max_max_loss_per_unit_size_ = 0;

CompareForDate ( );

GetStatsForPastDays ( );

DumpStatsInMailBody ( );

RemoveTempFiles ( );

sub CompareForDate {
  my $pickstrats_script_file_ = $MODELSCRIPTS_DIR."/pick_strats_and_install.pl";
# Get query ids corresponding to shortcode_ and timeofdays_
  foreach my $key ( keys %shortcode_timeofday_to_querystartid_ ) {
    my @key_words_ = split ( " " , $key );
    print $key."\n";
    my $shortcode_ = $key_words_ [ 0 ];
    my $timeofday_ = $key_words_ [ 1 ];
    my $querystartid_ = $shortcode_timeofday_to_querystartid_ { $key };
    my $queryendid_ = $shortcode_timeofday_to_queryendid_ { $key };
    my $config_file_ = $shortcode_timeofday_to_configfile_ { $key };

    if ( SkipWeirdDate( $date_ ) ||
        IsDateHoliday( $date_ ) ||
        IsProductHoliday( $date_, $shortcode_ ) ||
        NoDataDateForShortcode ( $date_ , $shortcode_ ) ||
        !ValidDate( $date_ ) ) {
      print "No Data for date ".$date_." for shortcode ".$shortcode_."\n";
      next;
    }

    @real_picked_strats_ = ( );
    @pickstrats_picked_strats_ = ( );
    my %query_real_picked_strats_ = ( );

    my $timeperiod_ = "";
    $read_OML_ = 0;
    $OML_lookback_days_ = 0;
    $OML_hit_ratio_ = 0;
    $OML_number_top_loss_ = 0;
    $max_loss_per_unit_size_ = 0;
    $min_max_loss_per_unit_size_ = 0;
    $max_max_loss_per_unit_size_ = 0;

# Get real picked query names
    for ( my $queryid_  = $querystartid_; $queryid_ <= $queryendid_; $queryid_++ ) {
      print $queryid_."\n";
      my $query_log_file_ = $query_log_dir_."/log.".$date_.".".$queryid_.".gz";
      print $query_log_file_."\n";
      if ( ! -e $query_log_file_ ) {
        next;
      }
      my $query_log_file_local_ = $PICKSTRAT_SIMOUT_DIR."/log.".$date_.".".$queryid_;
      print $query_log_file_local_."\n";
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
      next;
    }

# Modify the Config
#   {
# Change DATE in the config
      my $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." DATE";
      print $exec_cmd_."\n";
      my @date_output_ = `$exec_cmd_`; chomp ( @date_output_ );

      $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." DATE ".$date_;
      print $exec_cmd_."\n";
      my @exec_output_ = `$exec_cmd_`;


# Change NUM_STRATS_TO_INSTALL
      $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." NUM_STRATS_TO_INSTALL";
      print $exec_cmd_."\n";
      my @num_strats_to_install_output_ = `$exec_cmd_`; chomp ( @num_strats_to_install_output_ );

      $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." NUM_STRATS_TO_INSTALL ".( $#real_picked_strats_ + 1 );
      print $exec_cmd_."\n";
      @exec_output_ = `$exec_cmd_`;

# Change INSTALL_PICKED_STRATS
      $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." INSTALL_PICKED_STRATS";
      print $exec_cmd_."\n";
      my @install_picked_strats_output_ = `$exec_cmd_`; chomp ( @install_picked_strats_output_ );

      $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." INSTALL_PICKED_STRATS ".( 0 );
      print $exec_cmd_."\n";
      @exec_output_ = `$exec_cmd_`;

# Change TOTAL_SIZE_TO_RUN
      $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." TOTAL_SIZE_TO_RUN";
      print $exec_cmd_."\n";
      my @total_size_to_install_output_ = `$exec_cmd_`; chomp ( @total_size_to_install_output_ );

      $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." TOTAL_SIZE_TO_RUN ".( $#real_picked_strats_ + 1 );
      print $exec_cmd_."\n";
      @exec_output_ = `$exec_cmd_`;

# Change OPTIMAL_MAX_LOSS_SETTINGS
      $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." USE_OPTIMAL_MAX_LOSS_PER_UNIT_SIZE";
      print $exec_cmd_."\n";
      my @optimal_max_loss_output_ = `$exec_cmd_`; chomp ( @optimal_max_loss_output_ );
      my @optimal_max_loss_output_uncommented_ = grep { index ( $_ , "#" ) != 0 } @optimal_max_loss_output_;

      if ( $#optimal_max_loss_output_uncommented_ >= 0 ) {
        $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." USE_OPTIMAL_MAX_LOSS_PER_UNIT_SIZE ".( $optimal_max_loss_output_uncommented_ [0] );
        print $exec_cmd_."\n";
        @exec_output_ = `$exec_cmd_`;
      }

# Change MIN_VOLUME_PER_STRAT
      $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." MIN_VOLUME_PER_STRAT";
      print $exec_cmd_."\n";
      my @min_vol_per_strat_output_ = `$exec_cmd_`; chomp ( @min_vol_per_strat_output_ );
      my @min_vol_per_strat_output_uncommented_ = grep { index ( $_ , "#" ) != 0 } @min_vol_per_strat_output_;

      if ( $#min_vol_per_strat_output_uncommented_ >= 0 ) {
        $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." MIN_VOLUME_PER_STRAT ".( $min_vol_per_strat_output_uncommented_ [0] );
        print $exec_cmd_."\n";
        @exec_output_ = `$exec_cmd_`;
      }

# Change SORT_ALGO
      $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." SORT_ALGO";
      print $exec_cmd_."\n";
      my @sort_algo_output_ = `$exec_cmd_`; chomp ( @sort_algo_output_ );
      my @sort_algo_output_uncommented_ = grep { index ( $_ , "#" ) != 0 } @sort_algo_output_;

      if ( $#sort_algo_output_uncommented_ >= 0 ) {
        $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." SORT_ALGO ".( $sort_algo_output_uncommented_ [0] );
        print $exec_cmd_."\n";
        @exec_output_ = `$exec_cmd_`;
      }

# Change EMAIL
      $exec_cmd_ = $SCRIPTS_DIR."/get_config_field.pl ".$config_file_." EMAIL";
      print $exec_cmd_."\n";
      my @email_output_ = `$exec_cmd_`; chomp ( @email_output_ );

      $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." EMAIL #";
      print $exec_cmd_."\n";
      @exec_output_ = `$exec_cmd_`;
# }

# Finally run pickstrats
    $exec_cmd_ = $pickstrats_script_file_." ".$shortcode_." ".$timeofday_." ".$config_file_." 1 1";
    print $exec_cmd_."\n";
    @exec_output_ = `$exec_cmd_`;

    my $script_log_name_ = $PICKSTRATS_SCRIPT_LOG_DIR."/log.".$date_.".".$shortcode_."_".$timeofday_;
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
        print "reading OML_Setting..\n";
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
     print "No OML Setting or MAX_LOSS received from pickstrats.. Exiting..\n";
     next ;
    }
# Set the original things back in
#{
  
  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." DATE ".join(' ', map { qq/"$_"/ } @date_output_);
  print $exec_cmd_."\n"; @exec_output_ = `$exec_cmd_`;

  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." NUM_STRATS_TO_INSTALL ".join(' ', map { qq/"$_"/ } @num_strats_to_install_output_);
  print $exec_cmd_."\n"; @exec_output_ = `$exec_cmd_`;

  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." INSTALL_PICKED_STRATS ".join(' ', map { qq/"$_"/ } @install_picked_strats_output_);
  print $exec_cmd_."\n"; @exec_output_ = `$exec_cmd_`;

  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." TOTAL_SIZE_TO_RUN ".join(' ', map { qq/"$_"/ } @total_size_to_install_output_);
  print $exec_cmd_."\n"; @exec_output_ = `$exec_cmd_`;

  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." USE_OPTIMAL_MAX_LOSS_PER_UNIT_SIZE ".join(' ', map { qq/"$_"/ } @optimal_max_loss_output_);
  print $exec_cmd_."\n"; @exec_output_ = `$exec_cmd_`;

  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." MIN_VOLUME_PER_STRAT ".join(' ', map { qq/"$_"/ } @min_vol_per_strat_output_);
  print $exec_cmd_."\n"; @exec_output_ = `$exec_cmd_`;

  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." SORT_ALGO ".join(' ', map { qq/"$_"/ } @sort_algo_output_);
  print $exec_cmd_."\n"; @exec_output_ = `$exec_cmd_`;

  $exec_cmd_ = $SCRIPTS_DIR."/set_config_field.pl ".$config_file_." EMAIL ".join(' ', map { qq/"$_"/ } @email_output_);
  print $exec_cmd_."\n"; @exec_output_ = `$exec_cmd_`;
#}
  PrintNextDaySimResultsLog ( $shortcode_ , $timeofday_ );
  }

}

sub GetStatsForPastDays {
  my @days_lengths_ = (1, 5, 20, 63);
  my $date_stat_dir_ = $PICKSTRATS_RESULT_STAT_DIR."/logs.".$date_;

  foreach my $product_ ( keys %shortcode_timeofday_to_querystartid_ ) {
    my ( $t_shc_, $t_time_ ) = split ( ' ', $product_ );
    print $t_shc_." ".$t_time_."\n";
    my $date_stat_file_ = $date_stat_dir_."/log.".$t_shc_."_".$t_time_;

    my $exec_cmd_ = "ls ".$PICKSTRATS_RESULT_STAT_DIR."/logs.*/log.".$t_shc_."_".$t_time_." -1";
    my @all_date_stat_files_ = `$exec_cmd_`; chomp ( @all_date_stat_files_ );
#    print join("\n", @all_date_stat_files_ )."\n\n";
    my @stat_files_unsorted_ = grep { $_ le $date_stat_file_ } @all_date_stat_files_;
#    print join ("\n", @stat_files_unsorted_ )."\n\n";
    my @stat_files_ = reverse sort @stat_files_unsorted_;
#    print join ("\n", @stat_files_)."\n\n";
    
    my $longest_intv_ = $days_lengths_[-1];
    if ( $longest_intv_ > $#stat_files_+1 ) { $longest_intv_ = $#stat_files_+1; }

    my %date_to_real_avgpnl_ = ( );
    my %date_to_real_avgdrawdown_ = ( );
    my %date_to_picked_avgpnl_ = ( );
    my %date_to_picked_avgdrawdown_ = ( );
    my $stat_files_ind_ = 0;

    foreach my $stat_file_ ( @stat_files_ ) {
#      print "In ".$stat_file_."\n";

      my $stat_dir_ = `dirname $stat_file_`; chomp ( $stat_dir_ );
      my $this_date_ = substr $stat_dir_, -8;
      
      open STAT_HANDLE, "< $stat_file_" or PrintStacktraceAndDie ( "Could not open $stat_file_" );
      my @stat_file_lines_ = <STAT_HANDLE>; chomp ( @stat_file_lines_ );
      close ( STAT_HANDLE );

      my $in_sel_prod_ = 0;
      foreach my $stat_line_ ( @stat_file_lines_ )
      {
        my @t_words_ = split ( ' ', $stat_line_ );
        if ( $#t_words_ < 0 ) { next; }

        if ( $t_words_[0] eq "PRODUCT" ) {
          my $curr_product_ = $t_words_[1]." ".$t_words_[2];
          if ( $curr_product_ ne $product_ ) 
          {
            print "Error in stat file: ".$stat_file_." .. The Product names do not match!!\n";
          }
          if ( ! exists $product_name_to_last_traded_date_ { $product_ } ) {
            $product_name_to_last_traded_date_ { $product_ } = $this_date_;
          }

          $in_sel_prod_ = 1;
        }
        elsif ( $in_sel_prod_ == 1 && $t_words_[0] eq "REAL_PICKED" ) {
          if ( ! ( $t_words_[1] == 0 && $t_words_[2] == 0 ) ) {
            $date_to_real_avgpnl_ { $this_date_ } = $t_words_[1];
            $date_to_real_avgdrawdown_ { $this_date_ } = $t_words_[2];
          }
        }
        elsif ( $in_sel_prod_ == 1 && $t_words_[0] eq "AUTOMATED_PICKED" ) {
          if ( ! ( $t_words_[1] == 0 && $t_words_[2] == 0 ) ) {
            $date_to_picked_avgpnl_ { $this_date_ } = $t_words_[1];
            $date_to_picked_avgdrawdown_ { $this_date_ } = $t_words_[2];
          }
        }
      }
      if ( exists $date_to_real_avgpnl_ { $this_date_ } && exists $date_to_picked_avgpnl_ { $this_date_ } ) {
        $stat_files_ind_ ++;
      }
      if ( $stat_files_ind_ >= $longest_intv_ ) {
        last;
      }

    }
    print "Past result stats for product: ".$product_." exists for dates: ".join ( " ", keys %date_to_real_avgpnl_ )."\n";

    my @days_lengths_filt_ = grep { $_ <= $longest_intv_ } @days_lengths_;
    if ( $#days_lengths_filt_ < 0 )
    {
      next;
    }
    

    my $intv_ind_ = 0;
    my $real_avg_pnl_ = 0;
    my $real_avg_dd_ = 0;
    my $picked_avg_pnl_ = 0;
    my $picked_avg_dd_ = 0;
    my $day_no_ = 0;

    foreach my $t_date_ ( reverse sort ( keys %date_to_real_avgpnl_ ) ) {
      if ( ! exists $date_to_real_avgpnl_ { $t_date_ }
        || ! exists $date_to_picked_avgpnl_ { $t_date_ } ) {
        print "REAL_PICKED or AUTOMATED_PICKED for $product_ $t_date_ does NOT EXIST\n";
        next;
      }

      if ( $day_no_ < $days_lengths_filt_[ $intv_ind_ ] ) {
        $real_avg_pnl_ += $date_to_real_avgpnl_ { $t_date_ };
        $real_avg_dd_ += $date_to_real_avgdrawdown_ { $t_date_ };
        $picked_avg_pnl_ += $date_to_picked_avgpnl_ { $t_date_ };
        $picked_avg_dd_ += $date_to_picked_avgdrawdown_ { $t_date_ };
      }
      $day_no_ ++;
      
      if ( $day_no_ == $days_lengths_filt_[ $intv_ind_ ] ) {
        my $product_intv_ = $product_." ".$days_lengths_filt_[ $intv_ind_ ];

        $product_days_to_real_avgpnl_ { $product_intv_ } = $real_avg_pnl_ / $days_lengths_filt_[ $intv_ind_ ] ;
        $product_days_to_real_avgdrawdown_ { $product_intv_ } = $real_avg_dd_ / $days_lengths_filt_[ $intv_ind_ ] ;
        $product_days_to_picked_avgpnl_ { $product_intv_ } = $picked_avg_pnl_ / $days_lengths_filt_[ $intv_ind_ ] ;
        $product_days_to_picked_avgdrawdown_ { $product_intv_ } = $picked_avg_dd_ / $days_lengths_filt_[ $intv_ind_ ] ;

        print $product_intv_." : "."REAL_PNL: ".$product_days_to_real_avgpnl_ { $product_intv_ }.", REAL_DD: ".$product_days_to_real_avgdrawdown_ { $product_intv_ };
        print ", PICKED_PNL: ".$product_days_to_picked_avgpnl_ { $product_intv_ }.", PICKED_DD: ".$product_days_to_picked_avgdrawdown_ { $product_intv_ }."\n";

        $intv_ind_++;
        if ( $intv_ind_ > $#days_lengths_filt_ ) {
          last;
        }
      }
    }
  }
}

sub DumpStatsInMailBody {
  my @days_lengths_ = (1, 5, 20, 63);
  my @col_headers_ = qw( PROD TR_HRS LAST_DT );
  my @last_day_headers_ = qw( LAST_DAY LAST_5_DAYS LAST_20_DAYS LAST_63_DAYS ); # LAST_5_DAY_PNL_AVG LAST_20_DAY_PNL_AVG );
  my @last_day_subheaders_ = qw( REAL_AVGPNL PS_AVGPNL REAL_AVGDD PS_AVGDD );
  my @day_cols_ = ( "#cbece8", "#c9ffe5", "#d2f6de", "#ffff85" );

  open ( MAIL , "|/usr/sbin/sendmail -t" );

  print MAIL "To: $mail_address_\n";
  print MAIL "From: $mail_address_\n";
  printf MAIL "Subject: [ PICKSTRATS EVALUATION ] %s\n", $date_;
  print MAIL "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n";

  print MAIL "<html><body>\n";

  print MAIL "<table border = \"1\"><tr>" ;
    #  my $hostname_ = `hostname`;

  foreach my $elem_str_ ( @col_headers_ ) {
    printf MAIL "<td align=center rowspan=\"2\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $elem_str_;
  }
  foreach my $elem_str_ ( @last_day_headers_ ) {
    printf MAIL "<td align=center colspan=\"4\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $elem_str_;
  }
  printf MAIL "</tr>\n";

  printf MAIL "<tr>";
  foreach my $day_index_ ( 0 .. $#last_day_headers_ ) {
    my $this_col_ = $day_cols_ [ $day_index_ ];
    foreach my $elem_str_ ( @last_day_subheaders_ ) {
      printf MAIL "<td align=center bgcolor=\"%s\"><font font-weight = \"bold\" size = \"2\" color=darkblue>%s</font></td>", $this_col_, $elem_str_;
    }
  }
  printf MAIL "</tr>\n";
 
  my @hrs_shc_ = ( );
  my @sorted_products = ( ); 
  foreach my $product_ ( keys %product_name_to_last_traded_date_ ) {
    my ( $shc_, $hrs_ ) = split ( ' ', $product_ );
    push ( @hrs_shc_, $hrs_." ".$shc_ );
  }

  my @sorted_hrs_shc_ = sort @hrs_shc_;

  foreach my $t_hrs_shc_ ( @sorted_hrs_shc_ ) {
    printf MAIL "<tr>";
    my ( $hrs_, $shc_ ) = split ( ' ', $t_hrs_shc_ );
    my $product_ = $shc_." ".$hrs_;
    printf MAIL "<td align=center>%s</td>", $shc_;
    printf MAIL "<td align=center>%s</td>", $hrs_;
    printf MAIL "<td align=center>%s</td>", $product_name_to_last_traded_date_ { $product_ };

    my $col_ind_ = 0;
    foreach my $intv_ ( @days_lengths_ ) {
      my $product_intv_ = $product_." ".$intv_;
      if ( exists $product_days_to_real_avgpnl_ { $product_intv_ } ) {
#        print "here".$product_intv_."\n";
        printf MAIL "<td align=center bgcolor=\"%s\">%.2f</td>", $day_cols_[$col_ind_], $product_days_to_real_avgpnl_ { $product_intv_ };
        printf MAIL "<td align=center bgcolor=\"%s\">%.2f</td>", $day_cols_[$col_ind_], $product_days_to_picked_avgpnl_ { $product_intv_ };
        printf MAIL "<td align=center bgcolor=\"%s\">%.2f</td>", $day_cols_[$col_ind_], $product_days_to_real_avgdrawdown_ { $product_intv_ };
        printf MAIL "<td align=center bgcolor=\"%s\">%.2f</td>", $day_cols_[$col_ind_], $product_days_to_picked_avgdrawdown_ { $product_intv_ };
      }
      else {
#        print "there".$product_intv_."\n";
        printf MAIL "<td align=center bgcolor=\"%s\">%s</td>", $day_cols_[$col_ind_], "-";
        printf MAIL "<td align=center bgcolor=\"%s\">%s</td>", $day_cols_[$col_ind_], "-";
        printf MAIL "<td align=center bgcolor=\"%s\">%s</td>", $day_cols_[$col_ind_], "-";
        printf MAIL "<td align=center bgcolor=\"%s\">%s</td>", $day_cols_[$col_ind_], "-";
      }
      $col_ind_ ++;
    }
    printf MAIL "</tr>\n";
  }
  print MAIL "</table>";
  print MAIL "</body></html>\n";

  close ( MAIL );
}

sub PrintNextDaySimResultsLog {

  my $shortcode_ = shift;
  my $timeofday_ = shift;

  my $log_body_ = $log_body_."PRODUCT $shortcode_ $timeofday_ \n\n";

  my $temp_strategy_list_file_ = $PICKSTRAT_TEMP_DIR."/temp_strategy_list_file";
  push ( @temp_files_, $temp_strategy_list_file_ );
  open TEMP_FILE_HANDLE, "> $temp_strategy_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );
  for (my $idx_ = 0; $idx_ <= $#real_picked_strats_; $idx_++) {
    print TEMP_FILE_HANDLE $real_picked_strats_ [ $idx_ ]."\n";
  }
  for (my $idx_ = 0; $idx_ <= $#pickstrats_picked_strats_; $idx_++) {
    print TEMP_FILE_HANDLE $pickstrats_picked_strats_ [ $idx_ ]."\n";
  }
  close ( TEMP_FILE_HANDLE );
  my $exec_cmd_ = $BIN_DIR."/summarize_strategy_results $shortcode_ $temp_strategy_list_file_ $GLOBAL_RESULTS_DIR $date_ $date_";
  print $exec_cmd_."\n";
  my @exec_output_ = `$exec_cmd_`; chomp ( @exec_output_ );
#print join('\n', @exec_output_)."\n";
  my %strat_name_to_pnl_ = ( );
  my %strat_name_to_volume_ = ( );
  my %strat_name_to_drawdown_ = ( );
  foreach my $output_line_ ( @exec_output_ ) {
    my @output_line_words_ = split ( ' ' , $output_line_ );
    my $max_loss_per_uts_ = ComputeMaxLossPerUTS ( $output_line_words_ [ 1 ], $shortcode_, $timeofday_ );
    $exec_cmd_ = $BIN_DIR."/get_UTS_for_a_day ".$shortcode_." ".$output_line_words_[1]." ".$date_;
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

  my $real_avg_drawdown_ = 0;
  my $real_sum_pnl_ = 0;
  my $picked_avg_drawdown_ = 0;
  my $picked_sum_pnl_ = 0;

  if ( $#real_picked_strats_ >= 0 ) {
    for (my $idx_ = 0; $idx_ <= $#real_picked_strats_; $idx_++) {
#    print  $real_picked_strats_ [ $idx_ ] . "\n";
      $real_sum_pnl_ += $strat_name_to_pnl_ { $real_picked_strats_ [ $idx_ ] };
      $real_avg_drawdown_ += $strat_name_to_drawdown_ { $real_picked_strats_ [ $idx_ ] };
    }
    $real_avg_drawdown_ = $real_avg_drawdown_ / ( $#real_picked_strats_+1 );
  }
  else {
    print "NOTE: No Real strats for ".$shortcode_."_".$timeofday_."\n\n";
    return;
  }

  if ( $#pickstrats_picked_strats_ >= 0) {
    for (my $idx_ = 0; $idx_ <= $#pickstrats_picked_strats_; $idx_++) {
      $picked_sum_pnl_ += $strat_name_to_pnl_ { $pickstrats_picked_strats_ [ $idx_ ] };
      $picked_avg_drawdown_ += $strat_name_to_drawdown_ { $pickstrats_picked_strats_ [ $idx_ ] };
    }
    $picked_avg_drawdown_ = $picked_avg_drawdown_ / ( $#pickstrats_picked_strats_+1 );
  }
  else {
    print "NOTE: No Picked strats for ".$shortcode_."_".$timeofday_."\n\n";
    return;
  }


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

  my $date_simresults_dir_ = $PICKSTRATS_RESULT_STAT_DIR."/logs.".$date_;
  if ( ! -d $date_simresults_dir_ ) {
    `mkdir -p $date_simresults_dir_`;
  }
  my $date_simresults_file_ = $date_simresults_dir_."/log.".$shortcode_."_".$timeofday_;
  if ( -e $date_simresults_file_ ) {
    print "WARNING: ".$date_simresults_file_." already exists.. Overwriting it..\n";
  }
  open SIM_FILE_HANDLE, "> $date_simresults_file_" or PrintStacktraceAndDie ( "Could not open $date_simresults_file_\n" );
  print SIM_FILE_HANDLE $log_body_;
  close ( SIM_FILE_HANDLE );

}

sub ComputeMaxLossPerUTS { 
  my $strat_name_ = shift;
  my $shortcode_ = shift;
  my $timeofday_ = shift;
  my $FIND_OPTIMAL_MAX_LOSS = $MODELSCRIPTS_DIR."/find_optimal_max_loss.pl";

  my $optimal_max_loss_ = -1;
  my $max_loss_ = -1;


  if ( ( ! $read_OML_ ) && $max_loss_per_unit_size_ ) {
    $max_loss_ = $max_loss_per_unit_size_;
  }
  else {
    my $exec_cmd_ = $FIND_OPTIMAL_MAX_LOSS." $shortcode_ $timeofday_ $OML_lookback_days_ $OML_hit_ratio_ $OML_number_top_loss_ $strat_name_ $date_ ";
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



