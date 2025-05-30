# \file GenPerlLib/holiday_manager.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2016
#  Address:
#        Suite 217, Level 2, Prestige Omega,
#        No 104, EPIP Zone, Whitefield,
#        Bangalore - 560066, India
#        +91 80 4060 0717
#
use Carp;
my $INSTALL_BIN="/home/pengine/prod/live_execs";
my $HOLIDAY_MANAGER_LOG="/spare/local/logs/holiday_manager_logs";

sub IsExchangeHoliday
{
  if ( @_ < 2 ) 
  { 
    print STDERR "USAGE: IsExchangeHoliday Exchange YYYYMMDD\n"; 
    return 0;
  }
  my ($exchange, $date) = @_;
  my $is_holiday = `$INSTALL_BIN/holiday_manager EXCHANGE $exchange $date F`; chomp($is_holiday);
  if ($is_holiday == 2)
  {
    2; # Not a Holiday
  }
  elsif($is_holiday == 1)
  {
    1; # Holiday
  }
  else
  {
    my $hostname=`hostname`; chomp($hostname);
    my $key="ExchHoliday Error, User : $ENV{'USER'}, Host : $hostname, Exchange : $exchange, Date : $date";
    $is_reported=0;
    if( -e $HOLIDAY_MANAGER_LOG  )
    {
       $is_reported=`cat $HOLIDAY_MANAGER_LOG | grep "$key" | wc -l`;
    }
    else
    {
      `touch $HOLIDAY_MANAGER_LOG`;
      `chmod 666 $HOLIDAY_MANAGER_LOG`;
    }
    local *STDOUT;
    local *STDERR; 
    open(STDERR, ">>$HOLIDAY_MANAGER_LOG");
    open(STDOUT, ">>$HOLIDAY_MANAGER_LOG");

    if($is_reported == 0)
    {
      carp;
      print $key."\n\n";
    #`$INSTALL_BIN/send_slack_notification notification_monitor DATA " ExchHoliday Error \n User : $ENV{'USER'} \n Host : $hostname \n Exchange : $exchange \n Date : $date \n"`;
    }
    close(STDERR);
    close(STDOUT);
    0; # Error Case
  }
}

# Saturday and Sunday will also be considered as holidays
sub IsExchangeHolidayIncludingWeekends
{
  if ( @_ < 2 ) 
  { 
    print STDERR "USAGE: IsExchangeHolidayIncludingWeekends Exchange YYYYMMDD\n"; 
    return 0;
  }
  my ($exchange, $date) = @_;
  my $is_holiday = `$INSTALL_BIN/holiday_manager EXCHANGE $exchange $date T`; chomp($is_holiday);

  if ($is_holiday == 2)
  {
    2; # Not a Holiday
  }
  elsif ($is_holiday == 1)
  {
    1; # Holiday
  }
  else
  {
    my $hostname=`hostname`; chomp($hostname);
    my $key="ExchHolidayW Error, User : $ENV{'USER'}, Host : $hostname, Exchange : $exchange, Date : $date";
    $is_reported=0;
    if( -e $HOLIDAY_MANAGER_LOG  )
    {
       $is_reported=`cat $HOLIDAY_MANAGER_LOG | grep "$key" | wc -l`;
    }
    else
    {
      `touch $HOLIDAY_MANAGER_LOG`;
      `chmod 666 $HOLIDAY_MANAGER_LOG`;
    }
    local *STDOUT;
    local *STDERR;
    open(STDERR, ">>$HOLIDAY_MANAGER_LOG");
    open(STDOUT, ">>$HOLIDAY_MANAGER_LOG");

    if($is_reported == 0)
    {
      carp;
      print $key."\n\n";
      #`$INSTALL_BIN/send_slack_notification notification_monitor DATA " ExchHolidayW Error \n User : $ENV{'USER'} \n Host : $hostname \n Exchange : $exchange \n Date : $date \n"`;
    }
    close(STDERR);
    close(STDOUT);
    0; # Error Case
  }

}

sub IsProductHoliDay
{
  if ( @_ < 2 ) 
  { 
    print STDERR "USAGE: IsProductHoliDay SHORTCODE YYYYMMDD\n"; 
    return 0;
  }
  my ($product, $date) = @_;
  my $is_holiday = `$INSTALL_BIN/holiday_manager PRODUCT $product $date F`; chomp($is_holiday);

  if($is_holiday == 2)
  {
    2; # Not a Holiday
  }
  elsif($is_holiday == 1)
  {
    1; # Holiday
  }
  else
  {
    my $hostname=`hostname`; chomp($hostname);
    my $key="ProductHoliday Error, User : $ENV{'USER'}, Host : $hostname, Product : $product";
    $is_reported=0;
    if( -e $HOLIDAY_MANAGER_LOG  )
    {
       $is_reported=`cat $HOLIDAY_MANAGER_LOG | grep "$key" | wc -l`;
    }
    else
    {
      `touch $HOLIDAY_MANAGER_LOG`;
      `chmod 666 $HOLIDAY_MANAGER_LOG`;
    }
    local *STDOUT;
    local *STDERR;
    open(STDERR, ">>$HOLIDAY_MANAGER_LOG");
    open(STDOUT, ">>$HOLIDAY_MANAGER_LOG");

    if($is_reported == 0)
    {
      carp;
      print $key."\n\n";
      #`$INSTALL_BIN/send_slack_notification notification_monitor DATA " ProductHoliday Error \n User : $ENV{'USER'} \n Host : $hostname \n Product : $product \n Date : $date \n"`;
    }
    close(STDERR);
    close(STDOUT);
    0; # Error Case
  }
}

# Saturday and Sunday will also be considered as holidays
sub IsProductHoliDayIncludingWeekends
{
  if ( @_ < 2 ) 
  { 
    print STDERR "USAGE: IsProductHoliDayIncludingWeekends SHORTCODE YYYYMMDD\n"; 
    return 0;
  }
  my ($product, $date) = @_;
  my $is_holiday = `$INSTALL_BIN/holiday_manager PRODUCT $product $date T`; chomp($is_holiday);

  if($is_holiday == 2)
  {
    2; # Not a Holiday
  }
  elsif( $is_holiday == 1)
  {
    1; # Holiday
  }
  else
  {
    my $hostname=`hostname`; chomp($hostname);
    my $key="ProductHolidayW Error, User : $ENV{'USER'}, Host : $hostname, Product : $product";
    $is_reported=0;
    if( -e $HOLIDAY_MANAGER_LOG  )
    {
       $is_reported=`cat $HOLIDAY_MANAGER_LOG | grep "$key" | wc -l`;
    }
    else
    {
      `touch $HOLIDAY_MANAGER_LOG`;
      `chmod 666 $HOLIDAY_MANAGER_LOG`;
    }
    local *STDOUT;
    local *STDERR;
    open(STDERR, ">>$HOLIDAY_MANAGER_LOG");
    open(STDOUT, ">>$HOLIDAY_MANAGER_LOG");

    if($is_reported == 0)
    {
      carp;
      print $key."\n\n";
      #`$INSTALL_BIN/send_slack_notification notification_monitor DATA " ProductHolidayW Error \n User : $ENV{'USER'} \n Host : $hostname \n Product : $product \n Date : $date \n"`;
    }
    close(STDERR);
    close(STDOUT);
    0; # Error Case
  }
}

1;
