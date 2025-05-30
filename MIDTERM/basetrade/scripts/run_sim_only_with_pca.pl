#!/usr/bin/perl

use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $usage_ = "run_sim_only_with_pca.pl DATE UNIQUE_ID";

if ($#ARGV < 1) {
    print $usage_."\n";
    exit (0);
}

my $yyyymmdd_ = GetIsoDateFromStr ( $ARGV[0] ) ; 
my ( $yyyy_, $mm_, $dd_ ) = BreakDateYYYYMMDD ( $yyyymmdd_ );

my $unique_id_ = $ARGV[1]; chomp ($unique_id_);

my $sim_id_ = `date +%N`;
$sim_id_ = $sim_id_ + 0;

my $query_trades_dir_ = "/NAS1/logs/QueryTrades/".$yyyy_."/".$mm_."/".$dd_;
my $trades_file_name_ = $query_trades_dir_."/"."trades.".$yyyymmdd_.".".$unique_id_;

if ( ! ExistsWithSize ( $trades_file_name_ ) )
{
    exit(0);
}

# Copy possibly gzipped log file to a temp dir.
my $t_query_log_dir_ = $HOME_DIR."/"."querytemp";

`mkdir -p $t_query_log_dir_`;

my $t_query_file_name_ = $t_query_log_dir_."/"."log.".$yyyymmdd_.".".$unique_id_;

{ # Make a local copy of the Query Log from real.
    my $query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;

    # First try non .gz log.
    my $query_file_name_ = $query_log_dir_."/"."log.".$yyyymmdd_.".".$unique_id_;
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
	    `gunzip -f $t_gz_query_file_name_`;
	}
    } else {
	`cp $query_file_name_ $t_query_log_dir_`;
    }
}

#print "t_query_file_name = ".$t_query_file_name_."\n";

my $sim_strat_file_name_ = "";
my $dep_shortcode_ = "";

my $t_model_file_name_ = "";
my $t_param_file_name_ = "";

