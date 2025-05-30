#
#   file GenPerlLib/get_commission_for_shortcode.pl
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

sub GetCommissionForShortcode
{

    my $shortcode_ = shift;

    my $commission_ = 0.0;

    given ( $shortcode_ )
    {
        when ( "ES" )
        {
            $commission_ = 0.13;
        }
        when ( "ZF" )
        {
            $commission_ = 0.14;
        }
        when ( "ZN" )
        {
            $commission_ = 0.14;
        }
        when ( "ZB" )
        {
            $commission_ = 0.14;
        }
        when ( "ZT" )
        {
            $commission_ = 0.13;
        }
        when ( "GE" )
        {
            $commission_ = 0.13;
        }
        when ( "NKD" )
        {
            $commission_ = 1.04 ; 
        }
        when ( "NIY" )
        {
            $commission_ = 1.05 ;
        }
        when ( "UB" )
        {
            $commission_ = 0.14;
        }
        when ( "CGB" )
        {
            $commission_ = 0.15;
        }
        when ( "CGF" )
        {
            $commission_ = 0.15;
        }
        when ( "CGZ" )
        {
            $commission_ = 0.15;
        }
        when ( "BAX" )
        {
            $commission_ = 0.15;
        }
        when ( "SXF" )
        {
            $commission_ = 0.15;
        }
        when ( "FGBS" )
        {
            $commission_ = 0.22;
        }
        when ( "FOAT" )
        {
            $commission_ = 0.22;
        }
        when ( "FGBM" )
        {
            $commission_ = 0.22;
        }
        when ( "FBTP" )
        {
            $commission_ = 0.22;
        }
        when ( "FBTS" )
        {
            $commission_ = 0.22;
        }
        when ( "FGBL" )
        {
            $commission_ = 0.22;
        }
        when ( "FGBX" )
        {
            $commission_ = 0.22;
        }
        when ( "FESX" )
        {
            $commission_ = 0.32;
        }
        when ( "FVS" )
        {
            $commission_ = 0.22;
        }
        when ( "FDAX" )
        {
            $commission_ = 0.52;  #0.5 exch fees + 0.2 newedge
        }
	when ( "JFFCE" )
	{
	    $commission_ = 0.30;
	}
        when ( "YFEBM" )
        {
            $commission_ = 1.03 ;
        }
        when ( "XFC" )
        {
            $commission_ = 0.57 ;
        }
        when ( "XFRC" )
        {
            $commission_ = 0.85;
        }
	when ( "KFFTI" )
	{
	    $commission_ = 0.47;
	}
	when ( "LFZ" )
	{
	    $commission_ = 0.31;
	}
	when ( "LFL" )
	{
	    $commission_ = 0.31;
	}
	when ( "LFR" )
	{
	    $commission_ = 0.25;
	}
	when ( "LFI" )
	{
	    $commission_ = 0.37;
	}
        when ( "DOL" )
        {
            $commission_ = 0.566; # used to be 0.467
        }
        when ( "WDO" )
        {
            $commission_ = 0.2;
        }
        when ( "IND" )
        {
            $commission_ = 0.393;
        }
        when ( "WIN" )
        {
            $commission_ = 0.1; #used to 0.057
        }
        when ( "DI" )
        {
            $commission_ = 0.396; #used to be 0.353
        }
        when ( "HHI" )
        {
            $commission_ = 5.85 ;
        }
        when ( "HSI" )
        {
            $commission_ = 12.35 ;
        }
        when ( "MHI" )
        {
            $commission_ = 4.62 ; # this used to be 4.37
        }
        when ( "MCH" )
        {
            $commission_ = 2.62;
        }
        when ( "NK" )
        {
            $commission_ = 86; # yen supposed to be 86
        }
        when ( "NKM" )
        {
            $commission_ = 11; # yen supposed to be 11
        }
        when ( "Si" )
        {
            $commission_ = 0.75; # rub
        }
        when ( "RI" )
        {
            $commission_ = 1.5; # rub
        }
        when ( "GD" )
        {
            $commission_ = 1; # rub
        }
        when ( "USD000UTSTOM" )
        {
            $commission_ = 0.43; # rub
        }
        when ( "USD000000TOD" )
        {
            $commission_ = 0.43; # rub
        }
        when ( "JGBL" )
        {
            $commission_ = 194; 
        }
        when ( "TOPIX" )
        {
            $commission_ = 125; 
        }
        when ( "VX" )
        {
            $commission_ = 0.59 ; 
        }
        when ( "FVS" )
        {
            $commission_ = 0.22 ; 
        }
        default
        {
            $commission_ = 0.0;
        }
    }

    $commission_;
}

1

