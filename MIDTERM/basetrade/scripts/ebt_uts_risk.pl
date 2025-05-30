#!/usr/bin/perl
#
## \file ModelScripts/ebt_uts_risk.pl
##
## \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
##  Address:
##    Suite No 162, Evoma, #14, Bhattarhalli,
##    Old Madras Road, Near Garden City College,
##    KR Puram, Bangalore 560049, India
##    +91 80 4190 3551
#

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO="basetrade";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $USAGE="$0 CONFIGFILE RESULTS_DIR START_DATE END_DATE";
require "$GENPERLLIB_DIR/array_ops.pl";



if ( $#ARGV < 0 )
{ 
  print $USAGE."\n"; exit ( 0 );

}


my $global_config_file_ = $ARGV [ 0 ];
my $results_dir_ = $ARGV [ 1 ];
my $start_date_ ;
my $end_date_ ;
if ($#ARGV >2){
     $start_date_ = $ARGV [ 2 ];
     $end_date_ = $ARGV [ 3 ];
}

my @strategies=`cat $global_config_file_ | awk '{ if(\$1=="SHORTCODE-STRAT") { stflag=1; } if(stflag==1 && NF==2) { print \$2; } if(NF==0){stflag=0} }'`;
chomp(@strategies);
#ignoring the comment lines from the array
my @narr = ( );
@narr = grep(!/#/, @strategies);
@strategies = @narr;
my @shortcodes;
foreach my $strat_index (0..$#strategies){
     my $temp_shortcode= `/home/$USER/basetrade/scripts/print_strat_from_base.sh $strategies[$strat_index]|awk -F '/' '{print \$6}'`;chomp($temp_shortcode);
     push(@shortcodes,$temp_shortcode);


}

my $temp_sim_result="temp_db_result";
my $traded_ezone=`cat $global_config_file_  | awk '{ if(\$1=="SHORTCODE-STRAT") { stflag=1; } if(stflag==1 && NF==2) { print \$2; } if(NF==0){stflag=0} }'|head -1 |xargs $SCRIPTS_DIR/print_strat_from_base.sh|xargs cat|awk '{print \$9}' `;
chomp($traded_ezone);
#print "\n",$traded_ezone;
#my $gen_result_cmd="perl /home/dvctrader/basetrade/ModelScripts/ebt_summarize_for_event.pl ALL POOL DB 20150101 20161210 $traded_ezone > $temp_sim_result";
print $start_date_;
if (defined $start_date_){
      my $gen_result_cmd="$LIVE_BIN_DIR/summarize_local_results_dir_and_choose_by_algo kCNAPnlSharpe 4000 4000 -1 1 5000 100000 $results_dir_ $start_date_ $end_date_ > $temp_sim_result";
      print $gen_result_cmd,"\n";
     `$gen_result_cmd`;
}
else{
      my $gen_result_cmd="/home/$USER/basetrade_install/bin/summarize_local_results_dir_and_choose_by_algo kCNAPnlSharpe 4000 4000 -1 1 5000 100000 $results_dir_ > $temp_sim_result";
      print $gen_result_cmd,"\n";
     `$gen_result_cmd`;
}
my @total_pnl_=();
my $global_pnl_=0;



my @shortcode_uts=`cat $global_config_file_|awk '!/^(\$|#)/{ if(\$1=="SHORTCODE-UTS") { stflag=1; } if(stflag==1 && NF==2) { print \$0; } if(NF==0){stflag=0} }'`;

chomp(@shortcode_uts);
#ignoring the comment lines
@narr=( );
@narr = grep(!/#/, @shortcode_uts);
@shortcode_uts=@narr;
my %shortcode_uts_map=();


foreach my $shortcode_uts_index (0..$#shortcode_uts){
    my @temp_tokens=split / /,$shortcode_uts[$shortcode_uts_index];
    $shortcode_uts_map{$temp_tokens[0]}=$temp_tokens[1];
}


#reading the result for the strategies

my %daily_pnl=();
my %daily_max_pnl=();
my %daily_min_pnl=();
for my $strat_index_ (0..$#strategies){

    my @temp_sim_output=`awk '/$strategies[$strat_index_]/ {p=1}; p; /statistic/ {p=0}' $temp_sim_result`;
    chomp(@temp_sim_output);
    for my $date_index (0..$#temp_sim_output){
       my @temp_date_array=split / /,$temp_sim_output[$date_index];
       if ($temp_date_array[0] eq "STRATEGYFILEBASE"){
            next;
       }
       elsif($temp_date_array[0] eq "STATISTICS"){
           last;
       }

      else{
        my $temp_date=$temp_date_array[0];
        my $temp_date_uts=$temp_date_array[33];
        my $uts_scaling=$shortcode_uts_map{$shortcodes[$strat_index_]}/$temp_date_uts;
        my $temp_date_pnl=$temp_date_array[1]*$uts_scaling ;
        my $temp_date_min_pnl=$temp_date_array[9]*$uts_scaling;
        my $temp_date_max_pnl=$temp_date_array[11]*$uts_scaling;

       #adding the scaled PNL in hash table
        if (exists $daily_pnl{$temp_date}){
            $daily_pnl{$temp_date}+=$temp_date_pnl;
            $daily_max_pnl{$temp_date}+=$temp_date_max_pnl;
            $daily_min_pnl{$temp_date}+=$temp_date_min_pnl;     
      
      }
      else{
      
          $daily_pnl{$temp_date}=$temp_date_pnl;
          $daily_max_pnl{$temp_date}=$temp_date_max_pnl;
          $daily_min_pnl{$temp_date}=$temp_date_min_pnl;
      
      }

    }



  }
}


foreach my $day (sort keys %daily_pnl){

     printf "%d PNL: %.2f max: %d min: %d \n",$day,$daily_pnl{$day},$daily_max_pnl{$day},$daily_min_pnl{$day};

}

my @pnl_values=values %daily_pnl;
my @pnl_values_min=values %daily_min_pnl;
my $pnl_avg = GetAverage ( \@pnl_values );
my $pnl_sum = GetSum(\@pnl_values);
my $pnl_sd = 0.01 + GetStdev (\@pnl_values);
my $pnl_sharpe =  $pnl_avg / $pnl_sd ;
my $num_days = scalar @pnl_values;
my $gain_pain = (1 + $pnl_avg*$num_days) / ( 1+ -1*GetSum ( \@pnl_values_min ) );


printf "Avg: %d Sharpe: %.2f GainPain: %.2f  \n",$pnl_avg,$pnl_sharpe,$gain_pain;

`rm $temp_sim_result`;

