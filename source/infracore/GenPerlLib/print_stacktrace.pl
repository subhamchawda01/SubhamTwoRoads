# \file GenPerlLib/print_stacktrace.pl
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
