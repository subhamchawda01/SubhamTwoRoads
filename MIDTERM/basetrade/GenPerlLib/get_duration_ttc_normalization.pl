# \file GenPerlLib/get_duration_ttc_normalization.pl
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
use feature "switch";

my $USER=$ENV{'USER'};

sub GetDurationTTCNormalization 
{
    my $shortcode_ = shift ;
    my $duration_for_ttc_normalization_ = 600;
    my $duration_for_ttc_normalization_filename_ = "/spare/local/tradeinfo/volume_duration_for_ttc.txt";
    if ( -e $duration_for_ttc_normalization_filename_ )
    {
	open TTC_NORMALIZATION_FILEHANDLE, "< $duration_for_ttc_normalization_filename_" ;
	while ( my $mline_ = <WATCH_DURATION_FILE> )
	{
	    my @mwords_ = split ( ' ', $mline_ ) ;
	    if ( $mwords_[0] eq $shortcode_ )
	    {
		$duration_for_ttc_normalization_ = $mwords_[1];
		last;
	    }
	}
	close ( TTC_NORMALIZATION_FILEHANDLE );
    }
    $duration_for_ttc_normalization_ ;
}

1
