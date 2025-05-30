#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use List::MoreUtils qw(firstidx uniq);
use List::Util qw/max min/; # for max

sub GetNewStratName ;

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

my $USAGE="<script> strat end_date num_of_days num_roll_days(0 for no rolling) [static_param_list=INVALIDFILE] [install(2<remove & install>|1<install>|0<nochange>)=0] [sort_algo=kCNAPnlAdjAverage] [vol_thresh_(-1 to 1 for frac_change|>1 for abs_val)=-0.3] [ttc_thresh_(same as vol_thresh_)=0.3] [pnl_thresh_frac_change(-1 to 1)=0.1]";
if($#ARGV < 3)	{print $USAGE."\n"; exit(1);}
my $strat_ = shift;
my $ed_ = shift;
my $num_days_ = shift;
my $num_roll_days_ = shift;
my $static_param_list_file_ = "INVALIDFILE";
my $to_install_ = 0;
my $sort_algo_ = "kCNAPnlAdjAverage";
my $vol_thresh_ = -0.3; 
my $ttc_thresh_ = 0.3;
my $pnl_thresh_frac_ = 0.1;

if($#ARGV >= 0) {$static_param_list_file_ = shift;}
if($#ARGV >= 0)	{$to_install_ = shift;}
if($#ARGV >= 0) {$sort_algo_ = shift;} 
if($#ARGV >= 0) {$vol_thresh_ = shift;} 
if($#ARGV >= 0) {$ttc_thresh_ = shift;} 
if($#ARGV >= 0) {$pnl_thresh_frac_ = shift;} 


my $uid_ = `date +%N`; chomp($uid_);
my $work_dir_ = "$SPARE_HOME/ParamRolls/$uid_";

for (my $i=0; $i<30;$i++)
{
    if ( -d $work_dir_)
    {
	$uid_ = `date +%N`; chomp($uid_);
	$work_dir_ = "$SPARE_HOME/ParamRolls/$uid_";
    }
    else
    {
	last;
    }
}

if(-d $work_dir_)	{`rm -r $work_dir_`;}
`mkdir -p $work_dir_`;
print "wokring dir : $work_dir_\n";

my $results_dir_ = "$work_dir_/results";
`mkdir -p $results_dir_`;

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "ERROR: Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

my $stime_ = `date`; chomp($stime_);
print $main_log_file_handle_ "START: $stime_\n";

$ed_ = GetIsoDateFromStrMin1($ed_);
my $sd_ = CalcPrevWorkingDateMult($ed_, $num_days_);

if (not -e $strat_)
{
    my $strat_full_path_ = `find $HOME_DIR/modelling/*strats/ -name $strat_`; chomp($strat_full_path_);
    if (-e $strat_full_path_)	{$strat_ = $strat_full_path_;}
    else	{PrintStacktraceAndDie("ERROR: can't find the stratfile : $strat_\n");} 
}

my $tp_dir_ = `dirname $strat_`; chomp($tp_dir_);
my $strat_basename_ = `basename $strat_`; chomp($strat_basename_);

my $orig_roll_param_ = "";
my $is_orig_strat_roll_ = IsRollStrat($strat_);

if( $is_orig_strat_roll_ )
{
    $orig_roll_param_ = `awk '{print \$5}' $strat_`; chomp($orig_roll_param_); 
    $orig_roll_param_ = substr($orig_roll_param_,5);
    my $t_basename_ = `grep ORIGINAL $orig_roll_param_ | head -n1 | awk '{print \$2}'`; chomp($t_basename_);
    if($t_basename_)
    {
	$strat_basename_ = $t_basename_;
    }  
    else
    {
	PrintStacktraceAndDie("ERROR: roll strat $strat_ dont have orignal strat name in $orig_roll_param_ file.\nPlease add <#ORIGINAL original_strat_name> on top of $orig_roll_param_\n");
    }
}

#just copying original strat so that if removed later on then no issues
my $orig_strat_copy_ = "$work_dir_/$strat_basename_";
if($is_orig_strat_roll_)
{
    my $def_param_ = GetRollParams($orig_roll_param_, "DEFAULT");
    `awk -vnpf=$def_param_ '{\$5=npf; print \$_;}' $strat_ > $orig_strat_copy_`;
}
else
{
    `cp $strat_ $orig_strat_copy_`;
}

my $shc_ = `awk '{print \$2}' $orig_strat_copy_`; chomp($shc_);
my $strattype_ = `awk '{print \$3}' $orig_strat_copy_`; chomp($strattype_);
my $model_filename_ = `awk '{print \$4}' $orig_strat_copy_`; chomp($model_filename_);
my $model_type_ = `awk 'NR==2{print \$2}' $model_filename_`; chomp($model_type_);
my $orig_param_ = `awk '{print \$5}' $orig_strat_copy_`; chomp($orig_param_);


my $run_string_ = $num_days_."_".$ed_."_".$num_roll_days_."_".substr($uid_, length($uid_) - 5, 5);
my $roll_strat_ = "$work_dir_/w_r_".$run_string_."_".$strat_basename_;
my $roll_param_ = "$work_dir_/roll_".$run_string_."_".$strat_basename_;
open ROLLPARAM, "> $roll_param_" or PrintStacktraceAndDie("ERROR: can't open file :  $roll_param_ for writing\n");
print ROLLPARAM "#ORIGINAL $strat_basename_\n";
print ROLLPARAM "DEFAULT DEFAULT $orig_param_\n";

`awk -vnpf=$roll_param_ '{\$5="ROLL:"npf; print \$_;}' $orig_strat_copy_ > $roll_strat_`;

#loading static params from file
my @static_params_ = ();
if(not $static_param_list_file_ eq "INVALIDFILE")
{
    open PARAMLIST, "<  $static_param_list_file_" or PrintStacktraceAndDie("ERROR: can't open file :  $static_param_list_file_ for reading\n");
    while(my $line_=<PARAMLIST>)
{
    chomp($line_);
    if(-e $line_)	
    {
	push (@static_params_, $line_)
    }
}
close PARAMLIST;
}

my $next_ed_ = CalcNextWorkingDateMult($sd_, $num_roll_days_);
my $next_sd_ = $sd_;
my $summarize_sd_ = CalcNextWorkingDateMult($next_ed_,1);
my $last_best_param_ = $orig_param_;

  
#filling up rolling params, if num_roll_days_ > 0
if($num_roll_days_ > 0)
{
  my @all_params = ();
  push (@all_params, $orig_param_);   #strat_0 is the original strat as original param has 0th index in this vector

  while($next_ed_ <= $ed_)
  {
      my $this_work_dir_ = "$work_dir_/$next_ed_";
      my $this_strat_dir_ = "$this_work_dir_/strats";
      `mkdir -p $this_strat_dir_`;
      
      my $exec_cmd_ = "$MODELSCRIPTS_DIR/get_best_param_in_pool.pl $orig_strat_copy_ $next_sd_ $next_ed_ $sort_algo_ | head -n2 | awk '{print \$1}'";
      print $main_log_file_handle_ "$exec_cmd_\n";
      my $output_lines_ = `$exec_cmd_`;
      print $main_log_file_handle_ $output_lines_;
      my @this_params_ = split("\n", $output_lines_);
      
      foreach my $t_param_ (@static_params_) 
      {
          push(@this_params_, $t_param_);
      }
      push (@this_params_, $orig_param_);
      push (@this_params_, $last_best_param_);
      
      @this_params_ = uniq @this_params_;
      print $main_log_file_handle_ "this_params_:\n";
      print $main_log_file_handle_ join("\n", @this_params_), "\n";

      for (my $i=0; $i<=$#this_params_; $i++)
      {
          my $t_param_ = $this_params_[$i];
          my $t_param_idx_ = firstidx {$_ eq $t_param_} @all_params;
          my $t_strat_ = "$this_strat_dir_/strat_".$t_param_idx_;
          if($t_param_idx_ == -1)
          {
              push(@all_params, $t_param_);
              $t_param_idx_ = firstidx {$_ eq $t_param_} @all_params;
              $t_strat_ = "$this_strat_dir_/strat_".$t_param_idx_;
          }
          if(! -e $t_strat_)
          {
              my $exec_cmd_ = "awk -vnpf=$t_param_ '{\$5=npf; print \$_}' $orig_strat_copy_ > $t_strat_";
              `$exec_cmd_`;
          }
          else
          {
              PrintStacktraceAndDie("ERROR: shouldn't be here\n");
          }
      }
      
      $exec_cmd_ = "$MODELSCRIPTS_DIR/run_simulations_to_result_dir.pl $shc_ $this_strat_dir_ $next_sd_ $next_ed_ ALL $results_dir_";
      print $main_log_file_handle_ "$exec_cmd_\n";
      `$exec_cmd_`; 

      $exec_cmd_ = "$BINDIR/summarize_strategy_results $shc_ $this_strat_dir_ $results_dir_ $next_sd_ $next_ed_ INVALIDFILE $sort_algo_ | grep 'STRATEGYFILEBASE' | awk '{print \$2}'";
      print $main_log_file_handle_ "$exec_cmd_\n";
      
      $output_lines_ = `$exec_cmd_`;
      print $main_log_file_handle_ $output_lines_;
      my @this_strats_ = split("\n", $output_lines_);
      
      my $this_best_strat_ = $this_strats_[0];
      my $last_best_param_ = `awk '{print \$5}' $this_work_dir_/strats/$this_strats_[0]`; chomp($last_best_param_);
      
      $next_sd_ = CalcNextWorkingDateMult($next_ed_, 1);
      $next_ed_ = CalcNextWorkingDateMult($next_sd_, $num_roll_days_);
      if($next_ed_ > $ed_)  {print ROLLPARAM "$next_sd_ TODAY $last_best_param_\n";}
      else  {print ROLLPARAM "$next_sd_ $next_ed_ $last_best_param_\n";}
  }
}

close ROLLPARAM;

#finding final strats to compare with
my $this_strat_dir_ = "$work_dir_/final_strats";
`mkdir -p $this_strat_dir_`;
`cp $orig_strat_copy_ $this_strat_dir_/`;
my $orig_strat_copy_basename_ = `basename $orig_strat_copy_`; chomp($orig_strat_copy_basename_);
my $roll_strat_basename_ = `basename $roll_strat_`; chomp($roll_strat_basename_);


my $same_roll_param_ = GetRollParamIfAllSame($roll_param_);
if($same_roll_param_ eq "MANYFILES")
{
#more than one roll param
  `cp $roll_strat_ $this_strat_dir_/`;
}
elsif($same_roll_param_ ne "INVALIDFILE")
{
#only one roll param  
  my $t_new_strat_name_ = GetNewStratName( $orig_strat_copy_basename_, $same_roll_param_ );
  my $t_cmd_ = "awk -vnpf=$same_roll_param_ '{\$5=npf; print \$_;}' $orig_strat_copy_ > $this_strat_dir_/$t_new_strat_name_";  
  print $main_log_file_handle_ "roll strat has all params as same : $same_roll_param_, thus doing $t_cmd_\n";
  `$t_cmd_`;
}

for(my $i=0; $i<=$#static_params_; $i++)
{
    next if ( $static_params_[$i] eq $orig_param_ );
    next if ( $static_params_[$i] eq $same_roll_param_ );

    my $t_new_strat_name_ = GetNewStratName( $orig_strat_copy_basename_, $static_params_[$i] );

    my $t_cmd_ = "awk -vnpf=$static_params_[$i] '{\$5=npf; print \$_;}' $orig_strat_copy_ > $this_strat_dir_/$t_new_strat_name_";
    print $main_log_file_handle_ "$t_cmd_\n";
    `$t_cmd_`;
}

my $best_strat_basename_ = $orig_strat_copy_basename_;
my $num_strats_to_compare_ = `ls $this_strat_dir_ | wc -l`; chomp($num_strats_to_compare_);

if($num_strats_to_compare_ > 1)
{
#running simulations and summarizing
  my $exec_cmd_ = "$MODELSCRIPTS_DIR/run_simulations_to_result_dir.pl $shc_ $this_strat_dir_ $summarize_sd_ $ed_ ALL $results_dir_";
  print $main_log_file_handle_ "$exec_cmd_\n";
  `$exec_cmd_`;

  $exec_cmd_ = "$BINDIR/summarize_strategy_results $shc_ $this_strat_dir_ $results_dir_ $summarize_sd_ $ed_ INVALIDFILE $sort_algo_ | grep 'STRATEGYFILEBASE'";
  print $main_log_file_handle_ "$exec_cmd_\n";
  my $output_lines_ = `$exec_cmd_`;
  print $main_log_file_handle_ "FINALSTATS:\n$output_lines_\n";
  my @results = split("\n", $output_lines_);

#finding orginal strats performance
  my $orig_pnl_ = 0;
  my $orig_vol_ = 0;
  my $orig_ttc_ = 0;

  for (my $i=0; $i<=$#results; $i++)
  {
      my $t_result_ = $results[$i];
      my @words_ = split(" ", $t_result_);
      if($words_[1] eq $orig_strat_copy_basename_)
      {
          $orig_pnl_ = int($words_[2]);
          $orig_vol_ = int($words_[4]);
          $orig_ttc_ = int($words_[8]);
          last;
      }
  }

#setting thresh
  if (abs($vol_thresh_)<1)
  {
    $vol_thresh_ = (1+$vol_thresh_)*$orig_vol_;
  }
  else 
  {
    $vol_thresh_ = min( $vol_thresh_, $orig_vol_); #if orignal vol is already very less, then it would be unfair to expect the newparam to increase it
  }

  if (abs($ttc_thresh_)<1)
  {
    $ttc_thresh_ = (1+$ttc_thresh_)*$orig_ttc_;
  }
  else
  {
    $ttc_thresh_ = max( $ttc_thresh_, $orig_ttc_ ); #if orignal vol is already very high, then it would be unfair to expect the newparam to decrease it
  }

#finding best strat
  for (my $i=0; $i<=$#results; $i++)
  {
      my $t_result_ = $results[$i];
      my @words_ = split(" ", $t_result_);
      my $this_strat_ = $words_[1];
      if($this_strat_ eq $orig_strat_copy_basename_)
      {
        last;
      }

      my $this_pnl_ = int($words_[2]);;
      my $this_vol_ = int($words_[4]);
      my $this_ttc_ = int($words_[9]);

      if( ($this_pnl_ - $orig_pnl_) <= $pnl_thresh_frac_*abs($orig_pnl_) )
      {
        my $t_pnl_change_ = $this_pnl_ - $orig_pnl_;
        my $t_pnl_thresh_ = $pnl_thresh_frac_*abs($orig_pnl_);
        print $main_log_file_handle_ "$this_strat_ : can't satisfy pnl thresh, $t_pnl_change_ <= $t_pnl_thresh_\n";
        next;
      }

      if( $this_vol_ <= $vol_thresh_ )
      {
        print $main_log_file_handle_ "$this_strat_ : can't satisfy vol thresh, $this_vol_ <= $vol_thresh_\n";
        next;
      }

      if( $this_ttc_ >= $ttc_thresh_ )
      {
        print $main_log_file_handle_ "$this_strat_ : can't satisfy ttc thresh, $this_ttc_ >= $ttc_thresh_\n";
        next;
      }	
      $best_strat_basename_ = $words_[1];    
      print $main_log_file_handle_ "$this_strat_ is best strat\n";
      last;
  }
}

#installing/removing
if($is_orig_strat_roll_ && $to_install_)
{
    `rm -f $strat_`;
    print $main_log_file_handle_ "removing roll strat : $strat_\n";
    my $roll_param_used_ = `grep $orig_roll_param_ $HOME_DIR/modelling/*strats/$shc_/*/* | wc -l`; chomp($roll_param_used_);
    if($roll_param_used_>0)
    {
	print $main_log_file_handle_ "not removing roll param : $orig_roll_param_ because it is being used in other strats($roll_param_used_)\n";
    }
    else
    {
	`rm -f $orig_roll_param_`;
	print $main_log_file_handle_ "removing roll param : $orig_roll_param_\n";
    }
}

if( $best_strat_basename_ ne $orig_strat_copy_basename_ )  
{
    print $main_log_file_handle_ "INSTALLING:\nthreshholds satisfied\n";
    if($to_install_)
    {
        if($best_strat_basename_ eq $roll_strat_basename_)
	    
	{
	    my $orig_param_dir_ = `dirname $orig_param_`; chomp($orig_param_dir_);
	    my $roll_param_dir_ = $orig_param_dir_."/roll_params";
	    `mkdir -p $roll_param_dir_`;
	    my $roll_param_basename_ = `basename $roll_param_`; chomp($roll_param_basename_);
	    my $roll_param_full_path_ = "$roll_param_dir_/$roll_param_basename_";
	    `cp $roll_param_ $roll_param_full_path_`;
	    my $roll_strat_full_path_ = "$tp_dir_/$roll_strat_basename_";
	    `awk -vrpf=$roll_param_full_path_ '{\$5="ROLL:"rpf; print \$_;}' $roll_strat_ > $roll_strat_full_path_`;
	    print $main_log_file_handle_ "installing roll strat : $roll_strat_full_path_\n";
	    print $main_log_file_handle_ "installing roll param : $roll_param_full_path_\n";
	    
	}
	else
	{
	    `cp $this_strat_dir_/$best_strat_basename_ $tp_dir_`;
	    my $t_new_param_ = `awk '{print \$5;}' $tp_dir_/$best_strat_basename_`; chomp($t_new_param_);
	    print $main_log_file_handle_ "installing strat : $tp_dir_/$best_strat_basename_ with new param : $t_new_param_\n";
	}
	if($to_install_>1 && not $is_orig_strat_roll_) #remove old strat if installing roll strat and to_install>1
	{
	    `rm -f $strat_`;
	    print  $main_log_file_handle_ "removing original strat : $strat_\n";
	}
    }
}
else
{
    print $main_log_file_handle_ "NOTINSTALLING:\nNot installing rolled strat as can't satisfy threshholds\n";
    if($is_orig_strat_roll_ && $to_install_)
    {
	`cp $orig_strat_copy_ $tp_dir_/$orig_strat_copy_basename_`;
	print $main_log_file_handle_ "installed orignal strat : $tp_dir_/$orig_strat_copy_basename_\n";
    }
}

my $etime_ = `date`; chomp($etime_);
print $main_log_file_handle_ "END: $etime_\n";

$main_log_file_handle_->close;
exit(0);


sub GetNewStratName 
{
  my $orig_strat_name_ = shift;
  my $new_param_ = shift; $new_param_ = `basename $new_param_`; chomp($new_param_);
  my $new_strat_name_ = $orig_strat_name_;
  $new_strat_name_ =~ s/pfi_.*/pfi_c_$new_param_/ ;
  return $new_strat_name_ eq $orig_strat_name_ ? $orig_strat_name_.".pfi_c_".$new_param_ : $new_strat_name_;
}
