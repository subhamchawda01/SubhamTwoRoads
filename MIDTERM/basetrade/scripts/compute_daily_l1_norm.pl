#!/usr/bin/perl

# \file ModelScripts/compute_daily_l1_norm.pl
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
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

# start
my $USAGE="$0 shortcode date ";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit (0 ); }
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
my $model_filename_ = $HOME_DIR."/tmp_model_".$unique_gsm_id_;
my $out_file_prefix_ = "/spare/local/L1Norms/".$tradingdate_;
my $out_file_ = $out_file_prefix_."/".$shortcode_."_l1norm_value";
if ( -e $out_file_ ) { exit(0 ); }
my $out_file_handle_ = FileHandle->new;




open OUTMODEL, "> $model_filename_" or PrintStacktraceAndDie ( "Could not open output_model_filename_ $model_filename_ for writing\n" );
print OUTMODEL "MODELINIT DEPBASE ".$shortcode_." MktSizeWPrice MktSizeWPrice\n";
print OUTMODEL "MODELMATH LINEAR CHANGE\n";
print OUTMODEL "INDICATORSTART\n";
print OUTMODEL "INDICATOR 1.00 SimpleTrend ".$shortcode_." 900 MktSizeWPrice\n";
print OUTMODEL "INDICATOREND\n";
close OUTMODEL;

if ( !(SkipWeirdDate ( $tradingdate_ ) || NoDataDate ( $tradingdate_ ) || IsDateHoliday ( $tradingdate_ ) || ( ! ValidDate ( $tradingdate_ ) ) || IsProductHoliday ( $tradingdate_ , $shortcode_ )))
{
    my $exec_cmd_="$MODELSCRIPTS_DIR/get_stdev_model.pl $model_filename_ $tradingdate_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ | head -n1";

    my $res_ = `$exec_cmd_`;
    chomp ($res_);
    my @words_ = split( ' ', $res_ );
    my $l1_norm_ = $words_[ 1 ];
    my $stdev_ = $words_[ 0 ];
    if ( $l1_norm_ > 0 && $stdev_ > 0 )
    {
      if ( ! ( -d $out_file_prefix_ ) ) { `mkdir -p $out_file_prefix_`; }
      $out_file_handle_ -> open ( "> $out_file_ " );
      print $out_file_handle_ "$l1_norm_ $stdev_\n";
      $out_file_handle_ ->close;
    }
}
`rm $model_filename_`;
