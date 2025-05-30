# \file GenPerlLib/get_insample_date.pl
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

sub GetInsampleDate {
    my $t_full_strategy_name_ = shift;
    
    my $t_last_insample_date_ = 20110101 ;
    
#    w_strategy_ilist_ZN_0_US_Mkt_Mkt_10_na_t3_20110711_20110810_EST_820_EST_1230_1000_17_3_fsr1_5_FSHLR_0.01_0_0_0.7.tt_EST_915_EST_1545.pfi_13

    my @swords_ = split ( '_', $t_full_strategy_name_ );
    
    for ( my $s_idx_ = 0 ; $s_idx_ < $#swords_ ; $s_idx_ ++ )
    {
	my $this_word_ = $swords_[$s_idx_];
	if ( ( $this_word_ =~ /201/ ) &&
	     ( int ( $this_word_ ) > 20110101 ) &&
	     ( int ( $this_word_ ) < 20210101 ) )
	{
	    $t_last_insample_date_ = int ( $swords_[$s_idx_ +1] );
	    last;
	}
    }
    $t_last_insample_date_ ;
}

1;
