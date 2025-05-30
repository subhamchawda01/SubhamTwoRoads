#!/usr/bin/perl

#use strict;
#use warnings;
use FileHandle;
use List::Util qw/max min/; # for max

#initialise paths:
my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";

require "$GENPERLLIB_DIR/get_spare_local_dir_for_aws.pl"; # GetSpareLocalDir
require "$GENPERLLIB_DIR/array_ops.pl"; #GetStdev

sub PrintPermuteParam
{  
  my ($dir_, $tag_array_ref_, $key_array_ref_, $value_mat_ref_) = @_;
  my $param_permute_file_ = $dir_."param_permute_file";
  open PARAM, "> $param_permute_file_" or PrintStacktraceAndDie ( "Could not open $param_permute_file_ for writing\n" ); 
  for( my $i = 0; $i<=$#$tag_array_ref_; $i++ )
  {
    print PARAM "$$tag_array_ref_[$i] ";
    print PARAM "$$key_array_ref_[$i] ";
    my $row_ref_ = $$value_mat_ref_[$i];
    for ( my $j = 0; $j <= $#$row_ref_ ; $j ++)
    {
      print PARAM "$$row_ref_[$j] ";
    }
    print PARAM "\n" ;
  }
}

sub PrintMat
{
  my ( $mat ) = @_;
  for ( my $i = 0; $i <= $#$mat ; $i ++)
  {
    my $row_ref_ = $$mat[$i];
    for ( my $j = 0; $j <= $#$row_ref_ ; $j ++)
    { 
      print "$$row_ref_[$j] ";
    }
    print "\n" ;
  }
}

sub PrintArray
{
  my ($array_) = @_;
  for( my $i = 0; $i<=$#$array_; $i++ )
  {   
    print "$$array_[$i] ";
  }
  print "\n";
}

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
  return $work_dir_;
}


my $USAGE="$0 config_file_ stratfile_path_ start_date end_date dir_";
if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $config_ = $ARGV [ 0 ];
my $stratfile_ = $ARGV [ 1 ];
my $start_date_ = $ARGV [ 2 ];
my $end_date_ = $ARGV [ 3 ];
my $dir_ = $ARGV [ 4 ];
#print "$dir_\n";

my $param_ = `cat $stratfile_ | awk '{print \$5}'`; chomp($param_);
my $param_content_ = `cat $param_`;
my @param_line_ = split '\n', $param_content_;

my @tag_array_ = ();
my @key_array_ = ();
my @value_mat_ = ();

my $zeropos_keep_ = 0;
my $is_keep_present_ = `cat $param_ | awk 'BEGIN{FS=OFS="#";}{print \$1;}' | grep ZEROPOS_KEEP | wc -l`;
if($is_keep_present_ > 0)
{
  $zeropos_keep_ = `cat $param_ | grep ZEROPOS_KEEP | awk '{print \$3}'`;
}
else
{
  $zeropos_keep_ = `cat $param_ | grep ZEROPOS_PLACE | awk '{print \$3}'`;
}
my $max_unit_ratio_ = `cat $param_ | grep MAX_UNIT_RATIO | awk '{print \$3}'`;


for( my $i = 0; $i <= $#param_line_; $i ++ )
{
  my $temp_line_ = $param_line_[$i];
  chomp($temp_line_);
  my @words_ = split " ", $temp_line_;
  push( @tag_array_, $words_[0] );
  push( @key_array_, $words_[1] );
  my @value_array_ = ();
  push( @value_array_, $words_[2] );
  push( @value_mat_, \@value_array_ );
}

my $config_content_ = `cat $config_`;
my @config_line_ = split '\n', $config_content_;
for( my $j = 0; $j <= $#config_line_; $j ++ )
{
  my $temp_line_ = $config_line_[$j];
  chomp($temp_line_);
  my @words_ = split " ", $temp_line_;
  my $permute_ = $words_[0];
  my $permute_percent_ = $words_[1];
  for( my $i = 0; $i <= $#param_line_; $i ++ )
  {
    if( $key_array_[$i] eq $permute_ )
    {
      my $row_ref_ = $value_mat_[$i];
      my $oreg_value_ = $$row_ref_[0];
      if($permute_ eq "WORST_CASE_UNIT_RATIO")
      {
        push ( $row_ref_, 0 );
        push ( $row_ref_, int($max_unit_ratio_) );
      }
      elsif($permute_ eq "NUM_NON_BEST_LEVELS_MONITORED")
      {
        push ( $row_ref_, 0 );
        push ( $row_ref_, 1 );
        push ( $row_ref_, 2 );
        push ( $row_ref_, 3 );
      }
      elsif($permute_ eq "ALLOWED_TO_AGGRESS")
      {
         if($$row_ref_[0] == 0)
         {
           push ( $row_ref_, 1 );
         }
         else{
           push ( $row_ref_, 0 );
         }
      }
      elsif($permute_ eq "AGGRESSIVE")
      {
        my $temp1_ = max(0.1, 0.2 * $zeropos_keep_);
        my $max_ = max(2, 0.2 * $zeropos_keep_); 
        push ( $row_ref_, $temp1_ );
        push ( $row_ref_, 2 * $temp1_ );
        push ( $row_ref_, 3 * $temp1_ );
        push ( $row_ref_, 4 * $temp1_ );
        push ( $row_ref_, $max_ );
      }
      elsif($permute_ eq "PLACE_KEEP_DIFF")
      {
        my $temp1_ = max(0.1, 0.1 * $zeropos_keep_);       
        push ( $row_ref_, 1 * $temp1_ );
        push ( $row_ref_, 2 * $temp1_ );
        push ( $row_ref_, 3 * $temp1_ ); 
      }
      elsif($permute_ eq "INCREASE_ZEROPOS_DIFF")
      {
        my $temp1_ = max(0.1, 0.1 * $zeropos_keep_);       
        push ( $row_ref_, 1 * $temp1_ );
        push ( $row_ref_, 2 * $temp1_ );
        push ( $row_ref_, 3 * $temp1_ ); 
      }
      elsif($permute_ eq "ZEROPOS_DECREASE_DIFF")
      {
        my $temp1_ = max(0.1, 0.1 * $zeropos_keep_);       
        push ( $row_ref_, 1 * $temp1_ );
        push ( $row_ref_, 2 * $temp1_ );
        push ( $row_ref_, 3 * $temp1_ ); 
      }
      elsif($permute_ eq "MAX_UNIT_RATIO")
      {
        push ( $row_ref_, $oreg_value_ - $permute_percent_ );
        if( $oreg_value_ < 3 )
        {
          push ( $row_ref_, $oreg_value_ + $permute_percent_ );
        }
      }
      else{
        if ( $permute_percent_ < 1 )
        {
          push ( $row_ref_, $oreg_value_* (1 + $permute_percent_ ) );
          push ( $row_ref_, $oreg_value_* (1 - $permute_percent_ ) );
        }
        else{
          push ( $row_ref_, $oreg_value_ + $permute_percent_ );
          push ( $row_ref_, $oreg_value_ - $permute_percent_ );
        }
      }
    } 
  }
}

PrintPermuteParam( $dir_, \@tag_array_, \@key_array_, \@value_mat_ );
my $param_copy_ = $dir_."param";
chomp($param_);
my $cmd_ = "cp $param_ $param_copy_";
`$cmd_`;
#PrintMat(\@value_mat_);

my $script_ = "$MODELSCRIPTS_DIR/parllel_params_permute_param_refresh.pl";
#print "$script_ $param_permuted_file_ $start_date_ $end_date_ $stratfile_ INVALID -1 $dir_";
#print "hi\n";
my $param_permuted_file_ = $dir_."param_permute_file";
my $out_ = `$script_ $param_permuted_file_ $start_date_ $end_date_ $stratfile_ INVALID -1 $dir_`;

my $local_dir_ = $dir_."local_results_base_dir/";

my $result_ = `~/basetrade_install/bin/summarize_local_results_dir_and_choose_by_algo kCNAPnlSharpeAverage 5000 5000 -1 1 5000 100000 $local_dir_ | grep ST`;
my @result_array_ = split '\n', $result_;

my $oreg_strat_name_;
my $oreg_sharpe_;
my $oreg_gain_pain_ratio_;
my $oreg_pnl_by_maxloss_;
my $oreg_volume_;

my @strat_name_array_ = ();
my @sharpe_array_ = ();
my @gain_pain_ratio_array_ = ();
my @pnl_by_maxloss_array_ = ();
my @score_array_ = ();
my @volume_array_ = ();

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

  if($strat_name_ eq "strat_0_0_1001")
  {
    if( $sharpe_ == 0 ){ $sharpe_ = 0.01; }
    if( $gain_pain_ratio_ == 0 ){ $gain_pain_ratio_ = 0.01; }
    if( $pnl_by_maxloss_ == 0 ){ $pnl_by_maxloss_ = 0.01; }

    $oreg_strat_name_ = $strat_name_;
    $oreg_sharpe_ = $sharpe_;
    $oreg_gain_pain_ratio_ = $gain_pain_ratio_;
    $oreg_pnl_by_maxloss_ = $pnl_by_maxloss_;
    $oreg_volume_ = $volume_;
  }
  push( @strat_name_array_, $strat_name_);
  push( @sharpe_array_, $sharpe_ );
  push( @gain_pain_ratio_array_, $gain_pain_ratio_);
  push( @pnl_by_maxloss_array_, $pnl_by_maxloss_);
  push( @volume_array_, $volume_);
}

sub SortArray
{
  my ( $array1_, $array2_, $array3_ ) = @_;
  for ( my $i = 0; $i <= $#$array1_ ; $i ++)
  {
    for ( my $j = 0; $j <= $#$array2_ ; $j ++)
    {
      if($$array3_[$i] > $$array3_[$j])
      {
        my $temp_ = $$array3_[$j];
        $$array3_[$j] = $$array3_[$i];
        $$array3_[$i] = $temp_;
        my $temp1_ = $$array2_[$j];
        $$array2_[$j] = $$array2_[$i];
        $$array2_[$i] = $temp1_;
        my $temp2_ = $$array1_[$j];
        $$array1_[$j] = $$array1_[$i];
        $$array1_[$i] = $temp2_;
      }
    }
  }
}

for( my $i = 0; $i<= $#strat_name_array_; $i++ )
{
  $sharpe_array_[$i] = ($sharpe_array_[$i] - $oreg_sharpe_)/abs($oreg_sharpe_);
  $gain_pain_ratio_array_[$i] = ($gain_pain_ratio_array_[$i] - $oreg_gain_pain_ratio_)/abs($oreg_gain_pain_ratio_);
  $pnl_by_maxloss_array_[$i] = ($pnl_by_maxloss_array_[$i] - $oreg_pnl_by_maxloss_)/abs($oreg_pnl_by_maxloss_);

  if( $oreg_sharpe_ < 0 && $sharpe_array_[$i] > 0) { $sharpe_array_[$i] ++; }
  if( $oreg_gain_pain_ratio_ < 0 && $gain_pain_ratio_array_[$i] > 0){ $gain_pain_ratio_array_[$i] ++; }
  if( $oreg_pnl_by_maxloss_ < 0 && $pnl_by_maxloss_array_[$i] > 0){ $pnl_by_maxloss_array_[$i] ++; }

  $volume_array_[$i] = $volume_array_[$i] /$oreg_volume_;
  my $score_ = ($sharpe_array_[$i] + $gain_pain_ratio_array_[$i] + $pnl_by_maxloss_array_[$i])/3;
  push( @score_array_, $score_);
}

SortArray( \@strat_name_array_, \@volume_array_, \@score_array_);

for( my $i = 0; $i <= $#strat_name_array_; $i ++ )
{
  if($volume_array_[$i] > 0.7)
  {
    my $improvement_ = ($score_array_[$i]) ;
    print "$improvement_\n";
    print "$strat_name_array_[$i]\n";
    print $strat_name_array_[$i]." ";
    my $stat_new_ = `~/basetrade_install/bin/summarize_local_results_dir_and_choose_by_algo kCNAPnlSharpeAverage 5000 5000 -1 1 5000 100000 $local_dir_ | grep ST | grep -A1 $strat_name_array_[$i] | grep STATISTICS`; chomp($stat_new_);
    print "$stat_new_\n";
    print "$oreg_strat_name_ ";
    my $stat_old_ = `~/basetrade_install/bin/summarize_local_results_dir_and_choose_by_algo kCNAPnlSharpeAverage 5000 5000 -1 1 5000 100000 $local_dir_ | grep ST | grep -A1 $oreg_strat_name_ | grep STATISTICS`; chomp($stat_old_);
    print "$stat_old_\n";
    last;
  }
}
