#!/usr/bin/perl

# \file ModelScripts/call_is_model_corr_consistent.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/is_model_corr_consistent.pl"; # IsModelCorrConsistent

if ( $#ARGV >= 0 )
{
    if ( IsModelCorrConsistent ( $ARGV[0] ) )
    {
	print "CONSISTENT\n";
    }
    else
    {
	print "NOTCON\n";
    }
}
