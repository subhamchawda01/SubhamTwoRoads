#!/usr/bin/perl

# \file scripts/copy_staged_strat_results_to_globresults.pl
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

my $USAGE="$0 prod strategyfile staged_result_dir globalresultsdir local[Y/N] verbose[0/1, default:1]";
my $HOSTNAME = `hostname`; chomp ( $HOSTNAME );

if ( $#ARGV < 4 ) { print $USAGE."\n";exit; }
my $prod_ = $ARGV[0];
my $strat_name_ = $ARGV[1];
my $staged_results_dir_ = $ARGV[2];
my $global_results_base_dir_ = $ARGV[3];
my $is_local_copy_ = $ARGV[4] ;
my $verbose = 1;
if ($#ARGV > 4) { $verbose = $ARGV[5]; }

if  ( index( $HOSTNAME, 'ip-10-0' ) < 0 && $is_local_copy_ eq 'N' )
{
  print "This is not ec2, Copying results might be overwritten\n";
  exit;
}
$staged_results_dir_ = $staged_results_dir_."/".$prod_."/";
$global_results_base_dir_= $global_results_base_dir_."/".$prod_."/";

opendir DIR, $staged_results_dir_ or PrintStacktraceAndDie ( "Could not open product directory $staged_results_dir_ for reading.");
my @yearlist_ = readdir ( DIR ) ;
closedir ( DIR );

foreach my $year_ ( @yearlist_ ) 
{
  if ( $year_ eq ".." or $year_ eq "." ) { next ; }
  $year_ = $staged_results_dir_."/".$year_;
  if ( $verbose == 1) { print "this Year : " . $year_ ."\n"; }
  opendir YEAR, $year_ or PrintStacktraceAndDie ( "Could not open year directory $year_ for reading ") ;
  my @monthlist_ = readdir ( YEAR ) ;
  closedir ( YEAR );
  foreach my $month_ ( @monthlist_ ) 
  {
    if ( $month_ eq ".." or $month_ eq "." ) { next ; } 
    $month_ = $year_."/".$month_ ;
    if ( $verbose == 1) { print "This month: " . $month_."\n"; }
    opendir MONTH, $month_ or PrintStacktraceAndDie ( "Could not open month directory - $month_ for reading ") ;
    my @datelist_ = readdir ( MONTH ) ;
    closedir ( MONTH ) ;
    foreach my $date_  ( @datelist_ ) 
    {
      if ( $date_ eq ".." or $date_ eq "." ) { next;} 
      $date_ = $month_."/".$date_;
      my $source_results_file_ = $date_."/results_database.txt";
      my $dest_results_file_ = $source_results_file_ ;
      $dest_results_file_ =~ s/$staged_results_dir_/$global_results_base_dir_/g;
      if ( $verbose == 1) { print $source_results_file_."\n".$dest_results_file_."\n" ; }
      my $dir_ = `dirname $dest_results_file_ `; chomp ( $dir_ ); `mkdir -p $dir_ `;
      open SOURCE_FILE, "< $source_results_file_ " or PrintStacktraceAndDie (" Could not open the source result $source_results_file_ file..");
      my @results_ = <SOURCE_FILE>;
      close ( SOURCE_FILE ) ;
      if ( -e $dest_results_file_ ) 
      {
        open DEST_READ_FILE, "< $dest_results_file_ " or PrintStacktraceAndDie ( "Could not open the dest file $dest_results_file_ fro reading.. " ) ;
        my @dest_file_cont_ = <DEST_READ_FILE>;
        my $res_exist_ = "";
        foreach my $dest_file_line_ ( @dest_file_cont_ )
        {
          my @dest_file_line_words_ = split ( ' ', $dest_file_line_  ) ;
          if ( $dest_file_line_words_[0] eq $strat_name_ ) 
          {
            my $yyyy = `basename $year_`; chomp($yyyy); my $mm = `basename $month_`; chomp($mm); my $dd = `basename $date_`; chomp($dd);
            print "Staged_Results Exist for $yyyy/$mm/$dd for $strat_name_\n";
            $res_exist_="true";
            last; 
          }
        }
        close ( DEST_READ_FILE ) ;
        if ( $res_exist_ ) { next ;}
      }

      open DEST_FILE , ">> $dest_results_file_ " or PrintStacktraceAndDie ( "Could not open the dest res $dest_results_file_ for writing.. " ) ;
      for my $resline_ ( @results_ ) 
      {
        my @resline_words_ = split (' ',$resline_);
        if ( $resline_words_[0] eq $strat_name_ ) 
        {
          print DEST_FILE  $resline_;
          last; 
        }
      }
      close ( DEST_FILE ) ;
    }

  }
}
