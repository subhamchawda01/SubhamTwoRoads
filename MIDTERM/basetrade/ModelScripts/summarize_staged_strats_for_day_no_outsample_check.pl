#!/usr/bin/perl

# \file ModelScripts/summarize_strats_for_day_no_outsample_check.pl
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

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "ankit" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/staged_strats"; # this directory is used to store the chosen strategy files
my $GLOBALRESULTSDBDIR=$HOME_DIR."/ec2_staged_globalresults"; # Changed for DVCTrader ... so that it does not clash with others

my $hostname_s_ = `hostname -s`; chomp ( $hostname_s_ );
if ( ! ( ( $hostname_s_ eq "sdv-ny4-srv11" && $USER eq "dvctrader" ) || 
	 ( $hostname_s_ eq "sdv-crt-srv11" && $USER eq "dvctrader" ) ) )
{
    $GLOBALRESULTSDBDIR="/NAS1/ec2_globalresults"; # on non ny4srv11 and crtsrv11 ... look at NAS
}

require "$GENPERLLIB_DIR/get_choose_strats_config.pl"; # GetChooseStratsConfig
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_excluding_sets.pl"; # MakeStratVecFromDirInTpExcludingSets
require "$GENPERLLIB_DIR/get_insample_date.pl"; # GetInsampleDate
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir

# start 
my $USAGE="$0 shortcode timeperiod [TILL_DATE=TODAY-1] [NUM_PAST_DAYS=14] [skip_dates_file_=INVALIDFILE] [time_period_is_folder_name_=0] [use_max_loss_=0]";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $timeperiod_ = $ARGV[1];

my $trading_end_yyyymmdd_ = 20111201;
{ # only for scope
    my $enddatestr_ = "TODAY-1";
    if ( $#ARGV >= 2 ) { $enddatestr_ = $ARGV[2]; }
    $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $enddatestr_,$shortcode_ );
}

print $trading_end_yyyymmdd_."\n";

my $num_days_past_ = 14;
if ( $#ARGV >= 3 ) { $num_days_past_ = $ARGV[3]; }

my $skip_dates_file_ = "INVALIDFILE";
if ( $#ARGV >= 4 ) { $skip_dates_file_ = $ARGV[4]; }

#fi 1, then time-period is actually folder name - used while pruning
my $folder_name_ = 0;
if ( $#ARGV >= 5) { $folder_name_ = int($ARGV[5]); }

my $use_max_loss_ = 0;
if ( $#ARGV >= 6) { $use_max_loss_ = int($ARGV[6]); }


my @exclude_tp_dirs_ = ();

# compute the startdate by taking enddate and skipping to previous $num_days_past_ number of working days 
my $trading_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_end_yyyymmdd_, $num_days_past_ , $shortcode_ );
print $trading_start_yyyymmdd_."\n";
if ( $skip_dates_file_ eq "INVALIDFILE" )
{
}
else
{
	open FH, "<$skip_dates_file_" or die "can't open '$skip_dates_file_': $!";
	my @skip_dates_vector_ = <FH>;
	close FH;
	@skip_dates_vector_ = sort {$a <=> $b}  @skip_dates_vector_ ;
	
	for ( my $i = $#skip_dates_vector_; $i >= 0; $i = $i -1 )
	{
		my $dt = $skip_dates_vector_[$i];
		if ( ( $dt >= $trading_start_yyyymmdd_ ) && ( $dt <= $trading_end_yyyymmdd_ ) )
		{
			$trading_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $trading_start_yyyymmdd_, 1);
		}
	}
}

my @all_strats_in_dir_ = ();
if($folder_name_ > 0)
{
    @all_strats_in_dir_ = MakeStratVecFromDir("$MODELING_STRATS_DIR/$shortcode_/$timeperiod_");
}
else
{
    @all_strats_in_dir_ = MakeStratVecFromDirInTpExcludingSets ( $MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_, @exclude_tp_dirs_ );
}
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
#    my $last_insample_date_ = GetInsampleDate ( $strat_basename_ );
#    if ( $last_insample_date_ < $trading_start_yyyymmdd_ ) #ignoring outsample check
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

{
    my $cstempfile_ = GetCSTempFileName ( $HOME_DIR."/cstemp" );
    open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );
    foreach my $t_eligible_strat_file_ ( @outsample_strat_basenames_ )
    {
	print CSTF "$t_eligible_strat_file_\n";
    }
    close CSTF;
    
    my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $cstempfile_ $GLOBALRESULTSDBDIR $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ $skip_dates_file_ 5 $use_max_loss_";
    print $exec_cmd."\n";
    my @ssr_output_ = `$exec_cmd`;
    chomp ( @ssr_output_ );

    printf ( "STRATEGYFILEBASE  _fn_  pnl  pnl_stdev  volume  pnl_shrp  pnl_cons  pnl_median  ttc  norm_ttc avg_min_adj PPT  S B A I  MAXDD  dd_adj_pnl  avg_abs_pos  avg_msg_count gpr pnl_by_ml nf ol \n" );
    print join ( "\n", @ssr_output_ )."\n\n";
    `rm -f $cstempfile_`;
}

exit ( 0 );