{ # Generate local strategy, model & param file.
    # This map will need changes when moving instrument trading locations.
    my %shortcode_to_trading_location_ = 
	(
	 "ZN_0" => "sdv-chi-srv11",
	 "UB_0" => "sdv-chi-srv12", 
	 "ZF_0" => "sdv-chi-srv13", 
	 "ZT_0" => "sdv-chi-srv14", "ZB_0" => "sdv-chi-srv14", 

	 "FGBS_0" => "sdv-fr2-srv11", "FGBL_0" => "sdv-fr2-srv11", 
	 "FGBM_0" => "sdv-fr2-srv13", "FESX_0" => "sdv-fr2-srv13", 

	 "CGB_0" => "sdv-tor-srv11",
	 "BAX_0" => "sdv-tor-srv12", "BAX_1" => "sdv-tor-srv12", "BAX_2" => "sdv-tor-srv12", "BAX_3" => "sdv-tor-srv12", "BAX_4" => "sdv-tor-srv12", "BAX_5" => "sdv-tor-srv12"
	);

    # Get the model & param file names from the log.
    open QUERY_LOG_FILE_HANDLE, "< $t_query_file_name_" or PrintStacktraceAndDie ( "Could not open $t_query_file_name_\n" );
    my @query_log_file_lines_ = <QUERY_LOG_FILE_HANDLE>;
    close QUERY_LOG_FILE_HANDLE;
    #`rm -rf $t_query_file_name_`;

    # Find the time of first call to StartTrading
    my $unix_time_script_ = $SCRIPTS_DIR."/unixtime2gmtstr.pl";
    
    my $sim_start_time_ = "";
    for (my $line_ = 0; $line_ <= $#query_log_file_lines_; $line_++) {
	if (index ($query_log_file_lines_[$line_], "StartTrading Called") > 0) {
	    # First field is the timestamp.
	    my @strategy_info_words_ = split (' ', $query_log_file_lines_[$line_]);
	    $sim_start_time_ = $strategy_info_words_[0]; chomp ($sim_start_time_);

	    # Convert the sec.usec time to UTC.
	    $sim_start_time_ = `$unix_time_script_ $sim_start_time_`;

	    # "Mon Jan 16 08:01:47 2012" => "08:01:47"
	    my @start_time_words_ = split (' ', $sim_start_time_);
	    $sim_start_time_ = $start_time_words_[3]; chomp ($sim_start_time_);

	    # "08:01:47" => "0801"
	    @start_time_words_ = split (':', $sim_start_time_);
	    $sim_start_time_ = $start_time_words_[0].$start_time_words_[1]; chomp ($sim_start_time_);

	    last;
	}
    }


#    print "sim start time ".$sim_start_time_."\n";
    

    use File::Basename;

    # Line 0 is the strategyfile name & line 1 contains it's contents.
    my $remote_strat_file_name_ = $query_log_file_lines_[0]; chomp ($remote_strat_file_name_);
    $sim_strat_file_name_ = basename ($remote_strat_file_name_); chomp ($sim_strat_file_name_);
    # Print out the strategy name.
#    print "log_dir=".$t_query_log_dir_."\n";
    $sim_strat_file_name_ = $t_query_log_dir_."/".$sim_strat_file_name_;
#    print "sim_strat_file=".$sim_strat_file_name_."\n";

    my @strategy_info_words_ = split (' ', $query_log_file_lines_[1]);

    if ( $#strategy_info_words_ < 6 )
    {
	exit;
    }

    $dep_shortcode_ = $strategy_info_words_[1]; chomp ($dep_shortcode_);

    my $trading_location_ = $shortcode_to_trading_location_{$dep_shortcode_};
    my $remote_login_ = "dvctrader\@".$trading_location_;

    my $remote_model_file_name_ = $strategy_info_words_[3]; chomp ($remote_model_file_name_);
    my $remote_param_file_name_ = $strategy_info_words_[4]; chomp ($remote_param_file_name_);
    
    $t_model_file_name_ = basename ($remote_model_file_name_); chomp ($t_model_file_name_);
    $t_model_file_name_ = $t_query_log_dir_."/".$t_model_file_name_;
    $t_param_file_name_ = basename ($remote_param_file_name_); chomp ($t_param_file_name_);
    $t_param_file_name_ = $t_query_log_dir_."/".$t_param_file_name_;
    
#    print $remote_param_file_name_."\n";
#    print $remote_model_file_name_."\n";
    
    # rsync the model & param files to this server.
    `rsync -avz $remote_login_:$remote_model_file_name_ $t_query_log_dir_`;
    `rsync -avz $remote_login_:$remote_param_file_name_ $t_query_log_dir_`;
    
    # Generate the strategy file to use in sim.
    open SIM_STRAT_FILE_HANDLE, "> $sim_strat_file_name_" or PrintStacktraceAndDie ( "Could not create file $sim_strat_file_name_\n" );
#    print SIM_STRAT_FILE_HANDLE "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $sim_start_time_ $strategy_info_words_[6] $strategy_info_words_[7]\n";
    print SIM_STRAT_FILE_HANDLE "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $sim_start_time_ $strategy_info_words_[6] $sim_id_\n";
    close SIM_STRAT_FILE_HANDLE;

}

{ # Run SIM Strategy and get SIM pnl & vol.
    my $sim_exec_=$BIN_DIR."/sim_strategy";
    
    require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

    my $mkt_model_ = GetMarketModelForShortcode ($dep_shortcode_);
    
#    my $sim_output_ = `$sim_exec_ SIM $sim_strat_file_name_ $sim_id_ $yyyymmdd_ $mkt_model_ ADD_DBG_CODE TRADING_INFO ADD_DBG_CODE OM_INFO ADD_DBG_CODE PLSMM_INFO`;
    my $sim_output_ = `$sim_exec_ SIM $sim_strat_file_name_ $sim_id_ $yyyymmdd_ $mkt_model_ ADD_DBG_CODE -1`;
    print $sim_strat_file_name_." ".$sim_output_;
}

if ( ! ( -e $trades_file_name_ ) )
{
    exit ;
}

#`$SCRIPTS_DIR/plot_trades_pnl_nyc_2_all.sh /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ $trades_file_name_`;

#{ # Remove local strategy, model & param file.
#    `rm -rf $sim_strat_file_name_`;
#    `rm -rf $t_model_file_name_`;
#    `rm -rf $t_param_file_name_`;

    # SIM trade & log file.
    `rm -f /spare/local/logs/tradelogs/log.$yyyymmdd_.$sim_id_`;
    `rm -f /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_`;
#}

