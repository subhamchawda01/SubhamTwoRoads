#!/usr/bin/perl
use strict;
use warnings;

my $USER = $ENV { 'USER' };
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/get_query_type_for_id_date.pl"; # GetQueryTypeForIdDate
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

sub FetchSyncedParamsAndModels;
sub GetSimEndTimeStratFile;


my $print_strat_exec_cmd = 0;
my $usage_ = "run_accurate_no_end_sim_real.pl DATE UNIQUE_ID [REMOVE_STRAT_FILES=1] [STRAT_FILE_DIR] [PARAM_FILE_DIR] [MODEL_FILE_DIR]";

if ($#ARGV < 1) {
  print $usage_."\n";
  exit (0);
}

my $yyyymmdd_ = GetIsoDateFromStr ( $ARGV[0] ) ; 
my ( $yyyy_, $mm_, $dd_ ) = BreakDateYYYYMMDD ( $yyyymmdd_ );

my $unique_id_ = $ARGV[1]; chomp ($unique_id_);

print $ARGV[0]." ".$ARGV[1]."\n";

my $to_remove_strat_files_ = 1;
my $t_strat_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_param_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_model_file_dir_ = $HOME_DIR."/"."querytemp";
if ( $#ARGV > 1 )
{
  $to_remove_strat_files_ = $ARGV [ 2 ];
  if ( $#ARGV > 2 ) { $t_strat_file_dir_ = $ARGV [ 3 ]; }
  if ( $#ARGV > 3 ) { $t_param_file_dir_ = $ARGV [ 4 ]; }
  if ( $#ARGV > 4 ) { $t_model_file_dir_ = $ARGV [ 5 ]; }

  `mkdir -p $t_strat_file_dir_`;
  `mkdir -p $t_param_file_dir_`;
  `mkdir -p $t_model_file_dir_`;
}


my $query_trades_dir_ = "/NAS1/logs/QueryTrades/".$yyyy_."/".$mm_."/".$dd_;
my $trades_file_name_ = $query_trades_dir_."/"."trades.".$yyyymmdd_.".".$unique_id_;
print $trades_file_name_;

if ( ! ExistsWithSize ( $trades_file_name_ ) )
{
  print "Trades file ".$trades_file_name_." does not exist with size\n";
  exit(0);
}

my $real_query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;
# Copy possibly gzipped log file to a temp dir.
my $t_query_log_dir_ = $HOME_DIR."/"."querytemp";

`mkdir -p $t_query_log_dir_`;

my $t_=`date +%N |cut -c6-`;chomp($t_);
my $sim_id_ = "$unique_id_"."$t_";
if ( $sim_id_ > 9999999 ) { $sim_id_ = substr ( $sim_id_ , 0 , 8 ); }

my $t_query_file_name_ = $t_query_log_dir_."/"."log.".$yyyymmdd_.".".$unique_id_;

{ # Make a local copy of the Query Log from real.

# First try non .gz log.
  my $query_file_name_ = $real_query_log_dir_."/"."log.".$yyyymmdd_.".".$unique_id_;
  if (! -e $query_file_name_) {
# Try the .gz version.
    $query_file_name_ = $query_file_name_.".gz";

    if (! -e $query_file_name_) {
      print "File $query_file_name_ doesnot exist\n";
      exit (0);
    } else {
      `cp $query_file_name_ $t_query_log_dir_`;
# Gunzip this .gz log file.
      my $t_gz_query_file_name_ = $t_query_file_name_.".gz";
      `gunzip $t_gz_query_file_name_`;
    }
  } else {
    `cp $query_file_name_ $t_query_log_dir_`;
  }
}

my $sim_strat_file_name_ = "";
my $dep_shortcode_ = "";

my $t_model_file_name_ = "";
my $t_param_file_name_ = "";
{ # Generate local strategy, model & param file.
# Get the model & param file names from the log.
  open QUERY_LOG_FILE_HANDLE, "< $t_query_file_name_" or PrintStacktraceAndDie ( "Could not open $t_query_file_name_\n" );
  my @query_log_file_lines_ = <QUERY_LOG_FILE_HANDLE>;
  close QUERY_LOG_FILE_HANDLE;
  `rm -rf $t_query_file_name_`;

# Find the time of first call to StartTrading
  my $unix_time_script_ = $SCRIPTS_DIR."/unixtime2gmtstr.pl";

  my $sim_start_time_ = "";
  my $sim_start_time_sec_ = "";
  for ( my $line_ = 0; $line_ <= $#query_log_file_lines_; $line_++ ) {
    if ( index ( $query_log_file_lines_[$line_], "StartTrading Called" ) > 0 ) {
# First field is the timestamp.
      my @strategy_info_words_ = split (' ', $query_log_file_lines_[$line_]);
      $sim_start_time_ = $strategy_info_words_[0]; chomp ($sim_start_time_);
      $sim_start_time_sec_ = $sim_start_time_;
# Convert the sec.usec time to UTC.
      $sim_start_time_ = `$unix_time_script_ $sim_start_time_`;

# "Mon Jan 16 08:01:47 2012" => "08:01:47"
      my @start_time_words_ = split (' ', $sim_start_time_);
      $sim_start_time_ = $start_time_words_[3]; chomp ($sim_start_time_);

# "08:01:47" => "0801"
      @start_time_words_ = split (':', $sim_start_time_);
      $sim_start_time_ = $start_time_words_[0].$start_time_words_[1].$start_time_words_[2]; chomp ($sim_start_time_);
      print "KP".$sim_start_time_."\n";

      last;
    }


  }

  for ( my $line_ = 0; $line_ <= $#query_log_file_lines_; $line_++ ) {
    if ( index ( $query_log_file_lines_[$line_], "OnIndicatorUpdate" ) > 0 ) {
      my @strategy_info_words_ = split(' ', $query_log_file_lines_[$line_]);
      my $prep_start_time_ = $strategy_info_words_[0]; chomp ($prep_start_time_);
      last; 
    }
  }

  my $sim_end_time_ = -1;
# my $take_end_from_strat_ = 1;
# if ( $take_end_from_strat_ > 0 )
# {
# 	for ( my $line_ = $#query_log_file_lines_ ; $line_ >= 0 ; $line_ -- )
# 	{
# 	    if ( 
# 		( index ( $query_log_file_lines_ [ $line_ ] , "getflat_due_to_external_getflat_" ) >= 0 ) ||
# 		( index ( $query_log_file_lines_ [ $line_ ] , "getflat_due_to_max_loss_" ) >=0 )
# 		)
# 	    {
# 		$sim_end_time_ = ( split ( ' ' , $query_log_file_lines_ [ $line_ ] ) ) [ 0 ];

# 		# Convert the sec.usec time to UTC.
# 		$sim_end_time_ = `$unix_time_script_ $sim_end_time_`;

# 		# "Mon Jan 16 08:01:47 2012" => "08:01:47"
# 		my @end_time_words_ = split ( ' ' , $sim_end_time_ );
# 		$sim_end_time_ = $end_time_words_ [ 3 ]; chomp ( $sim_end_time_ );

# 		# "08:01:47" => "0801"
# 		@end_time_words_ = split ( ':' , $sim_end_time_ );
# 		$sim_end_time_ = $end_time_words_ [ 0 ].$end_time_words_ [ 1 ]; chomp ( $sim_end_time_ );

# 		last;
# 	    }
# 	}
# }

  use File::Basename;
# to take care of multiplse srtas that get 
  my %has_processed_strat=();

  my $strategy_line_ = 0;
  for ( $strategy_line_ = $#query_log_file_lines_;$strategy_line_ >= 0; $strategy_line_-- ) {
    if ( index ( $query_log_file_lines_[$strategy_line_], "STRATEGYLINE" ) >= 0 ) {

      my $remote_strat_file_name_ = $query_log_file_lines_[0]; chomp ($remote_strat_file_name_);
      my $sim_strat_file_basename_ = basename ($remote_strat_file_name_); chomp ($sim_strat_file_basename_);
# Print out the strategy name.
      if ( $print_strat_exec_cmd > 0 )
      {
# Print out the strategy name.
        print $sim_strat_file_basename_."\n";
      }

      $sim_strat_file_name_ = $t_strat_file_dir_."/".$sim_strat_file_basename_;
      my $saved_strat_file_name_ = $real_query_log_dir_."/".$sim_strat_file_basename_;
#    print STDERR "$saved_strat_file_name_\n";
      if ( -e $saved_strat_file_name_ ) 
      { 
        $sim_end_time_ = GetSimEndTimeStratFile ( $saved_strat_file_name_ ) ; 
#	print STDERR "$saved_strat_file_name_ $sim_end_time_\n";
      }

      my @strategy_info_words_ = split (' ', $query_log_file_lines_[$strategy_line_]);

      if ( $#strategy_info_words_ < 6 )
      {
        print "Strategy info words less than 6\n";
        exit;
      }
      if ( $has_processed_strat{ join(' ', @strategy_info_words_[1..7]) } ) {
        next;
      }
      else {  
        $has_processed_strat{ join(' ', @strategy_info_words_[1..7]) }  = 1;
      }

      $dep_shortcode_ = $strategy_info_words_[1]; chomp ($dep_shortcode_);

      my $trading_location_ = GetTradingLocationForShortcode ( $dep_shortcode_ , $yyyymmdd_ );
      my $remote_login_ = "dvctrader\@".$trading_location_;

      my $remote_model_file_name_ = $strategy_info_words_[3]; chomp ($remote_model_file_name_);
      my $remote_param_file_name_ = $strategy_info_words_[4]; chomp ($remote_param_file_name_);

      my $t_model_file_basename_ = basename ($remote_model_file_name_); chomp ($t_model_file_basename_);
      $t_model_file_name_ = $t_model_file_dir_."/".$t_model_file_basename_;
      my $t_param_file_basename_ = basename ($remote_param_file_name_); chomp ($t_param_file_basename_);
      $t_param_file_name_ = $t_param_file_dir_."/".$t_param_file_basename_;

# rsync the model & param files to this server.
      if ( $USER eq "sghosh" && $to_remove_strat_files_ )
      {
        if ( ! ExistsWithSize ( $t_param_file_name_ ) )
        {
          my $real_param_filename_ = $real_query_log_dir_."/".$t_param_file_basename_;
          if ( -e $real_param_filename_ )
          {
            `cp $real_param_filename_ $t_param_file_name_`;
          }
          else
          {
            `rsync -avz $remote_login_:$remote_param_file_name_ $t_param_file_dir_`;
          }
        }
        if ( ! ExistsWithSize ( $t_model_file_name_ ) )
        {
          my $real_model_filename_ = $real_query_log_dir_."/".$t_model_file_basename_;
          if ( -e $real_model_filename_ )
          {
            `cp $real_model_filename_ $t_model_file_name_`;
          }
          else
          {
            `rsync -avz $remote_login_:$remote_model_file_name_ $t_model_file_dir_`;
          }
        }
      }
      else
      { # Even for sghosh with $to_remove_strat_files_ == 0 , rsync the model & param files.
        my $real_param_filename_ = $real_query_log_dir_."/".$t_param_file_basename_;
        if ( -e $real_param_filename_ )
        {
          `cp $real_param_filename_ $t_param_file_name_`;
        }
        else
        {
          `rsync -avz $remote_login_:$remote_param_file_name_ $t_param_file_dir_`;
        }

        my $real_model_filename_ = $real_query_log_dir_."/".$t_model_file_basename_;
        if ( -e $real_model_filename_ )
        {
          `cp $real_model_filename_ $t_model_file_name_`;
        }
        else
        {
          `rsync -avz $remote_login_:$remote_model_file_name_ $t_model_file_dir_`;
        }
      }

#    # Try to get the params and models from the /NAS1/logs/QueryLogs directory
#    # if available , they will be a 100 % accurate.
#    # not sure why we wrote this comment but left the rsync part above.
#    FetchSyncedParamsAndModels ( $t_model_file_name_ , $t_param_file_name_ );

      my $UTC_TIME_EXEC = $BIN_DIR."/get_utc_hhmmss_str";

      { # Start time is max (Call_to_StartTrading, Stratfile_Start_Time).
        use List::Util qw/max min/; # for max

            my $t_strat_start_time_ = $strategy_info_words_[5];

        $t_strat_start_time_ = `$UTC_TIME_EXEC $t_strat_start_time_ $yyyymmdd_`;
	print $t_strat_start_time_."\n";
        $sim_start_time_ = max ( $sim_start_time_, $t_strat_start_time_);
      }

      if ( $sim_end_time_ eq -1 )
      {
        $sim_end_time_ = $strategy_info_words_ [ 6 ];
      } 
# commented out since we are taking end time from strat file
# else
# { # End time is min ( Last_call_to_external_getflat , stratfile_end_time )
# 	use List::Util qw/max min/; # for min

# 	my $t_strat_end_time_ = $strategy_info_words_ [ 6 ];

# 	$t_strat_end_time_ = `$UTC_TIME_EXEC $t_strat_end_time_ $yyyymmdd_`;

# 	$sim_end_time_ = min ( $sim_end_time_, $t_strat_end_time_);
# }

      if ( GetQueryTypeForIdDate ( $unique_id_ , $yyyymmdd_ ) eq "EQUITY" )
      { # Delay start time for queries which were not ready , since waiting for equity data.
        my $t_equity_start_time_ = `$UTC_TIME_EXEC EST_905 $yyyymmdd_`;

        $sim_start_time_ = max ( $sim_start_time_ , $t_equity_start_time_ );
      }

      $t_=`date +%N |cut -c6-`;chomp($t_);
      $sim_id_ = "$strategy_info_words_[7]"."$t_" ;
      if ( $sim_id_ > 9999999 ) { $sim_id_ = substr ( $sim_id_ , 0 , 8 ); }

      if ( $USER eq "sghosh" && $to_remove_strat_files_ )
      {
        if ( ! ExistsWithSize ( $sim_strat_file_name_ ) )
        {
# Generate the strategy file to use in sim.
          open SIM_STRAT_FILE_HANDLE, ">> $sim_strat_file_name_" or PrintStacktraceAndDie ( "Could not create file $sim_strat_file_name_\n" );
#    print SIM_STRAT_FILE_HANDLE "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $sim_start_time_ $strategy_info_words_[6] $sim_id_\n";
          print SIM_STRAT_FILE_HANDLE "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $sim_start_time_ $sim_end_time_ $sim_id_\n";
          close SIM_STRAT_FILE_HANDLE;
        }
      }
      else
      {
# Generate the strategy file to use in sim.
        open SIM_STRAT_FILE_HANDLE, ">> $sim_strat_file_name_" or PrintStacktraceAndDie ( "Could not create file $sim_strat_file_name_\n" );
#    print SIM_STRAT_FILE_HANDLE "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $sim_start_time_ $strategy_info_words_[6] $sim_id_\n";
        print SIM_STRAT_FILE_HANDLE "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $sim_start_time_ $sim_end_time_ $sim_id_\n";
        close SIM_STRAT_FILE_HANDLE;
      }

    }
  }

# Line 0 is the strategyfile name & line 1 contains it's contents -- This might not necessarily be true (tradeinit restarted) FIXED above.
}

{ # Run SIM Strategy and get SIM pnl & vol.
  my $sim_exec_=$HOME_DIR."/LiveExec/bin/sim_strategy";
  if ( ! ( -e $sim_exec_ ) ) {
    $sim_exec_=$BIN_DIR."/sim_strategy";
  }

#    $sim_exec_ = "/home/sghosh/basetrade_install/bin/sim_strategy";

  my $mkt_model_ = GetMarketModelForShortcode ($dep_shortcode_);

  if ( $USER eq "sghosh" )
  {
    print "SIMID ".$sim_id_."\n";
  }

  my $exec_cmd="$sim_exec_ SIM $sim_strat_file_name_ $sim_id_ $yyyymmdd_ $mkt_model_ 0 0.0 0 ADD_DBG_CODE -1";

  if ( $print_strat_exec_cmd || $USER eq "sghosh" || $USER eq "dvctrader" || $USER eq "ravi" || $USER eq "diwakar" )
  {
    print "$exec_cmd\n";
  }


  my $sim_output_ = `$exec_cmd`;
  print $sim_output_;
}

if ( ! ( -e $trades_file_name_ ) )
{
  print "Trades file does not exist\n";
  exit ;
}
{ # Find real pnl & vol.
  my $t_trades_file_name_ = $t_query_log_dir_."/"."trades.".$yyyymmdd_.".".$unique_id_;
  `cp $trades_file_name_ $t_query_log_dir_`;

  open TRADES_LOG_FILE_HANDLE, "< $t_trades_file_name_" or PrintStacktraceAndDie ( "Could not open $t_trades_file_name_\n" );
  my @trades_file_lines_ = <TRADES_LOG_FILE_HANDLE>;
  close TRADES_LOG_FILE_HANDLE;
  `rm -rf $t_trades_file_name_`;

  my $real_pnl_ = 0;
  my $real_vol_ = 0;

  for (my $trades_ = 0; $trades_ <= $#trades_file_lines_; $trades_++) {
    my @trades_words_ = split (' ', $trades_file_lines_[$trades_]);

    if ($#trades_words_ >= 15) 
    {
      my $t_pnl_ = $trades_words_[8]; chomp ($t_pnl_);
      $real_pnl_ = $t_pnl_;

      my $trade_size_ = $trades_words_[4]; chomp ($trade_size_);
      $real_vol_ += $trade_size_;
    }
  }

  print "REALRESULT $real_pnl_ $real_vol_\n";
}

if ( $USER eq "sghosh" )
{
  my $t_lpm_file_name_ = $HOME_DIR."/lpms.txt";
  my $t_lt_file_name_ = $HOME_DIR."/lts.txt";
  my $t_pr_file_name_ = $HOME_DIR."/prs.txt";

  `grep \" LPM \" /spare/local/logs/tradelogs/log.$yyyymmdd_.$sim_id_ | awk '{ print \$1" -10"; print \$1" 10"; print ""; }' > $t_lpm_file_name_`;
  `grep \" LT \" /spare/local/logs/tradelogs/log.$yyyymmdd_.$sim_id_ | awk '{ print \$1" -10"; print \$1" -30"; print ""; }' > $t_lt_file_name_`;
  `grep \" PR \" /spare/local/logs/tradelogs/log.$yyyymmdd_.$sim_id_ | awk '{ print \$1" 10"; print \$1" 30"; print ""; }' > $t_pr_file_name_`;

  my $plot_cmd_ = $SCRIPTS_DIR."/plot_multifile_cols.pl /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ 9 $yyyymmdd_.$sim_id_ WL $trades_file_name_ 9 $yyyymmdd_.$unique_id_ WL";

  if ( ExistsWithSize ( $t_lpm_file_name_ ) )
  {
    $plot_cmd_ = $plot_cmd_." "."$t_lpm_file_name_ 2 $yyyymmdd_.$sim_id_.LPM.pxs WL";
  }

  if ( ExistsWithSize ( $t_lt_file_name_ ) )
  {
    $plot_cmd_ = $plot_cmd_." "."$t_lt_file_name_ 2 $yyyymmdd_.$sim_id_.LT.pxs WL";
  }

  if ( ExistsWithSize ( $t_pr_file_name_ ) )
  {
    $plot_cmd_ = $plot_cmd_." "."$t_pr_file_name_ 2 $yyyymmdd_.$sim_id_.PR.pxs WL";
  }

  `$plot_cmd_`;

  my $PLOT_PNL_DIFF_SCRIPT = $SCRIPTS_DIR."/plot_pnl_diffs.pl";

  $plot_cmd_ = "$PLOT_PNL_DIFF_SCRIPT $trades_file_name_ /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_";
  `$plot_cmd_`;
}
else
{
  `$SCRIPTS_DIR/plot_trades_pnl_utc_2_all.sh /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ $trades_file_name_`;
}

# Remove local strategy, model & param file.
if ( $to_remove_strat_files_ )
{
  `rm -rf $sim_strat_file_name_`;
  `rm -rf $t_model_file_name_`;
  `rm -rf $t_param_file_name_`;
}

# SIM trade & log file.
`rm -f /spare/local/logs/tradelogs/log.$yyyymmdd_.$sim_id_`;
`rm -f /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_`;

exit ( 0 );

sub FetchSyncedParamsAndModels
{
  my ( $t_model_file_name_ , $t_param_file_name_ ) = @_;

  my $synced_file_location_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_."/";

  my $synced_model_file_name_ = $synced_file_location_.basename ( $t_model_file_name_ );
  my $synced_param_file_name_ = $synced_file_location_.basename ( $t_param_file_name_ );

  if ( ExistsWithSize ( $synced_param_file_name_ ) )
  {
# print "cp $synced_param_file_name_ $t_param_file_name_\n";
    `cp $synced_param_file_name_ $t_param_file_name_`;
  }

  if ( ExistsWithSize ( $synced_model_file_name_ ) )
  {
# print "cp $synced_model_file_name_ $t_model_file_name_\n";
    `cp $synced_model_file_name_ $t_model_file_name_`;
  }

  return;
}

sub GetSimEndTimeStratFile
{
  my $retval_ = 235900;
  my ( $t_saved_strat_file_name_ ) = @_;
  if ( -e $t_saved_strat_file_name_ )
  {
    my @saved_strategy_info_words_ = split (' ', $t_saved_strat_file_name_);
    if ( $#saved_strategy_info_words_ >= 6 )
    {
      $retval_ = $saved_strategy_info_words_[6]; #the value returned here might also be in hhmm format
    }
  }
  $retval_ ;
}
