#!/usr/bin/perl

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
Usage: $script --samplingrate=0.30 --f=input_file --o=output_file

RandomSample the input file and print only a few lines to output file

 ${BB}Options${NN}
   --samplingrate=<sampling_probability>      Probability of a line emerging in output file
   --f                                        Input file
   --o                                        Output file
";

$| = 1;  # flush STDOUT
my ( 
    $opt_samplingrate ,
    $opt_fromfile ,
    $opt_tofile ,
   );
my $getopt_success = GetOptions (
   "samplingrate=s"  => \$opt_samplingrate ,
   "f=s"             => \$opt_fromfile ,
   "t=s"             => \$opt_tofile ,
  );
die "$usage \n" unless $getopt_success;

if ( ( ! $opt_tofile ) ||
     ( ! $opt_fromfile ) ||
     ( ! $opt_samplingrate ) )
{
    printf "%s\n", $usage;
    exit ( 0 );
}

# printf ( "Output file : %s\n", $opt_tofile );
# printf ( "Sampling Rate : %s\n", $opt_samplingrate );

open ( OUTFILE, "> $opt_tofile" ) or die "Could not open file $opt_tofile for writing\n";

if ( $opt_fromfile ) 
{
    # printf ( "Input file : %s\n", $opt_fromfile );
    open ( INFILE, "< $opt_fromfile" ) or die " Could not open file $opt_fromfile for reading \n" ;
    while ( my $inline_ = <INFILE> ) 
    {
	my $random_number = rand();
	if ( $random_number <= $opt_samplingrate )
	{
	    printf OUTFILE "%s", $inline_;
	}
    }
} 
else 
{
    while ( my $inline_ = <>) 
    {
	my $random_number = rand();
	if ( $random_number <= $opt_samplingrate )
	{
	    printf OUTFILE "%s", $inline_;
	}
    }
}
