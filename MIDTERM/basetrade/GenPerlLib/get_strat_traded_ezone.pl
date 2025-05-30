# \file GenPerlLib/get_strat_traded_ezone.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes inputs :
# STRATFILE
# and returns the start and end hhmm

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetStratTradedEzone
{
    my $traded_ezone_ = "INVALID";
    my ($stratfilename_) = @_;

    if ( -e $stratfilename_ )
    {

	open STRATFILEHANDLE, "< $stratfilename_ " or PrintStacktraceAndDie ( "$0 Could not open $stratfilename_\n" );
	my @slines_ = <STRATFILEHANDLE>;
	close STRATFILEHANDLE;

	foreach my $t_sline_ ( @slines_ )
	{
	    chomp ( $t_sline_ );
	    my @swords_ = split ( ' ', $t_sline_ );
	    if ( $#swords_ >= 8 )
	    {
                $traded_ezone_ = $swords_[8]; 
		last;
	    }
	}
    }
    $traded_ezone_;
}

1
