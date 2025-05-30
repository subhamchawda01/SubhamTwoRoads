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
use Math::Complex ; # sqrt

use Text::Tabs qw { expand };
use Cwd qw { cwd };

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/array_ops.pl"; # GetSum, GetSumLosses

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
my @vals_;
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
	    if ( $#words_ >= $startcolindex_ ) {
	    if ( $lastcolindex_ == -1 ) 
	    { #first iteration
		$lastcolindex_ = $#words_;
	    }
	    if ( $#words_ >= $lastcolindex_ ) 
	    {
		push ( @vals_, $words_[$startcolindex_] );
		$num_lines_ ++;
	    }
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
	if ( $#words_ >= $startcolindex_ ) {
	if ( $lastcolindex_ == -1 ) 
	{ #first iteration
	    $lastcolindex_ = $#words_;
	}
	if ( $#words_ >= $lastcolindex_ ) 
	{
	    push ( @vals_, $words_[$startcolindex_] ) ;
	    $num_lines_ ++;
	}
	}
    }
}

printf "%f\n", ( GetSum ( \@vals_ ) / GetSumLosses ( \@vals_ ) ) ;

