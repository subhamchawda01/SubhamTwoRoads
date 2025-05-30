#!/usr/bin/perl

# \file ModelScripts/generate_timed_indicator_data.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes as input : indicator_list_filename, start_yyyymmdd, end_yyyymmdd, start_time_hhmm, end_time_hhmm [progid] [TIMED_DATA_DAILY_DIR]
# For each of these days, it generates timed data ( just print of all the indicators )

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

if ( $USER eq "rkumar" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}
if ( $USER eq "sghosh" || $USER eq "ravi" )
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
require "$GENPERLLIB_DIR/check_ilist_data.pl"; #CheckIndicatorData

my $USAGE="$0 indicatorlistfilename datagen_start_yyyymmdd datagen_end_yyyymmdd starthhmm endhhmm msecs_timeout_ l1events_timeout_ num_trades_timeout_ outfile_";

if ( $#ARGV < 8 ) { print $USAGE."\n"; exit ( 0 ); }
my $indicator_list_filename_ = $ARGV[0];
my $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[1] );
my $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $datagen_start_hhmm_ = $ARGV[3];
my $datagen_end_hhmm_ = $ARGV[4];
my $datagen_msecs_timeout_ = $ARGV[5];
my $datagen_l1events_timeout_ = $ARGV[6];
my $datagen_num_trades_timeout_ = $ARGV[7];
my $to_print_on_economic_times_ = 1;
my $this_timed_data_filename_ = $ARGV[8];

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

my $indicator_list_filename_base_ = basename ( $indicator_list_filename_ ); chomp ($indicator_list_filename_base_);

`rm -f $this_timed_data_filename_`;

if ( ValidDate ( $datagen_start_yyyymmdd_ ) &&
     ValidDate ( $datagen_end_yyyymmdd_ ) ) 
{

    my $tradingdate_ = $datagen_end_yyyymmdd_;
    my $max_days_at_a_time_ = 2000; # just a hard limit of studies can be run at most 90 days at a time
    for ( my $i = 0 ; $i < $max_days_at_a_time_ ; $i ++ ) 
    {
	if ( SkipWeirdDate ( $tradingdate_ ) ||
	     NoDataDate ( $tradingdate_ ) ||
	     IsDateHoliday ( $tradingdate_ ) ||
             CheckIndicatorData ( $tradingdate_ , $indicator_list_filename_ ) )
	{
	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	    next;
	}

	if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	     ( $tradingdate_ < $datagen_start_yyyymmdd_ ) )
	{
	    last;
	}
	else 
	{
	    # name chosen isn't unique to this invokation, so that we can reuse this file if the next day the same model file is chosen
	    my $this_day_timed_data_filename_ = $this_timed_data_filename_.".".$tradingdate_ ; 

	    if ( ! ( -e $this_day_timed_data_filename_ ) ) 
	    { # if there is no such file then data for this day needs to be generated
		my $exec_cmd="$LIVE_BIN_DIR/datagen $indicator_list_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
		`$exec_cmd`;
#		print "$exec_cmd\n";
		if ( -e $this_day_timed_data_filename_ )
		{
#		    system ( "wc -l $this_day_timed_data_filename_" );
		    `cat $this_day_timed_data_filename_ >> $this_timed_data_filename_`;
#		    system ( "wc -l $this_timed_data_filename_" );
		    `rm -f $this_day_timed_data_filename_`; 
		}
	    }

	    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	}
    }
}

print $this_timed_data_filename_;
