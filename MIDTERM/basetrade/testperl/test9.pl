#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $YYYYMMDD=`date +%Y%m%d`; chomp ( $YYYYMMDD );

require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl";
require "$GENPERLLIB_DIR/calc_next_date.pl";

my $datagen_end_yyyymmdd_ = $YYYYMMDD;
my $max_days_at_a_time_ = 2000;
my $datagen_start_yyyymmdd_ = CalcPrevDateMult ( $datagen_end_yyyymmdd_, $max_days_at_a_time_ ) ;

print "$datagen_start_yyyymmdd_\n";
