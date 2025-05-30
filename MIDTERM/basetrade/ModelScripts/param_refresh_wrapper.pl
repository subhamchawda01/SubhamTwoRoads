#!/usr/bin/perl
#use strict;
#use warnings;
use FileHandle;

#initialise paths:
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";

require "$GENPERLLIB_DIR/get_spare_local_dir_for_aws.pl"; # GetSpareLocalDir
require "$GENPERLLIB_DIR/array_ops.pl"; #GetStdev

sub CreateWorkDirectory
{
  my $unique_gsm_id_ = `date +%N`;
  chomp ( $unique_gsm_id_ ); 
  $unique_gsm_id1_ = int($unique_gsm_id_) + 0;
  my $SPARE_LOCAL="/spare/local/";
  my $hostname_ = `hostname`;
  if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
  {
    $SPARE_LOCAL = GetSpareLocalDir();
  }
  my $work_dir_ = $SPARE_LOCAL."paramgen";
  $work_dir_ = $work_dir_."/".$unique_gsm_id1_."/";

  if ( -d $work_dir_ ) { `rm -rf $work_dir_`; }
  `mkdir -p $work_dir_ `; #make_work_dir
  my $dir_run1_ = $work_dir_."run1/";
  my $dir_run2_ = $work_dir_."run2/";
  my $dir_test_ = $work_dir_."test/";
  my $dir_strat_ = $dir_test_."strat/";
  my $dir_result_ = $dir_test_."result/";
  `mkdir -p $dir_run1_`;
  `mkdir -p $dir_run2_`;
  `mkdir -p $dir_test_`;
  `mkdir -p $dir_strat_`;
  `mkdir -p $dir_result_`;
  return $work_dir_;
}

