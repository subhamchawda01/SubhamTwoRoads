#!/usr/bin/perl
use strict;
use warnings;
use File::Basename; # for basename and dirname
use File::Path qw(mkpath);
use POSIX qw ( ceil floor );
#use Math::Round;

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO = "basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/array_ops.pl"; # NoDataDate
require "$GENPERLLIB_DIR/sample_data_utils.pl"; #GetFilteredDays

sub ReadDataFileIntoMatrix;
sub Histogram;
sub GenerateData;
sub RoundOff;

my $BASETRADEINFODIR="/spare/local/tradeinfo/";
my $SPARE_HOME="/spare/local/".$USER."/";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $MAX_INDICATORS = 18;

# start 
my $USAGE2="$0 MODELFILENAME TRADING_SDATE TRADING_EDATE STARTHHMM ENDHHMM MSECS_TIMEOUT L1_EVENTS_TIMEOUT NUM_TRADES_TIMEOUT TO_PRINT_ON_ECO PREDICTION_DURATION_MSECS=32000 <FEATURE AUX 0.2 0.8/HIGH/LOW>";

my $model_file_name_ ;
my $datagen_start_date_ = `date +'%Y%m%d'` ;
my $datagen_end_date_ = `date +'%Y%m%d'`;
my $datagen_start_hhmm_ ;
my $datagen_end_hhmm_ ;
my $datagen_msecs_timeout_ ;
my $datagen_l1events_timeout_ ;
my $datagen_num_trades_timeout_ ;
my $to_print_on_economic_times_ ;
my $data_file_ = "NA";
my $pred_duration_ = 32000 ;

my ($feature_, $feature_aux_, $lb_perc_, $hb_hl_) = ("NA", "NA", "NA", "NA");


