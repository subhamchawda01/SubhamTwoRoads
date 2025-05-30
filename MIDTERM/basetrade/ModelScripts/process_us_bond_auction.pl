#!/usr/bin/perl

# \file ModelScripts/process_us_bond_auction.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes output of basetrade_install/ModelScripts/us_bond_auction_prep.sh
# in file full_auction_history.txt
# and processes it to the sort of output that CommonTradeUtils/EconomicEventsManager
# expects.


use Time::ParseDate;
use DateTime;

sub TSYConvertUTCEpoch;

my $USAGE="$0 full_auction_filename\n";
if ( $#ARGV < 0 ) { die "$USAGE\n"; }

my $full_auction_filename_ = $ARGV[0];

my @full_data_ = ();

if ( -e $full_auction_filename_ )
{
# open full auction file
    open(FULL_AUCTION_FILEHANDLE, "< $full_auction_filename_") or die("Unable to open $full_auction_filename_");
    
# read file into an array
    @full_data_ = <FULL_AUCTION_FILEHANDLE>;
    chomp ( @full_data_ );
    
# close file 
    close(FULL_AUCTION_FILEHANDLE);
}

for ( my $i = 0 ; $i <= $#full_data_; $i ++ )
{
    my @this_words_ = split ( ' ', $full_data_[$i], 2 );
    if ( $#this_words_ == 1 )
    {

    	my $eco_zone_ = "USD";
	my ( $epoch_time_, $utc_time_string_ ) = TSYConvertUTCEpoch ( $this_words_[1] );
    	my $concat_event_name_ = $this_words_[0]."-Auction";
	my $event_severity_ = 3;
    	printf ( "%d %s %s %d %s\n", $epoch_time_, $eco_zone_, $concat_event_name_, $event_severity_, $utc_time_string_ );
    }
}

exit ( 0 );

sub TSYConvertUTCEpoch
{
    my $str_ = shift;
    my $epoch_time_ = parsedate( $str_ ); 

    my $dt = DateTime->from_epoch ( epoch => $epoch_time_ );

    $dt->set_time_zone ( 'UTC' );

    my $utc_time_string_ = sprintf ( "%s%02d%02d_%02d:%02d:%02d_UTC", $dt->year, $dt->month, $dt->day, $dt->hour, $dt->minute, $dt->second );

    $epoch_time_,$utc_time_string_ ;
}
