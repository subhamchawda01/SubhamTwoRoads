#!/usr/bin/perl

# \file scripts/sum_mean_calc.pl
#
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
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

Calculate the sum of the given column

 ${BB}Options${NN}
   --f                             Read data from file.
";

$| = 1;  # flush STDOUT
my ( 
    $opt_fromfile ,
   );
my $getopt_success = GetOptions (
   "f"              => \$opt_fromfile ,
  );
die "\n" unless $getopt_success;

my $l1_sum_ = 0;
my $num_lines_ = 0;
if ( $opt_fromfile ) {
    if ( $#ARGV >= 0 ) {
	my $datafilename_ = $ARGV[0];
	open ( INFILE, "< $datafilename_" ) or die " --f and no filename!\n" ;
	while ( my $inline_ = <INFILE> ) {
	    my @words_ = split ( ' ', $inline_ );
	    if ( $#words_ >= 0 ) {
		$l1_sum_ += $words_[0];
		$num_lines_ ++;
	    }
	}
    }
} else {
    while ( my $inline_ = <>) {
	my @words_ = split ( ' ', $inline_ );
	if ( $#words_ >= 0 ) {
	    $l1_sum_ += $words_[0];
	    $num_lines_ ++;
	}
    }
}

my $l1_mean_ = 0;
if ( $num_lines_ > 0 ) { $l1_mean_ = $l1_sum_ / $num_lines_ ; }
printf "%d %d\n", $l1_sum_, $l1_mean_;
