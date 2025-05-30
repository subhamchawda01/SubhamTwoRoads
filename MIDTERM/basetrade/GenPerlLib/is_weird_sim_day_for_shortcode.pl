# \file GenPerlLib/is_weird_sim_day_for_shortcode.pl
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

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktraceAndDie

require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate

# Keep track of weird dates where 
#   sizes were different
#   queries were restarted
#   external getflats , etc.
sub IsWeirdSimDayForShortcode
{
    my ( $shortcode_ , $tradingdate_ ) = @_;

    my $weird_day_file_ = $HOME_DIR."/".$REPO."/SysInfo/TradingInfo/SimConfig/weird_days.".$shortcode_;

    my $is_weird_ = 0;

    if ( -e $weird_day_file_ )
    {
	open ( WEIRD_DAY_FILE , "< $weird_day_file_ " ) or PrintStacktraceAndDie ( "$0 Couldnot open $weird_day_file_" );

	while ( my $t_line_ = <WEIRD_DAY_FILE> )
	{
	    chomp ( $t_line_ );

	    if ( $t_line_ == $tradingdate_ )
	    {
		$is_weird_ = 1;
		last;
	    }
	}

	close ( WEIRD_DAY_FILE );
    }

    if ( ! ValidDate ( $tradingdate_ ) ||
	 SkipWeirdDate ( $tradingdate_ ) ||
	 IsDateHoliday ( $tradingdate_ ) ||
	 IsProductHoliday ( $tradingdate_ , $shortcode_ ) )
    { $is_weird_ = 1; }

    if ( $tradingdate_ < 20120401 ) # Sim-Real before this date might not be sanitized for use.
    { $is_weird_ = 1; }

    return $is_weird_;
}

1
