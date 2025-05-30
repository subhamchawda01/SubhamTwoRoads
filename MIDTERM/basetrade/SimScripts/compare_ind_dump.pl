#!/usr/bin/perl
use strict;
use warnings;
use Data::Dumper;
use List::Util qw(sum);

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

if ( $#ARGV < 1 )
{
  print "USAGE: $0 sim_logfile live_logfile\n";
  exit(0);
}

my $inp_sim_file_ = $ARGV[0];
my $inp_live_file_ = $ARGV[1];

my $uid_=`date +%N`; chomp($uid_);
my $sim_file_="/tmp/comp_ind_sim_".$uid_;
my $live_file_="/tmp/comp_ind_liv_".$uid_;

if ( $inp_sim_file_ =~ m/\.gz$/ )
{
  `zcat $inp_sim_file_ > $sim_file_`
}
else
{
  `cp $inp_sim_file_ $sim_file_`;
}

if ( $inp_live_file_ =~ m/\.gz$/ )
{
  `zcat $inp_live_file_ > $live_file_`
}
else
{
  `cp $inp_live_file_ $live_file_`;
}

open SFILE, " < $sim_file_ ";
my @sim_lines_ = <SFILE>; chomp(@sim_lines_);
close SFILE;

open LFILE, " < $live_file_ ";
my @live_lines_ = <LFILE>; chomp(@live_lines_);
close LFILE;

my %sim_ts_indval_map_ = ();
my %sim_ts_indstr_map_ = ();
my $curr_ts_ = -1;

foreach my $t_line_  (@sim_lines_)
{
  my @words_ = split(' ', $t_line_);
  if ( @words_ >= 2 )
  {
    if ( $words_[0] eq "value:" )
    {
      push( @ { $sim_ts_indval_map_{$curr_ts_} }, $words_[1] );
      push( @ { $sim_ts_indstr_map_{$curr_ts_} }, $t_line_ );
    }
    elsif ( $words_[1] =~ m/DumpIndicatorValues|ShowIndicatorValues$/ )
    {
      $curr_ts_ = $words_[0];
    }
  }
}

my %live_ts_indval_map_ = ();
my %live_ts_indstr_map_ = ();
$curr_ts_ = -1;

foreach my $t_line_  (@live_lines_)
{
  my @words_ = split(' ', $t_line_);
  if ( @words_ >= 2 )
  {
    if ( $words_[0] eq "value:" )
    {
      push( @ { $live_ts_indval_map_{$curr_ts_} }, $words_[1] );
      push( @ { $live_ts_indstr_map_{$curr_ts_} }, $t_line_ );
    }
    elsif ( $words_[1] =~ m/DumpIndicatorValues|ShowIndicatorValues$/ )
    {
      $curr_ts_ = $words_[0];
    }
  }
}

foreach my $ts_ (sort {$a <=> $b} keys %live_ts_indval_map_)
{
  if ( exists($sim_ts_indval_map_{$ts_}) )
  {
    my $sref_ = $sim_ts_indval_map_{$ts_};
    my $lref_ = $live_ts_indval_map_{$ts_};
    if ( $#$sref_ == $#$lref_ )
    {
      my $s_sumvars_ = 0.0;
      my $l_sumvars_ = 0.0;
      for ( my $i=0; $i<=$#$lref_; $i++)
      {
        printf "%s %.3f LIV: %s SIM: %s\n", $ts_, abs($$lref_[$i] - $$sref_[$i]), @ {$live_ts_indstr_map_{$ts_}} [$i], @ {$sim_ts_indstr_map_{$ts_}} [$i];
        $s_sumvars_ += $$sref_[$i];
        $l_sumvars_ += $$lref_[$i];
      }
      printf "%s sumvars: LIV: %.3f SIM: %.3f\n", $ts_, $l_sumvars_, $s_sumvars_;
    }
  }
}

`rm -rf $sim_file_ $live_file_`;

