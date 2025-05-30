#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";

require "$GENPERLLIB_DIR/results_db_access_manager.pl"; #ExecuteReadQueryOnResultsDB, SanitizeStratType
require "$SCRIPTS_DIR/is_valid_shc.pl";

if ( $#ARGV < 0 )
{
  print "$0 shc|ALL [del_strats_from_DB=0]\n";
  exit 0;
}

my $shc = $ARGV[0];
my $to_del = 0;
if ( $#ARGV >= 1 ) { $to_del = $ARGV[1]; }

my $strat_vec_vec_ref_;
if ( $shc eq "ALL" )
{
  $strat_vec_vec_ref_=  ExecuteReadQueryOnResultsDB( "SELECT sname from strats" );
}
else
{
  $strat_vec_vec_ref_=  ExecuteReadQueryOnResultsDB( "SELECT sname from strats WHERE shortcode = ?", $shc );
}

foreach my $t_strat_vec_ref_ ( @$strat_vec_vec_ref_ )
{
  my $t_strat_ = $$t_strat_vec_ref_[0]; chomp($t_strat_);
  my @temp_ = split '_', $t_strat_;
  my $sch_ = $temp_[$#temp_];
  my $fut_ = substr($sch_, 0, 3);
  if (($fut_ ne "FUT") && (SanitizeStratType($t_strat_) < 0))
  {
    print "Warning! NotFoundOnFS $t_strat_ $to_del\n";
    if ( $to_del )
    {
      my $num_res_rem_ = DeleteStratFromDB ( $t_strat_ );
      if ( $num_res_rem_ > 0 )
      {
        print "REMOVED FROM DB [$num_res_rem_] : $t_strat_\n";
      } 
    }
  }
}

