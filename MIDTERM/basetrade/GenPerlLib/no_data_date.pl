# \file GenPerlLib/valid_date.pl
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

sub NoDataDate
{
    my $input_date = shift;
    my $MINDATADATE_ = 20120101;
    if ( ( $input_date < $MINDATADATE_ ) )
    {
	1;
    }
    else 
    {
	0;
    }
}

sub NoDataDateForShortcode
{
    my ( $input_date_ , $shortcode_ ) = @_;

    if ( ! $shortcode_ )
    {
	return NoDataDate ( $input_date_ );
    }

    my $MINDATADATE = 20120101;
    given ( $shortcode_ )
    {
    	when ( "FXXP_0" )
    	{
    	    $MINDATADATE = 20120705;
    	}

    	when ( "FSTB_0" )
    	{
    	    $MINDATADATE = 20120705;
    	}

	when ( "LFL_0" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_1" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_2" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_3" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_4" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_5" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_6" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_7" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_8" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_9" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFL_10" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_0" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_1" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_2" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_3" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_4" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_5" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_6" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_7" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_8" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_9" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFI_10" )
	{
	    $MINDATADATE = 20120612;
	}
	
	when ( "LFI06" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "KFFTI_0" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "JFFCE_0" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFR_0" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "LFZ_0" )
	{
	    $MINDATADATE = 20120612;
	}

	when ( "HHI_0" )
	{
	    $MINDATADATE = 20121005;
	}

	when ( "HSI_0" )
	{
	    $MINDATADATE = 20121005;
	}

	when ( "MCH_0" )
	{
	    $MINDATADATE = 20121005;
	}

	when ( "MHI_0" )
	{
	    $MINDATADATE = 20121005;
	}
        when ( "GAZP" )
	{
	    $MINDATADATE = 20131101;
	}

    	default
    	{ }
    }

    return ( ( $input_date_ < $MINDATADATE ) || NoDataDate ( $input_date_ ) );
}

1;