sub MakeConfig1
{
  my ($param_, $dir_) = @_;
  my $config_file_ = $dir_."config_file";
  open CONFIG, "> $config_file_" or PrintStacktraceAndDie ( "Could not open $config_file_ for writing\n" );
 
  my @content_ = ("INCREASE_PLACE", "INCREASE_KEEP", "ZEROPOS_PLACE", "ZEROPOS_KEEP", "DECREASE_PLACE", "DECREASE_KEEP");
  my @content1_ = ("PLACE_KEEP_DIFF", "INCREASE_ZEROPOS_DIFF", "ZEROPOS_DECREASE_DIFF");
  my @content2_ = ("MAX_UNIT_RATIO", "MAX_POSITION");
  for (my $i = 0; $i <= $#content_; $i ++)
  {
    my $is_param_present_ = `cat $param_ | awk 'BEGIN{FS=OFS="#";}{print \$1;}' | grep "$content_[$i] " | wc -l`;
    if($is_param_present_ > 0)
    {
      
      print CONFIG "$content_[$i] 0.2\n";
    }
  } 
  for (my $i = 0; $i <= $#content1_; $i ++)
  {
    my $is_param_present_ = `cat $param_ | awk 'BEGIN{FS=OFS="#";}{print \$1;}' | grep "$content1_[$i] " | wc -l`;
    if($is_param_present_ > 0)
    {
      print CONFIG "$content1_[$i] 0.3\n";
    }
  } 
  for (my $i = 0; $i <= $#content2_; $i ++)
  {
    my $is_param_present_ = `cat $param_ | awk 'BEGIN{FS=OFS="#";}{print \$1;}' | grep "$content2_[$i] " | wc -l`;
    if($is_param_present_ > 0)
    {
      print CONFIG "$content2_[$i] 1\n";
    }
  } 
  return( $config_file_ );
}

sub MakeConfig2
{
  my ($param_, $dir_) = @_;
  my $config_file_ = $dir_."config_file";
  open CONFIG, "> $config_file_" or PrintStacktraceAndDie ( "Could not open $config_file_ for writing\n" );
  my @content_ = ("WORST_CASE_UNIT_RATIO", "AGGRESSIVE", "ALLOWED_TO_AGGRESS", "NUM_NON_BEST_LEVELS_MONITORED");
  for (my $i = 0; $i <= $#content_; $i ++)
  {
    my $is_param_present_ = `cat $param_ | awk 'BEGIN{FS=OFS="#";}{print \$1;}' | grep "$content_[$i] " | wc -l`;
    if($is_param_present_ > 0)
    {
      print CONFIG "$content_[$i] 1\n";
    }
  }
  return( $config_file_ );
}

my $USAGE="$0 stratfile_complete_path_ train_start_date train_end_date test_start_date test_end_date  [Auto replace params 0/1, default = 0] [0,1,2  0 = changeparam,  1 = remame param,  2 = rename strat and param both, default = 0]";
if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $stratfile_ 	= $ARGV [ 0 ];
my $start_date_ = $ARGV [ 1 ];
my $end_date_ 	= $ARGV [ 2 ];

my $test_start_date_ = $ARGV [ 3 ];
my $test_end_date_   = $ARGV [ 4 ];

my $auto_replace_params_ = 0;
if( $#ARGV >= 5 )
{
  $auto_replace_params_ = $ARGV [ 5 ];
}

my $rename_param_ = 0;
if( $#ARGV >= 6 )
{
  $rename_param_ = $ARGV [ 6 ];
}

#print "Auto Replace = $auto_replace_params_\n";

my $dir_ = CreateWorkDirectory();
print "$dir_\n";
my $param_ = `cat $stratfile_ | awk '{print \$5}'`; chomp($param_);
my $shortcode_ = `cat $stratfile_ | awk '{print \$2}'`; chomp($shortcode_);

my $config1_ = MakeConfig1($param_, $dir_."run1/");
my $config2_ = MakeConfig2($param_, $dir_."run2/");

my $dir_iter1_ = $dir_."run1/";

my $param_refresh_script_ = "$MODELSCRIPTS_DIR/param_refresh.pl";

#print "hi\n";
my $exec_ = "$param_refresh_script_ $config1_ $stratfile_ $start_date_ $end_date_ $dir_iter1_";
#print "$exec_\n";
my $out1_ = `$param_refresh_script_ $config1_ $stratfile_ $start_date_ $end_date_ $dir_iter1_`;
print $out1_;
#print "hi\n";
my @out_array_ = split '\n', $out1_;
my $improvement1_ = $out_array_[0];
my $new_strat_ = $dir_iter1_."strats_dir/".$out_array_[1];

print "\n";
my $dir_iter2_ = $dir_."run2/";
my $out2_ = `$param_refresh_script_ $config2_ $new_strat_ $start_date_ $end_date_ $dir_iter2_`;
my @out_array2_ = split '\n', $out2_;
my $improvement2_ = $out_array2_[0];
my $final_strat_ = $dir_iter2_."strats_dir/".$out_array2_[1];

print $out2_;
print "\n";
my $new_param_ = `cat $final_strat_ | awk '{print \$5}'`; chomp($new_param_);
my $net_improvement_ = 100 * ( (1 + $improvement1_) *  (1 + $improvement2_) - 1 );
if( $net_improvement_  > 0.2){
  print "Net train set Improvement = $net_improvement_%\n";
  print "Old param = $param_\n";
  print "New param = $new_param_\n";
  
  my $test_strat_ = $dir_."test/strat";
  my $test_results_ = $dir_."test/result/";

  `cp $final_strat_ $test_strat_/`;
  `cp $stratfile_ $test_strat_/`;
  `/home/dvctrader/basetrade_install/ModelScripts/run_simulations_to_result_dir.pl $shortcode_ $test_strat_ $test_start_date_ $test_end_date_ ALL $test_results_`;
  my $result_ = `~/basetrade_install/bin/summarize_local_results_dir_and_choose_by_algo kCNAPnlSharpeAverage 5000 5000 -1 1 5000 100000 $test_results_ | grep ST`;
  #print "$result_\n";
 
  print "\n";
  my @result_array_ = split '\n', $result_;
  
  my $old_score_ = 0;
  my $new_score_ = 0;
  for( my $i = 0; $i<= $#result_array_; $i++ )
  {
    my $line1_ = $result_array_[$i];
    my @array_line1_ = split ' ', $line1_;
    my $strat_name_ = $array_line1_[1];

    $i ++;
    my $line2_ = $result_array_[$i];
    my @array_line2_ = split ' ', $line2_;
    my $pnl_ = $array_line2_[1];
    my $sharpe_ = $array_line2_[4];
    my $gain_pain_ratio_ = $array_line2_[20];
    my $pnl_by_maxloss_ = $array_line2_[21];
    my $volume_ = $array_line2_[3];
    
    if($strat_name_ eq $out_array2_[1]){
      $new_score_ = ($sharpe_ + $gain_pain_ratio_ + $pnl_by_maxloss_)/3;
      print "New $line2_\n";
    }
    else{
      $old_score_ = ($sharpe_ + $gain_pain_ratio_ + $pnl_by_maxloss_)/3; 
      print "Old $line2_\n";
    }
  }
  my $test_improvement_ = 100 * ($new_score_ - $old_score_)/abs($old_score_);
  print "Test improvement = $test_improvement_%\n";

  print "\n*********************Difference in Params**************************\n\n";
  print "     Old Param                                    New Param          \n\n";
  my $diff_ = `sdiff $param_ $new_param_ | grep "|"`;
  print "$diff_\n";
  my $new_param_content_ = `cat $new_param_`;
  print "******************************New Param*******************************\n";
  print "$new_param_content_\n\n";

  if($auto_replace_params_ == 1 && $test_improvement_ >= 10)
  {
    if($rename_param_ == 0)
    {
      `cp $new_param_ $param_`;
      `chmod 777 $param_`;
      print "The param has been changed successfully, percentage improvement = $test_improvement_\n";
    }    
    elsif($rename_param_ == 1){
      my $p1_ = $param_;
      my $new_p1_ = "";
      my @split_ = split '_ref', $p1_;

      my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );

      if ($#split_ <= 0 )
      {
        $new_p1_ = $p1_."_ref$unique_gsm_id_";
      }
      else{
        my $number_ = $split_[1];
        my $new_number_ = $unique_gsm_id_;
        $new_p1_ = $split_[0]."_ref".$new_number_;
      }
      my $cmd_ = "sed -i \"s#$p1_#$new_p1_#\" $stratfile_";
      `$cmd_`;
      `cp $new_param_ $new_p1_`;
      `chmod 777 $new_p1_`;
      print "The param has been changed and renamed successfully, percentage improvement = $test_improvement_\n";
    }

    elsif($rename_param_ == 2){
      my $new_stratfile_ = $stratfile_."_ref";
      `cp $stratfile_ $new_stratfile_`;
      my $p1_ = $param_;
      my $new_p1_ = "";

      my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );

      my @split_ = split '_ref', $p1_;
      
      if ($#split_ <= 0 )
      { 
        $new_p1_ = $p1_."_ref$unique_gsm_id_";
      }
      else{
        my $number_ = $split_[1];
        my $new_number_ = $unique_gsm_id_;
        $new_p1_ = $split_[0]."_ref".$new_number_;
      }
      my $cmd_ = "sed -i \"s#$p1_#$new_p1_#\" $new_stratfile_";
      `$cmd_`;
      `cp $new_param_ $new_p1_`;
      `chmod 777 $new_p1_`;
      print "The param and strat have been changed and renamed successfully, percentage improvement = $test_improvement_\n";
    }

  }
}
else{
  print "Net train improvement = $net_improvement_ , less than threshold"; 
}

