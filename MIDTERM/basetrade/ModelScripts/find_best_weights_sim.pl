#!/usr/bin/perl
use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub GetSimWeights;
sub GetSimEndTimeStratFile;
my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";
my $TRADELOG_DIR = "/spare/local/logs/tradelogs/";
my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $SIM_TRADES_LOCATION = "/spare/local/logs/tradelogs/";
my $SIM_LOG_LOCATION = "/spare/local/logs/tradelogs/";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "dvctrader" || $USER eq "kishenp" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $SIM_STRATEGY_EXEC = $LIVE_BIN_DIR."/sim_strategy_ind";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode

my $USAGE="$0 SHORTCODE STRATID START_DATE END_DATE SIM_WEIGHTS_FILE";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $stratid_ = $ARGV [ 1 ];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 2 ] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 3 ] );
my $simweights_file_ = $ARGV [ 4 ];

my $mkt_model_ = GetMarketModelForShortcode ( $shortcode_ );

my $delete_intermediate_files_ = 1;

my @intermediate_files_ = ( );
my $to_remove_strat_files_ = 1;
my $t_strat_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_param_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_model_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_query_log_dir_ = $HOME_DIR."/"."querytemp";
`mkdir -p $t_strat_file_dir_`;
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );

my @sim_weights_list_1_ = ( );
my @sim_weights_list_2_ = ( );
my @pnl_mean_list_ = ( );
my @pnl_stdev_list_ = ( );
GetSimWeights ( );

