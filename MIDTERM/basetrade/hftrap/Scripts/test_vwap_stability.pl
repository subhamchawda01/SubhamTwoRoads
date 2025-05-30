#!/usr/bin/perl
use List::Util qw(first);
use strict;
use warnings;

if($#ARGV < 6)
{
  print "Usage: <script> <start_date> <end_date> <instrument_1> <instrument_2> <param> <exec> <vwap_duration>\n";
  exit(0);
}

my $start_date_ = $ARGV[0];
my $end_date_ = $ARGV[1];
my $instrument1_ = $ARGV[2];
my $instrument2_ = $ARGV[3];
my $param_ = $ARGV[4];
my $exec_ = $ARGV[5];
my $vwap_duration_ = $ARGV[6];

my $cmd_ = "rm /spare/local/rkumar/log.tws;".$exec_." --paramfile ".$param_." --start_date ".$start_date_." --end_date ".$end_date_." --logfile /spare/local/rkumar/log.tws --instrument_1 ".$instrument1_." --instrument_2 ".$instrument2_." --notify_last_event --trade_file /spare/local/rkumar/tradefile --use_adjusted_data ";
printf "%s\n", $cmd_;
system($cmd_);

my @pxvals = ();
my @tmvals = ();
my @volvals = ();
my @vwap_1 = ();
my @vwap_2 = ();
my @tmvals_1 = ();
my @tmvals_2 = ();

my $first_file_ = "/NAS1/data/NSEBarData/FUT_BarData_Adjusted/".$instrument1_;
my $second_file_ = "/NAS1/data/NSEBarData/FUT_BarData_Adjusted/".$instrument2_;

open FILE_1, "<$first_file_" or die;
while( my $data_line_ = <FILE_1> )
{
  chomp($data_line_);
  my @data_tokens_ = split(' ', $data_line_);
  chomp @data_tokens_;
  if( substr($data_tokens_[0], 0, 1 ) eq "#" )
  {
    next; 
  }
  push @pxvals, $data_tokens_[6];
  push @tmvals, $data_tokens_[0];
  push @volvals, $data_tokens_[9];
}
close FILE_1;

my $sum = 0;
my $vol = 0;
for( my $ctr = 0; $ctr <= $#pxvals; $ctr = $ctr + 1 )
{
  $sum = $sum + $pxvals[$ctr]*$volvals[$ctr];
  $vol = $vol + $volvals[$ctr];
  if( $ctr >= $vwap_duration_ )
  {
    $sum = $sum - $pxvals[$ctr-$vwap_duration_]*$volvals[$ctr-$vwap_duration_];
    $vol = $vol - $volvals[$ctr-$vwap_duration_];
    push @vwap_1, $sum/$vol;
    push @tmvals_1, $tmvals[$ctr-$vwap_duration_+1];
  }
}


@pxvals = ();
@tmvals = ();
@volvals = ();
open FILE_2, "<$second_file_" or die;
while( my $data_line_ = <FILE_2> )
{
  chomp($data_line_);
  my @data_tokens_ = split(' ', $data_line_);
  chomp @data_tokens_;
  if( substr($data_tokens_[0], 0, 1 ) eq "#" )
  {
    next; 
  }
  push @pxvals, $data_tokens_[6];
  push @tmvals, $data_tokens_[0];
  push @volvals, $data_tokens_[9];
}
close FILE_2;

$sum = 0;
$vol = 0;
for( my $ctr = 0; $ctr <= $#pxvals; $ctr = $ctr + 1 )
{
  $sum = $sum + $pxvals[$ctr]*$volvals[$ctr];
  $vol = $vol + $volvals[$ctr];
  if( $ctr >= $vwap_duration_ )
  {
    $sum = $sum - $pxvals[$ctr-$vwap_duration_]*$volvals[$ctr-$vwap_duration_];
    $vol = $vol - $volvals[$ctr-$vwap_duration_];
    push @vwap_2, $sum/$vol;
    push @tmvals_2, $tmvals[$ctr-$vwap_duration_+1];
  }
}

#vwap maps have been initialized. 

#now open and process tradefile -- extracting individual trade information
open FILE_3, "</spare/local/rkumar/log.tws" or die;
my @trade_times_ = ();
my @lotsize1_ = ();
my @lotsize2_ = ();
my @numlots1_ = ();
my @numlots2_ = ();
my @price1_ = ();
my @price2_ = ();
my $initial_nav_ = 0;
my $final_nav_ = 0;

while( my $data_line_ = <FILE_3> )
{
  chomp($data_line_);
  my @data_tokens_ = split(',', $data_line_);
  chomp @data_tokens_;
  #dependent on format of line
  if( $#data_tokens_ == 22 && $data_tokens_[0] > 0 && $data_tokens_[0] < 10000 )
  { 
    if( $initial_nav_ == 0 )
    {
      $initial_nav_ = $data_tokens_[5];
    }
    $final_nav_ = $data_tokens_[6];
  }
 
  if( $#data_tokens_ == 24 && $data_tokens_[0] > 0 && $data_tokens_[0] < 10000 )
  {
    if( $data_tokens_[1] eq "" || $data_tokens_[1] =~ /^\s*$/)
    {  
      push @trade_times_, $data_tokens_[15];
      push @lotsize1_, $data_tokens_[19];
      push @lotsize2_, $data_tokens_[22];
      push @numlots1_, $data_tokens_[20];
      push @numlots2_, $data_tokens_[23];
      push @price1_, $data_tokens_[21];
      push @price2_, $data_tokens_[24]; 
    }
  }
}
close FILE_3;

#process data via new and old price vectors and see performance
my $final_processed_nav_ = $initial_nav_;
my $index_1_ = 0;
my $index_2_ = 0;
for( my $ctr = 0; $ctr <= $#trade_times_; $ctr = $ctr + 1 )
{
  $index_1_ = first {$tmvals_1[$_] >= $trade_times_[$ctr] } 0..$#tmvals_1;
  $index_2_ = first {$tmvals_2[$_] >= $trade_times_[$ctr] } 0..$#tmvals_2;
#  printf "%f %f \n", $vwap_1[$index_1_], $vwap_2[$index_2_];
  $final_processed_nav_ = $final_processed_nav_ - $lotsize1_[$ctr]*$numlots1_[$ctr]*$vwap_1[$index_1_] - $lotsize2_[$ctr]*$numlots2_[$ctr]*$vwap_2[$index_2_] - 0.0003*(abs($lotsize1_[$ctr]*$numlots1_[$ctr]*$vwap_1[$index_1_]) + abs($lotsize2_[$ctr]*$numlots2_[$ctr]*$vwap_2[$index_2_]));
}

my $trade_hist_duration_ = $trade_times_[$#trade_times_ -1] - $trade_times_[0];
printf "Initial Return: %f New Return: %f \n", 100.0*($final_nav_/$initial_nav_ - 1.0), 100.0*($final_processed_nav_/$initial_nav_ - 1.0);



