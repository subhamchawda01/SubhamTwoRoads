# \file GenPerlLib/get_weighted_sum.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use List::Util qw/max min/; # for max

sub GetWeightedSum
{
    my ( $array_ref_res_, $array_ref1_, $array_ref2_, $alpha_ ) = @_;
    for ( my $i = 0 ; $i <= $#$array_ref_res_ ; $i ++ )
    {
	if ( ( $i <= $#$array_ref1_ ) &&
	     ( $i <= $#$array_ref2_ ) )
	{
	    $$array_ref_res_[$i] = ( $alpha_ * $$array_ref1_[$i] ) + ( ( 1 - $alpha_ ) * $$array_ref2_[$i] ) ;
	}
    }
}

1;
