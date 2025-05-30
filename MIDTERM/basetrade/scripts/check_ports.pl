#!/usr/bin/perl
use strict;
use warnings;
use Fcntl qw (:flock);

use Digest::MD5 qw(md5 md5_hex md5_base64);
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl"; # GetExchFromSHC
require "$GENPERLLIB_DIR/get_port_constituents.pl"; # GetPortConstituents
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ($#ARGV < 0)
{
  print "USAGE: <script> <eigenfile>\n";
  exit 0;
}

my %stdev_map_;

my $STDEV_FILE="/spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt";
open STDEV_FILEHANDLE, "< $STDEV_FILE" or PrintStacktraceAndDie ( "could not open $STDEV_FILE" );	
while(my $line = <STDEV_FILEHANDLE>)
{
  chomp($line);
  my @words = split(' ', $line);
  if ( $#words >= 2 && $words[0] eq "SHORTCODE_STDEV")
  {
    $stdev_map_{$words[1]} = $words[2];
  }
}
close(STDEV_FILEHANDLE);

my %pca_map_;
my $PCA_FILE=$ARGV[0];
open PCA_FILEHANDLE, "< $PCA_FILE" or PrintStacktraceAndDie ( "could not open $PCA_FILE" );	
while(my $line = <PCA_FILEHANDLE>)
{
  chomp($line);
  my @words = split(' ', $line);
  if ( $#words >= 5 && $words[0] eq "PORTFOLIO_EIGEN" && $words[2] eq "1" )
  {
    my $port_ = $words[1];
    next if ( index($port_,"HYB_") == 0 );
    $pca_map_{$port_} = ();
    for (my $i=4; $i<=$#words; $i++)
    {
      push ( @ { $pca_map_ { $port_ } }, $words[$i]);
    }
  }
}
close(PCA_FILEHANDLE);

my $PORT_FILE="/spare/local/tradeinfo/PCAInfo/portfolio_inputs";
open PORT_FILEHANDLE, "< $PORT_FILE" or PrintStacktraceAndDie ( "could not open $PORT_FILE" );	
while(my $line = <PORT_FILEHANDLE>)
{
  chomp($line);
  my @words = split(' ', $line);
  if ( $#words >= 3 && $words[0] eq "PLINE" )
  {
    my $port_ = $words[1];
    next if ( index($port_,"HYB_") == 0 );
    if ( ! exists($pca_map_{$port_}) )
    {
      print "port: $port_ not in eigen file\n";
      next;
    }
    my @pca_wts_ = @ { $pca_map_{$port_} };
      
#print "$port_ $#words $#pca_wts_\n";
    if ( $#words != $#pca_wts_+2)
    {
      print "INVALID: $port_ $#words $#pca_wts_\n";
      next;
    }
    my $sum_=0;
    for(my $i=0; $i<=$#pca_wts_; $i++)
    {
#      print "SHC : $words[$i+2]\n";
      $sum_ += $pca_wts_[$i]/$stdev_map_{$words[$i+2]};
    }
    if ( $sum_ < 0.0 )
    {
      print "-ve: $port_ $sum_\n";
    }
    if ( abs($sum_) < 0.1 )
    {
      print "small: $port_ $sum_\n";
    }
    print "NORMAL: $port_ $sum_\n";
  }
}
close(PORT_FILEHANDLE);


