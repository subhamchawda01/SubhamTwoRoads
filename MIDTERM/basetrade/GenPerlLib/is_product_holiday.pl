# \file GenPerlLib/is_product_holiday.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $INFRA_GENPERLLIB_DIR=$HOME_DIR."/infracore_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_port_constituents.pl";
require "$INFRA_GENPERLLIB_DIR/holiday_manager.pl"; 

sub IsProductHoliday
{
    if ( @_ < 2 ) 
    { 
		print STDERR "USAGE: IsProductHoliday YYYYMMDD SHORTCODE\n"; 
		return 0;
    }
    
    my ($date, $shortcode) = @_;
    my $is_holiday = IsProductHoliDay($shortcode, $date);
    if( $is_holiday == 1)
    {
      return 1;
    }
    else
    {
      if( $is_holiday == 0 )
      {
#        `echo "Holiday Manager Error : Shortcode : $shortcode, Date : $date " | mail -s "Holiday Manager Error" -r "pranjal.jain\@tworoads.co.in" "nseall@tworoads.co.in"`;
      }
      return 0; # Not a Holiday , Error
    }
}

sub IsProductHolidayIncludingWeekends
{
    if ( @_ < 2 ) 
    { 
		print STDERR "USAGE: IsProductHoliday YYYYMMDD SHORTCODE\n"; 
		return 0;
    }
    
    my ($date, $shortcode) = @_;
    my $is_holiday = IsProductHoliDayIncludingWeekends($shortcode, $date);
    if( $is_holiday == 1)
    {
      return 1;
    }
    else
    {
      if( $is_holiday == 0 )
      {
#        `echo "Holiday Manager Error : Shortcode : $shortcode, Date : $date " | mail -s "Holiday Manager Error" -r "pranjal.jain\@tworoads.co.in" "nseall@tworoads.co.in"`;
      }
      return 0; # Not a Holiday , Error
    }
}

sub IsPortHoliday
{
  if ( @_ < 2 ) 
  { 
    print STDERR "USAGE: IsProductHoliday YYYYMMDD PORTFOLIO\n"; 
    return 0;
  }

  my ($date, $portfolio_) = @_;
  my $is_holiday = 0;

  if ( IsValidPort($portfolio_) ) {
    my @port_constituents_ = GetPortConstituents($portfolio_);
    foreach my $shc_ ( @port_constituents_ ) {
      if ( IsProductHoliDay($shc_, $date) ) {
        $is_holiday = 1;
        last;
      }
    }
  }
  return $is_holiday;
}

sub IsPortHolidayIncludingWeekends
{
  if ( @_ < 2 ) 
  { 
    print STDERR "USAGE: IsProductHoliday YYYYMMDD PORTFOLIO\n"; 
    return 0;
  }

  my ($date, $portfolio_) = @_;
  my $is_holiday = 0;

  if ( IsValidPort($portfolio_) ) {
    my @port_constituents_ = GetPortConstituents($portfolio_);
    foreach my $shc_ ( @port_constituents_ ) {
      if ( IsProductHoliDayIncludingWeekends($shc_, $date) ) {
        $is_holiday = 1;
        last;
      }
    }
  }
  return $is_holiday;
}

1
