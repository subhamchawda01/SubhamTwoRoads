#!/usr/bin/perl

# \file scripts/exclude_non_prod_strats_from_modelling.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub GetProdStrats;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELLING_DIR = $HOME_DIR."/modelling";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/is_weird_sim_day_for_shortcode.pl"; # IsWeirdSimDayForShortcode

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetStdev , GetMedianConst

require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

# start
my $USAGE="$0 SHORTCODE TIMEPERIOD START_DATE END_DATE";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];

my $start_date_ = $ARGV [ 2 ];
my $end_date_ = $ARGV [ 3 ];

my @prod_strats_list_ = ( );
GetProdStrats ( );

RemoveNonProdStratsFromModelling ( );

exit ( 0 );

sub GetProdStrats
{
    my $RANK_HIST_QUERIES = $SCRIPTS_DIR."/rank_hist_queries.pl";

    my $exec_cmd_ = "$RANK_HIST_QUERIES $timeperiod_ $shortcode_ $start_date_ $end_date_ | awk '{ print \$3 }'";

    @prod_strats_list_ = `$exec_cmd_`;

    return;
}

sub RemoveNonProdStratsFromModelling
{
    my $STRATS_DIR = $MODELLING_DIR."/strats/".$shortcode_;

    my $exec_cmd_ = "find $STRATS_DIR";

    my @all_strats_list_ = `$exec_cmd_`;

    foreach my $strat_line_ ( @all_strats_list_ )
    {
	if ( ! FindItemFromVec ( basename ( $strat_line_ ) , @prod_strats_list_ ) )
	{
	    my $rm_cmd_ = "rm -f $strat_line_";
#	    print $rm_cmd_."\n"
	    `$rm_cmd_`;
	}
	else
	{
#	    print "Excluding ".basename ( $strat_line_ )." since found in prod_list\n";
	}
    }

    return;
}
