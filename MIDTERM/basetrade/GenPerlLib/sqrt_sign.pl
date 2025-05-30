# \file GenPerlLib/sqrt_sign.pl
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
use Math::Complex ; # sqrt

sub SqrtSign
{
   my $input_ = shift;
   my $retval_ = 1;
   if ( $input_ < 0 )
   {
       $retval_ = -1;
   }
   $retval_ = $retval_ * sqrt ( abs ( $input_ ) );
   $retval_;
}

1;
