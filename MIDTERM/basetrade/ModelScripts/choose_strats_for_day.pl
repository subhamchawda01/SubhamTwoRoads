#!/usr/bin/perl

# \file ModelScripts/choose_strats_for_day.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
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
# Where the config file has lines like
# startdate
# enddate
# vector < stratfiles_specs_andstrat_id >
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $GLOBALRESULTSDBDIR=$HOME_DIR."/ec2_globalresults"; # Changed for DVCTrader ... so that it does not clash with others

my $hostname_s_ = `hostname -s`; chomp ( $hostname_s_ );
if ( ! ( ( $hostname_s_ eq "sdv-ny4-srv11" ) || 
	 ( $hostname_s_ eq "sdv-crt-srv11" ) ) )
{
    $GLOBALRESULTSDBDIR="/NAS1/ec2_globalresults"; # on non ny4srv11 and crtsrv11 ... look at NAS
}

require "$GENPERLLIB_DIR/get_choose_strats_config.pl"; # GetChooseStratsConfig
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName

# start 
my $USAGE="$0 shortcode timeperiod [TILL_DATE=TODAY-1] [NUM_PAST_DAYS=from_file] [MAX_TTC=from_file] [MIN_PPT=from_file] ";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $timeperiod_ = $ARGV[1];

my $show_all_ = 1;
my $no_count_ = 5;

my $trading_end_yyyymmdd_ = 20111201;
{ # only for scope
    my $enddatestr_ = "TODAY-1";
    if ( $#ARGV >= 2 ) { $enddatestr_ = $ARGV[2]; }
    $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $enddatestr_ );
}

