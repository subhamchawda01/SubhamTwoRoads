# \file GenPerlLib/get_numeric_value_from_line.pl
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

sub GetNumericValueFromLine
{
    @_ == 2 or die ('Sub usage: $value = GetNumericValueFromLine( $string_, \@array );');
    my ($string_to_search_, $array_ref) = @_;
    my $retval_ = 0;
    my $count = scalar @$array_ref;
    for ( my $widx_ = 0; $widx_ < $#$array_ref ; $widx_ ++ )
    { 
	if ( $$array_ref[$widx_] eq $string_to_search_ ) 
	{ 
	    $retval_ = $$array_ref[$widx_+1] ;
	}
    }
    return $retval_ ;
}

1;
