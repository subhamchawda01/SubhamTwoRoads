use strict;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw(max); # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "diwakar" )
{
      $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}
my $MODELING_BASE_DIR=$HOME_DIR."/modelling";

my $TRADELOG_DIR="/spare/local/tradeinfo/volatilitylogs/";

my $USAGE = "$0 PRODUCT ilist START_DATE END_DATE START_TIME END_TIME";

my $DATAGEN_EXEC = $LIVE_BIN_DIR."/datagen";
my $CONVERT_TO_REG_DATA_EXEC = $LIVE_BIN_DIR."/timed_data_to_reg_data";
my $SPLIT_DATA_EXEC = $MODELSCRIPTS_DIR."/split_reg_data_file.pl";
my $VOL_PERIOD = $MODELSCRIPTS_DIR."/get_volatile_periods_for_shortcode.pl";
my $get_prv_date_ = $LIVE_BIN_DIR."/calc_prev_week_day";
my $REGRESS_EXEC = $LIVE_BIN_DIR."/callFSLR";
my $PLACE_COEFF_EXEC = $MODELSCRIPTS_DIR."/place_coeffs_in_model.pl";
if ( $#ARGV < 0 ){
  print $USAGE."\n";
  exit;
}

my $product_ = $ARGV[0];
my $ilist = $ARGV[1];
my $start_date_ = $ARGV[2];
my $end_date_ = $ARGV[3];
my $start_time_ = $ARGV[4];
my $end_time_ = $ARGV[5];
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
`mkdir -p $unique_gsm_id_`;
`cd $unique_gsm_id_`;

print $product_." ".$ilist." ".$start_date_." ".$end_date_." ".$end_time_." ".$start_time_."\n";
my $temp_date_ = $end_date_;
my $no_time_reg_data_file_ = "no_time_reg_data_complete_".$product_;
my $split_reg_data_1_ = "split_reg_dat_complete_".$product_; 
my $split_reg_data_2_ = "split_reg_dat_complete_".$product_."_1";
my $split_reg_data_3_ = "split_reg_dat_complete_".$product_."_2";
my $split_reg_data_4_ = "split_reg_dat_complete_".$product_."_3";
`cat /dev/null > $split_reg_data_1_`;
`cat /dev/null > $split_reg_data_2_`;
`cat /dev/null > $split_reg_data_3_`;
`cat /dev/null > $split_reg_data_4_`;

while ( $temp_date_ >= $start_date_ )
{
  print " This date: ".$temp_date_." ".$start_date_."\n";
  my $datagen_out_file_name_ = "timed_data_".$product_.$temp_date_;
  my $datagen_cmd_ = $DATAGEN_EXEC." ".$ilist." ".$temp_date_." ".$start_time_." ".$end_time_." 1 ". $datagen_out_file_name_." 1000 1 1 0";
  `$datagen_cmd_`;
  print $datagen_cmd_."\n";

  my $convert_reg_data_file_name_ = "no_time_reg_data".$product_.$temp_date_;
  my $no_time_reg_data_cmd_ = $CONVERT_TO_REG_DATA_EXEC ." ".$ilist." ".$datagen_out_file_name_." 2000 na_t3 ".$convert_reg_data_file_name_." NULL 0";
  `$no_time_reg_data_cmd_`;
 
  print $no_time_reg_data_cmd_."\n" ;

  `cat $convert_reg_data_file_name_ >> $no_time_reg_data_file_`; `rm $convert_reg_data_file_name_`;
  
  my $timed_reg_data_file_name_ = "with_time_reg_data".$product_.$temp_date_;
  my $with_time_reg_data_cmd_ = $CONVERT_TO_REG_DATA_EXEC ." ".$ilist." ".$datagen_out_file_name_." 2000 na_t3 ".$timed_reg_data_file_name_." NULL 1";
  `$with_time_reg_data_cmd_`;
  my $reg_data_out_base_ = "split_reg_data_".$product_.$temp_date_; 
  my $split_file_ = $TRADELOG_DIR.$product_.".".$temp_date_.".600_2_0.1";
  my $split_reg_data_cmd_ = "perl ".$SPLIT_DATA_EXEC." ".$timed_reg_data_file_name_." ".$split_file_." ".$reg_data_out_base_;
  `$split_reg_data_cmd_`;
  print $split_reg_data_cmd_."\n";
  `cat $reg_data_out_base_ >> $split_reg_data_1_`; `rm $reg_data_out_base_`;
  `cat $reg_data_out_base_"_1" >> $split_reg_data_2_`; `rm $reg_data_out_base_"_1"`;
  `cat $reg_data_out_base_"_2" >> $split_reg_data_3_`; `rm $reg_data_out_base_"_2"`;
  `cat $reg_data_out_base_"_3" >> $split_reg_data_4_`; `rm $reg_data_out_base_"_3"`;
  `rm $datagen_out_file_name_`;
  $temp_date_ = `$get_prv_date_ $temp_date_`; chomp ($temp_date_);
  print $temp_date_." ".$start_date_."\n";
}
print $no_time_reg_data_file_."\n";
print $split_reg_data_1_."\n";
print $split_reg_data_2_."\n";
print $split_reg_data_3_."\n";
print $split_reg_data_4_."\n";
my $single_coeff_file_ = "single_coeff_file_".$product_;
my $coeff_file1_ = "combined_coeff_1_".$product_;
my $coeff_file2_ = "combined_coeff_2_".$product_;
my $coeff_file3_ = "combined_coeff_3_".$product_;
my $coeff_file4_ = "combined_coeff_4_".$product_;

