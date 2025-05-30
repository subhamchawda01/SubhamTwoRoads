#!/usr/bin/env perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

#get_event_name 
sub getEventName {
  my $file_name="/spare/local/tradeinfo/TradedEconomicEvents/traded_economic_events.txt";
  my ($value_to_compare)=@_;
  
  open FHANDLE, "<".$file_name or PrintStacktraceAndDie ( "Could not open $file_name for reading\n" );
  
  my @final_list = ( );
  while (my $row = <FHANDLE>){
    chomp $row;
    my @tokens_ = split(" ",$row);
    if ($tokens_[0] eq $value_to_compare and $tokens_[2] ne "IGNORE_ALL"){
      my $event_string_ = "$tokens_[1] $tokens_[2]";
      push(@final_list,$event_string_);
    }
  }
  return @final_list;
}

sub getDatesForEvent {
  my ($event_to_extract)=@_;
  
  my $date_log_file= "/home/dvctrader/infracore/SysInfo/BloombergEcoReports/merged_eco_*_processed.txt";
  my $grep_cmd="grep -h \"$event_to_extract\" $date_log_file | cut -d' ' -f5 | cut -d'_' -f1";
  #print $grep_cmd,"\n";
  my @date_list=`$grep_cmd 2>/dev/null`; chomp (@date_list);

  return @date_list;
}

sub main{
  my ($event_name,$start_date,$end_date)=@_;
  my @list_of_events = getEventName ($event_name);

  my @dates_vec_ = ( );
  push ( @dates_vec_, getDatesForEvent($_) ) foreach @list_of_events;
  my %dates_map_ = map { $_ => 1 } @dates_vec_;
  @dates_vec_ = grep { $_ >= $start_date && $_ <= $end_date } sort keys %dates_map_;

  print $_."\n" foreach @dates_vec_;
}

my $USAGE = "$0 TRADED_EZONE START_DATE END_DATE";
if ( $#ARGV < 2 ) { print "$USAGE\n"; exit ( 0 ); }

my ($event_sym, $start_date, $end_date) = @ARGV;
main($event_sym,$start_date,$end_date);
