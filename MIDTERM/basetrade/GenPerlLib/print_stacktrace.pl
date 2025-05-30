# \file GenPerlLib/print_stacktrace.pl
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
use Carp;

sub PrintStacktrace
{
    my ( $custom_message_ ) = @_;

    chomp ( $custom_message_ );

    carp $custom_message_."\n";

    return;
}

sub PrintStacktraceAndDie
{
    my ( $custom_message_ ) = @_;

    PrintStacktrace ( $custom_message_ );

    die;

    return;
}

1
