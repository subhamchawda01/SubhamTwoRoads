#!/usr/bin/perl

# \file ModelScripts/get_one_stdev_implied_vol_value_in_dollars.pl
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#      Suite 217, Level 2, Prestige Omega,
#      No 104, EPIP Zone, Whitefield,
#      Bangalore - 560066, India
#      +91 80 4060 0717
#

use strict;
use warnings;
use feature "switch"; # for given, when
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $REPO="cvquant_install";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/basetrade/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."/basetrade/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl";               # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays, GetDatesFromStartDate

# ~/cvquant_install/basetrade/bin/datagen ~/temp1 20170718 IST_915 IST_1530 2897 STATS 1000 0 0 0
my $usage_ = "script_name base_name end_date look_back start_time end_time\n";
if ( $#ARGV < 4 ) {
    print $usage_;
    exit(-1);
}
    
my $basename_ = $ARGV[0];
my $end_date_ = $ARGV[1];
my $look_back_ = $ARGV[2];
my $start_time_ = $ARGV[3];
my $end_time_ = $ARGV[4];

my $n2d_ = `/home/dvctrader/cvquant_install/basetrade/bin/get_numbers_to_dollars NSE_NIFTY_FUT0 $end_date_`;
chomp($n2d_);

my @dates_ = GetDatesFromNumDays("NSE_".$basename_."_FUT0", $end_date_, $look_back_, "INVALIDFILE");

my $vega_iv_ilist_file_ = "/tmp/vega_iv_ilist";
if ( -e $vega_iv_ilist_file_ ) { `rm -rf $vega_iv_ilist_file_` }

open VEGA_IV_ILIST_FILE, "> $vega_iv_ilist_file_ " or PrintStacktraceAndDie("could not open $vega_iv_ilist_file_ for writing\n");
printf VEGA_IV_ILIST_FILE "MODELINIT DEPBASE NSE_%s_FUT0 Midprice Midprice\n", $basename_;
print VEGA_IV_ILIST_FILE "MODELMATH LINEAR CHANGE\n";
print VEGA_IV_ILIST_FILE "INDICATORSTART\n";
printf VEGA_IV_ILIST_FILE "INDICATOR 1.00 OptionsGreek NSE_%s_C0_A 3 30 Midprice\n", $basename_;
printf VEGA_IV_ILIST_FILE "INDICATOR 1.00 ImpliedVolCalculator NSE_%s_C0_A Midprice\n", $basename_;
print VEGA_IV_ILIST_FILE "INDICATOREND\n";


my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 1;
# first line 4 th token * second line 6th token

foreach my $date_ (@dates_) {
    my $exec_ = "$BIN_DIR/datagen $vega_iv_ilist_file_ $date_ $start_time_ $end_time_  $unique_gsm_id_ STATS 1000 0 0 0 | awk '{if(vega==0) {vega=\$4} else if(iv_stdev==0) {iv_stdev=\$6}}END{print vega*iv_stdev*$n2d_}'";
    print $date_." ";
    print `$exec_`;
}
