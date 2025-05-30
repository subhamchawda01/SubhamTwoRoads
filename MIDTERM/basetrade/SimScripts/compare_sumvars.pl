#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

if ( $#ARGV < 1 ) 
{
  print "$0 simfile livefile [mpi=1]\n";
  exit 0;
}

my $sim_file_ = $ARGV[0];
my $live_file_ = $ARGV[1];
my $mpi_ = 1;
if ( $#ARGV >= 2 )
{
  $mpi_ = $ARGV[2] + 0;
}

my @sim_sumvars_ = ();
my @live_sumvars_ = ();

if ( $sim_file_ =~ m/\.gz$/ )
{
  @sim_sumvars_ = `zgrep tgtbias: $sim_file_ | awk '{print \$1, \$NF}' | sed 's/tgtbias:/ /g'`; chomp(@sim_sumvars_);
}
else
{
  @sim_sumvars_ = `grep tgtbias: $sim_file_ | awk '{print \$1, \$NF}' | sed 's/tgtbias:/ /g'`; chomp(@sim_sumvars_);
}

if ( $live_file_ =~ m/\.gz$/ )
{
  @live_sumvars_ = `zgrep tgtbias: $live_file_ | awk '{print \$1, \$NF}' | sed 's/tgtbias:/ /g'`; chomp(@live_sumvars_);
}
else
{
  @live_sumvars_ = `grep tgtbias: $live_file_ | awk '{print \$1, \$NF}' | sed 's/tgtbias:/ /g'`; chomp(@live_sumvars_);
}

my %sim_ts_to_sumvars_ ;
my %live_ts_to_sumvars_ ;

foreach my $line_ ( @sim_sumvars_ )
{
  my @words_ = split(' ', $line_);
  $sim_ts_to_sumvars_{$words_[0]} = $words_[1];
} 

foreach my $line_ ( @live_sumvars_ )
{
  my @words_ = split(' ', $line_);
  $live_ts_to_sumvars_{$words_[0]} = $words_[1];
} 

my $sim_idx_ = 0;
my $live_idx_ = 0;
  
my ( $sim_ts_, $sim_sv_ ) = split( ' ', $sim_sumvars_[$sim_idx_] );
my ( $live_ts_, $live_sv_ ) = split( ' ', $live_sumvars_[$live_idx_] );

while ( $sim_idx_ <= $#sim_sumvars_ and $live_idx_ <= $#live_sumvars_ )
{
  ( $sim_ts_, $sim_sv_ ) = split( ' ', $sim_sumvars_[$sim_idx_] );
  ( $live_ts_, $live_sv_ ) = split( ' ', $live_sumvars_[$live_idx_] );
  $sim_sv_ /= $mpi_;
  $live_sv_ /= $mpi_;

  if ( $sim_ts_ < $live_ts_ )
  {
    print $sim_ts_." SIM: ".$sim_sv_."\n";
    $sim_idx_+=1;
  }
  elsif( $live_ts_ < $sim_ts_ )
  {
    print $live_ts_." LIV: ".$live_sv_."\n";
    $live_idx_+=1;
  }
  else # ( $live_ts_ == $sim_ts_ )
  {
    printf "%s LIV: %s SIM: %s DIF: %.6f\n" , $live_ts_, $live_sv_, $sim_sv_, abs($sim_sv_-$live_sv_);
    $live_idx_+=1;
    $sim_idx_+=1
  }
}

for(; $sim_idx_<=$#sim_sumvars_; $sim_idx_++)
{
  ( $sim_ts_, $sim_sv_ ) = split( ' ', $sim_sumvars_[$sim_idx_] );
  $sim_sv_ /= $mpi_;
  print $sim_ts_." SIM: ".$sim_sv_."\n";
}

for(; $live_idx_<=$#live_sumvars_; $live_idx_++)
{
  ( $live_ts_, $live_sv_ ) = split( ' ', $live_sumvars_[$live_idx_] );
  $live_sv_ /= $mpi_;
  print $live_ts_." SIM: ".$live_sv_."\n";
}
