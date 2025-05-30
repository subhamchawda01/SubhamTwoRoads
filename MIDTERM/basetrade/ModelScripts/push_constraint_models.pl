#!/usr/bin/perl

use FileHandle;

my $USAGE="$0 genstrat_path_ sharp_threshold_";
if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }
my $genstrat_path_  = $ARGV [ 0 ];
my $sharp_threshold_ = $ARGV [ 1 ];

my $strat_ = `ls $genstrat_path_/strats_dir/* | grep w_strat_im__ | head -n1`; chomp($strat_);
my $prods_ = `cat $strat_ | grep STRATEGYLINE | awk '{ print \$2 }'`;

my @prod_array_ = split '\n', $prods_;

for( my $i = 0; $i <= $#prod_array_; $i ++)
{
  my $lines_ = `~/basetrade_install/bin/summarize_local_results_dir_and_choose_by_algo kCNAPnlSharpeAverage 4000 4000 -1 1 5000 100000 $genstrat_path_/strats_dir/results/ | grep ST | grep -A1 $prod_array_[$i]  | less | head -n2`;

  my @line_array_ = split '\n', $lines_;
  my $line1_ = $line_array_[0]; chomp($line1_);
  my $line2_ = $line_array_[1];

  my @array1_ = split ' ', $line1_;
  my $strat_name_ = $array1_[1];
  my $dir_ = "/home/dvctrader/modelling/strats/$prod_array_[$i]/IST_917-IST_1528";
  if (-d "$dir_") { }
  else { `mkdir -p $dir_`; }

  my $strat_path_ = $dir_."/$strat_name_";

  my @array2_ = split '_', $strat_name_;
  my $this_strat_ = `ls $genstrat_path_/strats_dir/w_strat_im__*$array2_[$#array2_ - 3] | head -n1`; chomp($this_strat_);
  my $param_ = `cat $this_strat_ | grep " $prod_array_[$i] " | awk '{ print \$4}'`; chomp($param_);
  my $model_ = `cat $this_strat_ | grep " $prod_array_[$i] " | awk '{ print \$3}'`; chomp($model_);

  my @array3_ = split '/', $model_;
  my $modelshort_ = $array3_[$#array3_];
  my @array2_ = split ' ', $line2_;
  my $sharpe_ = $array2_[4];  

  print "$prod_array_[$i]  ";
  if($sharpe_ >= $sharp_threshold_){
    my $model_path_ = "/home/dvctrader/modelling/models/$prod_array_[$i]/IST_917-IST_1528/";
    if (-d "$model_path_") { }
    else { `mkdir -p $model_path_`; }

    `cp $model_ $model_path_`;
    open STRAT, "> $strat_path_" or PrintStacktraceAndDie ( "Could not open $strat_path_ for writing\n" );   
    print STRAT "STRATEGYLINE $prod_array_[$i] DirectionalAggressiveTrading /home/dvctrader/modelling/models/$prod_array_[$i]/IST_917-IST_1528/$modelshort_ $param_ IST_917 IST_1528 999999\n";
    print "   $line2_\n";
    `chmod 777 /home/dvctrader/modelling/models/$prod_array_[$i]/IST_917-IST_1528/$modelshort_`;
    `chmod 777 $strat_path_`;
  }
  else{
    print "\n";
  }
}
