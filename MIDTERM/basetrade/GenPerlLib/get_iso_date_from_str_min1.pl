# \file GenPerlLib/get_iso_date_from_str_min1.pl
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
use Scalar::Util qw(looks_like_number);

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult

sub GetIsoDateFromStrMin1
{
  my $date_str_ = shift;
  my $shc_ = @_ > 0 ? shift : '';

  my $retval = $date_str_;
  if ( $date_str_ =~ /TODAY/ )
  { # expecting something like "TODAY -30" or "TODAY-30"
    my $today_yyyymmdd_ = `date +%Y%m%d`; chomp ( $today_yyyymmdd_ );
    $retval = $today_yyyymmdd_;

    my $substr = 'TODAY';
    my $result = index($date_str_, $substr) + length($substr);
    my $days_str_ = substr ( $date_str_, $result ) ;
    my $num_prev_ = looks_like_number($days_str_) ? -1 * int ( $days_str_ ) : 0;

    if ( $num_prev_ > 0 )
    {
      $num_prev_ = min(2000, $num_prev_);
      $retval = CalcPrevWorkingDateMult ( $today_yyyymmdd_, $num_prev_ , $shc_ ) ;
    }
  }
  $retval;
}

1;
