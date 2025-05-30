#!/usr/bin/perl
use strict;
use warnings;
use List::Util qw /min max/;
use Fcntl qw (:flock);

use Digest::MD5 qw(md5 md5_hex md5_base64);
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $USAGE = "<script> shc strats_dir result_dir [short_term=10] [long_terms_csv=60,30] [oml_setting_csv=60,0.1,3] [st_sort_fn=kCNAPnlSharpeAverage] [lt_sort_fn=kCNAPnlSharpeAverage] [skip_file=INVALIFILE]\n";

if ( $#ARGV < 2 )
{
  print $USAGE;
  exit (0);
}

my $shc = shift; 
my $strats_dir = shift;
my $result_dir = shift;
my $sfreq = 10; 
my @lfreqs = (60, 30); 
my $oml_setting = "60 0.1 3";
my $st_sort_fn = "kCNAPnlSharpeAverage";
my $lt_sort_fn = "kCNAPnlSharpeAverage";
my $skip_file = "INVALIDFILE";
my $ed = `date  +%Y%m%d`; chomp($ed);

if ( $#ARGV >= 0  ) { $sfreq = shift; }
if ( $#ARGV >= 0  ) { my $t_str_ = shift; @lfreqs = split ( "," , $t_str_ ); chomp(@lfreqs); }
if ( $#ARGV >= 0  ) { my $t_str_ = shift; my @t_str_words = split ( "," , $t_str_ ); chomp(@t_str_words); $oml_setting = join( " ", @t_str_words ); }
if ( $#ARGV >= 0  ) { $st_sort_fn = shift; }
if ( $#ARGV >= 0  ) { $lt_sort_fn = shift; }
if ( $#ARGV >= 0  ) { $skip_file = shift; }

my $sd=`~/basetrade_install/bin/calc_prev_week_day $ed $sfreq`; 
my $summ_exec = $HOME_DIR."/basetrade_install/bin/summarize_strategy_results";
my $summ_exec_cmd = "$summ_exec $shc $strats_dir $result_dir $sd $ed $skip_file $st_sort_fn 0 INVALIDFILE 0 2>/dev/null";
my @output = `$summ_exec_cmd`; chomp(@output);

my @strats = ();
my @omls = ();
my %strat_to_st_results;
my $curr_strat = "";
foreach my $line(@output)
{
  my @words = split(' ', $line);
  if ( $#words >= 1 && $words[0] eq "STRATEGYFILEBASE" )
  {
    $curr_strat = $words[1];
    push ( @strats, $curr_strat );

    my $oml_cmd = "$HOME_DIR/basetrade/scripts/oml_short.sh $shc $curr_strat $oml_setting $skip_file $ed 2>/dev/null";
    my $oml_out = `$oml_cmd`; chomp ( $oml_out );
    push ( @omls, $oml_out  );
  }
  elsif ( $#words > 5 && $words[0] eq "STATISTICS" )
  {
    $line = $line." ".$omls[-1];
    push ( @ { $strat_to_st_results{$curr_strat} }, $line ) ;
  }
  elsif ( $#words > 5 )
  {
    push ( @ { $strat_to_st_results{$curr_strat} }, $line ) ;
  }
}

my %strat_to_lt_results;
my $num_strats = $#strats + 1;
foreach my $lt_dur ( @lfreqs )
{
  my $t_sd=`~/basetrade_install/bin/calc_prev_week_day $ed $lt_dur`; 
  my $summ_exec_cmd = "$summ_exec $shc $strats_dir $result_dir $t_sd $ed $skip_file $lt_sort_fn 0 INVALIDFILE 0 2>/dev/null";
  my @output = `$summ_exec_cmd`; chomp(@output);

  my $curr_strat = "";
  my $num_days_res = 0;
  my $curr_rank = 0;
  foreach my $line(@output)
  {
    my @words = split(' ', $line);
    if ( $#words >= 1 && $words[0] eq "STRATEGYFILEBASE" )
    {
      $curr_strat = $words[1];
      $num_days_res = 0;
      $curr_rank += 1;
    }
    elsif ( $#words > 5 && $words[0] eq "STATISTICS" )
    {
      $words[0] = "$lt_dur($num_days_res) DAYS: $curr_rank/$num_strats";
      my $t_line = join ( ' ' , @words );
      push ( @ { $strat_to_lt_results{$curr_strat} }, $t_line ) ;
    }
    elsif ( $#words > 5 )
    {
      $num_days_res += 1;
    }
  }
}

foreach my $curr_strat(@strats)
{
  print "STRATEGYFILEBASE $curr_strat\n";
  print join( "\n", @ { $strat_to_st_results{$curr_strat} } )."\n"; 
  print join( "\n", @ { $strat_to_lt_results{$curr_strat} } )."\n\n"; 
}

