# \file GenPerlLib/get_high_volume_days_for_shortcode.pl
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
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetHighVolumeDaysForShortcode
{
    my $shortcode_ = shift;
    my ($array_ref) = shift;

    my $high_volume_day_file_ = $HOME_DIR."/".$REPO."/files/datageninfo/high_volume_days.".$shortcode_;
    if ( -e $high_volume_day_file_ )
    {
	open HIGH_VOLUME_DAY_FILE_HANDLE, "< $high_volume_day_file_ " or PrintStacktraceAndDie ( "$0 Could not open $high_volume_day_file_\n" );

	while ( my $thisline_ = <HIGH_VOLUME_DAY_FILE_HANDLE> ) 
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

	close HIGH_VOLUME_DAY_FILE_HANDLE;
    }
}

1
