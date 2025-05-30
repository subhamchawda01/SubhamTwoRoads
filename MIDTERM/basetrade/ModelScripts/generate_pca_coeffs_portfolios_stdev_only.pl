#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname
use List::Util qw(max);
my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $BASETRADEINFODIR="/spare/local/tradeinfo/";
my $PCAFILE_BASE_DIR=$BASETRADEINFODIR."PCAInfo";

my $SPARE_HOME="/spare/local/".$USER."/";
my $SPARE_PCA_DIR=$SPARE_HOME."pcadata";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $yyyymmdd_=`date +%Y%m%d`; chomp ( $yyyymmdd_ );

my $PCAFILE_OUT_DIR=$SPARE_PCA_DIR."/".$yyyymmdd_;

require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/sync_to_all_machines.pl"; # SyncToAllMachines
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 my_pca_pairs_file_prefix output_filepath [start_hhmm end_hhmm] [seconds_to_compare] [stdev_only(0)=1] [num_days=15]";

my $pca_based_pairs_file_ = $BASETRADEINFODIR."/portfolio_inputs"; # $PCAFILE_BASE_DIR."/my_pcapairs";

if ( $#ARGV < 1 ) 
{
    print "USAGE: $USAGE\n";
    exit ( 0 );
}
$pca_based_pairs_file_ = $ARGV[0];    

# my $INCOMPLETE_PCA_FILE = $PCAFILE_BASE_DIR."/incomplete_pca_portfolio_stdev_eigen_".$yyyymmdd_.".txt";
my $FINAL_PCA_FILE = $PCAFILE_BASE_DIR."/pca_portfolio_stdev_eigen_".$yyyymmdd_.".txt";

# For Stdev Only Calucaltion 
my $STDEV_ONLY=0;
#my $FINAL_STDEV_FILE = $PCAFILE_BASE_DIR."/pca_portfolio_stdev_".$yyyymmdd_.".txt";
my $stdev_log_file = $PCAFILE_BASE_DIR."/pca_portfolio_stdev_log.txt";

my @stdevs_ = ();
my @sum_=( 0 );
my @sum2_ = (0);
my $n = 0;
# ----
if($#ARGV >= 1) 
{
    $FINAL_PCA_FILE=$ARGV[1];
    my $FINAL_PCA_FILE_BASENAME=basename($FINAL_PCA_FILE);
    my $FINAL_PCA_FILE_DIRNAME=dirname($FINAL_PCA_FILE);
#    $INCOMPLETE_PCA_FILE=$FINAL_PCA_FILE_DIRNAME."/incomplete_".$FINAL_PCA_FILE_BASENAME;
}

my $datagen_start_hhmm_utc_ = "EST_800";
my $datagen_end_hhmm_utc_ =  "EST_1600";
if($#ARGV >= 3)
{
    $datagen_start_hhmm_utc_ = $ARGV[2] ;
    $datagen_end_hhmm_utc_ = $ARGV[3] ;
}

my $break_datagen_into_parts_for_ose = 0 ;
if ( ( $FINAL_PCA_FILE =~ m/_OSE$/ ) && ( GetUTCHHMMStr($datagen_start_hhmm_utc_, $yyyymmdd_) < GetUTCHHMMStr("JST_1515", $yyyymmdd_) ) && ( GetUTCHHMMStr($datagen_end_hhmm_utc_, $yyyymmdd_) > GetUTCHHMMStr("JST_1630", $yyyymmdd_) ) )
{
    $break_datagen_into_parts_for_ose = 1;
}

my $seconds_to_compare_ = 120; # 120 seconds convergence relationship estimated
my $t_final_pca_file_ = $FINAL_PCA_FILE;
if($#ARGV >= 4)
{
    $seconds_to_compare_ = $ARGV[4];
    $t_final_pca_file_ = $PCAFILE_BASE_DIR."/pca_portfolio_stdev_eigen_".$yyyymmdd_.".txt_".$seconds_to_compare_;
}

if($#ARGV >= 5)
{
    $STDEV_ONLY=$ARGV[5];
    if ( $STDEV_ONLY ) {
	print "Running STDEV only version\n";
    }
    open STDEV_LOG , ">$stdev_log_file" or PrintStacktraceAndDie("Could not open $stdev_log_file for writing\n" ); 
}

my $this_pca_based_pairs_filename_ = $pca_based_pairs_file_;
if ( !( -e $this_pca_based_pairs_filename_ ))
{
    printf STDERR " $this_pca_based_pairs_filename_  doesnot exist \n";
    PrintStacktraceAndDie ( "File $this_pca_based_pairs_filename_ doesnot exits" );
}
`mkdir -p $PCAFILE_OUT_DIR`;
my $this_pca_based_pairs_outfilename = $PCAFILE_OUT_DIR."/tdata_";

my @port_vec_ = ();
my @const_vec_ = (); # vector of all constituents that we need to comute simple trend
my %port_to_const_list_map_=(); # map from portfolio_shortcode to list of shortcodes
my %const_to_index_map_=(); # map from constituent_shortcode to index at which it is in the list

#open the portfolio file
open PCA_PAIRS_FILE, "< $this_pca_based_pairs_filename_ " or PrintStacktraceAndDie ( "Could not open $this_pca_based_pairs_filename_\n" );

#first collect shortodes of interest to process only once all the things
while ( my $this_pca_based_pairs_filename_line_ = <PCA_PAIRS_FILE> )
{
    chomp ( $this_pca_based_pairs_filename_line_ );
    my @pcf_words_ = split( ' ', $this_pca_based_pairs_filename_line_ );
    if ( ($#pcf_words_ >= 2 ) && ( $pcf_words_[0] eq "PLINE" ))
    {
	my $port_shortcode_ = $pcf_words_[1];
	push (@port_vec_, $port_shortcode_ );
	splice ( @pcf_words_, 0, 2 );
	$port_to_const_list_map_{$port_shortcode_} = join ( '^', @pcf_words_ );
	for ( my $i = 0; $i <= $#pcf_words_; $i ++)
	{	 
	    my $const_ = $pcf_words_[$i];
	    if ( ! ( exists $const_to_index_map_{$const_} ))
	    {
		push ( @const_vec_, $const_);
		$const_to_index_map_{$const_} = $#const_vec_;		
	    }
	}
    }
}
close PCA_PAIRS_FILE ;

if ( $#const_vec_ < 0 )
{
    printf STDERR "No constituents found\n";
    exit ( 0 );
}

# print "@const_vec_";
# foreach my $ss_ ( sort keys %const_to_index_map_ ) 
# {
#     printf "%s %d \n", $ss_, $const_to_index_map_{$ss_};
# }
#generate only pca_ilist_file with simple trend

my $pca_out_ilist_filename_ = $PCAFILE_OUT_DIR."/Portfolio_All_constituent_ilist.out.txt";
open PCA_ILIST_FILE, "> $pca_out_ilist_filename_ " or PrintStacktraceAndDie ( "Could not open $pca_out_ilist_filename_ for writting \n" );

print PCA_ILIST_FILE "MODELINIT DEPBASE NONAME\n";
print PCA_ILIST_FILE "MODELMATH LINEAR CHANGE\n";
print PCA_ILIST_FILE "INDICATORSTART\n";

print "const_vec_size: $#const_vec_\n" ;
for ( my $i =0 ; $i <= $#const_vec_; $i++ ) 
{
    printf PCA_ILIST_FILE "INDICATOR 1.00 SimpleTrend %s %d MktSizeWPrice\n", $const_vec_[$i], $seconds_to_compare_;
}
print PCA_ILIST_FILE "INDICATOREND\n";
close PCA_ILIST_FILE;

if ( ! -e $pca_out_ilist_filename_ )
{
    PrintStacktraceAndDie ( "$pca_out_ilist_filename_ ilist is not readable!" );
}

#compute one big file that contains all the datagen column wise TIME, L1, DELP1, DELP2, DELP3....

#run the exec
my $datagen_end_yyyymmdd_ = CalcPrevWorkingDateMult ( $yyyymmdd_, 1 );
my $max_days_at_a_time_ = 15;
if($#ARGV >= 6)
{
    $max_days_at_a_time_ = $ARGV[6]; 
}
my $datagen_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $datagen_end_yyyymmdd_, $max_days_at_a_time_ );
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;


# # Hardcoded...TODO{} take args later
my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 0;
my $datagen_num_trades_timeout_ = 0;	
my $to_print_on_economic_times_ = 0;
my $tradingdate_ = $datagen_end_yyyymmdd_;
my $out_file_final_catted= $this_pca_based_pairs_outfilename."Portfolio_All_"."out.datagen.txt";

while ( $tradingdate_ >= $datagen_start_yyyymmdd_ ) 
{
    if ( ! ValidDate ( $tradingdate_ ) )
    {
 	last;
    }

    if ( SkipWeirdDate ( $tradingdate_ ) ||
	 NoDataDate ( $tradingdate_ ) ||
	 IsDateHoliday ( $tradingdate_ ) ) 
    {
	$tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
	next;
    }

    my $out_file_for_this_date= $this_pca_based_pairs_outfilename."Portfolio_All_Constituents_".$tradingdate_.".out.datagen.txt";
    
    if ( $break_datagen_into_parts_for_ose == 0 )
    {
    my $exec_cmd = "$LIVE_BIN_DIR/datagen $pca_out_ilist_filename_ $tradingdate_ ".$datagen_start_hhmm_utc_." ".$datagen_end_hhmm_utc_." $unique_gsm_id_ $out_file_for_this_date $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
    
    print STDERR "$exec_cmd\n";
    
    #my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
#     #`rm -f $datagen_logfile_`;
    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
#    if ( ! -e $out_file_for_this_date ) {
	my @rep_matrix_text_ = `$exec_cmd`;
#	print STDOUT @rep_matrix_text_."\n";
#    }
    }
    else {
	my $out_file_for_this_date_1=$out_file_for_this_date."_1";
        my $out_file_for_this_date_2=$out_file_for_this_date."_2";
        my $exec_cmd_1 = "$LIVE_BIN_DIR/datagen $pca_out_ilist_filename_ $tradingdate_ ".$datagen_start_hhmm_utc_." "."JST_1515"." $unique_gsm_id_ $out_file_for_this_date_1 $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1 2>/dev/null";
        my $exec_cmd_2 = "$LIVE_BIN_DIR/datagen $pca_out_ilist_filename_ $tradingdate_ "."JST_1630"." ".$datagen_end_hhmm_utc_." $unique_gsm_id_ $out_file_for_this_date_2 $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1 2>/dev/null";
        $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
        my @rep_matrix_text_ = `$exec_cmd_1`;
        @rep_matrix_text_ = `$exec_cmd_2`;
        `cat $out_file_for_this_date_1 $out_file_for_this_date_2 > $out_file_for_this_date` ;
        `rm -f $out_file_for_this_date_1 $out_file_for_this_date_2`;
    }


    if ( $STDEV_ONLY ) {
	my @res = `awk '{ for(i=3;i<=NF;i++){s[i] += \$i; s2[i] += \$i * \$i;} }END{ print NR; for(i=3;i<=NF;i++) print s[i], s2[i];}' $out_file_for_this_date`; 
	chomp(@res); # print ">>> $#res\n";
	$n += $res[0];
	for ( my $i=0; $i<$#res; $i++ ) {
	    if( ! defined $sum_[$i] || $sum_[$i] eq '' ) { $sum_[$i]=$sum2_[$i]=0; }
	    my @t_ = split(/ /, $res[$i+1]); 
	    if ( $#t_ == 1 ) {
		$sum_[$i] += $t_[0]; $sum2_[$i] += $t_[1];
	    }
	}
	print STDERR $#sum_."\n";
	print STDEV_LOG "$tradingdate_ | @res\n";
	# print "STDEVS:\n@stdevs_\nSUM:\n@sum_\nSquarenSUM:\n@sum2_\nn: $n\n";
    }
    else {
	#cat the file 
	`cat $out_file_for_this_date >> $out_file_final_catted`;
	#remove the date_wise_temp file
    }
    if ( -e $out_file_for_this_date ) 
    {
 	`rm -f $out_file_for_this_date `;
    }
}

`rm -f $pca_out_ilist_filename_`;

if ( $STDEV_ONLY ) {
    for( my $i =0 ; $i<=$#sum_; $i++ ){
	$stdevs_[$i] = sqrt( max( 0.0, $sum2_[$i]/ $n - ($sum_[$i]*$sum_[$i])/($n*$n) ));
    }
    #open STDEV_OUT_FILE, "> $FINAL_STDEV_FILE" or PrintStacktraceAndDie ( "Could not open $FINAL_STDEV_FILE for writting \n" );
    # print "STDEVS:\n@stdevs_\nSUM:\n@sum_\nSquarenSUM:\n@sum2_\nn: $n\n";
}


#Now we have all the datagen, we will make separate files for each portfolio i.e their corresponding datagen 
foreach my  $my_portfolio_name (sort keys %port_to_const_list_map_ ) 
{
    if ( $STDEV_ONLY ) {
	my $my_constituent_ = $port_to_const_list_map_{$my_portfolio_name};
	#split
	my @my_constituent_vec = split ( /\^/, $my_constituent_ );
	#Get index of each constuent
	my $t_str_ = "PORTFOLIO_STDEV $my_portfolio_name";
	for ( my $ii = 0; $ii <= $#my_constituent_vec; $ii++ ) 
	{
	    if ( exists ( $const_to_index_map_ {$my_constituent_vec[$ii]} ) )
	    {
		my $idx = $const_to_index_map_ {$my_constituent_vec[$ii]}; #index settlement
		if ( ! defined $stdevs_[$idx] ) {
		    print STDERR "Port: $my_portfolio_name const: $my_constituent_vec[$ii]; IDX: $idx\n"; 
		}
		$t_str_ .= sprintf(" %.6f", $stdevs_[$idx]);
	    }
	}
	print $t_str_."\n";
    }
    elsif ( -e $out_file_final_catted ) 
    {
	my $my_constituent_ = $port_to_const_list_map_{$my_portfolio_name};
	#split
	my @my_constituent_vec = split ( /\^/, $my_constituent_ );
	#Get index of each constuent
	my $awk_args = "print \$1 \" \" \$2  \" \" ";
	for ( my $ii = 0; $ii <= $#my_constituent_vec; $ii++ ) 
	{
	    if ( exists ( $const_to_index_map_ {$my_constituent_vec[$ii]} ) )
	    {
		my $idx = $const_to_index_map_ {$my_constituent_vec[$ii]} + 3; #index settlement
		$awk_args = $awk_args." \$".$idx."\" \"";
	    }
	}
	my $individual_port_filename = $this_pca_based_pairs_outfilename.$my_portfolio_name.".out.datagen.txt";
	my $awk_file_separate_cmd = "awk '{ $awk_args }' $out_file_final_catted > $individual_port_filename";
	`$awk_file_separate_cmd`;
	my $pca_gen="$SCRIPTS_DIR/com_pca_port.R  $my_portfolio_name $individual_port_filename >> $t_final_pca_file_ " ;
	print "Executing $pca_gen \n";
	` $pca_gen ` ;
    }
}

if ( $STDEV_ONLY ) {
#    close STDEV_OUT_FILE;
    close STDEV_LOG;
    `rm -f $PCAFILE_OUT_DIR/*`; exit;
}

if ( $t_final_pca_file_ ne $FINAL_PCA_FILE )
{ # Move this temporary PCA file to the expected output file name that the script was provided.
    `mv $t_final_pca_file_ $FINAL_PCA_FILE`;
}

#Remove the Main file since they have been split into their portfolio constitunets already
if ( -e $out_file_final_catted )
{
    `rm -f $out_file_final_catted`;
}


#clear the datgen content
`rm -f $PCAFILE_OUT_DIR/*`;
