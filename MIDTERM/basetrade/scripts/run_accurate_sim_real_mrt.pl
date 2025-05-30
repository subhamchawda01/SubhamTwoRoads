#!/usr/bin/perl
use strict;
use warnings;
use File::Basename;
use List::Util qw/max min/; # for max
use List::Util qw/max min/; # for min

sub FetchSyncedParamsAndModels;

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
require "$GENPERLLIB_DIR/get_unix_time_from_utc.pl"; #GetUnixtimeFromUTC
require "$GENPERLLIB_DIR/strat_utils.pl"; #CheckIfRegimeParam
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; #InsertSimRealPnlsForMRT

sub LoadLocsForQueryId {
  my $qid_to_loc_ref_ = shift;

  my $mapfile_ = "/home/dvctrader/dvccode/configs/mrt_query_server_map";
  my @maplines_ = `cat $mapfile_`; chomp @maplines_;

  foreach my $line_ (@maplines_) {
    my @ltoks_ = split(/\s+/, $line_);
    next if $#ltoks_ < 1;

    my $qid_ = $ltoks_[0];
    my $srvstr_ = $ltoks_[2];
    $$qid_to_loc_ref_{$qid_} = $srvstr_;
  }
}

sub GetLocForQueryId {
  my $qid_ = shift;

  my $mapfile_ = "/home/dvctrader/dvccode/configs/mrt_query_server_map";
  my $line_ = `grep -wm1 $qid_ $mapfile_`; chomp $line_;

  my @ltoks_ = split(/\s+/, $line_);
  return -1 if $#ltoks_ < 1;

  return $ltoks_[2];
}

my $print_strat_exec_cmd = 0;
my $usage_ = "run_accurate_sim_real.pl DATE UNIQUE_ID [REMOVE_STRAT_FILES=1] [OVERRIDE_SACI_STR=D] [STRAT_FILE_DIR] [PARAM_FILE_DIR] [MODEL_FILE_DIR]";

if ($#ARGV < 1) {
  print $usage_."\n";
  exit (0);
}

my $delete_trades_files = 1;
if (defined $ENV{"KEEP_TRADES_FILES"}) {
  $delete_trades_files = 0;
}

my $plot_simreal_ = 1;
if (defined $ENV{"NO_PLOT_SIMREAL"}) {
  $plot_simreal_ = 0;
}

my $yyyymmdd_ = GetIsoDateFromStr ( $ARGV[0] ) ;
my ( $yyyy_, $mm_, $dd_ ) = BreakDateYYYYMMDD ( $yyyymmdd_ );

my $unique_id_ = $ARGV[1]; chomp ($unique_id_);

