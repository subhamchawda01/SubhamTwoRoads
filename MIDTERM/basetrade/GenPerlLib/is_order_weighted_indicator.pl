# \file GenPerlLib/is_order_weighted_indicator.pl
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
use feature "switch";
    
my @orderw_inds_ = ( "OrderWBPMomentum2" ,"OrderWBPMomentum" ,"SizeWBPMomentum2" ,"BookOrderCDiff" ,"BookOrderDiff" ,"MultMidOrderPrice" ,"MultMktComplexOrderPrice" ,"MultMktOrderPrice" ,"MultMktOrderPriceTopOff" ,"MultMktPerOrderComplexPrice" );
my @stdev_req_inds_ = ( "BollingerBand" ,"ProjectedPriceConstPairs" ,"StableScaledTrend2" );

sub IsOWIndicator
{
    my $ind_string_ = shift;
    return ( grep { $_ eq $ind_string_ || $_."Combo" eq $ind_string_ } @orderw_inds_ ); 
}

sub IsStdevReqForIndicator
{
    my $ind_string_ = shift;
    return ( grep { $_ eq $ind_string_ || $_."Combo" eq $ind_string_ } @stdev_req_inds_ ); 
}

1
