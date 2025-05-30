# \file GenPerlLib/is_strat_dir_in_timeperiod.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes inputs like :
# DIRNAME = UTC_715-UTC_1015
# TIME_DURATION = [ EU_MORN_DAY | EU_MORN_DAY_US_DAY | US_EARLY_MORN | US_MORN | US_DAY | US_MORN_DAY  ]
# and returns if it false in that period

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $GET_UTC_HHMM_STR_EXEC_ = "$BIN_DIR/get_utc_hhmm_str" ;
if ( -e "$LIVE_BIN_DIR/get_utc_hhmm_str" )
{
    $GET_UTC_HHMM_STR_EXEC_ = "$LIVE_BIN_DIR/get_utc_hhmm_str" ;
}

sub IsStratDirInTimePeriod
{
  my $dirname_ = shift;
  my $timeperiod_ = shift;

  my $retval = 0;

  my ($start_str_, $end_str_, $tag_ );
  ($start_str_, $end_str_) = ("CET_800", "EST_1600");

  given ( $timeperiod_ )
  {
    when ( "AS_MORN" ) { ($start_str_, $end_str_) = ("HKT_0900", "HKT_1200"); }
    when ( "AS_DAY" ) { ($start_str_, $end_str_) = ("HKT_1200", "HKT_1700"); }
    when ( "EU_MORN_DAY" ) { ($start_str_, $end_str_) = ("CET_700", "EST_930"); }
    when ( "EU_MORN_DAY_US_DAY" ) { ($start_str_, $end_str_) = ("CET_800", "EST_1600"); }
    when ( "US_EARLY_MORN" ) { ($start_str_, $end_str_) = ("EST_600", "EST_830"); }
    when ( "US_MORN" ) { ($start_str_, $end_str_) = ("EST_700", "EST_945"); }
    when ( "US_DAY" ) { ($start_str_, $end_str_) = ("EST_915", "EST_1600"); }
    when ( "US_MORN_DAY" ) { ($start_str_, $end_str_) = ("EST_600", "EST_1700"); }
    default { }
  }

  if ( index ( $timeperiod_ , "EST" ) >= 0 ||
      index ( $timeperiod_ , "BRT" ) >= 0 ||
      index ( $timeperiod_ , "UTC" ) >= 0 ||
      index ( $timeperiod_ , "HKT" ) >= 0 ||
      index ( $timeperiod_ , "JST" ) >= 0 ||
      index ( $timeperiod_ , "MSK" ) >= 0 ||
      index ( $timeperiod_ , "ASX" ) >= 0 ||
      index ( $timeperiod_ , "CET" ) >= 0 )
  { # Very complex time period requirments have necessitated this support.
    my @time_period_words_ = split ( '-' , $timeperiod_ );
    if ( $#time_period_words_ >= 1 )
    {
      $start_str_ = $time_period_words_ [ 0 ];
      $end_str_ = $time_period_words_ [ 1 ];
      if ( $#time_period_words_ == 2 ) {
        $tag_ = $time_period_words_ [ 2 ];
      }
    }
  }

  my @words_ = split ( '-', $dirname_ ) ;
  if ( $#words_ >= 1 )
  {
    my $gststr_ = $words_[0];
    my $getstr_ = $words_[1];
    my $gtag_ = $words_[2];

    my $st_ = `$GET_UTC_HHMM_STR_EXEC_ $start_str_`;
    my $et_ = `$GET_UTC_HHMM_STR_EXEC_ $end_str_`;

    my $gst_ = `$GET_UTC_HHMM_STR_EXEC_ $gststr_`;
    my $get_ = `$GET_UTC_HHMM_STR_EXEC_ $getstr_`;

    if ( ( $gst_ >= $st_ ) && ( $get_ <= $et_ )
        && ( ! defined $tag_ || ( defined $gtag_ && $gtag_ eq $tag_ ) ) )
    { $retval = 1; }
  }
  $retval;
}

1
