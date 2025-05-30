#!/usr/bin/perl

# \file ModelScripts/get_average_event_count_per_second.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# This script takes :
# TIMED_DATA_INPUT_FILENAME

use strict;
use warnings;

# start 
my $USAGE="$0  R_TIMED_DATA_INPUT_FILENAME  MIN_STDEV_MULTIPLIER  MAX_STDEV_MULTIPLIER  RW_REG_DATA_OUTPUT_FILENAME";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $r_timed_data_input_filename_ = $ARGV[0];

if ( ( -s $r_timed_data_input_filename_ ) > 2 )
{
    my $ms1_ = 0;
    my $ev1_ = 0;
    my $line1_ = `head -1 $r_timed_data_input_filename_`; chomp ( $line1_ );
    my @words_ = split ( ' ', $line1_ );
    if ( $#words_ > 1 )
    {
	$ms1_ = $words_[0];
	$ev1_ = $words_[1];
    }

    my $msN_ = 0;
    my $evN_ = 0;
    my $lineN_ = `tail -1 $r_timed_data_input_filename_`; chomp ( $lineN_ );
    @words_ = split ( ' ', $lineN_ );
    if ( $#words_ > 1 )
    {
	$msN_ = $words_[0];
	$evN_ = $words_[1];
    }

    my $events_per_second_ = ( $evN_ - $ev1_ ) / ( ( $msN_ - $ms1_ ) / 1000 ) ;
    printf ( "%.4f\n", $events_per_second_ );
}