my $single_model_ = "single_model_".$product_;
my $model_part1_ = "combined_model_1_".$product_;
my $model_part2_ = "combined_model_2_".$product_;
my $model_part3_ = "combined_model_3_".$product_;
my $model_part4_ = "combined_model_4_".$product_;
my $reg_cmd_ =$REGRESS_EXEC ." ".$no_time_reg_data_file_." 0.012 0 0 0.5 ".$single_coeff_file_." 20 0 ".$ilist." N ";
my $reg_1_cmd_ =$REGRESS_EXEC ." ".$split_reg_data_1_." 0.012 0 0 0.5 ".$coeff_file1_." 20 0 ".$ilist." N ";
my $reg_2_cmd_ =$REGRESS_EXEC ." ".$split_reg_data_2_." 0.012 0 0 0.5 ".$coeff_file2_." 20 0 ".$ilist." N ";
my $reg_3_cmd_ =$REGRESS_EXEC ." ".$split_reg_data_3_." 0.012 0 0 0.5 ".$coeff_file3_." 20 0 ".$ilist." N ";
my $reg_4_cmd_ =$REGRESS_EXEC ." ".$split_reg_data_4_." 0.012 0 0 0.5 ".$coeff_file4_." 20 0 ".$ilist." N ";

`$reg_cmd_`;
`$reg_1_cmd_`;
`$reg_2_cmd_`;
`$reg_3_cmd_`;
`$reg_4_cmd_`;
my $single_place_ = $PLACE_COEFF_EXEC." ".$single_model_." ".$ilist." ".$single_coeff_file_;
my $place_1_ = $PLACE_COEFF_EXEC." ".$model_part1_." ".$ilist." ".$coeff_file1_;
my $place_2_ = $PLACE_COEFF_EXEC." ".$model_part2_." ".$ilist." ".$coeff_file2_;
my $place_3_ = $PLACE_COEFF_EXEC." ".$model_part3_." ".$ilist." ".$coeff_file3_;
my $place_4_ = $PLACE_COEFF_EXEC." ".$model_part4_." ".$ilist." ".$coeff_file4_;
`$single_place_`;
`$place_1_`;
`$place_2_`;
`$place_3_`;
`$place_4_`;
`rm $no_time_reg_data_file_`;
`rm $split_reg_data_1_`;
`rm $split_reg_data_2_`;
`rm $split_reg_data_3_`;
`rm $split_reg_data_4_`;
my $combined_model_ = "combined_model_".$product_;
`python /home/diwakar/basetrade/ModelScripts/concatenate_models.py $model_part1_ $model_part2_ $model_part3_ $model_part4_ $combined_model_ SELECTIVE `;
print $combined_model_."\n";
