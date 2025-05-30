#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

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

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 DATE [ my_pca_pairs_file_prefix ]";

if ( $#ARGV < 0 )
{
    print $USAGE."\n";
    exit ( 0 );
}

$yyyymmdd_ = $ARGV [ 0 ];

my $pca_based_pairs_file_ = $BASETRADEINFODIR."/portfolio_inputs"; # $PCAFILE_BASE_DIR."/my_pcapairs";
if ( $#ARGV >= 1 ) 
{
    $pca_based_pairs_file_ = $ARGV[1];    
}

my $seconds_to_compare_ = 120; # 120 seconds convergence relationship estimated

my $this_pca_based_pairs_filename_ = $pca_based_pairs_file_;
if ( !( -e $this_pca_based_pairs_filename_ ))
{
    printf STDERR " $this_pca_based_pairs_filename_  doesnot exist \n";
    PrintStacktraceAndDie ( "File $this_pca_based_pairs_filename_ doesnot exits" );
}
`mkdir -p $PCAFILE_OUT_DIR`;
my $this_pca_based_pairs_outfilename = $PCAFILE_OUT_DIR."/tdata_";
my $FINAL_PCA_FILE=$PCAFILE_BASE_DIR."/pca_portfolio_stdev_eigen_".$yyyymmdd_.".txt";

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
#foreach my $ss_ ( sort keys %const_to_index_map_ ) 
#{
#    printf "%s %d \n", $ss_, $const_to_index_map_{$ss_};
#}
#generate only pca_ilist_file with simple trend

my $pca_out_ilist_filename_ = $PCAFILE_OUT_DIR."/Portfolio_All_constituent_ilist.out.txt";

open PCA_ILIST_FILE, "> $pca_out_ilist_filename_ " or PrintStacktraceAndDie ( "Could not open $pca_out_ilist_filename_ for writting \n" );

print PCA_ILIST_FILE "MODELINIT DEPBASE NONAME\n";
print PCA_ILIST_FILE "MODELMATH LINEAR CHANGE\n";
print PCA_ILIST_FILE "INDICATORSTART\n";

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
my $datagen_end_yyyymmdd_ = $yyyymmdd_;
my $max_days_at_a_time_ = 2000;
my $datagen_start_yyyymmdd_ = CalcPrevWorkingDateMult ( $datagen_end_yyyymmdd_, $max_days_at_a_time_ );
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;


# # Hardcoded...TODO{} take args later
my $datagen_start_hhmm_utc_ = "EST_800";
my $datagen_end_hhmm_utc_ =  "EST_1600";
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
    
    my $exec_cmd = "$LIVE_BIN_DIR/datagen $pca_out_ilist_filename_ $tradingdate_ ".$datagen_start_hhmm_utc_." ".$datagen_end_hhmm_utc_." $unique_gsm_id_ $out_file_for_this_date $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
    
#    print STDOUT "$exec_cmd\n";
    
    #my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
#     #`rm -f $datagen_logfile_`;
    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    my @rep_matrix_text_ = `$exec_cmd`;
#    print STDOUT @rep_matrix_text_."\n";
    
    #cat the file 
    `cat $out_file_for_this_date >> $out_file_final_catted`;
    #remove the date_wise_temp file
    if ( -e $out_file_for_this_date ) 
    {
 	`rm -f $out_file_for_this_date `;
    }
}

`rm -f $pca_out_ilist_filename_`;

#Now we have all the datagen, we will make separate files for each portfolio i.e their corresponding datagen 
foreach my  $my_portfolio_name (sort keys %port_to_const_list_map_ ) 
{
    if ( -e $out_file_final_catted ) 
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
	my $pca_gen="$SCRIPTS_DIR/com_pca_port.R  $my_portfolio_name $individual_port_filename >> $FINAL_PCA_FILE " ;
	print "Executing $pca_gen \n";
	` $pca_gen ` ;
	
    }
}

#Remove the Main file since they have been split into their portfolio constitunets already
if ( -e $out_file_final_catted )
{
    `rm -f $out_file_final_catted`;
}

my $this_pca_coeff_filename_ = $PCAFILE_BASE_DIR."/pca_portfolio_stdev_eigen_".$yyyymmdd_.".txt";
 if ( ExistsWithSize ( $this_pca_coeff_filename_ ) )
 {
     SyncToAllMachines ( $this_pca_coeff_filename_ ) ;
 }
my $this_pca_coeff_filename_removed_ = $this_pca_coeff_filename_."_removed";
`mv $this_pca_coeff_filename_ $this_pca_coeff_filename_removed_`;

#clear the datgen content
`rm -f $PCAFILE_OUT_DIR/*`;





