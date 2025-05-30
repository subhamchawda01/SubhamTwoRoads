#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $REPO="basetrade";

my $CODE_DIR=$HOME_DIR."/".$REPO;
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/di1_utils.pl";

my %shc_exch_map_ = ();
my $def_date_ = `date +%Y%m%d`; chomp($def_date_);

sub load_sch_exch_map
{
  my $dt_ = $def_date_;
  $dt_ = shift if @_ ;
  my $cmd = "$BIN_DIR/get_contract_specs ALL $dt_ EXCHANGE";
  my @lines = `$cmd`; chomp(@lines);
  foreach my $t_line_ (@lines) {
    my @words = split /\s+/, $t_line_ ;
    if ($#words == 1 ) { 
      my $shc_ = $words[0];
      my $exch_ = $words[1];
      $shc_exch_map_{$dt_}{$shc_} = $exch_ ;
    }
  }
};

sub GetExchFromSHC
{
  if ( @_ < 1 )
  {
    print STDERR "USAGE: GetExchFromSHC SHC [YYYYMMDD=TODAY]\n";
    return "";
  }
  else  
  {
    my $shc = shift;
    my $dt_ = $def_date_;
    $dt_ = shift if @_ ;
    load_sch_exch_map($dt_) if !exists($shc_exch_map_{$dt_}) ; 
    if ( exists $shc_exch_map_{$dt_}{$shc} ) { return $shc_exch_map_{$dt_}{$shc}; } 
    else { return ""; }
  }
};

sub IsValidShc
{
  if ( @_ < 1 )
  {
    print STDERR "USAGE: IsValidShc SHC [YYYYMMDD=TODAY]\n";
    return "";
  }
  else  
  {
    my $shc = shift;
    my $dt_ = $def_date_;
    $dt_ = shift if @_ ;
    load_sch_exch_map($dt_) if !exists($shc_exch_map_{$dt_}) ; 
    if ( exists $shc_exch_map_{$dt_}{$shc} ) 
    { 
      my $exch_ = $shc_exch_map_{$dt_}{$shc};
      if (!(
            ( $exch_ eq "CHIX" && $dt_ >= 20151001 ) ||
            ( $exch_ eq "NASDAQ" && $dt_ >= 20131101 ) ||
            ( ExpiredDIContract($shc) )
           ))
      {
        return 1;
      } 
    } 

    return 0; 
  }
};

1
