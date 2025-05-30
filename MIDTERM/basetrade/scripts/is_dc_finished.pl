use strict;
use warnings;
use POSIX;
use feature "switch";
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/;
use FileHandle;
use sigtrap qw(handler signal_handler normal-signals error-signals);

my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade"; 
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib"; 
require "$GENPERLLIB_DIR/date_utils.pl"; 

my $time_period_ = $ARGV [ 0 ];
my $exch_timing_ = $ARGV [ 1 ];
#print $time_period_."\n";
my $hh_ = ` date +%H%M`; chomp ( $hh_ );

#print "$time_period_ $exch_timing_ \n";
my $curr_time_ = GetUTCTime($exch_timing_) ;
my $end_time_ = GetUTCTime( GetEndTimeFromTP($time_period_));
#print "$curr_time_ $start_time_ \n";

if ( $curr_time_ > $end_time_ ) {
  print "1";
}
else {
  print "0";
}
