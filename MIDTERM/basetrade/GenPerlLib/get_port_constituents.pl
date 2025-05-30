#!/usr/bin/perl

my $PORT_INPUT_FILE = "/spare/local/tradeinfo/PCAInfo/portfolio_inputs";
my $PORT_STDEV_EIGEN_FILE = "/spare/local/tradeinfo/PCAInfo/pca_portfolio_stdev_eigen_DEFAULT.txt";

my %port_constituents = ();
my %port_eigen1_ = ();
my %port_eigen2_ = ();
my %port_stdevs_ = ();

sub load_input {
  my @lines = `cat $PORT_INPUT_FILE | grep ^PLINE | sed 's/^PLINE //'` ;
  foreach my $line ( @lines ) {
    $line =~ s/^\s*(.*?)\s*$/$1/;
    my @parts = split ( /\s+/, $line ) ;
    @{$port_constituents{$parts[0]}} = @parts[1..$#parts];
  }

  my @plines_ = `cat $PORT_STDEV_EIGEN_FILE`;
  foreach my $line ( @plines_ ) {
    $line =~ s/^\s*(.*?)\s*$/$1/;
    my @parts = split ( /\s+/, $line ) ;
    if ( ! exists $port_constituents{$parts[1]} ) { next; }

    if ( $parts[0] eq "PORTFOLIO_STDEV" ) {
      @{$port_stdevs_{$parts[1]}} = @parts[2..$#parts];
    }

    if ( $parts[0] eq "PORTFOLIO_EIGEN" ) {
      if ( $parts[2] eq "1" ) {
        @{$port_eigen1_{$parts[1]}} = @parts[3..$#parts];
      }
      elsif ( $parts[2] eq "2" ) {
        @{$port_eigen2_{$parts[1]}} = @parts[3..$#parts];
      }
    }
  }
}

sub GetPortConstituents{
  if ( @_ < 1 ) { 
    print STDERR "USAGE: GetPortConstituents PORT\n"; 
    return ();
  }
  
  my $shc = shift;
  if ( exists $port_constituents{$shc} ) { return @{$port_constituents{$shc}} ; } 
  else { return (); }
}

sub IsValidPort 
{
  my $port_ = shift;
  if ( exists $port_constituents{$port_} && exists $port_stdevs_{$port_} && exists $port_eigen1_{$port_} && exists $port_eigen2_{$port_} ) {
    return 1;
  } else {
    return 0;
  }
}

load_input();
1;


