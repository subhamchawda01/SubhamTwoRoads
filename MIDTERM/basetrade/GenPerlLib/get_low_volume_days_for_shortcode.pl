# \file GenPerlLib/get_low_volume_days_for_shortcode.pl
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
my $REPO="basetrade";
my $STORE="/spare/local/tradeinfo";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; # GetDatesFromNumDays
require "$GENPERLLIB_DIR/sample_data_utils.pl"; # GetFilteredDays
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetLowVolumeDaysForShortcode
{
    my $shortcode_ = shift;
    my ($array_ref_) = shift;
    my $date_ = shift;
    my $numdays_ = shift || 200;

    if ( ! defined $date_ ) {
      $date_ = `date +%Y%m%d`; chomp ( $date_ );
    }

    my $factor_ = "VOL";
    my @factor_aux_ = ( );

    my @dates_vec_ = GetDatesFromNumDays( $shortcode_, $date_, $numdays_ );

    my @filtered_dates_ = ( );

    my $percentile_ = 0.3;

    @$array_ref_ = ( );
    GetFilteredDays ( $shortcode_, \@dates_vec_, $percentile_, "LOW", $factor_, \@factor_aux_, $array_ref_ );
}

sub GetLowVolumeDaysForShortcode_DatagenInfo
{
    my $shortcode_ = shift;
    my ($array_ref) = shift;

    my $low_volume_day_file_ = $STORE."/datageninfo/low_volume_days.".$shortcode_;
    if ( -e $low_volume_day_file_ )
    {
	open LOW_VOLUME_DAY_FILE_HANDLE, "< $low_volume_day_file_ " or PrintStacktraceAndDie ( "$0 Could not open $low_volume_day_file_\n" );

	while ( my $thisline_ = <LOW_VOLUME_DAY_FILE_HANDLE> ) 
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

	close LOW_VOLUME_DAY_FILE_HANDLE;
    }
}

1
