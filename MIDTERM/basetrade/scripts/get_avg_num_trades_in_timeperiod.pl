#!/usr/bin/perl
# \file scripts/get_avg_l1_events_in_timeperiod.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/array_ops.pl"; # GetAverage , GetMedianConst

require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $GET_NUM_TRADES_EXEC = $LIVE_BIN_DIR."/get_num_trades_on_day";
if ( ! -e $GET_NUM_TRADES_EXEC ) 
{ $GET_NUM_TRADES_EXEC = $BIN_DIR."/get_num_trades_on_day"; }
my $GET_UTC_HHMM_STR_EXEC = $BIN_DIR."/get_utc_hhmm_str";


my $USAGE="$0 shortcode start_date end_date start_time end_time";

if ( $#ARGV < 4 )
{ 
    printf "$USAGE\n"; 
    exit ( 0 ); 
}

my $shortcode_ = $ARGV [ 0 ];
my $start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 1 ] );
my $end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV [ 2 ] );
my $start_time_exec_cmd_ = $GET_UTC_HHMM_STR_EXEC." ".$ARGV [ 3 ];
my $end_time_exec_cmd_ = $GET_UTC_HHMM_STR_EXEC." ".$ARGV [ 4 ];
my $start_time_ = `$start_time_exec_cmd_`;
my $end_time_ = `$end_time_exec_cmd_`;

my @l1_ = ( );
my $exch_symbol_ = " ";

for ( my $date_ = $end_yyyymmdd_ ; $date_ >= $start_yyyymmdd_ ; )
{
    if ( SkipWeirdDate ( $date_ ) ||
	 IsDateHoliday ( $date_ ) ||
	 IsProductHoliday ( $date_ , $shortcode_ ) ||
	 NoDataDate ( $date_ ) )
    {
	$date_ = CalcPrevWorkingDateMult ( $date_, 1 );
	next;
    }

    if ( ( ! ValidDate ( $date_ ) ) ||
	 ( $date_ < $start_yyyymmdd_ ) )
    {
	last;
    }

    my $get_l1_events_exec_cmd_ = $GET_NUM_TRADES_EXEC." ".$shortcode_." ".$date_." ".$start_time_." ".$end_time_;

    my $get_l1_events_output_ = `$get_l1_events_exec_cmd_ 2>/dev/null`; chomp ( $get_l1_events_output_ );
    my @t_l1_output_words_ = split ( ' ' , $get_l1_events_output_ );
    if ( $#t_l1_output_words_ >= 2 )
    {
	my $t_l1_ = $t_l1_output_words_ [ $#t_l1_output_words_ ];
	if ( $t_l1_ > 0 )
	{
	    push ( @l1_ , $t_l1_ );
	    $exch_symbol_ = $t_l1_output_words_ [ 1 ];
	    print "$date_ $shortcode_ $t_l1_ \n";
	}
    }

    $date_ = CalcPrevWorkingDateMult ( $date_ , 1 );
}

my $t_mean_l1_ = GetAverage ( \@l1_ );
my $t_median_l1_ = GetMedianConst ( \@l1_ );

print $shortcode_." ".$exch_symbol_." ".$ARGV [ 1 ]." ".$ARGV [ 2 ]." ".$t_mean_l1_." ".$t_median_l1_."\n";

my $duration_seconds_ = ( ( $end_time_ - $start_time_ ) * 60 * 60 ) / 100;
print $shortcode_." ".$exch_symbol_." ".$ARGV [ 1 ]." ".$ARGV [ 2 ]." ".( $t_mean_l1_ / $duration_seconds_ )." ".( $t_median_l1_ / $duration_seconds_ )."\n";