my $to_remove_strat_files_ = 1;
my $saci_str_ = "D";
my $t_strat_file_dir_ = $HOME_DIR."/querytemp";
my $t_param_file_dir_ = $HOME_DIR."/querytemp";
my $t_model_file_dir_ = $HOME_DIR."/querytemp";
if ( $#ARGV > 1 )
{
  $to_remove_strat_files_ = $ARGV [ 2 ];
  if ( $#ARGV > 2 ) { $saci_str_ = $ARGV [ 3 ]; }
  if ( $#ARGV > 3 ) { $t_strat_file_dir_ = $ARGV [ 3 ]; }
  if ( $#ARGV > 4 ) { $t_param_file_dir_ = $ARGV [ 4 ]; }
  if ( $#ARGV > 5 ) { $t_model_file_dir_ = $ARGV [ 5 ]; }

  `mkdir -p $t_strat_file_dir_`;
  `mkdir -p $t_param_file_dir_`;
  `mkdir -p $t_model_file_dir_`;
}


my $query_trades_dir_ = "/NAS1/logs/QueryTrades/".$yyyy_."/".$mm_."/".$dd_;
my $trades_file_name_ = $query_trades_dir_."/"."trades.".$yyyymmdd_.".".$unique_id_;

my $real_query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;
my $query_file_name_ = $real_query_log_dir_."/"."log.".$yyyymmdd_.".".$unique_id_;
my $gz_query_file_name_ = $query_file_name_.".gz";

if ( ! ExistsWithSize ( $query_file_name_ ) && ! ExistsWithSize ( $gz_query_file_name_ ) ) {
  exit(0);
}


# Copy possibly gzipped log file to a temp dir.
my $t_query_log_dir_ = $HOME_DIR."/"."querytemp";
`mkdir -p $t_query_log_dir_`;

# sim_id_: SIM for simreal
# sim_id2_: normal SIM
my $t_ = `date +%N |cut -c6-`; chomp($t_);
my $sim_id_ = "$unique_id_"."$t_" + 0;
while ( $sim_id_ > 2147483647 ) {
  $sim_id_ = int($sim_id_/10);
}
my $sim_id2_ = int($sim_id_/10);

my $t_query_file_name_ = $t_query_log_dir_."/"."log.".$yyyymmdd_.".".$unique_id_;

# Make a local copy of the Query Log from real.
if ( -e $gz_query_file_name_ ) {
 my $t_gz_query_file_name_ = $t_query_file_name_.".gz";
 `cp $gz_query_file_name_ $t_gz_query_file_name_`;
 `gunzip $t_gz_query_file_name_`;
}
elsif ( -e $query_file_name_ ) {
  `cp $query_file_name_ $t_query_file_name_`;
}
  

my $sim_strat_file_name_ = "";
my $sim_strat_file_name_orig_ = "";
my $dep_shortcode_ = "";

my $t_model_file_name_ = "";
my $t_param_file_name_ = "";
my $sim_start_time_sec_ = "";
my %query_id_to_sim_start_time_ = ();
my %query_id_to_real_saci_map_ = ();
my $first_build_index_timestamp_ = 0;
my @stratnames_vec_ = ();
my %query_id_to_sbase_ = ();
my %query_id_to_stratline_map_ = (); #to avoid multiple plots in case of a late/re-install
my %query_id_to_stratline_map_orig_ = (); #to avoid multiple plots in case of a late/re-install
      
my $trading_location_ = GetLocForQueryId ($unique_id_);

{ # Generate local strategy, model & param file.
# Get the model & param file names from the log.
  open QUERY_LOG_FILE_HANDLE, "< $t_query_file_name_" or PrintStacktraceAndDie ( "Could not open $t_query_file_name_\n" );
  my @query_log_file_lines_ = <QUERY_LOG_FILE_HANDLE>;
  close QUERY_LOG_FILE_HANDLE;
  `rm -rf $t_query_file_name_`;

# Find the time of first call to StartTrading
  my $unix_time_script_ = $SCRIPTS_DIR."/unixtime2gmtstr.pl";

  my $sim_start_time_ = "";
  for ( my $line_ = 0; $line_ <= $#query_log_file_lines_; $line_++ )
  {  
    my @strategy_info_words_ = split (' ', $query_log_file_lines_[$line_]) ;
    if ( index ( $query_log_file_lines_[$line_], "StartTrading Called" ) > 0 )
    {
      next if defined $query_id_to_sim_start_time_{$unique_id_};
# First field is the timestamp.
      $sim_start_time_ = $strategy_info_words_[0]; chomp ($sim_start_time_) ;
      print "start-msg time: $sim_start_time_\n";
      if(! $sim_start_time_sec_)	{$sim_start_time_sec_ = $sim_start_time_;}
# Convert the sec.usec time to UTC.
      $sim_start_time_ = `$unix_time_script_ $sim_start_time_`;

# "Mon Jan 16 08:01:47 2012" => "08:01:47"
      my @start_time_words_ = split (' ', $sim_start_time_) ;
      $sim_start_time_ = $start_time_words_[3]; chomp ($sim_start_time_) ;

# "08:01:47" => "0801"
      @start_time_words_ = split (':', $sim_start_time_) ;
      $sim_start_time_ = $start_time_words_[0].$start_time_words_[1].$start_time_words_[2]; chomp ($sim_start_time_) ;
      $query_id_to_sim_start_time_{$unique_id_}=$sim_start_time_;
#last;
    }
#Ignore 0.00 timestamps (due to BuildIndex on intermediate packets)
    elsif ( index ( $query_log_file_lines_[$line_], "BuildIndex" ) > 0  )
    {
      if ( ( not $first_build_index_timestamp_ ) && ( index ( $strategy_info_words_[0], "0.00" ) == -1 ) )
      {
        $first_build_index_timestamp_ = $strategy_info_words_[0]; chomp ($first_build_index_timestamp_);
      }
    }
    elsif ( index ( $query_log_file_lines_[$line_], "PORT_STRATEGYLINE" ) >= 0  )
    {
#use first BuildIndex timestamp , SACI got ready : after last STRATEGYLINE, to help in case query restarts in live
      $first_build_index_timestamp_ = 0;
      %query_id_to_real_saci_map_ = ();
      # On query restart, we wan't sim_start time to be the first StartTrading line after the last IndexBuild
      %query_id_to_sim_start_time_= ();
    }  
    elsif ( $#strategy_info_words_ >=3 && index ( $query_log_file_lines_[$line_], "SACI" ) > 0 && index ( $query_log_file_lines_[$line_], "got ready" ) > 0 )
    {
      my $t_queryid_ = -1;
      if ( index ( $query_log_file_lines_[$line_], "queryid" ) > 0  )
      {
        $t_queryid_ = $strategy_info_words_[-1];
      }
      $query_id_to_real_saci_map_{$t_queryid_} = $strategy_info_words_[3];
    }
    elsif ( $#strategy_info_words_ == 0 && $strategy_info_words_[0] =~ /mean_revert_strats/ ) {
      push ( @stratnames_vec_, basename($strategy_info_words_[0]) );
    }
  }
  my @query_ids_ = sort keys %query_id_to_sim_start_time_;
  %query_id_to_sbase_ = map { $query_ids_[$_] => $stratnames_vec_[$_] } (0 .. $#query_ids_);

  my $sim_end_time_ = -1;
  my $take_end_from_strat_ = 0;
  if ( $take_end_from_strat_ > 0 )
  {
    for ( my $line_ = $#query_log_file_lines_ ; $line_ >= 0 ; $line_ -- )
    {
      if (
          ( index ( $query_log_file_lines_ [ $line_ ] , "getflat_due_to_external_getflat_" ) >= 0 ) ||
          ( index ( $query_log_file_lines_ [ $line_ ] , "getflat_due_to_max_loss_" ) >=0 ) ||
          ( index ( $query_log_file_lines_ [ $line_ ] , "getflat_due_to_close_" ) >=0 )
         )
      {
        print $query_log_file_lines_ [ $line_ ]."\n";
        $sim_end_time_ = ( split ( ' ' , $query_log_file_lines_ [ $line_ ] ) ) [ 0 ];

# Convert the sec.usec time to UTC.
        $sim_end_time_ = `$unix_time_script_ $sim_end_time_`;

# "Mon Jan 16 08:01:47 2012" => "08:01:47"
        my @end_time_words_ = split ( ' ' , $sim_end_time_ );
        $sim_end_time_ = $end_time_words_ [ 3 ]; chomp ( $sim_end_time_ );

# "08:01:47" => "0801"
        @end_time_words_ = split ( ':' , $sim_end_time_ );
        $sim_end_time_ = $end_time_words_ [ 0 ].$end_time_words_ [ 1 ].$end_time_words_[2]; chomp ( $sim_end_time_ );

        last;
      }
    }
  }


  my %has_processed_strat=();
  my @all_strat_files_ = ();

  my $strategy_line_ = 0;
  for ( $strategy_line_ = $#query_log_file_lines_;$strategy_line_ >= 0; $strategy_line_-- ) {
    if ( index ( $query_log_file_lines_[$strategy_line_], "PORT_STRATEGYLINE" ) >= 0 ) {

      push ( @all_strat_files_, $query_log_file_lines_[$strategy_line_] ) ;

      my @strategy_info_words_ = split (' ', $query_log_file_lines_[$strategy_line_]);


      if ( $#strategy_info_words_ < 6 )
      {
        exit;
      }
      if ( $has_processed_strat{ join(' ', @strategy_info_words_[1..7]) } ) {
        next;
      }
      else {
        $has_processed_strat{ join(' ', @strategy_info_words_[1..7]) }  = 1;
      }

      $dep_shortcode_ = $strategy_info_words_[1]; chomp ($dep_shortcode_);

      my $remote_login_ = "dvctrader\@".$trading_location_;

      my $remote_model_file_name_ = $strategy_info_words_[3]; chomp ($remote_model_file_name_);
      my $remote_param_file_name_ = $strategy_info_words_[4]; chomp ($remote_param_file_name_);

      my $t_model_file_basename_ = basename ($remote_model_file_name_); chomp ($t_model_file_basename_);
      $t_model_file_name_ = $t_model_file_dir_."/".$t_model_file_basename_;
      my $t_param_file_basename_ = basename ($remote_param_file_name_); chomp ($t_param_file_basename_);
      $t_param_file_name_ = $t_param_file_dir_."/".$t_param_file_basename_;


# rsync the model & param files to this server.
      {
        my $real_param_filename_ = $real_query_log_dir_."/".$t_param_file_basename_;
        if ( -e $real_param_filename_ ) {
          `cp $real_param_filename_ $t_param_file_name_`;
        }
        else {
          `rsync -avz $remote_login_:$remote_param_file_name_ $t_param_file_dir_`;
        }

        if ( CheckIfRegimeParam ( $t_param_file_name_ ) != 0 && $yyyymmdd_ < 20170627 )
        {
          my $t_reg_param_file_name_ = $t_param_file_name_.".reg";
          open PARAM, "< $t_param_file_name_";
          open REGPARAM, "> $t_reg_param_file_name_";
          my @param_lines_ = <PARAM>; chomp(@param_lines_);
          foreach my $param_line_ ( @param_lines_ )
          {
            my @words_ = split ' ', $param_line_;
            if ( index($words_[ 0 ], "PARAMFILELIST" ) >= 0 )
            {
              my $remote_regime_param_file_name_ = $words_[ 1 ];
              my $t_regime_param_file_basename_ = basename ( $remote_regime_param_file_name_ ); chomp ( $t_regime_param_file_basename_ );
              my $t_regime_param_file_ = $t_param_file_dir_."/".$t_regime_param_file_basename_;
              if ( ! ExistsWithSize ( $t_regime_param_file_ ) )
              {
                my $real_regime_param_filename_ = $real_query_log_dir_."/".$t_regime_param_file_basename_;
                if ( -e $real_regime_param_filename_ )
                {
                  `cp $real_regime_param_filename_ $t_regime_param_file_`;
                } 
                else
                {
                  `rsync -avz $remote_login_:$remote_regime_param_file_name_ $t_param_file_dir_`;
                }
              }
              print REGPARAM "PARAMFILELIST $t_regime_param_file_\n";
            }
            else
            {
              print REGPARAM "$param_line_\n";
            }

          }
          close (REGPARAM);
          `cp $t_reg_param_file_name_ $t_param_file_name_`;
        }


        my $real_model_filename_ = $real_query_log_dir_."/".$t_model_file_basename_;
        if ( -e $real_model_filename_ ) {
          `cp $real_model_filename_ $t_model_file_name_`;
        }
        else {
          `rsync -avz $remote_login_:$remote_model_file_name_ $t_model_file_dir_`;
        }
      }


      my $UTC_TIME_EXEC = $BIN_DIR."/get_utc_hhmmss_str";

      { # Start time is max (Call_to_StartTrading, Stratfile_Start_Time).

        my $t_strat_start_time_ = $strategy_info_words_[5];

        $t_strat_start_time_ = `$UTC_TIME_EXEC $t_strat_start_time_ $yyyymmdd_`;

        $query_id_to_sim_start_time_{$strategy_info_words_[7]} = max($query_id_to_sim_start_time_{$strategy_info_words_[7]}, $t_strat_start_time_);
#$sim_start_time_ = max ( $sim_start_time_, $t_strat_start_time_);

        my $utc_time_ = GetUnixtimeFromUTC($yyyymmdd_, $t_strat_start_time_);
      }

      if ( (not $take_end_from_strat_) ||  $sim_end_time_ == -1 )
      {
        $sim_end_time_ = $strategy_info_words_ [ 6 ];
      }
      else
      { # End time is min ( Last_call_to_external_getflat , stratfile_end_time )

        my $t_strat_end_time_ = $strategy_info_words_ [ 6 ];

        $t_strat_end_time_ = `$UTC_TIME_EXEC $t_strat_end_time_ $yyyymmdd_`;

        $sim_end_time_ = min ( $sim_end_time_, $t_strat_end_time_);
      }

      if ( GetQueryTypeForIdDate ( $unique_id_ , $yyyymmdd_ ) eq "EQUITY" )
      { # Delay start time for queries which were not ready , since waiting for equity data.
        my $t_equity_start_time_ = `$UTC_TIME_EXEC EST_905 $yyyymmdd_`;

        $query_id_to_sim_start_time_{$strategy_info_words_[7]} = max($query_id_to_sim_start_time_{$strategy_info_words_[7]}, $t_equity_start_time_);
#$sim_start_time_ = max ( $sim_start_time_ , $t_equity_start_time_ );
      }

      $t_=`date +%N |cut -c6-`;chomp($t_);
      $sim_id_ = "$strategy_info_words_[7]"."$t_" ;
      $sim_id_ = $sim_id_ + 0;
      while ( $sim_id_ > 2147483647 )
      {
        $sim_id_ = int( $sim_id_/10);
      }

      if(!exists($query_id_to_stratline_map_{$strategy_info_words_[7]}))
      {
        if ( $strategy_info_words_[2] =~ "EventDirectionalAggressiveTrading" ) {
          if ( $#strategy_info_words_ == 9 ) {
            $sim_strat_file_name_ = $t_strat_file_dir_."/".$strategy_info_words_[9]."_".$strategy_info_words_[7] ;
            $query_id_to_stratline_map_{$strategy_info_words_[7]} = "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $query_id_to_sim_start_time_{$strategy_info_words_[7]} $sim_end_time_ $strategy_info_words_[7] $strategy_info_words_[8]\n";
            $query_id_to_stratline_map_orig_{$strategy_info_words_[7]} = "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $strategy_info_words_[5] $strategy_info_words_[6] $strategy_info_words_[7] $strategy_info_words_[8]\n";
          }
        } else {
          if ( $#strategy_info_words_ >= 7 )
          {
            $sim_strat_file_name_ = $t_strat_file_dir_."/_".$strategy_info_words_[7] ;
          }
          $query_id_to_stratline_map_{$strategy_info_words_[7]} = "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $query_id_to_sim_start_time_{$strategy_info_words_[7]} $sim_end_time_ $strategy_info_words_[7]\n";
          $query_id_to_stratline_map_orig_{$strategy_info_words_[7]} = "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $strategy_info_words_[5] $strategy_info_words_[6] $strategy_info_words_[7]\n";
        }
        $sim_strat_file_name_orig_ = $sim_strat_file_name_."_orig";
      }
    }
  }


  open SIM_STRAT_FILE_HANDLE, "> $sim_strat_file_name_" or PrintStacktraceAndDie ( "Could not create file $sim_strat_file_name_\n" );
  foreach (keys %query_id_to_stratline_map_)
  {
    my $sim_line = $query_id_to_stratline_map_{$_};
    #For error cases where we had used same model file for _0 and _1 fut expiries
    if (index($dep_shortcode_, "NSE_") != -1) {
      my @words = split(' ', $sim_line);
      $words[3] = FixModelFile($words[1], $words[3]);
      chomp($words[3]);
      $sim_line = join(' ', @words)."\n";
    }
    print SIM_STRAT_FILE_HANDLE $sim_line;
  }
  close SIM_STRAT_FILE_HANDLE;

  open SIM_STRAT_FILE_HANDLE, "> $sim_strat_file_name_orig_" or PrintStacktraceAndDie ( "Could not create file $sim_strat_file_name_orig_\n" );
  foreach (keys %query_id_to_stratline_map_orig_)
  {
    my $sim_line = $query_id_to_stratline_map_orig_{$_};
    #For error cases where we had used same model file for _0 and _1 fut expiries
    if (index($dep_shortcode_, "NSE_") != -1) {
      my @words = split(' ', $sim_line);
      $words[3] = FixModelFile($words[1], $words[3]);
      chomp($words[3]);
      $sim_line = join(' ', @words)."\n";
    }
    print SIM_STRAT_FILE_HANDLE $sim_line;
  }
  close SIM_STRAT_FILE_HANDLE;
}


{ # Run SIM Strategy and get SIM pnl & vol.
  my $sim_exec_="/home/dvctrader/cvquant_install/basetrade/bin/sim_strategy"; #$BIN_DIR."/sim_strategy";

  require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
  my $mkt_model_ = GetMarketModelForShortcode ($dep_shortcode_);

  my $num_strat_ = `wc -l $sim_strat_file_name_ | awk '{print \$1}'`; chomp ( $num_strat_ ) ;
  my @t_queryids_ = `awk '{print \$8}' $sim_strat_file_name_`; chomp(@t_queryids_);
  my $count_ = keys %query_id_to_real_saci_map_;

  if ( $saci_str_ eq "D" )
  {
    $saci_str_ = -1;
    if ( $count_ == $#t_queryids_ + 1)
    {
      if ( $count_==1 && exists( $query_id_to_real_saci_map_{-1} ) )
      {
        $saci_str_ = $query_id_to_real_saci_map_{-1};
      }
      else
      {
        my @saci_vec_ = ();
        foreach my $t_queryid_ ( @t_queryids_ )
        {
          if ( !exists($query_id_to_real_saci_map_{$t_queryid_}) ) { @saci_vec_ = ( -1 ); last; }
          else
          {
            push ( @saci_vec_, $query_id_to_real_saci_map_{$t_queryid_} );
          }
        }
        $saci_str_ = join(',', @saci_vec_);
      }
    }
  }

  my $exec_cmd = "$sim_exec_ SIM $sim_strat_file_name_ $sim_id_ $yyyymmdd_ $mkt_model_ $saci_str_ $first_build_index_timestamp_ 0 ADD_DBG_CODE -1";
  print "$exec_cmd\n";
  my $sim_output_ = `$exec_cmd`;
  #print $sim_output_;

  $exec_cmd = "$sim_exec_ SIM $sim_strat_file_name_orig_ $sim_id2_ $yyyymmdd_ $mkt_model_ 0 ADD_DBG_CODE -1";
  print "$exec_cmd\n";
  `$exec_cmd`;
}

if ( ! -e $trades_file_name_ ) {
  exit ;
}
  
my %real_pnl_per_query_ = ();
my %sim_pnl_per_query_ = ();
my %sim2_pnl_per_query_ = ();


{ # Find real pnl & vol.
  my $t_trades_file_name_ = $t_query_log_dir_."/"."trades.".$yyyymmdd_.".".$unique_id_;
  {
    `cp $trades_file_name_ $t_query_log_dir_`;

    open TRADES_LOG_FILE_HANDLE, "< $t_trades_file_name_" or PrintStacktraceAndDie ( "Could not open $t_trades_file_name_\n" );
    my @trades_file_lines_ = <TRADES_LOG_FILE_HANDLE>;
    close TRADES_LOG_FILE_HANDLE;
    `rm -rf $t_trades_file_name_`;

    for (my $trades_ = 0; $trades_ <= $#trades_file_lines_; $trades_++)
    {
      my @trades_words_ = split (' ', $trades_file_lines_[$trades_]);

      if ($#trades_words_ >= 15)
      {
        my $t_pnl_ = $trades_words_[8]; chomp ($t_pnl_);
        my $trade_size_ = $trades_words_[4]; chomp ($trade_size_);
        if ( ! exists $real_pnl_per_query_{$trades_words_[2]} ) {
          $real_pnl_per_query_{$trades_words_[2]} = [0, 0];
        }
        $real_pnl_per_query_{$trades_words_[2]}[1] += $trade_size_;
        $real_pnl_per_query_{$trades_words_[2]}[0] =  $t_pnl_;
      }
    }
    foreach my $sh (keys %real_pnl_per_query_) {
      print "$sh REALRESULT @{$real_pnl_per_query_{$sh}}\n";
    }
    # Compute Total per query incase 
    foreach my $sh (keys %real_pnl_per_query_) {
      my $qid = (split(/\./, $sh))[1];
      if ( ! exists $real_pnl_per_query_{$qid}{"Total"} ) {
        $real_pnl_per_query_{$qid}{"Total"} = 0;
      }
      $real_pnl_per_query_{$qid}{"Total"} += $real_pnl_per_query_{$sh}[0];
    }
    foreach my $sh (keys %real_pnl_per_query_) {
      if ($sh =~ /^[0-9]*$/ and defined $real_pnl_per_query_{$sh}{"Total"}) {
        print "$sh TOTAL-REALRESULT ".$real_pnl_per_query_{$sh}{"Total"}."\n";
      }
    }
  }

  # Find SIM pnl & vol
  {
    `cp /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ $t_trades_file_name_`;

    open TRADES_LOG_FILE_HANDLE, "< $t_trades_file_name_" or PrintStacktraceAndDie ( "Could not open $t_trades_file_name_\n" );
    my @trades_file_lines_ = <TRADES_LOG_FILE_HANDLE>;
    close TRADES_LOG_FILE_HANDLE;
    `rm -rf $t_trades_file_name_`;

    for (my $trades_ = 0; $trades_ <= $#trades_file_lines_; $trades_++)
    {
      my @trades_words_ = split (' ', $trades_file_lines_[$trades_]);

      if ($#trades_words_ >= 15 && ( ($trades_words_[3] eq "B") || ($trades_words_[3] eq "S") ) )
      {
        my $t_pnl_ = $trades_words_[8]; chomp ($t_pnl_);
        my $trade_size_ = $trades_words_[4]; chomp ($trade_size_);
        if ( ! exists $sim_pnl_per_query_{$trades_words_[2]} ) {
          $sim_pnl_per_query_{$trades_words_[2]} = [0, 0];
        }
        $sim_pnl_per_query_{$trades_words_[2]}[1] += $trade_size_;
        $sim_pnl_per_query_{$trades_words_[2]}[0] =  $t_pnl_;
      }
    }
    foreach my $sh (keys %sim_pnl_per_query_) {
      print "$sh SIMRESULT @{$sim_pnl_per_query_{$sh}}\n";
    }
    # Compute Total per query incase 
    foreach my $sh (keys %sim_pnl_per_query_) {
      my $qid = (split(/\./, $sh))[1];
      if ( ! exists $sim_pnl_per_query_{$qid}{"Total"} ) {
        $sim_pnl_per_query_{$qid}{"Total"} = 0;
      }
      $sim_pnl_per_query_{$qid}{"Total"} += $sim_pnl_per_query_{$sh}[0];
    }
    foreach my $sh (keys %sim_pnl_per_query_) {
      if ($sh =~ /^[0-9]*$/ and defined $sim_pnl_per_query_{$sh}{"Total"}) {
        print "$sh TOTAL-SIMRESULT ".$sim_pnl_per_query_{$sh}{"Total"}."\n";
      }
    }
  }

  # Find normal SIM pnl & vol
  {
    `cp /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id2_ $t_trades_file_name_`;

    open TRADES_LOG_FILE_HANDLE, "< $t_trades_file_name_" or PrintStacktraceAndDie ( "Could not open $t_trades_file_name_\n" );
    my @trades_file_lines_ = <TRADES_LOG_FILE_HANDLE>;
    close TRADES_LOG_FILE_HANDLE;
    `rm -rf $t_trades_file_name_`;

    for (my $trades_ = 0; $trades_ <= $#trades_file_lines_; $trades_++)
    {
      my @trades_words_ = split (' ', $trades_file_lines_[$trades_]);

      if ($#trades_words_ >= 15 && ( ($trades_words_[3] eq "B") || ($trades_words_[3] eq "S") ) )
      {
        my $t_pnl_ = $trades_words_[8]; chomp ($t_pnl_);
        my $trade_size_ = $trades_words_[4]; chomp ($trade_size_);
        if ( ! exists $sim2_pnl_per_query_{$trades_words_[2]} ) {
          $sim2_pnl_per_query_{$trades_words_[2]} = [0, 0];
        }
        $sim2_pnl_per_query_{$trades_words_[2]}[1] += $trade_size_;
        $sim2_pnl_per_query_{$trades_words_[2]}[0] =  $t_pnl_;
      }
    }
    # Compute Total per query incase 
    foreach my $sh (keys %sim2_pnl_per_query_) {
      my $qid = (split(/\./, $sh))[1];
      if ( ! exists $sim2_pnl_per_query_{$qid}{"Total"} ) {
        $sim2_pnl_per_query_{$qid}{"Total"} = 0;
      }
      $sim2_pnl_per_query_{$qid}{"Total"} += $sim2_pnl_per_query_{$sh}[0];
    }
  }
}

# Insert them into DB
{
  foreach my $qid (keys %query_id_to_sbase_) {
    next if ! defined $real_pnl_per_query_{$qid}{"Total"};

    my $real_pnl_ = $real_pnl_per_query_{$qid}{"Total"};
    my $sim_pnl_ = $sim_pnl_per_query_{$qid}{"Total"};
    my $sim2_pnl_ = $sim2_pnl_per_query_{$qid}{"Total"};
    my $sname_ = $query_id_to_sbase_{$qid};

    print "InsertSimRealPnlsForMRT($qid, $yyyymmdd_, $sname_, $real_pnl_, $sim_pnl_, $sim2_pnl_)\n";
    InsertSimRealPnlsForMRT($qid, $yyyymmdd_, $sname_, $real_pnl_, $sim_pnl_, $sim2_pnl_);
  }
}


# PLOT the sim-real plots
{
  my $temp_trade_file_ = "/spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_"."_tmp";
  my $sim_temp_trade_file_ = "/spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_"."_simtmp";
  
  foreach (keys %real_pnl_per_query_)
  {
    $_ =~ s/\..*$//;
    print "KEY: $_\n";

    my $tok_num_;
    if (defined $real_pnl_per_query_{$_}{"Total"}) {
      `grep "OPEN\\\|FLAT" /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ > $sim_temp_trade_file_`;
      `grep "OPEN\\\|FLAT" $trades_file_name_ > $temp_trade_file_`;
      $tok_num_ = 18;
    }
    else {
      `grep $_ /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ > $sim_temp_trade_file_`;
      `grep $_ $trades_file_name_ > $temp_trade_file_`;
      $tok_num_ = 9;
    }

    if ( $plot_simreal_ == 1 ) {
      my $plot_cmd_ = $SCRIPTS_DIR."/plot_multifile_cols.pl $temp_trade_file_ $tok_num_ $yyyymmdd_.$_"."_REAL"." WL $sim_temp_trade_file_ $tok_num_ $yyyymmdd_.$_"."_SIM"." WL";
      print $plot_cmd_."\n";
      `$plot_cmd_`;
    } else {
      print "REAL_TRADE_FILE $sim_temp_trade_file_\n";
      print "SIM_TRADE_FILE $temp_trade_file_\n";
    }
  }

  if ($delete_trades_files == 1 && $plot_simreal_ == 1) {
    `rm -f $temp_trade_file_`;
    `rm -f $sim_temp_trade_file_`;
  }
}

# Remove local strategy, model & param file.
if ( $to_remove_strat_files_ )
{
  `rm -rf $sim_strat_file_name_`;
  `rm -rf $t_model_file_name_`;
  `rm -rf $t_param_file_name_`;
# SIM trade & log file.

  `rm -f /spare/local/logs/tradelogs/log.$yyyymmdd_.$sim_id_`;
  `rm -f /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_`;
}

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

sub FixModelFile
{
  my ( $shc, $model_file ) = @_;
  my $other_shc = $shc;
  if ($other_shc =~ /_FUT0$/) {
    chop($other_shc);
    $other_shc = $other_shc."1";
  }
  elsif ($other_shc =~ /_FUT1$/) {
    chop($other_shc);
    $other_shc = $other_shc."0";
  }
  else {
    return $model_file;
  }
  my $updated_model_file = $model_file."_fixed";
  my $cmd = "occurrence=`grep -c $shc $model_file`; if [ \$occurrence -lt 1 ]; then sed 's/$other_shc/$shc/g' $model_file > $updated_model_file; echo $updated_model_file; else echo $model_file; fi";
  my $new_model_file = `$cmd`;
  chomp($new_model_file);
  return $new_model_file;
}
