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
require "$GENPERLLIB_DIR/strat_utils.pl"; #GetRollParams, IsRollStrat, GetStrattype, GetRegression
require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; #MakeStratVecFromDir
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_and_tt.pl"; #MakeStratVecFromDirAndTT
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_and_tt.pl"; #MakeStratVecFromDirAndTT
require "$GENPERLLIB_DIR/calc_prev_date.pl"; #CalcPrevDate

my $USAGE="<script> shc strats_dir/file/time_period(BST_0810-EST_0800) strattype regresstype start_date end_date [sort_algo=kCNAPnlAdjAverage] [result_dir=/NAS1/ec2_globalresults/]\n";
$USAGE = $USAGE."<script> strat start_date end_date [sort_algo=kCNAPnlAdjAverage] [result_dir=/NAS1/ec2_globalresults/]";

if($#ARGV < 2)	{print $USAGE."\n"; exit(1);}
my $shc_ = "";
my $strat_dir_list_tp_ = "";
my $strattype_ = "";
my $regresstype_ = "";
if($#ARGV >= 5)
{
    $shc_ = shift;
    $strat_dir_list_tp_ = shift;
    $strattype_ = shift;
    $regresstype_ = shift;
}
else
{
    my $strat_ = shift;
    if(not -f $strat_ )  {PrintStacktraceAndDie ("ERROR: strat file $strat_ does not exists.\n");}
    $shc_ = `awk '{print \$2}' $strat_`; chomp($shc_);
    $strattype_ = GetStrattype($strat_);
    $regresstype_ = GetRegression($strat_);
    my $st_ = `awk '{print \$6}' $strat_`; chomp($st_);
    my $et_ = `awk '{print \$7}' $strat_`; chomp($et_);
    $strat_dir_list_tp_ = $st_."-".$et_;
}
my $sd_ = shift;
my $ed_ = shift;
my $skip_file_ = "INVALIDFILE";
my $sort_algo_ = "kCNAPnlAdjAverage";
my $results_dir_ = "/NAS1/ec2_globalresults/";
my $srv_name_=`hostname | cut -d'-' -f2`; chomp($srv_name_);
if($srv_name_ =~ "crt")
{
    $results_dir_ = $HOME_DIR."/ec2_globalresults/";
}
my $num_top_strats_ = 20;
if($#ARGV >= 0) {$sort_algo_ = shift;}
if($#ARGV >= 0) {$results_dir_ = shift;}

my $uid_=`date +%N`;chomp($uid_);


$sd_ = GetIsoDateFromStrMin1($sd_);
$ed_ = GetIsoDateFromStrMin1($ed_);

my @strat_list_ = ();
if ( -d $strat_dir_list_tp_)
{
    @strat_list_ = MakeStratVecFromDir($strat_dir_list_tp_);
}
elsif ( -f $strat_dir_list_tp_ )
{
    open STRATLIST, "< $strat_dir_list_tp_" or PrintStacktraceAndDie("ERROR: can't open file : $strat_dir_list_tp_ for reading.\n");
    while(my $line_ = <STRATLIST>)
    {
        chomp($line_);
        my $t_strat_ = `find $HOME_DIR/modelling/strats/$shc_/ -name $line_`; chomp($t_strat_);
        if (-f $t_strat_)   { push(@strat_list_, $t_strat_)}
    }
    close STRATLIST;
}
else
{
    @strat_list_ = MakeStratVecFromDirAndTT("$HOME_DIR/modelling/strats/$shc_/", $strat_dir_list_tp_);
}

my @t_strat_list_ = @strat_list_;
@strat_list_ = ();
my %strat_basename_to_fullname_map_ = ();
my %param_name_to_count_map_ = ();
my %param_name_to_best_strat_map_ = ();
my %param_name_to_map_entry_ = ();  #sort on the order of strat if two params appears same number of times in num_top_strats_

my $stratlist_file_ = "strats_".$uid_;
open STRATLIST, "> $stratlist_file_" or PrintStacktraceAndDie("ERROR: can't open file : $stratlist_file_ for writing.\n");
foreach my $t_strat_ (@t_strat_list_)
{
    my $t_strattype_ = GetStrattype($t_strat_);
    if( ( ($strattype_ eq $t_strattype_) || ( ($t_strattype_ eq "PriceBasedTrading" || $t_strattype_ eq "PriceBasedAggressiveTrading") && ($strattype_ eq "PriceBasedTrading" || $strattype_ eq "PriceBasedAggressiveTrading") )) #treating PriceBasedTrading & PriceBasedAggressiveTrading as same 
        && ($regresstype_ eq GetRegression($t_strat_)))
    {
        push(@strat_list_, $t_strat_);
        my $t_strat_basename_ = `basename $t_strat_`; chomp($t_strat_basename_);
        $strat_basename_to_fullname_map_{$t_strat_basename_} = $t_strat_;
        print STRATLIST "$t_strat_basename_\n";
    }
}
close STRATLIST;

my $exec_cmd_ = "$BINDIR/summarize_strategy_results $shc_ $stratlist_file_ $results_dir_ $sd_ $ed_ $skip_file_ $sort_algo_ | grep 'STRATEGYFILEBASE' | awk '{print \$2}'";
#print "$exec_cmd_\n";
my $output_ = `$exec_cmd_`;
my @sorted_strat_basename_ = split("\n", $output_); 
`rm -f $stratlist_file_`;

my $map_entry_count_ = 0;
for (my $i=0; $i<=min($num_top_strats_-1,$#sorted_strat_basename_); $i++)
{
    my $t_strat_basename_ = $sorted_strat_basename_[$i];
    my $t_strat_fullname_ = $strat_basename_to_fullname_map_{$t_strat_basename_};
    if(not IsRollStrat($t_strat_fullname_))
    {
      my $t_param_ = GetParam($t_strat_fullname_);
      if(exists $param_name_to_count_map_{$t_param_}) {$param_name_to_count_map_{$t_param_}++;}
      else    
      {
          $param_name_to_count_map_{$t_param_} = 1;
          $param_name_to_best_strat_map_{$t_param_} = $t_strat_fullname_;
          $param_name_to_map_entry_{$t_param_} = $map_entry_count_;
          $map_entry_count_++;
      }
    }
    else
    {
      my %t_param_name_to_count_map_ = ();
      my $t_count_ = 0;
      my $t_date_ = CalcPrevDate($sd_);
      $t_date_ = CalcNextWorkingDateMult($t_date_, 1);
      while($t_date_ <= $ed_)
      {
        $t_count_++;
        my $t_param_ = GetParam($t_strat_fullname_, $t_date_);
        if(exists $t_param_name_to_count_map_{$t_param_}) {$t_param_name_to_count_map_{$t_param_}++;}
        else  {$t_param_name_to_count_map_{$t_param_}=1;}
        $t_date_ = CalcNextWorkingDateMult($t_date_, 1);
      }
      foreach (sort {($t_param_name_to_count_map_{$b} <=> $t_param_name_to_count_map_{$a})} keys %t_param_name_to_count_map_)
      {
        my $t_param_contribution_ = ($t_param_name_to_count_map_{$_})/($t_count_);
        if(exists $param_name_to_count_map_{$_})  {$param_name_to_count_map_{$_} += $t_param_contribution_;}
        else
        {
          $param_name_to_count_map_{$_} = $t_param_contribution_;
          $param_name_to_best_strat_map_{$_} = $t_strat_fullname_;
          $param_name_to_map_entry_{$_} = $map_entry_count_;
          $map_entry_count_++;
        }
      }
    }
}

foreach (sort {($param_name_to_count_map_{$b} <=> $param_name_to_count_map_{$a}) || ($param_name_to_map_entry_{$a} <=> $param_name_to_map_entry_{$b})} keys %param_name_to_count_map_)
{
    print "$_ $param_name_to_count_map_{$_} $param_name_to_best_strat_map_{$_}\n";
}

exit(0);
