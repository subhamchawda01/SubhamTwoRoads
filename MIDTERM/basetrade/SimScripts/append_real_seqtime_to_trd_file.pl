#!/usr/bin/perl
use strict;
use warnings;
use Data::Dumper;
use List::Util qw(sum);

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

if ( $#ARGV < 0 )
{
  print "USAGE: $0 trdfile\n";
  exit(0);
}

my $trdfile = $ARGV[0];

my $f_ts_ = `head -n1 $trdfile | awk '{print \$1}'`; chomp($f_ts_);
my $dt=`date -d \@$f_ts_ +%Y%m%d`; chomp($dt);
#print STDERR $dt."\n";

my $secname = `head -n1 $trdfile | awk '{print \$3}' | awk -F"." '{print \$1}'`; chomp($secname);
#print STDERR $secname."\n";
my $shc=`$BINDIR/get_shortcode_for_symbol $secname $dt`; chomp($shc);
#print STDERR $shc."\n";

my @ors_lines_ = `$BINDIR/ors_binary_reader $shc $dt`; chomp(@ors_lines_);
my %saos_to_seqtime_map_ = ();
my %exec_time_to_seq_time_ = ();

foreach my $line_ ( @ors_lines_ )
{
  my @words_ = split(' ', $line_);

  my $st_idx_ = 9;
  my $dt_idx_ = $st_idx_ + 2;
  my $orr_idx_ = $dt_idx_ + 2;
  my $saos_idx_ = $orr_idx_ + 2;

  next if @words_ < ($saos_idx_+1);

  if ( $words_[$orr_idx_] eq "Seqd" )
  {
    $saos_to_seqtime_map_{$words_[$saos_idx_]} = $words_[$dt_idx_];
  }
  elsif ( $words_[$orr_idx_] =~ m/Exec$/ )
  {
    if ( exists($saos_to_seqtime_map_{$words_[$saos_idx_]}) )
    {
      $exec_time_to_seq_time_{$words_[$st_idx_]} =  $saos_to_seqtime_map_{$words_[$saos_idx_]};
    }
  }
}

open TFILE, " < $trdfile ";
my @trd_lines_ = <TFILE>; chomp(@trd_lines_);
close TFILE;

foreach my $line_ ( @trd_lines_ )
{
  my @words_ = split(' ', $line_);
  next unless $words_[1] eq "OPEN" || $words_[1] eq "FLAT";
  my $seq_time_ = $exec_time_to_seq_time_{$words_[0]};
  print $line_." ".$seq_time_."\n";
}

