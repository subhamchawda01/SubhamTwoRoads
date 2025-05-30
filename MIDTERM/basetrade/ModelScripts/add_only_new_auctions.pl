#!/usr/bin/perl

# \file ModelScripts/add_only_new_auctions.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes output of $HOME/$REPO"_install/ModelScripts/us_bond_auction_prep.sh"
# in file full_auction_history.txt
# and processes it to the sort of output that CommonTradeUtils/EconomicEventsManager
# expects.

my $USAGE="$0 new_auction_file full_auction_file";
if ( $#ARGV < 1 ) { die "$USAGE\n"; }

my $new_auction_filename_ = $ARGV[0];
my $full_auction_filename_ = $ARGV[1];

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

my $to_change_ = 0;
my @new_full_data_lines_ = ();

# open full auction file
open(NEW_AUCTION_FILEHANDLE, "< $new_auction_filename_") or die("Unable to open $new_auction_filename_");
while ( my $new_line_ = <NEW_AUCTION_FILEHANDLE> )
{
    chomp ( $new_line_ );
    my $found_ = 0;
    for ( my $i = 0 ; $i <= $#full_data_; $i ++ )
    {
	if ( $full_data_[$i] =~ $new_line_ )
	{ # $new_line_ is a substring of $full_data_[$i]
	    # allowing for time to be in $full_data_[$i], 
            # but not in $new_line_
	    $found_ = 1;
	    last;
	}
    }
    if ( $found_ == 0 )
    {
	$new_line_ = $new_line_." 13:00 NewYork";
	push ( @new_full_data_lines_, $new_line_ );
	$to_change_ = 1;
    }
}
# close file 
close(NEW_AUCTION_FILEHANDLE);

if ( $to_change_ == 1 )
{
    open ( FULL_AUCTION_FILEHANDLE, ">> $full_auction_filename_") or die("Unable to w_open $full_auction_filename_");
    for ( my $i = 0 ; $i <= $#new_full_data_lines_; $i ++ )
    {
	printf "%s\n", $new_full_data_lines_[$i];
	printf FULL_AUCTION_FILEHANDLE "%s\n", $new_full_data_lines_[$i];
    }
# close file 
    close(FULL_AUCTION_FILEHANDLE);
}
