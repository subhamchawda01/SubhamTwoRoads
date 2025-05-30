# \file GenPerlLib/skip_combo_indicator.pl
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

sub SkipComboIndicator
{
    
    my $self_shortcode_ = shift;
    my $shortcode_ = shift;
    my $indicator_filename_ = shift;
    my $retval = 0;

	if( $indicator_filename_ =~ m/MultMkt*Combo/ )
	{
		$retval = 1;
		if( (($self_shortcode_ =~ m/BR_[WI][IN][ND]_0/ ) && ($shortcode_ =~ m/IND2/ )) ||
		    (($self_shortcode_ =~ m/NKM?_[0-4]/ )        && ($shortcode_ =~ m/OSENIK/))
		    )
		{
		    $retval = 0;
		}
	}
	
	
	if ( ( $shortcode_ eq "BR_WIN_0" ) || ( $shortcode_ eq "BR_IND_0" ) )
	{
		$retval = 0;
	}

    $retval;
}

sub SkipPCAIndicator
{
    my ($shortcode_, $combo_shortcode_, $indicator_filename_) = @_;
    my $retval = 0;
    if ( $indicator_filename_ =~ m/PCA/ ){
        $retval = 1;
        my @is_there = `grep $combo_shortcode_ /spare/local/tradeinfo/PCAInfo/portfolio_inputs_DEFAULT | grep $shortcode_`;
        if ( $#is_there > -1){ $retval = 0; }
    }
    $retval;
}

1
