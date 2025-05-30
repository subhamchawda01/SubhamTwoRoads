#!/usr/bin/perl

# \file testperl/test20.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate

my $datagen_start_yyyymmdd_ = 20110628;
my $datagen_end_yyyymmdd_ = 20110804;

my $tradingdate_ = $datagen_start_yyyymmdd_;
my $max_days_at_a_time_ = 2000;
for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
{
    if ( SkipWeirdDate ( $tradingdate_ ) ||
	 IsDateHoliday ( $tradingdate_ ) )
    {
	$tradingdate_ = CalcNextDate ( $tradingdate_ );
	next;
    }

    
    if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	 ( $tradingdate_ > $datagen_end_yyyymmdd_ ) )
    {
	last;
    }
    else 
    {   # for this trading date generate the reg_data_file
	print "$tradingdate_\n";
    }
    $tradingdate_ = CalcNextDate ( $tradingdate_ );
}
