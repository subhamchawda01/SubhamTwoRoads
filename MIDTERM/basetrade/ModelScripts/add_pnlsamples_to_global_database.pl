#!/usr/bin/perl

# \file ModelScripts/add_pnlsamples_to_global_database.pl
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

my $USAGE="$0 strategylistfile this_day_resultsfile tradingdate local_results_base_dir";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $strategylistfile_ = $ARGV[0];
my $resultsfile_ = $ARGV[1];
my $tradingdate_ = $ARGV[2];
my $GLOBALRESULTSBASEDIR = $ARGV[3];

my @strategy_filevec_ = ();
open STRATEGYLISTFILE, "< $strategylistfile_" or PrintStacktraceAndDie ( "add_pnlsamples_to_global_database.pl could not open strategylistfile $strategylistfile_\n" );
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
open RESULTSFILE, "< $resultsfile_" or PrintStacktraceAndDie ( "add_pnlsamples_to_global_database.pl could not open resultsfile $resultsfile_\n" );
while ( my $thisresultline_ = <RESULTSFILE> )
{
    chomp ( $thisresultline_ );
    push ( @results_linevec_, $thisresultline_ );
}
close RESULTSFILE;

if ( $#strategy_filevec_ != $#results_linevec_ )
{
    print "$resultsfile_ $strategylistfile_\n";
    exit ( 0 );
}

my ($tradingdateyyyy_, $tradingdatemm_, $tradingdatedd_) = BreakDateYYYYMMDD ( $tradingdate_ );

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
open ( RESULTSDBFILE, ">> $resultsfilename_" );
flock ( RESULTSDBFILE, LOCK_EX ); # write lock

for ( my $i = 0 ; $i <= $#strategy_filevec_; $i ++ )
{
    my $thisresultline_ = $results_linevec_[$i];
    my @resultslinewords_ = split ( ' ', $thisresultline_ );
    if ( ( $#resultslinewords_ >= 1 ) )
    { # at least two words and volume is > 0
	print RESULTSDBFILE $strategy_filevec_[$i]," ",$tradingdate_," ", $results_linevec_[$i],"\n";
    }
}
close ( RESULTSDBFILE );

print "$resultsfilename_";
