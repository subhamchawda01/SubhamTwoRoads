#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $SPARE_DIR="/spare/local/".$USER;
my $SPARE_PCF_DIR=$SPARE_DIR."/PCFDIR/";

my $YYYYMMD=`date +%Y%m%d`; chomp ( $YYYYMMD );

require "$GENPERLLIB_DIR/valid_date.pl";
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_next_date.pl";
require "$GENPERLLIB_DIR/get_kth_word.pl";
require "$GENPERLLIB_DIR/get_stdev_file.pl";

my $this_timed_data_filename_ = $SPARE_PCF_DIR."/timed_data_pcf_pcf_ilf_test_portfolio_constituent_input.txt_20110521_20110609_630_1000";

my @stdev_values_ = GetStdevOfNonZeroValues ( $this_timed_data_filename_, 4 );

printf ( STDOUT "%s\n", join ( ' ', @stdev_values_ ) );
