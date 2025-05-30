#!/usr/bin/perl

# \file ModelScripts/add_results_to_local_database.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#        Suite 217, Level 2, Prestige Omega,
#        No 104, EPIP Zone, Whitefield,
#        Bangalore - 560066, India
#        +91 80 4060 0717
#
# This script takes as input:
# strategylistfile ( a file with lines with just 1 word corresponding to the name of a strategyfile )
# resultslistfile ( a file with multiple lines corresponding to the results on that day for that file )
# tradingdate
#
# Basically after running simulations for a day,
# this script is called,
# for reading the strategylistfile and resultslistfile
# and printing the values in the appropriate file in local_results_base_dir

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
my $LOCALRESULTSBASEDIR = $ARGV[3];

my ($tradingdateyyyy_, $tradingdatemm_, $tradingdatedd_) = BreakDateYYYYMMDD ( $tradingdate_ );

my @model_filevec_ = ();
my @model_id_ = ();
open STRATEGYLISTFILE, "< $strategylistfile_" or PrintStacktraceAndDie ( "add_results_to_local_database.pl could not open strategylistfile $strategylistfile_\n" );
while ( my $thisstrategyfile_ = <STRATEGYLISTFILE> ) 
{
    chomp ( $thisstrategyfile_ );
    my @stratwords_ = split ( ' ', $thisstrategyfile_ );
    if ( $#stratwords_ >= 0 )
    {
        #my $this_model_id_ = `head -1 $stratwords_[0] | awk '{print \$NF}' ` ;
        my $this_model_id_ = $stratwords_[7];
        chomp($this_model_id_);
        my $this_model_basename_ = `basename $stratwords_[3]`; chomp ( $this_model_basename_ );
        push ( @model_filevec_, $this_model_basename_ ) ;
        push ( @model_id_, $this_model_id_ ) ;
    }
}
close STRATEGYLISTFILE;

my @results_linevec_ = ();
open RESULTSFILE, "< $resultsfile_" or PrintStacktraceAndDie ( "add_results_to_local_database.pl could not open resultsfile $resultsfile_\n" );
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

#if ( $#strategy_filevec_ != $#results_linevec_ )
#{
#    PrintStacktraceAndDie ( "For strategylistfile $strategylistfile_, and resultfile $resultsfile_: number of strategies $#strategy_filevec_ != number of result lines $#results_linevec_\n" );
#}

my $resultsfilename_ = $LOCALRESULTSBASEDIR."/".$tradingdateyyyy_."/".$tradingdatemm_."/".$tradingdatedd_."/results_database.txt";

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

my %id_to_results = ();
foreach my $res_line (@results_linevec_)
{
	my ($a,$b) = $res_line =~ /(.*)\s(.*)/;
	printf ("a=%s \nb=%s\n",$a,$b);
	$id_to_results{$b} = $a;
}

my $cnt = 0;
foreach my $strat_line(@model_filevec_)
{
	my $s_id_ =  $model_id_[ $cnt ] ;
	$cnt ++;
        if ( exists $id_to_results{$s_id_} )
        {
        	print RESULTSDBFILE $strat_line, " ", $tradingdate_, " ", $id_to_results{$s_id_}, "\n" ;
        }
       	else 
       	{
#                print "No SIM result for strategy line :: ".$strat_line."\n";
       	}
}
close ( RESULTSDBFILE );

print "$resultsfilename_";
