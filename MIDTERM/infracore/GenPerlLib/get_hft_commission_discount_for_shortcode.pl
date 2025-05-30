#
#   file GenPerlLib/get_hft_commission_discount_for_shortcode.pl 
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

sub GetHFTDiscount
{

    my $shortcode_ = shift;

    my $volume_ = shift || 0.00 ;

    my $discount_ = 0.0;

    given ( $shortcode_ )
    {
        when ( "DOL" )
        {


        }
        when ( "WDO" )
        {

        }
        when ( "IND" )
        {

       #     # Grace period 
	    #if( $volume_ > 1800 && $volume_ <= 3600 ){

		#$discount_ = 0.22 ;

	    #}elsif( $volume_ > 3600 && $volume_ <= 5400 ){

		#$discount_ = 0.27 ;

	    #}elsif( $volume_ > 5400 && $volume_ <= 9000 ){

		#$discount_ = 0.31 ;

	    #}elsif( $volume_ > 9000 ){

		#$discount_ = 0.33 ;

	    #}

          $discount_ = 0.0 ;

        }
        when ( "WIN" )
        {

          $discount_ = 0.0 ;

#          if( $volume_ > 9000 ) { 

            #$discount_ = 0.04 ;

          #}

        }
        default
        {
            $discount_ = 0.0;
        }
    }

    $discount_;
}

1

