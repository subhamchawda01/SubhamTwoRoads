#!/usr/bin/perl

# \file scripts/statcalcvec.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 162, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#

use warnings;
use strict;
use Getopt::Long;
use File::Basename;
use File::Temp qw { tempfile tempdir };
use File::Find;
use File::Path;
use File::Spec;
use IO::File;

use Text::Tabs qw { expand };
use Cwd qw { cwd };
my $ON_WINDOWS = 0;
my $NN     = chr(27) . "[0m";  # normal
   $NN     = "" if $ON_WINDOWS;
my $BB     = chr(27) . "[1m";  # bold
   $BB     = "" if $ON_WINDOWS;
my $script = basename $0;
my $usage  = "
Usage: $script <options> file 

Calculate the mean and standard deviation of the given columns

 ${BB}Options${NN}
   --startcol=<column_number>      Start processing from the given column number.
   --f                             Read data from file.
";

$| = 1;  # flush STDOUT
my ( 
    $opt_startcol ,
    $opt_fromfile ,
   );
my $getopt_success = GetOptions (
   "startcol=s"     => \$opt_startcol ,
   "f"              => \$opt_fromfile ,
  );
die "\n" unless $getopt_success;

my $startcolindex_ = 0;
if ( $opt_startcol ) { $startcolindex_ = $opt_startcol ; }

my $lastcolindex_ = -1;
my @l1_sums_;
my @l2_sums_;
my $num_lines_ = 0;
if ( $opt_fromfile ) 
{
    if ( $#ARGV >= 0 ) 
    {
	my $datafilename_ = $ARGV[0];
	open ( INFILE, "< $datafilename_" ) or die " --f and no filename!\n" ;
	while ( my $inline_ = <INFILE> ) 
	{
	    chomp ( $inline_ );
	    my @words_ = split ( ' ', $inline_ );
	    if ( $lastcolindex_ == -1 ) 
	    { #first iteration
		$lastcolindex_ = $#words_;
		for ( my $i = $startcolindex_ ; $i <= $lastcolindex_ ; $i ++ ) 
		{
		    push ( @l1_sums_, 0 );
		    push ( @l2_sums_, 0 );
		}
	    }
	    if ( $#words_ >= $lastcolindex_ ) 
	    {
		for ( my $i = $startcolindex_; $i <= $lastcolindex_ ; $i ++ ) 
		{
		    $l1_sums_[($i-$startcolindex_)] += $words_[$i] ;
		    $l2_sums_[($i-$startcolindex_)] += $words_[$i] * $words_[$i] ;
		}
		$num_lines_ ++;
	    }
	}
	close ( INFILE );
    }
} 
else 
{
    while ( my $inline_ = <STDIN> ) 
    {
	chomp ( $inline_ );
	my @words_ = split ( ' ', $inline_ );
	if ( $lastcolindex_ == -1 ) 
	{ #first iteration
	    $lastcolindex_ = $#words_;
	    for ( my $i = $startcolindex_ ; $i <= $lastcolindex_ ; $i ++ ) 
	    {
		push ( @l1_sums_, 0 );
		push ( @l2_sums_, 0 );
	    }
	}
	if ( $#words_ >= $lastcolindex_ ) 
	{
	    for ( my $i = $startcolindex_; $i <= $lastcolindex_ ; $i ++ ) 
	    {
		$l1_sums_[($i-$startcolindex_)] += $words_[$i] ;
		$l2_sums_[($i-$startcolindex_)] += $words_[$i] * $words_[$i] ;
	    }
	    $num_lines_ ++;
	}
    }
}

my @mean_values_;
my @stdev_values_;
for ( my $i = $startcolindex_ ; $i <= $lastcolindex_ ; $i ++ ) 
{
    my $column_index_ = ($i-$startcolindex_);
    push ( @mean_values_, ( $l1_sums_[$column_index_] / ($num_lines_) ) ) ;
    if ( ($num_lines_) > 1 ) 
    {
	push ( @stdev_values_, sqrt ( ( $l2_sums_[$column_index_] - ( $l1_sums_[$column_index_] * $l1_sums_[$column_index_] / ($num_lines_) ) ) / ($num_lines_ -1) ) ) ;
    } 
    else 
    {
	push ( @stdev_values_, 0 ) ;
    }
    if ( $stdev_values_[$column_index_] > 0 )
    {
	printf "%f %f %f %d\n", $mean_values_[$column_index_], $stdev_values_[$column_index_], ( $mean_values_[$column_index_] / $stdev_values_[$column_index_], $num_lines_ );
    }
    else
    {
	printf "%f %f %f %d\n", $mean_values_[$column_index_], $stdev_values_[$column_index_], -1, $num_lines_;
    }
}

