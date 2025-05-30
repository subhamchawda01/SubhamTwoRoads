# \file GenPerlLib/get_very_bad_days_for_shortcode.pl
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

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $STORE="/spare/local/tradeinfo";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetVeryBadDaysForShortcode
{
    my $shortcode_ = shift;
    my ($array_ref) = shift;

    my $very_bad_day_file_ = $STORE."/datageninfo/very_bad_days.".$shortcode_;
    if ( -e $very_bad_day_file_ )
    {
	open VERY_BAD_DAY_FILE_HANDLE, "< $very_bad_day_file_ " or PrintStacktraceAndDie ( "$0 Could not open $very_bad_day_file_\n" );

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
