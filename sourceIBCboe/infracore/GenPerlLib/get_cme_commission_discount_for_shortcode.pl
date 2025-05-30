#
#   file GenPerlLib/get_cme_commission_discount_for_shortcode.pl 
#
#   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#   Address:
#   Suite No 162, Evoma, #14, Bhattarhalli,
#   Old Madras Road, Near Garden City College,
#   KR Puram, Bangalore 560049, India
#   +91 80 4190 3551

use strict;
use warnings;
use feature "switch";

sub GetCMEDiscount
{

    my $shortcode_ = shift;

    my $trade_hours_ = shift || "EU" ;

    my $discount_ = 0.0;

    given ( $shortcode_ )
    {
        when ( "ZN" )
        {

	    if( index( $trade_hours_ , "EU" ) == 0 ){

		$discount_ = 0.0 ;

	    }elsif( index( $trade_hours_ , "US" ) == 0 ){

		$discount_ = 0.0 ;

	    }

        }
        when ( "ZF" )
        {

	    if( index( $trade_hours_ , "EU" ) == 0 ){

		$discount_ = 0.0 ;

	    }elsif( index( $trade_hours_ , "US" ) == 0 ){

		$discount_ = 0.0 ;

	    }


        }
        when ( "ZB" )
        {

	    if( index( $trade_hours_ , "EU" ) == 0 ){

		$discount_ = 0.0 ;

	    }elsif( index( $trade_hours_ , "US" ) == 0 ){

		$discount_ = 0.0 ;

	    }


        }
        when ( "UB" )
        {

	    if( index( $trade_hours_ , "EU" ) == 0 ){

		$discount_ = 0.0 ;

	    }elsif( index( $trade_hours_ , "US" ) == 0 ){

		$discount_ = 0.0 ;

	    }


        }
        default
        {
            $discount_ = 0.0;
        }
    }

    $discount_;
}

1

