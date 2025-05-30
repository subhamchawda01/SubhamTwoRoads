# \file GenPerlLib/get_unique_list.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite No 162, Evoma, #14, Bhattarhalli,
#        Old Madras Road, Near Garden City College,
#        KR Puram, Bangalore 560049, India
#        +91 80 4190 3551
#

use strict;
use warnings;

sub GetUniqueList {
    my @uniq = keys %{{ map {$_ => 1} @_ }};
    @uniq;
}

1;