if ( $#ARGV == 1 )
{
    $model_file_name_ = $ARGV[0];
    $data_file_ = $ARGV[1];
}
elsif ( $#ARGV == 8 )
{
    $model_file_name_ = $ARGV[0];
    $datagen_start_date_ = $ARGV[1];
    $datagen_end_date_ = $ARGV[2];
    $datagen_start_hhmm_ = $ARGV[3];
    $datagen_end_hhmm_ = $ARGV[4];
    $datagen_msecs_timeout_ = $ARGV[5];
    $datagen_l1events_timeout_ = $ARGV[6];
    $datagen_num_trades_timeout_ = $ARGV[7];
    $to_print_on_economic_times_ = $ARGV[8];

}
elsif ( $#ARGV == 9 )
{
    $model_file_name_ = $ARGV[0];
    $datagen_start_date_ = $ARGV[1];
    $datagen_end_date_ = $ARGV[2];
    $datagen_start_hhmm_ = $ARGV[3];
    $datagen_end_hhmm_ = $ARGV[4];
    $datagen_msecs_timeout_ = $ARGV[5];
    $datagen_l1events_timeout_ = $ARGV[6];
    $datagen_num_trades_timeout_ = $ARGV[7];
    $to_print_on_economic_times_ = $ARGV[8];
    $pred_duration_ = $ARGV[9];

}
elsif ( $#ARGV == 13 )
{
    $model_file_name_ = $ARGV[0];
    $datagen_start_date_ = $ARGV[1];
    $datagen_end_date_ = $ARGV[2];
    $datagen_start_hhmm_ = $ARGV[3];
    $datagen_end_hhmm_ = $ARGV[4];
    $datagen_msecs_timeout_ = $ARGV[5];
    $datagen_l1events_timeout_ = $ARGV[6];
    $datagen_num_trades_timeout_ = $ARGV[7];
    $to_print_on_economic_times_ = $ARGV[8];
    $pred_duration_ = $ARGV[9];
    ($feature_, $feature_aux_, $lb_perc_, $hb_hl_) = @ARGV[10..13];
}
else
{
    print "$USAGE2\n";
    exit (-1);
}


my $work_dir_=$SPARE_HOME."GCI/";
mkpath $work_dir_;
chdir $work_dir_;

my $unique_gci_id_ = `date +%N`; chomp ( $unique_gci_id_ ); $unique_gci_id_ = int($unique_gci_id_) + 0;
# Start Up
# Map from indicator index to indicator weight.
my $indicator_count_ = 0;
my @indicator_string_ = ( ) ;
my $shortcode_ = "NA" ;

# Start model info
open MODEL_FILE, "< $model_file_name_" or PrintStacktraceAndDie ( "Could not open $model_file_name_\n" );

while ( my $model_file_line_ = <MODEL_FILE> ) 
{
    chomp ($model_file_line_);
    my @model_file_lines_ = split ( ' ', $model_file_line_ );
   
    if ( $model_file_lines_[0] eq "INDICATOR" )
    {
	$indicator_count_++;
	push ( @indicator_string_ , $model_file_line_ );
    }
    if ( $model_file_lines_[0] eq "MODELINIT" ) 
    {
	$shortcode_ = $model_file_lines_[2];
    }
}
close MODEL_FILE;


my @dates_vec_ = GetDatesFromStartDate( $shortcode_, $datagen_start_date_, $datagen_end_date_, "INVALIDFILE", 2000 );


if ( $feature_ ne "NA" ) {
    my @filtered_dates_vec_ = ();
    if ( $hb_hl_ eq "HIGH" || $hb_hl_ eq "LOW" ) {
	GetFilteredDays ( $shortcode_, \@dates_vec_, $lb_perc_, $hb_hl_, $feature_, $feature_aux_, \@filtered_dates_vec_, $datagen_start_hhmm_, $datagen_end_hhmm_ );
    } else {
	GetFilteredDaysInPercRange ( $shortcode_, \@dates_vec_, $lb_perc_, $hb_hl_, $feature_, $feature_aux_, \@filtered_dates_vec_, $datagen_start_hhmm_, $datagen_end_hhmm_ );
    }
 
    @dates_vec_ = @filtered_dates_vec_;
}


my $min_price_increment_ = 1 ;
if ( $shortcode_ ne "NA" )
{
    $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $datagen_end_date_` ; chomp ( $min_price_increment_ ); # using single date in hope that min_inc doesnt change much
}
# End model info

if ( $data_file_ eq "NA" )
{
    GenerateData ( ) ;
}

my @correlations_ = ( ) ;
my $corr_data_file_ = basename ( $model_file_name_ )."_".$unique_gci_id_.".output";

print $corr_data_file_."\n";
for ( my $i_ = 1 ; $i_ <= $indicator_count_  ; $i_ ++ )
{
    for ( my $j_ = $i_ + 1  ; $j_ <= $indicator_count_ + 1 ; $j_ ++ )
    {
	my $cor_cmd_ = "perl $MODELSCRIPTS_DIR/get_correlation.pl -i $data_file_ -col $i_ $j_ " ;
	`echo -n $i_." ".$j_."\t" >> $corr_data_file_` ;
	my $cor_ = `$cor_cmd_ >> $corr_data_file_` ;
	push ( @correlations_ , $cor_ );
    }  
}

my $cmd_ = "for k in `seq 2 19` ; do grep \$k\". \"  $work_dir_$corr_data_file_  | grep -v \"1\"\$k\". \" | awk '{ a = a\" \"\$5 } END { print a}' ; done";
#print $cmd_;
print `$cmd_`;

#my $exec_cmd_ = "~/basetrade_install/bin/get_dep_corr $data_file_ >> $corr_data_file_";
#`$exec_cmd_`;

`rm $work_dir_/$data_file_`;
#`rm $model_file_name_`;

# data start
sub GenerateData
{
    $data_file_ = basename ( $model_file_name_ )."_".$unique_gci_id_.".gci";
    my $temp_file_ = "tmp_".$unique_gci_id_.".gci";
    my $t_temp_file_ = "t_tmp_".$unique_gci_id_.".gci";

    my $cmd_head_ = "$LIVE_BIN_DIR/datagen ".$model_file_name_." ";
    my $cmd_tail_ = $datagen_start_hhmm_." ".$datagen_end_hhmm_." ".$unique_gci_id_." ".$temp_file_." ".$datagen_msecs_timeout_." ".$datagen_l1events_timeout_." ".$datagen_num_trades_timeout_." ".$to_print_on_economic_times_." ADD_DBG_CODE -1";
    
    my $datagen_date_ = $datagen_end_date_;
    my $max_days_at_a_time_ = 2000;
    
    `> $data_file_`;

    my $t2r_cmd_ = "$LIVE_BIN_DIR/timed_data_to_reg_data ".$model_file_name_." ".$temp_file_." $pred_duration_ na_t3 ".$t_temp_file_." fsg1";

    
    for ( my $t_day_index_ = 0 ; $t_day_index_ < scalar(@dates_vec_) ; $t_day_index_ ++ ) 
    {
	my $datagen_date_ = $dates_vec_[$t_day_index_];
	print $datagen_date_."\n";
	my $cmd_ = $cmd_head_." ".$datagen_date_." ".$cmd_tail_;
	# print $cmd_."\n";
	`$cmd_`;
	`$t2r_cmd_`;
	# print "cat $temp_file_ >> $data_file_\n";
	`cat $t_temp_file_ >> $data_file_`;
	`rm $work_dir_/$temp_file_`;
	`rm $work_dir_/$t_temp_file_`;
    }
}
# end
