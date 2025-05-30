#!/usr/bin/perl
use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # GetDataStartTimeForStrat
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # InsertResults

if ( $#ARGV < 2 )
{
  print "USAGE: $0 shc date result_dir [pnl_samples_dir=INVALIDDIR] [parse_strats=1]\n";
  exit(0);
}

my $shc_ = $ARGV[0];
my $date_ = $ARGV[1];
my $res_dir_ = $ARGV[2];
my $pnl_dir_ = "INVALIDDIR";
my $parse_strats_ = 1;
if ( $#ARGV >= 3 ) { $pnl_dir_ = $ARGV[3]; } 
if ( $#ARGV >= 4 ) { $parse_strats_ = $ARGV[4]; } 
my ($tradingdateyyyy_, $tradingdatemm_, $tradingdatedd_) = BreakDateYYYYMMDD ( $date_ );
my $r_file_ = "$res_dir_/$shc_/$tradingdateyyyy_/$tradingdatemm_/$tradingdatedd_/results_database.txt";

if ( ! -s $r_file_ ) { exit(0); } 

my %strat_to_res_map_ = ();
open RFILE, "< $r_file_" or PrintStacktraceAndDie( "can't open rfile for reading : $r_file_" );
while ( my $line_ = <RFILE> )
{
  my @words_ = split ( ' ', $line_ );  
  if ( $#words_ < 7 )
  {
    if ( $#words_ >= 0 ) { print "ShortPnlStr:  $#words_, $words_[0] , $r_file_ \n" };
    next;
  }
  my $strat_ =  $words_[0];
  splice @words_, 0, 2;
  $strat_to_res_map_{$strat_} = \@words_;
}
close RFILE;

my %strat_to_pnlsample_map_ = ();
if ( $pnl_dir_ ne "INVALIDDIR" )
{
  my $p_file_ = "$pnl_dir_/$shc_/$tradingdateyyyy_/$tradingdatemm_/$tradingdatedd_/results_database.txt";
  if ( -s $p_file_ )
  {
    open PFILE, "< $p_file_" or PrintStacktraceAndDie( "can't open pfile for reading : $p_file_" );
    while ( my $line_ = <PFILE> )
    {
      my @words_ = split ( ' ', $line_ );
      if ( $#words_ < 3 )
      {
        if ( $#words_ >= 0 ) { print "ShortSampleStr:  $#words_, $words_[0] , $p_file_\n" };
        next;
      }
      my $strat_ =  $words_[0];
      splice @words_, 0, 2;
      if (exists($strat_to_res_map_{$strat_}))
      {
        $strat_to_pnlsample_map_{$strat_} = join(" ", @words_ ) ;
      }
    }
    close RFILE;
  } 
}

my $res_ins_ = 0;
foreach my $strat_ ( keys %strat_to_res_map_ )  
{
  if ( $parse_strats_ == 0 ) 
  {
    $res_ins_ += InsertResults ( $strat_, $date_, $strat_to_res_map_{$strat_} , $strat_to_pnlsample_map_{$strat_}, $shc_ ) ;
  }
  else
  {
    $res_ins_ += InsertResults ( $strat_, $date_, $strat_to_res_map_{$strat_} , $strat_to_pnlsample_map_{$strat_} ) ;
  }
}

print "INSERTED [$res_ins_]\n";
