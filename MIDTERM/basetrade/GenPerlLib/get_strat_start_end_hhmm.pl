# \file GenPerlLib/get_strat_start_end_hhmm.pl
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

sub GetStratStartEndHHMM
{
    my $start_hhmm_ = "EST_800";
    my $end_hhmm_ = "EST_1600";
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
	    if ( $#swords_ >= 6 )
	    {
		$start_hhmm_ = $swords_[5];
		$end_hhmm_ = $swords_[6];
		last;
	    }
	}
    }
    $start_hhmm_, $end_hhmm_;
}

1
