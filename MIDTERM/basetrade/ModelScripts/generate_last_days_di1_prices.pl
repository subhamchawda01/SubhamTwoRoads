#!/usr/bin/perl

# \file ModelScripts/generate_last_days_di1_prices.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# output_model_filename_  indicator_list_filename_  regression_output_filename_

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 date start_time end_time filename  < list_of_shortcodes ( with spaces ) > ";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $date_ = $ARGV[0];
my $start_time_ = $ARGV[1];
my $end_time_ = $ARGV[2];
my $filename_ = $ARGV[3];

my @shortcodes_ = ( );

open OUTFILE, "> $filename_" or PrintStacktraceAndDie ( "Could not open filename_ $filename_ for writing\n" );

for ( my $i=4; $i <= $#ARGV; $i++ )
{
   push ( @shortcodes_ , $ARGV[$i] );
   my $shortcode_px_string_ = " MidPrice $ARGV[$i]";
   my $exec_cmd_ = "~/basetrade_install/bin/mult_price_printer $date_ $start_time_ $end_time_ $shortcode_px_string_ 2>/dev/null | head -n1";
   my $pxs_line_ = `$exec_cmd_`;
   my @px_words_ = split ' ', $pxs_line_; 
   if ( $#px_words_ >= 0 )
   {
       print OUTFILE $ARGV[$i]." ".$px_words_[1]."\n";
   }
}
close OUTFILE;

