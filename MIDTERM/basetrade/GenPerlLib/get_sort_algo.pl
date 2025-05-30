# \file GenPerlLib/get_sort_algo.pl                                            
#                                                                                   
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011             
#  Address:                                                                         
#        Suite No 353, Evoma, #14, Bhattarhalli,                                        
#        Old Madras Road, Near Garden City College,                                             
#        KR Puram, Bangalore 560049, India                                                  
#        +91 80 4190 3551                                                           
#                                                                                   
# This script takes inputs :                                                        
# SORTALGO
# and returns the pred duration of the strat                                        

use feature "switch"; # for given, when
use strict;
use warnings;

sub GetSortAlgo
{
    my $pred_algo_string_ = $_[0];
    my $ret_val_ = "";

    given ( $pred_algo_string_ )
    {
	when ( "PNL" )
	{
	    $ret_val_ = "kCNAPnlAverage";
	}
	when ( "PNLMINAVG" )
	{
	    $ret_val_ = "kCNAMinAdjPnlAverage";
	}
    	when ( "PNL_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnl";
    	}
    	when ( "PNL_VOL" )
    	{
    	    $ret_val_ = "kCNAPnlVol";
    	}
    	when ( "PNL_VOL_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnlVol";
    	}
    	when ( "PNL_DD" )
    	{
    	    $ret_val_ = "kCNAPnlDD";
    	}
    	when ( "PNL_SQRT_DD" )
    	{
    	    $ret_val_ = "kCNAPnlSqrtDD";
    	}
    	when ( "PNL_VOL_SQRT_BY_DD" )
    	{
    	    $ret_val_ = "kCNAPnlSqrtVolByDD";
    	}
    	when ( "PNL_VOL_SQRT_BY_DD_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnlVolByDD";
    	}
    	when ( "PNL_VOL_SQRT_BY_TTC_BY_DD_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnlVolBySqrtDDTTC";
    	}
    	when ( "PNL_VOL_SQRT_BY_TTC_SQRT_BY_DD_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnlVolByTTCDD";
    	}
    	when ( "PNL_SHARPE" )
    	{
    	    $ret_val_ = "kCNAPnlSharpe";
    	}
    	when ( "PNL_SHARPE_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnlSharpe";
    	}
    	when ( "PNL_SHARPE_VOL" )
    	{
    	    $ret_val_ = "kCNAPnlSharpeVol";
    	}
    	when ( "PNL_SHARPE_VOL_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnlVol";
    	}
    	when ( "PNL_SHARPE_DD" )
    	{
    	    $ret_val_ = "kCNAPnlSharpeByDD";
    	}
    	when ( "PNL_SHARPE_SQRT_DD" )
    	{
    	    $ret_val_ = "kCNASqrtPnlByDD";
    	}
    	when ( "PNL_SHARPE_VOL_SQRT_BY_DD" )
    	{
    	    $ret_val_ = "kCNASqrtPnlSharpeVolByDD";
    	}
    	when ( "PNL_SHARPE_VOL_SQRT_BY_DD_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnlSharpeVolBySqrtDD";
    	}
    	when ( "PNL_SHARPE_VOL_SQRT_BY_TTC_BY_DD_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnlSharpeVolByDDByTTC";
    	}
    	when ( "PNL_SHARPE_VOL_SQRT_BY_TTC_SQRT_BY_DD_SQRT" )
    	{
    	    $ret_val_ = "kCNASqrtPnlSharpeVolBySqrtDDTTC";
    	}
        when ( /^kCNA/ )
        {
            $ret_val_ = $pred_algo_string_;
        }
    	default
    	{
    	    $ret_val_ = "kCNAMAX" ;
    	}
    }
    $ret_val_;
}

1;
