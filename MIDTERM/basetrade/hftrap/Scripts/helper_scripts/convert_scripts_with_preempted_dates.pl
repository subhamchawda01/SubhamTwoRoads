#!/usr/bin/perl
use strict;
use warnings;
#input file 1 is list of exec commands of form
#~/basetrade_install/bin/spread_exec --paramfile /spare/local/rkumar/param_tamo_dvr_sample_hft --start_date 20100801 --end_date 20110731 --logfile /spare/local/rkumar/log_dir_tranche/tranche8/log.UNIONBANK_PNB_20100801_20110731  --instrument_1 UNIONBANK --instrument_2 PNB --notify_last_event --trade_file /spare/local/rkumar/tradefile --use_adjusted_data

#input file 2 has start and (preempted) end dates for pairs
#UNIONBANK_BANKBARODA 20080701 20090701

if( $#ARGV < 1 )
{
  die "Usage: <script> <bash_script_to_run> <preempted_date_file>\n";
}

my $script_file_ = $ARGV[0];
my $pre_date_file_ = $ARGV[1];

open FILE, "<$script_file_" or die "Could not open $script_file_\n";
while( my $line = <FILE> )
{
  chomp($line);
  my @tokens = split(' ', $line);
  if( $#tokens > 5 )
  {
    my $pair = $tokens[10]."_".$tokens[12];
    my $sdate = $tokens[4];
    my $edate = $tokens[6];
    open FILE_2, "<$pre_date_file_" or die "Could not open $pre_date_file_\n";
    while( my $line_2 = <FILE_2> )
    {
      chomp $line_2;
      my @tokens_2 = split(' ', $line_2);
      if($#tokens_2 == 2 && $tokens_2[0] eq $pair && $tokens_2[1] == $sdate )
      {
        if( $tokens_2[2] < $edate )
        {
          $tokens[6] = $tokens_2[2];
          $line = join(' ',@tokens);
        }
      }
    }
    close FILE_2;
  }
  printf "%s\n", $line;
}
