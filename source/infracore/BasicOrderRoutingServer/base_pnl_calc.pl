#!/usr/bin/perl
use strict;
use warnings;

my $USAGE="$0 tradesfilename";
if ( $#ARGV < 0 ) { print "$USAqGE\n"; exit ( 0 ); }
my $trades_filename_ = $ARGV[0];

my $tempfile_for_graphs_ = "_temp_file_for_graphs_.txt";
{
  my $tfilebase = `basename $trades_filename_`; chomp $tfilebase;
  $tempfile_for_graphs_ = "_temp_file_for_graphs_.".$tfilebase.".txt" ;
}

# open tradesfile for reading
open ( TRADESFILE, "< $trades_filename_" ) or die "Could not open $trades_filename_ for reading\n" ;
# open tempfile_for_graphs_ for writing
open ( TEMPFILEGRAPHS, "> $tempfile_for_graphs_" ) or die "Could not open $tempfile_for_graphs_ for writing\n" ;
# read file line by line

my %secname2pos=();
my %secname2pnldollars=();
my %secname2unrealizedpnl=();
my %secname2lastprice=();

while ( my $inline_ = <TRADESFILE> ) 
  {
    # ignore account
    # for each symbol maintain a running position, pnldollars, unrealizedpnl
    my @twords_ = split '\^', $inline_;
    if ( $#twords_ >= 4 ) 
      {
	my $account_ = $twords_[0] ;
	my $secname_ = $twords_[1] ;
	my $buysell_ = int ( $twords_[2] ) ;
	my $size_ = int ( $twords_[3] ) ;
	my $price_ = $twords_[4] ;
	$secname2lastprice{$secname_} = $price_ ;

	if ( $buysell_ == 0 )
	  {
	    $secname2pos{$secname_} += $size_;
	    $secname2pnldollars{$secname_} -= $size_ * $price_ ;
	  }
	else
	  {
	    $secname2pos{$secname_} -= $size_;
	    $secname2pnldollars{$secname_} += $size_ * $price_ ;
	  }
	$secname2unrealizedpnl{$secname_} = $secname2pnldollars{$secname_} + ( $secname2pos{$secname_} * $secname2lastprice{$secname_} ) ;
      }
  }

close ( TEMPFILEGRAPHS ) ;

close ( TRADESFILE ) ;

foreach my $this_secname_ ( keys %secname2unrealizedpnl )
  {
    printf "%s %s\n", $this_secname_, $secname2unrealizedpnl{$this_secname_} ;
  }
