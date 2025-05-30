#!/usr/bin/perl

# \file ModelScripts/get_periodic_trend.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
# modelfile startdate enddate start_hhmm end_hhmm

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

my $TRADELOG_DIR="/spare/local/logs/tradelogs/";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $GENRSMWORKDIR=$SPARE_HOME."RSM/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "rkumar" )
{
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}
if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "ankit" || $USER eq "anshul" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

# start
my $USAGE="$0 shortcode date ";

if ( $#ARGV < 1 ) { print $USAGE."\n".$#ARGV."\n"; exit (0 ); }
my $model_filename_ = $HOME_DIR."/tmp_model";
my $shortcode_ = $ARGV[0];
my $tradingdate_ = $ARGV[1];
my $this_day_timed_data_filename_ = $HOME_DIR."/tmp_dout.kkp";
my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 6;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;
my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $datagen_start_hhmm_ = "UTC_0000";
my $datagen_end_hhmm_ = "UTC_2359";
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

open OUTMODEL, "> $model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $model_filename_ for writing\n" );
print OUTMODEL "MODELINIT DEPBASE ".$shortcode_." MktSizeWPrice MktSizeWPrice\n";
print OUTMODEL "MODELMATH LINEAR CHANGE\n";
print OUTMODEL "INDICATORSTART\n";
print OUTMODEL "INDICATOR 1.00 SimpleTrend ".$shortcode_." 300 MktSizeWPrice\n";
print OUTMODEL "INDICATOREND\n";
close OUTMODEL;

if ( !(SkipWeirdDate ( $tradingdate_ ) || NoDataDate ( $tradingdate_ ) || IsDateHoliday ( $tradingdate_ ) || ( ! ValidDate ( $tradingdate_ ) ) ))
{
    my $exec_cmd_="$LIVE_BIN_DIR/datagen $model_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";

    `$exec_cmd_`;
    my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
    `rm $datagen_logfile_`;
    if ( -e $this_day_timed_data_filename_ )
{
    open DFILE, "< $this_day_timed_data_filename_" or PrintStacktraceAndDie ( "Could not open this_day_timed_data_filename_ $this_day_timed_data_filename_ for reading\n" );
    my $current_start_window_ = 0;
    my $count_current_window_ = 0;
    my $current_mean_ = 0;
    while ( my $data_line_ = <DFILE> )
{
    chomp ($data_line_ );
    my @words_ = split (' ', $data_line_ );
    my $this_trend_ = abs ( $words_[ 4 ] );
    my $start_window_ = int( $words_[ 0 ]/1800000 );
    if ( $start_window_ == $current_start_window_ )
{
    $current_mean_ = ( $current_mean_ * $count_current_window_ + $this_trend_ )/ ($count_current_window_ + 1 );
    $count_current_window_ = $count_current_window_ + 1;
}
else
{
    if ( $count_current_window_ > 0 )
{
    my $hh_= int ( $start_window_/2 );
    my $mm_= ($start_window_ - $hh_*2 )*30;
    printf "%02d%02d %f\n",$hh_,$mm_,$current_mean_;
}
$current_mean_ = $this_trend_;
$count_current_window_ = 1;
$current_start_window_ = $start_window_;
}
}
}
}
`rm $model_filename_`;
