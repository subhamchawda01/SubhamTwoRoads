# \file GenPerlLib/get_very_bad_days_for_shortcode.pl
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

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

sub GetVeryBadDaysForShortcode
{
    my $shortcode_ = shift;
    my ($array_ref) = shift;

    my $very_bad_day_file_ = $HOME_DIR."/".$REPO."/files/datageninfo/very_bad_days.".$shortcode_;
    if ( -e $very_bad_day_file_ )
    {
	open VERY_BAD_DAY_FILE_HANDLE, "< $very_bad_day_file_ " or die "$0 Could not open $very_bad_day_file_\n" ;

	while ( my $thisline_ = <VERY_BAD_DAY_FILE_HANDLE> ) 
	{
	    chomp ( $thisline_ );
	    my @rwords_ = split ( ' ', $thisline_ );
	    if ( $#rwords_ >= 0 )
	    {
		if ( ( ValidDate ( $rwords_[0] ) ) &&
		     ( ! ( SkipWeirdDate ( $rwords_[0] ) || IsDateHoliday ( $rwords_[0] ) ) ) )
		{
		    push ( @$array_ref, $rwords_[0] );
		}
	    }
	}

	close VERY_BAD_DAY_FILE_HANDLE;
    }
}

1