my ( $stratid_start_, $stratid_end_, $num_days_past_, $min_pnl_per_contract_, $min_volume_, $max_ttc_, $max_num_to_choose_, $min_num_mid_mid_, $min_num_mid_mkt_, $min_num_mkt_mkt_, @exclude_tp_dirs_ ) = GetChooseStratsConfig ( $shortcode_, $timeperiod_ );
{
    if ( $#ARGV >= 3 ) { $num_days_past_ = $ARGV[3]; }
    if ( $#ARGV >= 4 ) { $max_ttc_ = $ARGV[4]; }
    if ( $#ARGV >= 5 ) { $min_pnl_per_contract_ = $ARGV[5]; }
}

if ( $stratid_start_ < 0 )
{ print "No Config found for $shortcode_ $timeperiod_. \n"; exit ( 0 ); }

# compute the startdate by taking enddate and skipping to previous $num_days_past_ number of working days 
my $trading_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_end_yyyymmdd_, $num_days_past_ ) ;

my @all_strats_in_dir_ = MakeStratVecFromDirInTpExcludingSets ( $MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_, @exclude_tp_dirs_ );
#print join ( "\n", @all_strats_in_dir_ );

my @outsample_strats_ = ();
my @outsample_strat_basenames_ = ();

foreach my $full_strat_filename_ ( @all_strats_in_dir_ )
{
# for each file 
# get basename($file)
# find the last date of insample
# if file is outsample in the entire period
# add the file to the list, and basename to basename list
# if you detect a duplicate entry then do not add the next one and print

    my $strat_basename_ = basename ( $full_strat_filename_ );
    my $last_insample_date_ = GetInsampleDate ( $strat_basename_ );
    if ( $last_insample_date_ < $trading_start_yyyymmdd_ )
    {
	if ( ! ( FindItemFromVec ( $strat_basename_, @outsample_strat_basenames_ ) ) )
	{ # not a duplicate entry
	    push ( @outsample_strats_, $full_strat_filename_ );
	    push ( @outsample_strat_basenames_, $strat_basename_ );
	}
	else
	{
	    print STDERR "Ignoring $full_strat_filename_ since it was a duplicate\n";
	}
    }
}

my @chosen_strats_ = ();

print "# Y/N  STRATEGYFILEBASE  _fn_  avg_pnl  pnl-std  volume  pnl-sharpe  pnl-cons  pnl-med  ttc  Avg[daily_pnl_/stdev_closed_trade_pnls_]  Avg_Min-MAX-PNL  PPT  S  B  A  I  MAX_DD\n";

# if min_num_mkt_mkt_ or others like that are non 0 then make a pair of list-files for them
#
# for each pair of list-files that we need to find toppers in
# call summarize_strategy_resutls with a file with the list of strategyfile-basenames and 
if ( $min_num_mid_mid_ > 0 )
{
# TODO
}
if ( $min_num_mid_mkt_ > 0 )
{
# TODO
}
if ( $min_num_mkt_mkt_ > 0 )
{
# TODO
}

if ( $#chosen_strats_ + 1 < $max_num_to_choose_ )
{
    my $cstempfile_ = GetCSTempFileName ( $HOME_DIR."/cstemp" );
    open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
    foreach my $t_eligible_strat_file_ ( @outsample_strat_basenames_ )
    {
	print CSTF "$t_eligible_strat_file_\n";
    }
    close CSTF;
    
    my @ssr_output_ = `$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $cstempfile_ $GLOBALRESULTSDBDIR $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ INVALIDFILE 5`;
    chomp ( @ssr_output_ );
    `rm -f $cstempfile_`;
    
    my $num_chosen_here_ = 0;
    for my $ssr_line_ ( @ssr_output_ )
    {
	my @ssr_words_ = split ( ' ', $ssr_line_ );
	if ( $#ssr_words_ >= 16 )
	{
# STRATEGYFILEBASE _fn_ pnl pnl_stdev volume pnl_shrp cons_pnl pnl_median ttc pnl_zs avg_min_max_pnl ppt S B A DD Pnl/DD\n" );
	    my $t_strategy_filename_base_ = $ssr_words_[1];
	    my $t_pnl_average_ = $ssr_words_[2];
	    my $t_pnl_stdev_ = $ssr_words_[3];
	    my $t_volume_average_ = $ssr_words_[4];
	    my $t_pnl_sharpe_ = $ssr_words_[5];
	    my $t_pnl_conservative_average_ = $ssr_words_[6];
	    my $t_pnl_median_average_ = $ssr_words_[7];
	    my $t_median_time_to_close_trades_ = $ssr_words_[8];
	    my $t_average_pnl_zscore_ = $ssr_words_[9];
	    my $t_avg_min_adjusted_pnl_ = $ssr_words_[10];
	    my $t_pnl_per_contract_ = $ssr_words_[11];
	    my $t_supporting_order_filled_percent_ = $ssr_words_[12];
	    my $t_best_level_order_filled_percent_ = $ssr_words_[13];
	    my $t_aggressive_order_filled_percent_ = $ssr_words_[14];
	    my $t_improve_order_filled_percent_ = $ssr_words_[15];
	    my $t_drawdown_ = $ssr_words_[16];
	    my $t_dd_adj_pnl_ = $ssr_words_[17];
	    
	    if ( ( $t_pnl_per_contract_ > $min_pnl_per_contract_ ) &&
		 ( $t_volume_average_ > $min_volume_ ) && 
		 ( $t_median_time_to_close_trades_ < $max_ttc_ ) )
	    {
		print "Y $ssr_line_\n";

		if ( $#chosen_strats_ + 1 < $max_num_to_choose_ )
		{
		    push ( @chosen_strats_, $t_strategy_filename_base_ ) ;
		    $num_chosen_here_ ++;
		}

#		if ( $#chosen_strats_ + 1 == $max_num_to_choose_ 
#		     || $num_chosen_here_ >= $max_num_to_choose_ )
#		{
#		    last;
#		}
	    }
	    else
	    {
		if ( ( $show_all_ == 1 ) &&
		     ( $t_median_time_to_close_trades_ < $max_ttc_ ) &&
		     ( $no_count_ > 0 ) )
		{
		    printf "N %d %d %d %s\n", ( $t_pnl_per_contract_ > $min_pnl_per_contract_ ),
		    ( $t_volume_average_ > $min_volume_ ),
		    ( $t_median_time_to_close_trades_ < $max_ttc_ ), $ssr_line_;
		    $no_count_ --;
		}
	    }
	}
    }
}

for my $t_chosen_strat_basename_ ( @chosen_strats_ )
{
# for strategies in @chosen_strats_ find the full paths
# print
# InstallStrategyProduction
# SetupCrontab
}

# At the end ssh to each production and install crontab as dvctrader

exit ( 0 );

