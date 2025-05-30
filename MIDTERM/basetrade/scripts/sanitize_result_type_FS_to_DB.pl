#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/results_db_access_manager.pl"; #ExecuteReadQueryOnResultsDB, SanitizeStratType
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $#ARGV < 0 ) 
{
  print "$0 input_dir\n";
  exit 0;
}

my $dir = $ARGV[0];
my @result_files_ = MakeStratVecFromDir($dir);

foreach my $file_ ( @result_files_ )
{
  open FILE, " < $file_ " or PrintStacktraceAndDie ( "Can't open $file_ for reading\n");
  my @lines_ = <FILE>; chomp(@lines_);
  close FILE;
  foreach my $line_ ( @lines_ )
  {
    my @words_ = split( ' ', $line_ );
    next if ( $#words_ < 1 || substr($words_[0],0,1) eq '#' );
    my $strat_ = $words_[0];
    my $date_ = $words_[1];
    ExecuteWriteQueryOnResultsDB ( "UPDATE results JOIN strats ON results.stratid = strats.stratid SET regenerate = ? WHERE date = ? AND sname = ?", 'N', $date_, $strat_ );
  }
}

