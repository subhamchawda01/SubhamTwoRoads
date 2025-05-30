# \file GenPerlLib/get_query_type_for_id_date.pl
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

# Valid return types
# "NO_EQUITY" - Not waiting for equity data to be available
# "EQUITY" - Waiting for equity data to be available
sub GetQueryTypeForIdDate
{
    my ( $id_ , $yyyymmdd_ ) = @_;

    my $query_type_ = "NO_EQUITY";

    if ( $id_ >= 31011 && $id_ <= 31099 ) # IND
    {
	if ( $yyyymmdd_ > 20120504 )
	{
	    $query_type_ = "NO_EQUITY";
	}
	else
	{
	    $query_type_ = "EQUITY";
	}
    }

    return $query_type_;
}

1
