#!/usr/bin/perl

use strict;
use warnings;

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; #ExecuteWriteQueryOnResultsDB 
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; #BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; #CalcPrevWorkingDateMult

if($#ARGV<0)
{
  print "USAGE : $0 <username_email_csvfile>\n";
  exit(0);
}

my $file_ = $ARGV[0];

open FILE, "< $file_" or PrintStacktraceAndDie("ERROR: can't open file : $file_ for reading.\n");
my @lines_ = <FILE>;chomp(@lines_);
close FILE;

my $num_added_ = 0;
foreach my $t_line_ (@lines_)
{
  my @words_ = split ( ',' , $t_line_ ) ; 
  next if @words_ < 2 ;
  my $t_user_ = $words_[0]; my $t_email_ = $words_[1];
  print "Adding: $t_user_ $t_email_\n";
  if ( @{ExecuteReadQueryOnResultsDB("SELECT name FROM users WHERE name = ? OR email = ?", $t_user_, $t_email_)} > 0 )
  {
    print "WARNING! user already exists\n";
  }
  elsif ( ExecuteWriteQueryOnResultsDB("INSERT INTO users (name,email) VALUES (?,?)", $t_user_, $t_email_) > 0 )
  {
    print "SUCCESS: Added $t_user_ $t_email_\n";
    $num_added_++;
  }
}
print "Total Users Added : $num_added_\n";
