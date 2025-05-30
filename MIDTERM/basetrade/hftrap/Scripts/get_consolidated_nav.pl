#!/usr/bin/perl
use List::Util qw(first);
use FileHandle;
use strict;
use warnings;

if($#ARGV < 1)
{
  print "Usage: <logfile_file> <output_file>\n";
  exit(0);
}

my $logfile_file_ = $ARGV[0];
my $output_file_ = $ARGV[1];
#defines
my $sample_granularity = 60;

#need log files in better format, i.e. a file with start and end time 
#in secs for the strategy alongwith logfile of strat run and ordered by time, eg
#20100401 20110331 file1 
#20100401 20110331 file2

my %file_start_time_map_ = ();
my %file_end_time_map_ = ();
my @filenames_ = ();
open FILE_1, "<$logfile_file_" or die "Could not open $logfile_file_\n";
while( my $file_line_ = <FILE_1> )
{
  chomp $file_line_;
  my @line_tokens_ = split(' ', $file_line_);
  push @filenames_, $line_tokens_[2];
  my $tdate_ = `date --date=\"$line_tokens_[0]\" +%s`;
  chomp $tdate_;
  $file_start_time_map_{$line_tokens_[2]} = $tdate_;
  $tdate_ = `date --date=\"$line_tokens_[1]\" +%s`;
  chomp $tdate_;
  $tdate_ = $tdate_ + 86300;
  $file_end_time_map_{$line_tokens_[2]} = $tdate_;
}
close FILE_1;

#get sorted list of all timestamps in logfiles
my $all_filename_list_ = join(' ', @filenames_);
my $sh_cmd_ = "cat ".$all_filename_list_." | awk -F\, '{ if(NF == 14 && \$1 >= 631152000 && \$1 <= 1577836800 ){ print \$1 } }' | sort --key=1 -g | uniq";
my @all_tstamp_vec_ = `$sh_cmd_`;

#initialize comb pnl map to 0
my %comb_pnl_ = ();
for( my $ctr = 0; $ctr <= $#all_tstamp_vec_; $ctr = $ctr + 1 )
{
  $comb_pnl_{$all_tstamp_vec_[$ctr]} = 0.0;
}

for ( my $ctr = 0; $ctr <= $#filenames_; $ctr = $ctr + 1 )
{
  my $t_curr_file_ = $filenames_[$ctr];
  #find time during which strategy ran
  my $tstamp_vec_start_offset_ = first {$all_tstamp_vec_[$_] >= $file_start_time_map_{$filenames_[$ctr]} } 0..$#all_tstamp_vec_;
  my $tstamp_vec_offset_ = $tstamp_vec_start_offset_;

  my $this_file_last_nav_ = 0;
  open FILE, "<$t_curr_file_" or printf "Could not open $t_curr_file_\n";
  while( my $file_line_ = <FILE> )
  {
    chomp $file_line_;
    my @tokens_ = split(/[,\s]+/, $file_line_);
    if( $#tokens_ == 13 && $tokens_[0] >= 631152000 && $tokens_[0] <= 1577836800 )
    {
      my $t_ctime_ = $tokens_[0];
      while( $tstamp_vec_offset_ <= $#all_tstamp_vec_ && $t_ctime_ >= $all_tstamp_vec_[$tstamp_vec_offset_] )
      {
	$comb_pnl_{$all_tstamp_vec_[$tstamp_vec_offset_]} = $comb_pnl_{$all_tstamp_vec_[$tstamp_vec_offset_]} + $tokens_[11]; 
	$tstamp_vec_offset_ = $tstamp_vec_offset_ + 1;
      }
      $this_file_last_nav_ = $tokens_[11];
    }
  }
  close FILE;
  while( $tstamp_vec_offset_ <= $#all_tstamp_vec_ && $all_tstamp_vec_[$tstamp_vec_offset_] <= $file_end_time_map_{$filenames_[$ctr]} )
  {
    $comb_pnl_{$all_tstamp_vec_[$tstamp_vec_offset_]} = $comb_pnl_{$all_tstamp_vec_[$tstamp_vec_offset_]} + $this_file_last_nav_;
    $tstamp_vec_offset_ = $tstamp_vec_offset_ + 1;
  }
}

#handle NAV reallocation
#get pnl values at beginning and end of each subperiod
my @start_times_ = values %file_start_time_map_; #check and remove dups - TODO
@start_times_ = sort @start_times_;
my @end_times_ = values %file_end_time_map_;
@end_times_ = sort @end_times_;

my @unique_start_times_ = ();
my @unique_end_times_ = ();

my @start_navs_ = ();
my @end_navs_ = ();
my $st_seen_ = 0;

for( my $ctr = 0; $ctr <= $#start_times_; $ctr = $ctr + 1 )
{
  if( $start_times_[$ctr] == $st_seen_ )
  {
    next;
  }
  my $soffset_ = first {$all_tstamp_vec_[$_] >= $start_times_[$ctr] } 0..$#all_tstamp_vec_;
  my $eoffset_ = first {$all_tstamp_vec_[$_] > $end_times_[$ctr] } 0..$#all_tstamp_vec_;
  push @start_navs_, $comb_pnl_{$all_tstamp_vec_[$soffset_]};
  if( ! defined $eoffset_  ) #last end time
  {
    push @end_navs_, $comb_pnl_{$all_tstamp_vec_[$#all_tstamp_vec_]};    
  }
  else
  {
    push @end_navs_, $comb_pnl_{$all_tstamp_vec_[$eoffset_-1]};
  }
  $st_seen_ = $start_times_[$ctr];
  push @unique_start_times_, $start_times_[$ctr];
  push @unique_end_times_, $end_times_[$ctr];
}


#for ( my $ctr = 0; $ctr <= $#end_navs_; $ctr = $ctr + 1 )
#{
#  printf "%f %f %d %d \n", $start_navs_[$ctr], $end_navs_[$ctr], $unique_start_times_[$ctr], $unique_end_times_[$ctr];
#}
#multiply NAV series appropriately
my $mult_factor_ = 1.0;
my $tstamp_vec_offset_ = 0;

for( my $ctr = 1; $ctr <= $#unique_end_times_; $ctr = $ctr + 1 )
{
  while( $all_tstamp_vec_[$tstamp_vec_offset_] < $unique_start_times_[$ctr] )
  {
    $tstamp_vec_offset_ = $tstamp_vec_offset_ + 1;   
  }
  $mult_factor_ = $mult_factor_*$end_navs_[$ctr-1]/$start_navs_[$ctr];
#  printf "MultFact: %f at time %d\n", $mult_factor_, $all_tstamp_vec_[$tstamp_vec_offset_];
  while( ( $ctr < $#unique_end_times_ && $all_tstamp_vec_[$tstamp_vec_offset_] < $unique_start_times_[$ctr+1] )
        || ($ctr == $#unique_end_times_ && $tstamp_vec_offset_ <= $#all_tstamp_vec_ && $all_tstamp_vec_[$tstamp_vec_offset_] <= $unique_end_times_[$ctr]) )
  {
    $comb_pnl_{$all_tstamp_vec_[$tstamp_vec_offset_]} = $mult_factor_*$comb_pnl_{$all_tstamp_vec_[$tstamp_vec_offset_]};
    $tstamp_vec_offset_ = $tstamp_vec_offset_ + 1;
  }
}

for( my $ctr = 0; $ctr < $#all_tstamp_vec_; $ctr = $ctr + 1 )
{
  if(($comb_pnl_{$all_tstamp_vec_[$ctr+1]} < $comb_pnl_{$all_tstamp_vec_[$ctr]}*0.9) || ($comb_pnl_{$all_tstamp_vec_[$ctr+1]} > 1.1*$comb_pnl_{$all_tstamp_vec_[$ctr]}))
  {
    printf "%d %f %f\n", $all_tstamp_vec_[$ctr], $comb_pnl_{$all_tstamp_vec_[$ctr]}, $comb_pnl_{$all_tstamp_vec_[$ctr+1]}
  }
}

#output stats and data
my $max_nav_seen_ = 0;
my $max_dd_seen_ = 0;
my $stdev_comp_interval_ = 60;
my $count_ = 0;
my $sum_x_ = 0;
my $sum_xsqr_ = 0;
open FILE, ">$output_file_" or die "Could not open $output_file_\n";
for( my $ctr = $stdev_comp_interval_; $ctr <= $#all_tstamp_vec_; $ctr = $ctr + 1 )
{
  if( $comb_pnl_{$all_tstamp_vec_[$ctr]} > $max_nav_seen_ )
  {
    $max_nav_seen_ =  $comb_pnl_{$all_tstamp_vec_[$ctr]};
  }
  if( 1.0 - $comb_pnl_{$all_tstamp_vec_[$ctr]} /$max_nav_seen_  > $max_dd_seen_ )
  {
    $max_dd_seen_ = 1.0 - $comb_pnl_{$all_tstamp_vec_[$ctr]} /$max_nav_seen_;
  }
  my $t_ret_ = $comb_pnl_{$all_tstamp_vec_[$ctr]}/$comb_pnl_{$all_tstamp_vec_[$ctr-$stdev_comp_interval_]} - 1.0;
  $count_ = $count_ + 1;
  $sum_x_ = $sum_x_ + $t_ret_;
  $sum_xsqr_ = $sum_xsqr_ + $t_ret_*$t_ret_;
  printf FILE "%d %f\n", $all_tstamp_vec_[$ctr], $comb_pnl_{$all_tstamp_vec_[$ctr]};
}
close FILE;

my $ann_ret_ = (($comb_pnl_{$all_tstamp_vec_[$#all_tstamp_vec_]}/$comb_pnl_{$all_tstamp_vec_[0]})**(365.0*86400/($all_tstamp_vec_[$#all_tstamp_vec_-1] - $all_tstamp_vec_[0])) - 1.0 )*100.0;
my $ann_stdev_ = 100.0*sqrt($sum_xsqr_/$count_ - $sum_x_/$count_*$sum_x_/$count_)*sqrt(252*86400*(6.5/24)/($stdev_comp_interval_*60));
printf "Annualized Return: %f\n", $ann_ret_;

printf "Annualized Sharpe: %f \n", $ann_ret_/$ann_stdev_;
printf "Annualized Calmar: %f\n", $ann_ret_/($max_dd_seen_*100.0);

#open (GP, "|gnuplot -persist ") or die "no gnuplot";
#GP->autoflush(1);
#print GP "plot \'$output_file_\' using 2 with lines";
#print GP " \n";
#close GP;

