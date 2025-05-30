# \file GenPerlLib/get_utc_hhmm_str.pl
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

my $REPO="basetrade";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
if ( $USER eq "rkumar" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}
if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

sub GetUTCHHMMStr 
{
    my $hhmmstr_ = shift;
    my $yyyymmdd_ = shift;
    my $retval = `$LIVE_BIN_DIR/get_utc_hhmm_str $hhmmstr_ $yyyymmdd_`;

    $retval;
}

1
