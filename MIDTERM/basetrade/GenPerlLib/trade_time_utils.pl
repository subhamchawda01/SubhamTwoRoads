use strict;
use warnings;
use List::Util qw /min max/;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/date_utils.pl"; # GetUTCTime
require "$GENPERLLIB_DIR/parse_utils.pl"; # ParseConfigLines
require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl"; # GetExchFromSHC
require "$GENPERLLIB_DIR/get_port_constituents.pl"; # GetPortConstituents

my %prod_start_end_time_;
my %exch_start_end_time_;
my %prod_start_end_time_utc_;
my $def_date_ = `date +%Y%m%d`; chomp($def_date_);
LoadProdTradeTimes();
LoadExchTradeTimes();

sub LoadProdTradeTimes
{
  my @prod_trd_hr_files_ = `ls /spare/local/files/*/*-trd-hours.txt`; chomp(@prod_trd_hr_files_);
  foreach my $file_ ( @prod_trd_hr_files_ )
  {
    my $this_file_lines_ = ParseConfigLines ( $file_ );
    foreach my $line_ ( @$this_file_lines_ )
    {
      next if ( @$line_ < 4 );
      my $tz_ = $$line_[3];
      $$line_[1] =~ s/://g;
      $$line_[2] =~ s/://g;

      my $shc_ = $$line_[0];
      my $st_ = $tz_."_".$$line_[1];
      my $et_ = $tz_."_".$$line_[2];
      if ( $$line_[1] >= $$line_[2] ) { $st_ = "PREV_".$st_; }   #For ASX
      $prod_start_end_time_{$$line_[0]} = [ $st_, $et_ ];
    }
  }
}

sub LoadExchTradeTimes
{
  my $file_ = "/spare/local/tradeinfo/exch-trd-hours.txt"; 
  my $this_file_lines_ = ParseConfigLines ( $file_ );
  foreach my $line_ ( @$this_file_lines_ )
  {
    next if ( @$line_ < 3 );
    $exch_start_end_time_{$$line_[0]} = [ $$line_[1], $$line_[2] ];
  }
}

sub GetShcTradeTime
{
  my $shc_ = shift;
  my $date_ = @_ > 0 ? shift : $def_date_ ;

  #exists in prod mapping
  return @{$prod_start_end_time_{$shc_}} if ( exists($prod_start_end_time_{$shc_}) );

  #exists in exch mapping
  my $t_exch_ = GetExchFromSHC($shc_);
  return @{$exch_start_end_time_{$t_exch_}} if ( exists($exch_start_end_time_{$t_exch_}) );

  #port or invalid shc
  return GetPortTradeTime($shc_, $date_) ;
}

sub GetPortTradeTime
{
  my $port_ = shift;
  my $date_ = @_ > 0 ? shift : $def_date_ ;

  #invalid port
  return ( 0000, 2359 ) if !IsValidPort($port_); 

  #exists in prod mapping
  my $key_ = $port_." ".$date_;
  return @{$prod_start_end_time_{$key_}} if ( exists( $prod_start_end_time_{$key_} ) );

  #valid port , find the overlap
  my $st_ = -600;   #starting from -600 to cover for ASX
  my $et_ = 2359;
  foreach my $shc_ ( GetPortConstituents($port_) )
  {
    my ( $t_st_, $t_et_ ) = GetShcTradeTime($shc_, $date_);
    if ( GetTimestampFromTZ_HHMM_DATE( $t_st_, $date_ ) > GetTimestampFromTZ_HHMM_DATE( $st_, $date_ ) ) { $st_ = $t_st_; }
    if ( GetTimestampFromTZ_HHMM_DATE( $t_et_, $date_ ) < GetTimestampFromTZ_HHMM_DATE( $et_, $date_ ) ) { $et_ = $t_et_; }
  }

  $prod_start_end_time_{$key_} = [ $st_ , $et_ ];
  return @{$prod_start_end_time_{$key_}};
}

sub GetShcTradeTimeUTC
{
  my $shc_ = shift;
  my $date_ = @_ > 0 ? shift : $def_date_ ;

  my ( $st_, $et_ ) = GetShcTradeTime($shc_, $date_);
  return ( GetUTCTime($st_, $date_), GetUTCTime($et_, $date_) );
}

1

