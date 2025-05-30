#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/is_product_holiday.pl";
require "$GENPERLLIB_DIR/is_date_holiday.pl";

if ( $#ARGV < 0 )
{
  print "USAGE: $0 Date [Shc=INVALID]\n";
  exit(1);
}

my $dt_ = $ARGV[0];
my $shc_ = "INVALID";
if ( $#ARGV >= 1 )
{
  $shc_ = $ARGV[1];
}

my $is_holiday_ = IsDateHoliday($dt_);

if ( IsDateHoliday($dt_) )
{
  print "1\n";
}
elsif ( $shc_ ne "INVALID" && IsProductHoliday($dt_, $shc_) ) 
{
  print "1\n";
}
else
{
  print "0\n";
}
