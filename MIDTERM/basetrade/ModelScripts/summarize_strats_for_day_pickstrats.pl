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
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to store the chosen strategy files
my $GLOBALRESULTSDBDIR=$HOME_DIR."/ec2_globalresults"; # Changed for DVCTrader ... so that it does not clash with others

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
my $USAGE="$0 shortcode strat_list_file_ DATES_FILE [skip_dates_file_=INVALIDFILE] [time_period_is_folder_name_=0] [use_max_loss_=0]";
if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $strat_list_file_ = $ARGV[1];
my $dates_file_ = $ARGV[2];

my $skip_dates_file_ = "INVALIDFILE";
if ( $#ARGV >= 3 ) { $skip_dates_file_ = $ARGV[3]; }

#fi 1, then time-period is actually folder name - used while pruning
my $folder_name_ = 0;
if ( $#ARGV >= 4) { $folder_name_ = int($ARGV[4]); }

my $use_max_loss_ = 0;
if ( $#ARGV >= 5) { $use_max_loss_ = int($ARGV[5]); }

open FILE, "<$dates_file_" or die "can't open '$dates_file_': $!";
my @dates_lines_vector_ = <FILE>;
chomp ( @dates_lines_vector_ );
close FILE;

my @dates_vector_ = ( );
foreach my $d_line_ ( @dates_lines_vector_ ) {
  my @d_tokens_ = split(" ", $d_line_);
  push ( @dates_vector_, $d_tokens_[0] );
}


@dates_vector_ = sort {$a <=> $b} @dates_vector_;
if ($#dates_vector_ == -1) {
    print "dates file empty!\n";
    exit(0);
}
my $start_date_ = $dates_vector_ [ 0 ];
my $end_date_ = $dates_vector_ [ $#dates_vector_ ];
my $exec_cmd="$LIVE_BIN_DIR/summarize_strategy_results $shortcode_ $strat_list_file_ $GLOBALRESULTSDBDIR $start_date_ $end_date_ $skip_dates_file_ 5 $use_max_loss_ $dates_file_";

my @ssr_output_ = `$exec_cmd`;
chomp ( @ssr_output_ );

printf ( "STRATEGYFILEBASE  _fn_  pnl  pnl_stdev  volume  pnl_shrp  pnl_cons  pnl_median  ttc  norm_ttc avg_min_adj PPT  S B A I  MAXDD  dd_adj_pnl  avg_abs_pos  avg_msg_count gpr pnl_by_ml nf ol \n" );
print join ( "\n", @ssr_output_ )."\n\n";

exit ( 0 );
