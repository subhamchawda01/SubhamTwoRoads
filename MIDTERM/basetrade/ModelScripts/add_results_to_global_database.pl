#!/usr/bin/perl

# \file ModelScripts/add_results_to_global_database.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes as input:
# strategylistfile ( a file with lines with just 1 word corresponding to the name of a strategyfile )
# resultslistfile ( a file with multiple lines corresponding to the results on that day for that file )
# tradingdate
#
# Basically after running simulations for a day,
# this script is called,
# for reading the strategylistfile and resultslistfile
# and printing the values in the appropriate file in global_results_base_dir

use strict;
use warnings;
use File::Basename;
use Fcntl qw (:flock);

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # InsertResults
require "$GENPERLLIB_DIR/lock_utils.pl"; # TakeLock, RemoveLock
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

my $USAGE="$0 strategylistfile this_day_resultsfile tradingdate local_results_base_dir|INVALIDDIR [add_to_SQLDB=0] [pnl_samples_resultfile=INVALIDFILE] [local_pnl_samples_base_dir=INVALIDDIR]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $strategylistfile_ = $ARGV[0];
my $resultsfile_ = $ARGV[1];
my $tradingdate_ = $ARGV[2];
my $GLOBALRESULTSBASEDIR = $ARGV[3];
my $add_to_SQLDB  = 0;
my $pnl_samples_resultfile = "INVALIDFILE" ;
my $GLOBALPNLSAMPLESRESULTSBASEDIR = "INVALIDDIR";
if ( $#ARGV >= 4 )
{
  $add_to_SQLDB = $ARGV[4];
}
if ( $#ARGV >= 5 )
{
  $pnl_samples_resultfile = $ARGV[5];
}
if ( $#ARGV >= 6 )
{
  $GLOBALPNLSAMPLESRESULTSBASEDIR = $ARGV[6];
}



my @strategy_filevec_ = ();
open STRATEGYLISTFILE, "< $strategylistfile_" or PrintStacktraceAndDie ( "add_results_to_global_database.pl could not open strategylistfile $strategylistfile_\n" );
while ( my $thisstrategyfile_ = <STRATEGYLISTFILE> ) 
{
  chomp ( $thisstrategyfile_ );
  my @stratwords_ = split ( ' ', $thisstrategyfile_ );
  if ( $#stratwords_ >= 0 )
  {
    my $this_strat_basename_ = basename ( $stratwords_[0] ); chomp ( $this_strat_basename_ );
    push ( @strategy_filevec_, $this_strat_basename_ ) ;
  }
}
close STRATEGYLISTFILE;

my @results_linevec_ = ();
open RESULTSFILE, "< $resultsfile_" or PrintStacktraceAndDie ( "add_results_to_global_database.pl could not open resultsfile $resultsfile_\n" );
while ( my $thisresultline_ = <RESULTSFILE> )
{
  chomp ( $thisresultline_ );
  my @resultslinewords_ = split ( ' ', $thisresultline_ );
  if ( $#resultslinewords_ >= 1 ) 
  { # at least two words
    push ( @results_linevec_, $thisresultline_ );
  }
}
close RESULTSFILE;

my @pnl_samples_linevec_ = ();
if ( $pnl_samples_resultfile ne "INVALIDFILE" )
{
  open PNLSAMPLESFILE, "< $pnl_samples_resultfile" or PrintStacktraceAndDie ( "add_results_to_global_database.pl could not open pnl_samples_resultfile $pnl_samples_resultfile\n" );
  while ( my $thisresultline_ = <PNLSAMPLESFILE> )
  {
    chomp ( $thisresultline_ );
    push ( @pnl_samples_linevec_, $thisresultline_ );
  }
  close PNLSAMPLESFILE;
}
else
{
  @pnl_samples_linevec_ = ( "" ) x ( $#strategy_filevec_ + 1 ) ;
}

if ( $#strategy_filevec_ != $#results_linevec_ )
{
  print "FAILED $strategylistfile_ $resultsfile_\n";
  print STDERR "For strategylistfile $strategylistfile_, and resultfile $resultsfile_: number of strategies $#strategy_filevec_ != number of result lines $#results_linevec_\n";
  exit ( 0 );
}

if ( $#strategy_filevec_ != $#pnl_samples_linevec_ )
{
  print "FAILED $strategylistfile_ $pnl_samples_resultfile\n";
  print STDERR "For strategylistfile $strategylistfile_, and pnl_samples_resultfile $pnl_samples_resultfile: number of strategies $#strategy_filevec_ != number of pnl_sample result lines $#pnl_samples_linevec_\n";
  exit ( 0 );
}

my ($tradingdateyyyy_, $tradingdatemm_, $tradingdatedd_) = BreakDateYYYYMMDD ( $tradingdate_ );

#adding results to directory
if ( $GLOBALRESULTSBASEDIR ne "INVALIDDIR" )
{
  my $resultsfilename_ = $GLOBALRESULTSBASEDIR."/".$tradingdateyyyy_."/".$tradingdatemm_."/".$tradingdatedd_."/results_database.txt";
  
  # if the directory does not exist create it
  {
    my $resultsfilepathdir_ = dirname ( $resultsfilename_ ); chomp ( $resultsfilepathdir_ );
    if ( ! ( -d $resultsfilepathdir_ ) ) 
    {
      `mkdir -p $resultsfilepathdir_`; 
    }
  }
  
  
  # write results to the result file
  TakeLock($resultsfilename_);
  my @existing_results_ = ();
  if ( ExistsWithSize($resultsfilename_) )
  {
    open ( READRESULTSDBFILE, "< $resultsfilename_" ) or PrintStacktraceAndDie( "can't open $resultsfilename_ for reading");
    @existing_results_ = <READRESULTSDBFILE>; chomp(@existing_results_);
    close READRESULTSDBFILE;
  }
  
  open ( RESULTSDBFILE, "> $resultsfilename_" ) or PrintStacktraceAndDie("can't open $resultsfilename_ for writing");
  
  #Print Existing Records again to avoid duplicate entries
  foreach my $t_result_ (@existing_results_)
  {
    my @t_words_ = split ( ' ', $t_result_ );
    if ( ( $#t_words_ >= 3 ) && ( $t_words_[3] > 0 ) && 
        ! grep { $_ eq $t_words_[0] } @strategy_filevec_ )
    {
      print RESULTSDBFILE $t_result_."\n" ;
    }	
  }
  
  my $days_dumped_ = 0;
  for ( my $i = 0 ; $i <= $#strategy_filevec_; $i ++ )
  {
    my $thisresultline_ = $results_linevec_[$i];
    my @resultslinewords_ = split ( ' ', $thisresultline_ );
    if ( ( $#resultslinewords_ >= 1 ) && ( $resultslinewords_[1] > 0 ) )
    { 
      # at least two words and volume is > 0 
      print RESULTSDBFILE $strategy_filevec_[$i]," ",$tradingdate_," ", $results_linevec_[$i],"\n";
      $days_dumped_++;
    }
  }
  close ( RESULTSDBFILE );
  RemoveLock($resultsfilename_);
  print "[$days_dumped_] $resultsfilename_\n";
}

#adding results and pnl_samples to DB
if ( $add_to_SQLDB ne "0" )
{
  my $days_dumped_ = 0;
  for ( my $i = 0 ; $i <= $#strategy_filevec_; $i ++ )
  {
    my $thisresultline_ = $results_linevec_[$i];
    my @resultslinewords_ = split ( ' ', $thisresultline_ );
    my $this_pnl_samples_resultline_ = $pnl_samples_linevec_[$i];

    if ( $add_to_SQLDB ne "1" )
    { #add_to_SQLDB  can be used to convey the shc in case of stir strats for which strat parsing support is not there
      $days_dumped_ += InsertResults ( $strategy_filevec_[$i], $tradingdate_, \@resultslinewords_, $this_pnl_samples_resultline_ , $add_to_SQLDB);
    }
    else
    {
      $days_dumped_ += InsertResults ( $strategy_filevec_[$i], $tradingdate_, \@resultslinewords_, $this_pnl_samples_resultline_ );
    }
  }    
  print "[$days_dumped_] DB\n";
}

#adding pnl_samples to directory
if ( $GLOBALPNLSAMPLESRESULTSBASEDIR ne "INVALIDDIR" )
{
  my $pnl_samples_resultsfilename_ = $GLOBALPNLSAMPLESRESULTSBASEDIR."/".$tradingdateyyyy_."/".$tradingdatemm_."/".$tradingdatedd_."/results_database.txt";
# if the directory does not exist create it
  {
    my $pnl_samples_resultsfilepathdir_ = dirname ( $pnl_samples_resultsfilename_ ); chomp ( $pnl_samples_resultsfilepathdir_ );
    if ( ! ( -d $pnl_samples_resultsfilepathdir_ ) ) 
    {
      `mkdir -p $pnl_samples_resultsfilepathdir_`; 
    }
  }

  TakeLock($pnl_samples_resultsfilename_);
  my @existing_pnl_samples_ = ();
  if ( ExistsWithSize($pnl_samples_resultsfilename_) )
  {
    open ( READPNLSAMPLESFILE, "< $pnl_samples_resultsfilename_" ) or PrintStacktraceAndDie("can't open $pnl_samples_resultsfilename_ for reading\n");
    @existing_pnl_samples_ = <READPNLSAMPLESFILE>; chomp(@existing_pnl_samples_);
    close READPNLSAMPLESFILE;
  }

  open ( PNLSAMPLESRESULTSDBFILE, "> $pnl_samples_resultsfilename_" ) or PrintStacktraceAndDie("can't open $pnl_samples_resultsfilename_ for writing\n");

  #Print Existing Records again to avoid duplicate entries
  foreach my $t_pnl_sample (@existing_pnl_samples_)
  {
    my @t_words_ = split ( ' ', $t_pnl_sample );
    if ( ( $#t_words_ >= 3 ) && 
        ! grep { $_ eq $t_words_[0] } @strategy_filevec_ )
    {
      print PNLSAMPLESRESULTSDBFILE $t_pnl_sample."\n" ;
    }	
  }

  my $days_dumped_ = 0;
  for ( my $i = 0 ; $i <= $#strategy_filevec_; $i ++ )
  {
    my $this_pnl_samples_resultline_ = $pnl_samples_linevec_[$i];
    my @this_pnl_samples_resultwords_ = split ( ' ', $this_pnl_samples_resultline_ );
    if ( $#this_pnl_samples_resultwords_ >= 1 )
    { 
      #atleast 2 words, which is atleast one pnl_sample
      print PNLSAMPLESRESULTSDBFILE $strategy_filevec_[$i]," ",$tradingdate_," ", $pnl_samples_linevec_[$i],"\n";
      $days_dumped_++;
    }
  }
  close ( PNLSAMPLESRESULTSDBFILE );
  RemoveLock($pnl_samples_resultsfilename_);
  print "[$days_dumped_] $pnl_samples_resultsfilename_\n";
}
