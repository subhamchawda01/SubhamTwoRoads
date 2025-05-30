#!/usr/bin/perl

# \file scripts/get_list_of_dates_for_shortcode.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use POSIX;
use List::Util qw[min max]; # max , min

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
#my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
#my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

sub GetDatesFromNumDays
{
  my ($shortcode_, $end_date_, $days_to_look_behind_, $skip_dates_file_) = @_;

  my @skip_dates_ = ( );
  my @sample_dates_ =  ( );

  if ( defined $skip_dates_file_ && $skip_dates_file_ ne "INVALIDFILE" ) {
    open SKIPFILE, " < $skip_dates_file_" or die "can't open '$skip_dates_file_': $!";
    @skip_dates_ = <SKIPFILE>; chomp ( @skip_dates_ );
    close SKIPFILE;
  }

  my $current_yyyymmdd_ = $end_date_;
  for ( my $days_ = $days_to_look_behind_ ; $days_ != 0 && $current_yyyymmdd_ > 20100101 ; $days_ -- ) 
  {
    if ( ! ValidDate ( $current_yyyymmdd_ ) ) 
    { # Should not be here.	
      PrintStacktraceAndDie ( "Invalid date : $current_yyyymmdd_\n" );
      exit ( 0 );
    }

    if ( SkipWeirdDate ( $current_yyyymmdd_ ) ||
        IsDateHoliday ( $current_yyyymmdd_ ) ||
        FindItemFromVec ( $current_yyyymmdd_, @skip_dates_ ) ||
        ( $shortcode_ ne "ALL" &&
          ( NoDataDateForShortcode ( $current_yyyymmdd_, $shortcode_ ) ||
            IsProductHoliday ( $current_yyyymmdd_ , $shortcode_ ) ) ) )
    {
      $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
      $days_ ++;
      next;
    }

    push ( @sample_dates_, $current_yyyymmdd_ );   
    $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
  }
  return @sample_dates_;
}

sub IsValidStartEndDate
{
	my ( $start_date_, $end_date_) = @_;
	if ( ! ValidDate($start_date_) || !ValidDate($end_date_)) {
		return "";
	}
	if ( $start_date_ > $end_date_ ) {
		return "";
	}
	return "true";
}

sub GetDatesFromStartDate
{
  my ($shortcode_, $start_date_, $end_date_, $skip_dates_file_, $numdays_) = @_;

  my @skip_dates_ = ( );
  my @sample_dates_ =  ( );

  if ( defined $skip_dates_file_ && $skip_dates_file_ ne "INVALIDFILE" ) {
    open SKIPFILE, " < $skip_dates_file_" or die "can't open '$skip_dates_file_': $!";
    @skip_dates_ = <SKIPFILE>; chomp ( @skip_dates_ );
    close SKIPFILE;
  }

  my $current_yyyymmdd_ = $end_date_;
  while ( $current_yyyymmdd_ >= $start_date_ && ( ! defined $numdays_ || $numdays_ > 0 ) )
  {
    if ( ! ValidDate ( $current_yyyymmdd_ ) ) 
    { # Should not be here.	
      PrintStacktraceAndDie ( "Invalid date : $current_yyyymmdd_\n" );
      exit ( 0 );
    }

    if ( SkipWeirdDate ( $current_yyyymmdd_ ) ||
        IsDateHoliday ( $current_yyyymmdd_ ) ||
        FindItemFromVec ( $current_yyyymmdd_, @skip_dates_ ) ||
        ( $shortcode_ ne "ALL" &&
          ( NoDataDateForShortcode ( $current_yyyymmdd_, $shortcode_ ) ||
            IsProductHoliday ( $current_yyyymmdd_ , $shortcode_ ) ) ) )
    {
      $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
      next;
    }

    push ( @sample_dates_, $current_yyyymmdd_ );
    if ( defined $numdays_ ) { $numdays_--; }
    $current_yyyymmdd_ = CalcPrevWorkingDateMult ( $current_yyyymmdd_ , 1 );
  }
  return @sample_dates_;
}

