#!/usr/bin/perl

# \file scripts/call_is_holiday.pl

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

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday

# start 
my $USAGE="$0 date shortcode";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $datagen_date_ = $ARGV[0];
my $shortcode_ = $ARGV[1];

if ( IsDateHoliday ( $datagen_date_ ) || ( ( $shortcode_ ) && ( IsProductHoliday ( $datagen_date_, $shortcode_ ) ) ) )
{
    print "HOLIDAY\n";
}
else
{
    print "WORKDAY\n";
}
