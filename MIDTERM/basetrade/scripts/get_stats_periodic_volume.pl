#!/usr/bin/perl
# \file scripts/get_stats_periodic_volume.pl
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

my $VOLUME_ON_DAY_EXEC=$BIN_DIR."/get_periodic_volume_on_day";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/no_data_date.pl"; #NoDataDateForShortcode
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/array_ops.pl"; 

my $USAGE="$0 shortcode end_date days_to_look_behind window_in_secs";

if ( $#ARGV < 3 ) 
{ 
    printf "$USAGE\n"; 
    exit (0); 
}

my $shortcode_ = $ARGV[0];
my $end_date_ = $ARGV[1];
my $days_to_look_behind_ = $ARGV[2];
my $window_in_secs_ = $ARGV[3];


my $current_yyyymmdd_ = $end_date_;
my @volumes_ = ();

for ( my $days_ = $days_to_look_behind_; $days_ != 0; $days_-- ) 
{
    if ( ! ValidDate ( $current_yyyymmdd_ ) )
	{ # Should not be here.
	    printf ("Invalid date : $current_yyyymmdd_\n") ;
	    last ;
	}

    if ( SkipWeirdDate ( $current_yyyymmdd_ ) ||
	 IsDateHoliday ( $current_yyyymmdd_ ) || 
	 NoDataDateForShortcode ( $current_yyyymmdd_, $shortcode_ ) ||
	 IsProductHoliday ( $current_yyyymmdd_, $shortcode_ ) ) 
    {
	$current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_, 1 ) ;
	$days_++ ;
	next ;
    }

#~/basetrade_install/bin/get_periodic_volume_on_day BR_DOL_0 20140417 1800

    my $volume_exec_cmd_ = $VOLUME_ON_DAY_EXEC." $shortcode_ $current_yyyymmdd_ $window_in_secs_";
    #print $volume_exec_cmd_."\n";
    my $volume_string_ = `$volume_exec_cmd_`; chomp ($volume_string_);
	
    my @volume_string_output_ = split (' ', $volume_string_);

    if ( $#volume_string_output_ < 1 )
    {
	#$date_to_volumes_{$current_yyyymmdd_} = "-1";
	$current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
	next;
    }

    if ( $volume_string_output_[1] > 0 )
    {
	my $volume_ = $volume_string_output_[1]; 
	chomp ($volume_);
	push ( @volumes_ , $volume_ ) ;
    }
    $current_yyyymmdd_ = CalcPrevWorkingDateMult ($current_yyyymmdd_, 1);
}


my $avg_vol_ = GetAverage ( \@volumes_ ) ;
my $stdev_vol_ = GetStdev ( \@volumes_ ) ;
my $mean_high_quartile_ = GetMeanHighestQuartile ( \@volumes_ ) ;
my $mean_low_quartile_ = GetMeanLowestQuartile ( \@volumes_ ) ;


print int ( $avg_vol_ ) ." ".int ( $stdev_vol_ )." ".int ( $mean_low_quartile_ )." ".int ( $mean_high_quartile_)."\n" ;

