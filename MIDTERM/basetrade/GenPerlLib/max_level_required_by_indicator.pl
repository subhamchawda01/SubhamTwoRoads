# \file GenPerlLib/is_order_weighted_indicator.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#
use strict;
use warnings;
use feature "switch";

sub MaxLevelRequired
{
    my $indicator_line_ = shift;
    my $retval = 0;

    my @words_ = split(/\s+/, $indicator_line_);

    given ( $words_[0] )
    {
    	when ( "MultMidOrderPrice" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMidPrice" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMktComplexOrderPrice" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMktComplexPrice" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMktComplexPriceShortAvg" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMktComplexPriceTopOff" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMktComplexPriceTopOffCombo" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMktOrderPrice" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMktOrderPriceTopOff" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMktPerOrderComplexPrice" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	when ( "MultMktPrice" ) { if($#words_ >= 2) {$retval = int($words_[2]);} }
    	
    }
    $retval;
}


1
