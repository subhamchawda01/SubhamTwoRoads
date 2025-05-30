# \file GenPerlLib/calc_next_working_date_mult.pl
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

my $PEXECS_DIR="/home/pengine/prod/live_execs";
my $PSCRIPTS_DIR="/home/pengine/prod/live_scripts";

require "$PSCRIPTS_DIR/is_bmf_holiday.pl"; # Holiday

sub CalNumberOfWorkingDaysBMFBetweenDates {

    my $input_start_date = shift;
    my $input_end_date = shift;
    my $retval = $input_start_date ;

    my $num_times_ = 0 ;

    while ( $retval < $input_end_date )
    {
	$retval = `$PEXECS_DIR/calc_next_day $retval`; # no chomp required

	while ( IsBMFHolidayIncludingWeekends ( $retval ) == 1)  
	{
	    $retval = `$PEXECS_DIR/calc_next_day $retval`; # no chomp required
	}
	# maintains invariant that $retval is never a holiday

	$num_times_ ++ ;
    }

    $num_times_ ;
}

sub CalcNoWorkingDaysTillExpiryForDIs
{
  my $symbol_ = shift;
  my $input_date_ = shift;
  if ( index ( $symbol_, "DI" ) != 0 ) {
    return;
  }
  
  if ( index ( $symbol_ , "DI1F13" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20130102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F14" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20140102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F15" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20150102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F16" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20160102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F17" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20170102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F18" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20180102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F19" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20190102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F20" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20200102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F21" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20210102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F22" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20220102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F23" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20230102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F24" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20240102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F25" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20250102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F26" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20260102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1F27" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20270102 ) ;
  }
  elsif ( index ( $symbol_ , "DI1N13" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20130701 ) ;
  }
  elsif ( index ( $symbol_ , "DI1N14" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20140701 ) ;
  }
  elsif ( index ( $symbol_ , "DI1N15" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20150701 ) ;
  }
  elsif ( index ( $symbol_ , "DI1N16" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20160701 ) ;
  }
  elsif ( index ( $symbol_ , "DI1N17" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20170701 ) ;
  }
  elsif ( index ( $symbol_ , "DI1N18" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20180701 ) ;
  }
  elsif ( index ( $symbol_ , "DI1N19" ) >= 0 )
  {
     CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20190701 ) ;
  }
  elsif ( index ( $symbol_ , "DI1N20" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20200701 ) ;
  }
  elsif ( index ( $symbol_ , "DI1N21" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20210701 ) ;
  }
  elsif ( index ( $symbol_ , "DI1G13" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20130201 ) ;
  }
  elsif ( index ( $symbol_ , "DI1G14" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20140201 ) ;
  }
  elsif ( index ( $symbol_ , "DI1J15" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20150401 ) ;
  }
  elsif ( index ( $symbol_ , "DI1J16" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20160401 ) ;
  }
  elsif ( index ( $symbol_ , "DI1J17" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20170401 ) ;
  }
  elsif ( index ( $symbol_ , "DI1V19" ) >= 0 )
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20191001 ) ;
  }
  else
  {
    CalNumberOfWorkingDaysBMFBetweenDates ( $input_date_, 20170131 ) ;
  }
}

1;