if ( $#sim_weights_list_1_ < 0 )
{
    print "Exiting due to empty sim_config_file_list_\n";
}

for ( my $trading_yyyymmdd_ = $trading_end_yyyymmdd_ ; $trading_yyyymmdd_ >= $trading_start_yyyymmdd_ ; ){
    if ( ! ValidDate ( $trading_yyyymmdd_ ) ||
	 SkipWeirdDate ( $trading_yyyymmdd_ ) ||
	 IsDateHoliday ( $trading_yyyymmdd_ ) ||
	 IsWeirdSimDayForShortcode ( $shortcode_ , $trading_yyyymmdd_ ) )
    {
	$trading_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_yyyymmdd_ , 1 );
	next;
    }
    my ( $yyyy_, $mm_, $dd_ ) = BreakDateYYYYMMDD ( $trading_yyyymmdd_ );
    print "$yyyy_ $mm_ $dd_\n";
    my $query_trades_dir_ = "/NAS1/logs/QueryTrades/".$yyyy_."/".$mm_."/".$dd_;
    my $trades_file_name_ = $query_trades_dir_."/"."trades.".$trading_yyyymmdd_.".".$stratid_;
    if ( ! ExistsWithSize ( $trades_file_name_ ) ){
	print "Breaking out since $trades_file_name_ does not exist\n";
	next;
    }
    my $real_query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;
    my $query_file_name_ = $real_query_log_dir_."/"."log.".$trading_yyyymmdd_.".".$stratid_;
    my $t_query_file_name_ = $t_query_log_dir_."/"."log.".$trading_yyyymmdd_.".".$stratid_;
    if (! -e $query_file_name_) {
	$query_file_name_ = $query_file_name_.".gz";
	if (! -e $query_file_name_) {
	    print "File $query_file_name_ doesnot exist\n";
	    next;
	}
	else{
	    `cp $query_file_name_ $t_query_log_dir_`;
	    my $t_gz_query_file_name_ = $t_query_file_name_.".gz";
	    `gunzip $t_gz_query_file_name_`;
	}
    }
    else {
	`cp $query_file_name_ $t_query_log_dir_`;
    }
    my $sim_strat_file_name_ = "";
    my $dep_shortcode_ = "";

    my $t_model_file_name_ = "";
    my $t_param_file_name_ = "";

    open QUERY_LOG_FILE_HANDLE, "< $t_query_file_name_" or PrintStacktraceAndDie ( "Could not open $t_query_file_name_\n" );
    my @query_log_file_lines_ = <QUERY_LOG_FILE_HANDLE>;
    close QUERY_LOG_FILE_HANDLE;
    `rm -rf $t_query_file_name_`;
    my $unix_time_script_ = $SCRIPTS_DIR."/unixtime2gmtstr.pl";
    my $sim_start_time_ = "";
    for ( my $line_ = 0; $line_ <= $#query_log_file_lines_; $line_++ ) {
	if ( index ( $query_log_file_lines_[$line_], "StartTrading Called" ) > 0 ) {
	    my @strategy_info_words_ = split (' ', $query_log_file_lines_[$line_]);
	    $sim_start_time_ = $strategy_info_words_[0]; chomp ($sim_start_time_);

	    $sim_start_time_ = `$unix_time_script_ $sim_start_time_`;

	    my @start_time_words_ = split (' ', $sim_start_time_);
	    $sim_start_time_ = $start_time_words_[3]; chomp ($sim_start_time_);
	    @start_time_words_ = split (':', $sim_start_time_);
	    $sim_start_time_ = $start_time_words_[0].$start_time_words_[1]; chomp ($sim_start_time_);
	    last;
	}
    }
    my $sim_end_time_ = -1;
    my $strategy_line_ = 0;
    my $sim_id_ = "";
    for ( $strategy_line_ = $#query_log_file_lines_;$strategy_line_ >= 0; $strategy_line_-- ) {
        if ( index ( $query_log_file_lines_[$strategy_line_], "STRATEGYLINE" ) >= 0 ) {

            my $remote_strat_file_name_ = $query_log_file_lines_[0]; chomp ($remote_strat_file_name_);
            my $sim_strat_file_basename_ = basename ($remote_strat_file_name_); chomp ($sim_strat_file_basename_);

            $sim_strat_file_name_ = $t_strat_file_dir_."/".$sim_strat_file_basename_;
            my $saved_strat_file_name_ = $real_query_log_dir_."/".$sim_strat_file_basename_;
            if ( -e $saved_strat_file_name_ )
            {
                $sim_end_time_ = GetSimEndTimeStratFile ( $saved_strat_file_name_ ) ;
            }

            my @strategy_info_words_ = split (' ', $query_log_file_lines_[$strategy_line_]);

            if ( $#strategy_info_words_ < 6 )
            {
                exit;
            }

            $dep_shortcode_ = $strategy_info_words_[1]; chomp ($dep_shortcode_);

            my $trading_location_ = GetTradingLocationForShortcode ( $dep_shortcode_ , $trading_yyyymmdd_ );
            my $remote_login_ = "dvctrader\@".$trading_location_;

            my $remote_model_file_name_ = $strategy_info_words_[3]; chomp ($remote_model_file_name_);
            my $remote_param_file_name_ = $strategy_info_words_[4]; chomp ($remote_param_file_name_);

            my $t_model_file_basename_ = basename ($remote_model_file_name_); chomp ($t_model_file_basename_);
            $t_model_file_name_ = $t_model_file_dir_."/".$t_model_file_basename_;
            my $t_param_file_basename_ = basename ($remote_param_file_name_); chomp ($t_param_file_basename_);
            $t_param_file_name_ = $t_param_file_dir_."/".$t_param_file_basename_;
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
	    my $UTC_TIME_EXEC = $LIVE_BIN_DIR."/get_utc_hhmm_str";
	    use List::Util qw/max min/;
	    my $t_strat_start_time_ = $strategy_info_words_[5];

	    $t_strat_start_time_ = `$UTC_TIME_EXEC $t_strat_start_time_ $trading_yyyymmdd_`;

	    $sim_start_time_ = max ( $sim_start_time_, $t_strat_start_time_);
	    if ( $sim_end_time_ == -1 ){
		$sim_end_time_ = $strategy_info_words_ [ 6 ];
	    }
	    my $t_=`date +%N |cut -c6-`;chomp($t_);
	    $sim_id_ = "$strategy_info_words_[7]"."$t_" ;
            if ( $sim_id_ > 9999999 ) { $sim_id_ = substr ( $sim_id_ , 0 , 8 ); }
	    if ( ! ExistsWithSize ( $sim_strat_file_name_ ) )
	    {
		open SIM_STRAT_FILE_HANDLE, ">> $sim_strat_file_name_" or PrintStacktraceAndDie ( "Could not create file $sim_strat_file_name_\n" );
		print SIM_STRAT_FILE_HANDLE "$strategy_info_words_[0] $strategy_info_words_[1] $strategy_info_words_[2] $t_model_file_name_ $t_param_file_name_ $sim_start_time_ $sim_end_time_ $sim_id_\n";
		close SIM_STRAT_FILE_HANDLE;
	    }


	}
    }
    my $sim_exec_=$LIVE_BIN_DIR."/sim_strategy_ind";
    for (my $j=0; $j<=$#sim_weights_list_1_; $j++){
	my $exec_cmd="$sim_exec_ SIM $sim_strat_file_name_ $sim_id_ $trading_yyyymmdd_ $mkt_model_ ADD_DBG_CODE -1 $sim_weights_list_1_[$j] $sim_weights_list_2_[$j] ";
	print "$exec_cmd\n";
	my $sim_output_ = `$exec_cmd`;
	my $trades_diff_file_ = $t_query_log_dir_."/tradesdiff.".$trading_yyyymmdd_.".".$stratid_;
	my $TRADES_FILE_DIFF_SCRIPT = $SCRIPTS_DIR."/generate_trades_file_diffs.pl";
	my $exec_cmd_ = "$TRADES_FILE_DIFF_SCRIPT $trades_file_name_ /spare/local/logs/tradelogs/trades.$trading_yyyymmdd_.$sim_id_ xyz > $trades_diff_file_";
	print "$exec_cmd_\n";
	`$exec_cmd_`;
	open ( TRADES_DIFF_FILE , "<" , $trades_diff_file_ ) or PrintStacktraceAndDie ( "Could not open file $trades_diff_file_" );
	my @trades_diff_lines_ = <TRADES_DIFF_FILE>; chomp ( @trades_diff_lines_ );
	close ( TRADES_DIFF_FILE );

	$exec_cmd_ = "rm -f $trades_diff_file_";
	`$exec_cmd_`;

	my @abs_pnl_diff_ = ( );
	my @pnl_diff_ = ( );
	my @abs_vol_diff_ = ( );
	foreach my $trades_diff_line_ ( @trades_diff_lines_ )
	{
	    my @trades_diff_words_ = split ( ' ' , $trades_diff_line_ );
	    if ( $#trades_diff_words_ >= 3 )
	    {
		my $t_time_ = $trades_diff_words_ [ 0 ];
		my $t_pnl_diff_ = $trades_diff_words_ [ 1 ];
		my $t_pos_diff_ = $trades_diff_words_ [ 2 ];
		my $t_vol_diff_ = $trades_diff_words_ [ 3 ];

		push ( @abs_pnl_diff_ , abs ( $t_pnl_diff_ ) );
		push ( @abs_vol_diff_ , abs ( $t_vol_diff_ ) );

		push ( @pnl_diff_ , $t_pnl_diff_ );
	    }
	}
	my $pnl_diff_median_ = GetAverage ( \@pnl_diff_ ); # Given two equally incorrect sims , would prefer to have one that is pessimistic.
	my $signed_final_pnl_diff_ = $pnl_diff_ [ $#pnl_diff_ ];

	my $final_pnl_diff_ = abs ( $abs_pnl_diff_ [ $#abs_pnl_diff_ ] );

	my $abs_pnl_diff_mean_ = GetAverage ( \@abs_pnl_diff_ );
	my $abs_pnl_diff_median_ = GetMedianConst ( \@abs_pnl_diff_ );
	my $abs_pnl_diff_stdev_ = GetStdev ( \@abs_pnl_diff_ );

	my $abs_vol_diff_mean_ = GetAverage ( \@abs_vol_diff_ );
	my $abs_vol_diff_median_ = GetMedianConst ( \@abs_vol_diff_ );
	my $abs_vol_diff_stdev_ = GetStdev ( \@abs_vol_diff_ );

	my $avg_trades_ = ( $#abs_pnl_diff_ / 2.0 );
	printf ("Weights : %s %s Stats: Average abs diff : %s Average actual diff : %s \n", $sim_weights_list_1_[$j], $sim_weights_list_2_[$j], $abs_pnl_diff_mean_, $pnl_diff_median_);

    }
    $trading_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_yyyymmdd_ , 1 );


}

sub GetSimWeights
{
    open WEIGHTS_FILE, "< $simweights_file_" or PrintStacktraceAndDie ( "Could not open $simweights_file_\n" );
    my @sim_lines_ = <WEIGHTS_FILE>;
    close ( WEIGHTS_FILE );
    for ( my $i = 0 ; $i <= $#sim_lines_ ; $i ++ )
{
    my @weights_ = split (' ',$sim_lines_[$i]);
    if ($#weights_ >=1 )
{
    push (@sim_weights_list_1_, $weights_[0]);
    push (@sim_weights_list_2_, $weights_[1]);
}
}
return;
}

sub GetSimEndTimeStratFile
{
    my $retval_ = 2359;
    my ( $t_saved_strat_file_name_ ) = @_;
    if ( -e $t_saved_strat_file_name_ )
    {
        my @saved_strategy_info_words_ = split (' ', $t_saved_strat_file_name_);
        if ( $#saved_strategy_info_words_ >= 6 )
        {
            $retval_ = $saved_strategy_info_words_[6];
        }
    }
    $retval_ ;
}

