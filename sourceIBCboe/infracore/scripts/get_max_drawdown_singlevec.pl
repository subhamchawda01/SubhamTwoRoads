#!/usr/bin/perl

# \file scripts/get_max_drawdown_singlevec.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 162, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#

use warnings;
use strict;
use List::Util qw/max min/; # for max

my $max_drawdown_ = 0;
my $max_pnl_ = 0;
my $current_pnl_ = 0;
while ( my $inline_ = <STDIN> ) 
{
    chomp ( $inline_ );
    my @words_ = split ( ' ', $inline_ );
    if ( $#words_ >= 0 )
    {
	$current_pnl_ += $words_[0];
	$max_pnl_ = max ( $max_pnl_, $current_pnl_ );
	my $current_drawdown_ = $max_pnl_ - $current_pnl_;
	$max_drawdown_ = max ( $max_drawdown_, $current_drawdown_ );
    }
}
print "$max_drawdown_\n";
