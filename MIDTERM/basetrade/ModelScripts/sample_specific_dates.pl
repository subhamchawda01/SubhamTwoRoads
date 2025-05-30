#!/usr/bin/perl
# \file sample_specific_dates.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
use strict;
use warnings;
use List::Util qw/max min/; # for max

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";

require "$GENPERLLIB_DIR/sample_data_utils.pl";
my $LIST_OF_TRADING_DATES_SCRIPT = $SCRIPTS_DIR."/get_list_of_dates_for_shortcode.pl";

my $this_shc_;
my $tdate_;
my $ndays_;
my $factor_;
my $factor_aux_;
my $start_hhmm_;
my $end_hhmm_;
my @dates_vec_;
my $lbound_;
my $ubound_;
my @the_dates_;

if ( $#ARGV <= 5 )
{
    print $0." shc date ndays_ sample sample_aux start_hhmm end_hhmm lbound ubound\n";
    exit(0);
} else {
    $this_shc_ = $ARGV[0];
    $tdate_ = $ARGV[1];
    $ndays_ = $ARGV[2];
    $factor_ = $ARGV[3];
    $factor_aux_ = $ARGV[4];
    $start_hhmm_ = $ARGV[5];
    $end_hhmm_ = $ARGV[6];
    $lbound_ = $ARGV[7];
    $ubound_ = $ARGV[8];
}

my $exec_cmd_ = "$LIST_OF_TRADING_DATES_SCRIPT $this_shc_ $tdate_ $ndays_";
my $exec_cmd_output_ = `$exec_cmd_`; chomp ( $exec_cmd_output_ );
my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ );

foreach my $tradingdate_ ( @exec_cmd_output_words_ )
{
   push ( @dates_vec_ , $tradingdate_ );
}

GetFilteredDaysInPercRange ( $this_shc_, \@dates_vec_, $lbound_, $ubound_, $factor_, $factor_aux_, \@the_dates_, $start_hhmm_, $end_hhmm_);

foreach my $tradingdate_ ( @the_dates_ )
{
    print $tradingdate_."\n";
}
