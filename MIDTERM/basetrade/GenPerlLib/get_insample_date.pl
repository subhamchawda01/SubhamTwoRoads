# \file GenPerlLib/get_insample_date.pl
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

sub GetInsampleDate {
    my $t_full_strategy_name_ = shift;
    
    my $t_last_insample_date_ = 20110101 ;
    
# w_strategy_ilist_ZN_0_US_Mkt_Mkt_10_na_t3_20110711_20110810_EST_820_EST_1230_1000_17_3_fsr1_5_FSHLR_0.01_0_0_0.7.tt_EST_915_EST_1545.pfi_13
    my ( $st_dt, $end_dt) = ( $t_full_strategy_name_ =~ m/.*_(20[\d]{6})_(20[\d]{6})_.*/ );
    if ( !$st_dt || !$end_dt ) {
	print ("Could not find start_end date in Strategy: $t_full_strategy_name_\n");
	return $t_last_insample_date_;
    }
    else {
	return $end_dt>$t_last_insample_date_?$end_dt:$t_last_insample_date_;
    }

}

# scripts/call_get_insample_date.pl
#if ( __FILE__ eq $0 ){
#    print "$ARGV[0] - ".GetInsampleDate ( $ARGV[0] )."\n";
#}

1;
