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
require "$GENPERLLIB_DIR/get_num_from_numsecname.pl"; # GetNumFromNumSecname
require "$GENPERLLIB_DIR/get_utc_hhmm_from_unixtime.pl"; # GetUTCHHMMFromUnixTime
require "$GENPERLLIB_DIR/get_utc_hhmmss_from_unixtime.pl"; # GetUTCHHMMSSFromUnixTime
require "$GENPERLLIB_DIR/change_start_time_of_strategy_file.pl"; # ChangeStartTimeofStrategyFile
require "$GENPERLLIB_DIR/create_local_strat_file.pl"; # CreateLocalStratFile


my $usage_ = "run_accurate_no_end_sim_real_stratid.pl DATE PROGID [REMOVE_STRAT_FILES=1] [STRAT_FILE_DIR] [PARAM_FILE_DIR] [MODEL_FILE_DIR]";

if ($#ARGV < 1) {
    print $usage_."\n";
    exit (0);
}

my $yyyymmdd_ = GetIsoDateFromStr ( $ARGV[0] ) ; 
my $progid_ = $ARGV[1]; 
my $to_remove_strat_files_ = 1;
if ( $#ARGV > 1 )
{
    $to_remove_strat_files_ = $ARGV [ 2 ];
}

my ( $yyyy_, $mm_, $dd_ ) = BreakDateYYYYMMDD ( $yyyymmdd_ );

my $t_strat_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_param_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_model_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_query_log_dir_ = $HOME_DIR."/"."querytemp";

`mkdir -p $t_strat_file_dir_`;
`mkdir -p $t_param_file_dir_`;
`mkdir -p $t_model_file_dir_`;
`mkdir -p $t_query_log_dir_`;

my @temporary_files_ = ();
my $real_query_trades_dir_ = "/NAS1/logs/QueryTrades/".$yyyy_."/".$mm_."/".$dd_;
my $real_query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;
my $real_trades_file_name_ = $real_query_trades_dir_."/"."trades.".$yyyymmdd_.".".$progid_;

my %stratid_to_real_pnl_map_ ;
my %stratid_to_real_volume_map_ ;
my %stratid_to_local_trades_filename_ ;

if ( ! ExistsWithSize ( $real_trades_file_name_ ) )
{
    exit(0);
}
else
{ # Find real pnl & vol per stratid we have information for
    open REAL_TRADES_FILEHANDLE, "< $real_trades_file_name_" or PrintStacktraceAndDie ( "Could not open $real_trades_file_name_\n" );
    while ( my $trades_file_line_ = <REAL_TRADES_FILEHANDLE> ) {
      chomp ( $trades_file_line_ );
      
      my @trades_words_ = split ( ' ', $trades_file_line_ ) ;
      
	if ($#trades_words_ >= 15) 
	{
	  my $t_stratid_ = GetNumFromNumSecname ( $trades_words_[2] ) ;

	  $stratid_to_real_pnl_map_ { $t_stratid_ } = int($trades_words_[8]); 

	  my $trade_size_ = $trades_words_[4]; 
	  if ( exists $stratid_to_real_volume_map_ { $t_stratid_ } ) {
	    $stratid_to_real_volume_map_ { $t_stratid_ } += $trade_size_;
	  } else {
	    $stratid_to_real_volume_map_ { $t_stratid_ } = $trade_size_ ;
	  }
	}
    }
    close REAL_TRADES_FILEHANDLE;

    for my $t_stratid_ ( keys %stratid_to_real_pnl_map_ ) 
    {
	my $t_local_trades_file_name_ = $t_query_log_dir_."/trades.".$yyyymmdd_.".".$progid_.".".$t_stratid_.".txt";
	`grep \".$t_stratid_\" $real_trades_file_name_ > $t_local_trades_file_name_`;
	$stratid_to_local_trades_filename_ { $t_stratid_ } = $t_local_trades_file_name_ ;
	push ( @temporary_files_, $t_local_trades_file_name_ ) ;
    }
}


# for each stratid make a local stratfile ( with correct start time )
my @local_strat_filename_vec_ = () ;
my @local_trades_filename_vec_ = () ; # associated with the stratid there must be a real trades file ( or part of it )
my @local_stratid_vec_ = () ; # although some of the info is redundant, storng stratid as well. Note this vector might have duplicates


my $t_query_file_name_ = $t_query_log_dir_."/"."log.".$yyyymmdd_.".".$progid_;

{ # In a local file just grep / zgrep "StartTrading Called"

    # First try non .gz log.
    my $query_file_name_ = $real_query_log_dir_."/"."log.".$yyyymmdd_.".".$progid_;
    if ( ! -e $query_file_name_) 
    {
	# Try the .gz version.
	$query_file_name_ = $query_file_name_.".gz";
	
	if ( ! -e $query_file_name_) {
	    print "File $query_file_name_ doesnot exist\n";
	    # cleanup
	    for my $t_file_ ( @temporary_files_ ) 
	    {
		`rm -f $t_file_`;
	    }
	    exit (0);
	} else {
	    `zgrep \"StartTrading Called\\\|STRATEGYLINE\" $query_file_name_ > $t_query_file_name_`;
	    push ( @temporary_files_, $t_query_file_name_ );
	}
    }
    else 
    {
	`grep "StartTrading Called\|STRATEGYLINE" $query_file_name_ > $t_query_file_name_`;
	push ( @temporary_files_, $t_query_file_name_ );
    }
}

my $dep_shortcode_ = "";
my $sim_start_time_sec_ = "";
my $secs_to_prep_ = 1800;
{ # Generate local strategy, model & param file.
    # Get the model & param file names from the log.
    open QUERY_LOG_FILE_HANDLE, "< $t_query_file_name_" or PrintStacktraceAndDie ( "Could not open $t_query_file_name_\n" );
    my @query_log_file_lines_ = <QUERY_LOG_FILE_HANDLE>;
    chomp ( @query_log_file_lines_ );
    close QUERY_LOG_FILE_HANDLE;
    push ( @temporary_files_, $t_query_file_name_);

    my %stratid_to_local_strat_filename_ ;
    for ( my $line_ = 0; $line_ <= $#query_log_file_lines_; $line_++ ) 
      {
	if ( index ( $query_log_file_lines_[$line_], "STRATEGYLINE" ) >= 0 ) 
	  {
	    my @strategy_info_words_ = split (' ', $query_log_file_lines_[$line_]);
	    if ( $#strategy_info_words_ >= 8 ) {
	      # STRATEGYLINE DI1F18 PriceBasedAggressiveTrading /home/dvctrader/LiveModels/models/DI1F18/BRT_1000-BRT_1500/w_model_.N /home/dvctrader/LiveModels/params/DI1F18/param_.4500 BRT_905 BRT_1540 37011

	      if ( ! $dep_shortcode_ ) { 
		# set dep_shortcode_ if not set already
		$dep_shortcode_ = $strategy_info_words_[1];
	      }

	      my $t_stratid_ = $strategy_info_words_[7];
	      my $sim_strat_file_name_ = $t_strat_file_dir_."/temp_strat_file_".$t_stratid_.".txt";
	      
	      if ( -e $sim_strat_file_name_ ) {
		  $sim_strat_file_name_ = $sim_strat_file_name_."s";
	      }
	      if ( ! -e $sim_strat_file_name_ ) 
		{ # check to see it is not present
		  # fetch saved copies of modelfile, paramfile and create file
		    my $temp_unique_gsm_id_ = `date +%N`; chomp ( $temp_unique_gsm_id_ ); $temp_unique_gsm_id_ = int($temp_unique_gsm_id_) + 0;
		  CreateLocalStratFile ( $real_query_log_dir_, $sim_strat_file_name_, $temp_unique_gsm_id_, \@strategy_info_words_ ) ; 
		  if ( ExistsWithSize ( $sim_strat_file_name_ ) ) {
		    # successfully created
		    $stratid_to_local_strat_filename_ { $t_stratid_ } = $sim_strat_file_name_ ;
		    push ( @temporary_files_, $sim_strat_file_name_ );
		  } else {
		      print "For id $t_stratid_ file $sim_strat_file_name_ was not created\n";
		  }
		}
	    }
	  } 
	elsif ( index ( $query_log_file_lines_[$line_], "StartTrading Called" ) >= 0 ) 
	  {
	    my @strategy_info_words_ = split (' ', $query_log_file_lines_[$line_]);
	    # First field is the timestamp.
	    my $sim_start_time_ = GetUTCHHMMSSFromUnixTime ( $strategy_info_words_[0] ) ; 
            $sim_start_time_sec_ = $strategy_info_words_[0];
	    my $t_stratid_ = $strategy_info_words_[4];
	    if ( exists $stratid_to_local_strat_filename_ { $t_stratid_ } ) 
	      {
		  my $t_sim_strat_filename_ = $stratid_to_local_strat_filename_ { $t_stratid_ } ;
		# change the sim start time
		ChangeStartTimeofStrategyFile ( $t_sim_strat_filename_, $sim_start_time_ ) ;
		
		# add to the vector of graphs to plot
		push ( @local_strat_filename_vec_, $t_sim_strat_filename_ ) ;
		push ( @local_stratid_vec_, $t_stratid_ ) ;
		push ( @local_trades_filename_vec_, $t_query_log_dir_."/trades.".$yyyymmdd_.".".$progid_.".".$t_stratid_.".txt" ) ;

		delete $stratid_to_local_strat_filename_ { $t_stratid_ } ; # removed this entry so that it is not input twice
	      }
	  }
      }
    for ( my $line_ = 0; $line_ <= $#query_log_file_lines_; $line_++ ) {
      if ( index ( $query_log_file_lines_[$line_], "OnIndicatorUpdate" ) > 0 ) {
      my @strategy_info_words_ = split(' ', $query_log_file_lines_[$line_]);
      my $prep_start_time_ = $strategy_info_words_[0]; chomp ($prep_start_time_);
      $secs_to_prep_ = int ( $sim_start_time_sec_ - $prep_start_time_ );
      last; 
    }
  }

  }

  


for ( my $i = 0 ; $i <= $#local_trades_filename_vec_ ; $i ++ ) 
  {
      # Run SIM Strategy and get SIM pnl & vol.

    my $sim_strat_file_name_ = $local_strat_filename_vec_[$i] ;
    my $t_stratid_ = $local_stratid_vec_[$i] ;
    my $real_trades_file_name_for_stratid_ = $local_trades_filename_vec_[$i];

    my $sim_exec_=$HOME_DIR."/LiveExec/bin/sim_strategy";
    if ( ! -e $sim_exec_ ) {
      $sim_exec_=$BIN_DIR."/sim_strategy";
    }

    my $mkt_model_ = GetMarketModelForShortcode ($dep_shortcode_);

    my $t_=`date +%N |cut -c6-`;chomp($t_);
    my $sim_id_ = "$progid_"."$t_";
    if ( $sim_id_ > 9999999 ) { $sim_id_ = substr ( $sim_id_ , 0 , 8 ); }

    my $sim_trades_filename_ = "/spare/local/logs/tradelogs/trades.".$yyyymmdd_.".".$sim_id_;
    `rm -f $sim_trades_filename_`; # make sure it does not exist
    {
	my $exec_cmd = "$sim_exec_ SIM $sim_strat_file_name_ $sim_id_ $yyyymmdd_ $mkt_model_ $secs_to_prep_ 0.0 0 ADD_DBG_CODE -1";
	# print $exec_cmd;
	my $sim_output_ = `$exec_cmd`;
	print $sim_output_;
    }

    printf ( "REALRESULT %d %d\n", $stratid_to_real_pnl_map_{$t_stratid_}, $stratid_to_real_volume_map_{$t_stratid_} ) ;

    `$SCRIPTS_DIR/plot_trades_pnl_utc_2_all.sh $sim_trades_filename_ $real_trades_file_name_for_stratid_`;

    `rm -f $sim_strat_file_name_`;
    `rm -f $real_trades_file_name_for_stratid_`;
    # SIM trade & log file.
    `rm -f $sim_trades_filename_`;
    `rm -f /spare/local/logs/tradelogs/log.$yyyymmdd_.$sim_id_`;
  }

for my $t_file_ ( @temporary_files_ ) 
{
    `rm -f $t_file_`;
}

exit ( 0 );


