#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use List::MoreUtils qw(firstidx uniq);

my $USER = $ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SPARE_HOME="/spare/local/".$USER;

my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/"."LiveExec/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; #CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; #GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/strat_utils.pl"; #GetRollParams, IsRollStrat

my $USAGE="<script> shc [include_unistalled=0]";
if($#ARGV < 0)	{print $USAGE."\n"; exit(1);}
my $shc_ = shift;
my $include_unistalled_ = 0;
if($#ARGV >= 0) {$include_unistalled_ = shift;}

my @comp_files_ = `ls  $HOME_DIR/modelling/fbmfswork/$shc_/installed/comp_*`;  
if($include_unistalled_)  {@comp_files_ = `ls  $HOME_DIR/modelling/fbmfswork/$shc_/*/comp_*`;}
chomp(@comp_files_);

my %indicator_map_;
my %indicator_count_map_;
foreach my $comp_file_ (@comp_files_)
{
  open THISCOMPFILE, "< $comp_file_" or PrintStacktraceAndDie ( "ERROR: Could not open $comp_file_ for reading\n" );
  while(my $line_ = <THISCOMPFILE>)
  {
    chomp($line_);
    my @words_ = split(' ', $line_ );
    if($#words_ > 4)
    {
      my $t_initial_wt_ = $words_[1];
      my $t_final_wt_ = $words_[3];
      if(index($t_initial_wt_, ":") > 0)
      {
        my @t_list_ = split(":", $t_initial_wt_);
        $t_initial_wt_ = $t_list_[1];
        @t_list_ = split(":", $t_final_wt_);
        $t_final_wt_ = $t_list_[1];
      }
      if($t_initial_wt_*$t_final_wt_ <= 0) {next;}
      my $t_indicator_desc_ = "";
      for(my $i=4; $i<=$#words_; $i++)
      {
        if(substr($words_[$i],0,1) eq "#")
        {
          last;
        }
        else
        {
          $t_indicator_desc_ = $t_indicator_desc_.$words_[$i]." ";
        }
      }
      if($t_indicator_desc_)
      {
        my $t_contri_ = (abs($t_final_wt_)-abs($t_initial_wt_))/abs($t_initial_wt_);
        if(exists($indicator_map_{$t_indicator_desc_}))
        {
          $indicator_map_{$t_indicator_desc_} += $t_contri_;
          $indicator_count_map_{$t_indicator_desc_}++;
        }
        else
        {
          $indicator_map_{$t_indicator_desc_} = $t_contri_;
          $indicator_count_map_{$t_indicator_desc_} = 1;
        }
      }
    }
  }
  close THISCOMPFILE;
}

foreach (keys %indicator_map_)
{
  $indicator_map_{$_} /= $indicator_count_map_{$_};
}

print "#INDICATOR WEIGHT_CHANGE_FRACTION NUM_TIMES\n";
foreach (sort {$indicator_map_{$b} <=> $indicator_map_{$a}} keys %indicator_map_)
{
  print "$_ : $indicator_map_{$_} : $indicator_count_map_{$_}\n";
}
exit(0);
