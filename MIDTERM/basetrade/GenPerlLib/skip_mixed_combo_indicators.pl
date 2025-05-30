# \file GenPerlLib/skip_mixed_combo_indicators.pl
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


sub SkipMixedComboIndicator
{
    my ($dep_shortcode_, $combo_shortcode_, $indicator_filename_) = @_;
    my $retval_ = 0;
    if ( $indicator_filename_ =~m/Combo/ && not $indicator_filename_=~m/Offline/ )
    {
	if ( IsMixedPort ( $combo_shortcode_ ) )
	{
		$retval_ = 1;
	}
    }
    $retval_;
}

sub IsMixedPort
{
	
	my ($combo_shortcode_) = @_;
	my $retval_ = 0;
	if ( $combo_shortcode_ =~ m/ELBINDX/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/ELFRNCH/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/EXALL/ )
        { $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_UBEFC/ )
        { $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_UBEFX/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_UBE/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_UEBE2/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_UEBEFX/ )
        { $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_UEBE/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_UEEQFX/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_UELINDEBND/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_UEQFXC/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_USALL2/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/HYB_USALL/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/NDQBRETFOIL/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/NDQBRETFOILPORT/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/NDQBRIBOVPORT/ )
	{ $retval_ = 1; }
	elsif ( $combo_shortcode_ =~m/NDQBRIBOV/ )
	{ $retval_ = 1; }
        elsif ( $combo_shortcode_ =~m/NDQINDXPORT/ )
        { $retval_ = 1; }
        elsif ( $combo_shortcode_ =~m/UBEFC/ )
        { $retval_ = 1; }
        elsif ( $combo_shortcode_ =~m/UBEFX/ )
        { $retval_ = 1; }
        elsif ( $combo_shortcode_ =~m/UEBE2/ )
        { $retval_ = 1; }
        elsif ( $combo_shortcode_ =~m/UEBEFX/ )
        { $retval_ = 1; }
        elsif ( $combo_shortcode_ =~m/UEBE/ )
        { $retval_ = 1; }
        elsif ( $combo_shortcode_ =~m/UELINDEBND/ )
        { $retval_ = 1; }
        elsif ( $combo_shortcode_ =~m/UEQFXC/ )
        { $retval_ = 1; }
	
	$retval_
}	



1;
