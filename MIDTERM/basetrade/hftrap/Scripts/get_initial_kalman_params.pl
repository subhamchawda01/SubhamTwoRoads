#!/usr/bin/perl

use strict;
use warnings;
use POSIX qw/ceil/;
use List::Util qw/sum/;

my $USAGE="$0 <start_date> <end_date> <instrument_1> <instrument_2> <logfile> <sampling_time> <rolling_reg_duration> <mode> <exec> <adjusted>\n";

if( $#ARGV < 7 ) { print $USAGE." \n"; die; }

my $start_date_ = $ARGV[0];
my $end_date_ = $ARGV[1];
my $instrument1_ = $ARGV[2];
my $instrument2_ = $ARGV[3];
my $logfile_ = $ARGV[4];
my $sampling_time_ = $ARGV[5];
my $rolling_reg_period_ = $ARGV[6];
my $mode_ = $ARGV[7];
my $exec_ = $ARGV[8];
my $use_adjusted_data_ = 0;
if( $#ARGV == 9 )
{
  $use_adjusted_data_ = 1;
}
my @norm_factors_ = ( 1, 10, 100, 1000, 10000, 100000, 1000000 );
my $last_result_line_ = "";
my $pct_diff_ = 0.0;

for( my $ctr_ = 0; $ctr_ <= $#norm_factors_; $ctr_ = $ctr_ + 1 )
{
  my $cmdline_ = ""; 
  if( $use_adjusted_data_ == 0 )
  {
    $cmdline_ = "$exec_ --logfile $logfile_ --start_date $start_date_ --end_date $end_date_ --instrument_1 $instrument1_ --instrument_2 $instrument2_ --sampling_time $sampling_time_  --rolling_reg_period $rolling_reg_period_ --norm_factor $norm_factors_[$ctr_] --mode $mode_";
  }
  else
  {
    $cmdline_ = "$exec_ --logfile $logfile_ --start_date $start_date_ --end_date $end_date_ --instrument_1 $instrument1_ --instrument_2 $instrument2_ --sampling_time $sampling_time_  --rolling_reg_period $rolling_reg_period_ --norm_factor $norm_factors_[$ctr_] --mode $mode_ --use_adjusted_data";
  }
  my $result_line_ = `$cmdline_ 2>/dev/null | egrep -v \"Stop|UTS|Last_bardata\"`;
  chomp $result_line_;
   
  my @result_tokens_ = split(' ',$result_line_);
  chomp @result_tokens_;
                  
  #check to see if last run norm factor was accurate
  if( $result_tokens_[3] > $result_tokens_[5] )
  {
    my $curr_pct_diff_ = $result_tokens_[3]/$result_tokens_[5] - 1.0;
    if( $ctr_ > 0 && $curr_pct_diff_ > $pct_diff_ )  #previous value was a closer fit
    {
      printf "$last_result_line_\n";
      exit;
    }
    else 
    {
      printf "$result_line_\n";
      exit;
    }
  }
  else
  {
    $last_result_line_ = $result_line_;
    $pct_diff_ = 1 - $result_tokens_[3]/$result_tokens_[5];    
  }
}
printf "$last_result_line_ I\n";

