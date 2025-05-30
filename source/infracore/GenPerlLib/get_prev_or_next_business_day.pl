#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;


my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";
my $RECON_DIR=$ENV{"HOME"}."/".$REPO."/recon";

require "$GENPERLLIB_DIR/calc_next_business_day.pl"; #
require "$GENPERLLIB_DIR/calc_prev_business_day.pl"; #

my $USAGE="$0 0/1 (prev/next) YYYYMMDD";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $mode=shift;
my $YYYYMMDD=shift;
my $date=0;

if($mode == 1){
    $date= CalcNextBusinessDay ($YYYYMMDD);
}

if($mode == 0){
    $date= CalcPrevBusinessDay ($YYYYMMDD);
}

print $date;

1;