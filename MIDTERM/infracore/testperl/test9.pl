#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 
my $MODELSCRIPTS_DIR=$HOME_DIR."/infracore_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/infracore/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/infracore_install/bindebug"; # set to debug right now
my $SPARE_DIR="/spare/local/".$USER;
my $BASETRADEINFODIR="/spare/local/tradeinfo/";
my $BASEPORTFOLIOCONSTINFODIR=$BASETRADEINFODIR."PortfolioInfo/";
my $SPARE_PCF_DIR=$SPARE_DIR."/PCFDIR/";

my $YYYYMMDD=`date +%Y%m%d`; chomp ( $YYYYMMDD );

require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl";
require "$GENPERLLIB_DIR/calc_next_date.pl";

my $datagen_end_yyyymmdd_ = $YYYYMMDD;
my $max_days_at_a_time_ = 20;
my $datagen_start_yyyymmdd_ = CalcPrevDateMult ( $datagen_end_yyyymmdd_, $max_days_at_a_time_ ) ;

print "$datagen_start_yyyymmdd_\n";
