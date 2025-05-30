#!/usr/bin/perl

use strict;
use warnings;
use FileHandle;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $BASETRADEINFODIR="/spare/local/tradeinfo/";
my $BASEPORTFOLIOCONSTINFODIR=$BASETRADEINFODIR."PortfolioInfo/";
my $BASERISKPORTFOLIOCONSTINFODIR=$BASETRADEINFODIR."PortfolioRiskInfo/";
my $BASESTDEVINFODIR=$BASETRADEINFODIR."StdevInfo/";
my $LRDBFILE_BASE_DIR=$BASETRADEINFODIR."LRDBBaseDir";

my $SPARE_HOME="/spare/local/".$USER."/";
my $SPARE_PCF_DIR=$SPARE_HOME."PCFDIR/";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $yyyymmdd_=`date +%Y%m%d`; chomp ( $yyyymmdd_ );

require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/valid_date.pl";
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl";
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_next_date.pl";
require "$GENPERLLIB_DIR/get_kth_word.pl";
require "$GENPERLLIB_DIR/get_stdev_file.pl"; # for GetStdevOfNonZeroValues
require "$GENPERLLIB_DIR/clear_temporary_files.pl"; # for ClearTemporaryFiles
require "$GENPERLLIB_DIR/sync_to_all_machines.pl"; # for SyncToAllMachines

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 portfolio_constituent_sets_file starthhmm endhhmm";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }
my $portfolio_constituent_sets_filename_ = $ARGV[0];
my $datagen_start_hhmm_ = $ARGV[1];
my $datagen_end_hhmm_ = $ARGV[2];

my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 0;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;

my $portfolio_constituent_info_filename_prefix_ = "portfolio_constituent_info";
my $risk_portfolio_constituent_info_filename_prefix_ = "risk_portfolio_constituent_info";
my $stdev_filename_prefix_ = "stdev_info";

my $debug=0;

if ( ! ( -e $portfolio_constituent_sets_filename_ ) )
{
    print STDERR "PCF $portfolio_constituent_sets_filename_ not readable! Exiting\n" ;
    exit ( 0 );
}

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
#my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); 

open PCFILE, "< $portfolio_constituent_sets_filename_ " or PrintStacktraceAndDie ( "Could not open $portfolio_constituent_sets_filename_\n" );

my @port_vec_ = ();
my @const_vec_ = (); # vector of all constituents that we need to compute stdev of
my %port_to_const_list_map_=(); # map from portfolio_shortcode to list of shortcodes
my %const_to_stdev_map_=(); # map from constituent_shortcode to stdev
my %const_to_index_map_=(); # map from constituent_shortcode to index at which it is in the list

while ( my $pcf_line_ = <PCFILE> )
{
    chomp ( $pcf_line_ );
    my @pcf_words_ = split ( ' ', $pcf_line_ );
    if ( ( $#pcf_words_ >= 2 ) &&
	 ( $pcf_words_[0] eq "PLINE" ) )
    {
	my $port_shortcode_ = $pcf_words_[1];
	push ( @port_vec_, $port_shortcode_ );
	splice ( @pcf_words_, 0, 2 );
	$port_to_const_list_map_{$port_shortcode_} = join ( '^', @pcf_words_ );
	
	for ( my $i = 0 ; $i <= $#pcf_words_ ; $i ++ )
	{
	    my $const_ = $pcf_words_[$i];
	    if ( ! ( exists $const_to_index_map_{$const_} ) )
	    {
		push ( @const_vec_, $const_ ); 

		$const_to_index_map_{$const_} = $#const_vec_ ;
		$const_to_stdev_map_{$const_} = 10000 ; # initialization .. high value such that inverse 0 gives coeff 0
	    }
	}
    }
}

if ( $#const_vec_ < 0 )
{
    printf STDERR "No constituents found\n";
    exit ( 0 );
}

# create ilf
my $indicator_list_filename_ = "";
{
    my $portfolio_constituent_sets_filebase_ = basename ( $portfolio_constituent_sets_filename_ ); 
    chomp $portfolio_constituent_sets_filebase_ ;
    my $TEMP_DIR=$HOME_DIR."/tempif";
    if ( ! ( -d $TEMP_DIR ) ) 
    { 
	`mkdir -p $TEMP_DIR`; 
	if ( $debug == 1 ) 
	{
	    print STDOUT "mkdir -p $TEMP_DIR\n";
	}
    }
    $indicator_list_filename_ = $TEMP_DIR."/pcf_ilf_".$portfolio_constituent_sets_filebase_;
    
}

my @unknown_stdev_const_vec_ = (); # only the constituents whose stdev has not already been computed "today"
    # right now adding all constituents here, later we could move this 
    # to only the constituents whose stdev have not been already computed "today"
@unknown_stdev_const_vec_ = @const_vec_;

my $simple_trend_seconds_ = 120;
open ILFILE, "> $indicator_list_filename_ " or PrintStacktraceAndDie ( "Could not open $indicator_list_filename_ for writing\n" );
printf ( ILFILE "MODELINIT DEPBASE %s MktSizeWPrice MktSizeWPrice\n", $const_vec_[0] ); # does not matter which one is the dep_shortcode
printf ( ILFILE "MODELMATH LINEAR RETURNS\n" );
printf ( ILFILE "INDICATORSTART\n" );
for ( my $i = 0 ; $i <= $#unknown_stdev_const_vec_ ; $i ++ )
{
    printf ( ILFILE "INDICATOR 0 SimpleTrend %s %d MktSizeWPrice\n", $unknown_stdev_const_vec_[$i], $simple_trend_seconds_ );
}
printf ( ILFILE "INDICATOREND\n" );
close ILFILE;
sleep ( 1 ); # to make sure the file is written to, before opening it

if ( ! ( -e $indicator_list_filename_ ) )
{
    printf ( STDERR "Failed to create file %s\n", $indicator_list_filename_ );
    exit ( 0 );
}

my $datagen_end_yyyymmdd_ = $yyyymmdd_;
my $max_days_at_a_time_ = 2000;
my $datagen_start_yyyymmdd_ = CalcPrevDateMult ( $datagen_end_yyyymmdd_, $max_days_at_a_time_ ) ;

my $indicator_list_filename_base_ = basename ( $indicator_list_filename_ );


if ( ! ( -d $SPARE_PCF_DIR ) ) 
{ 
    if ( $debug == 1 )
    {
	print STDOUT "mkdir -p $SPARE_PCF_DIR\n"; 
    }
    `mkdir -p $SPARE_PCF_DIR`; 
}

my @temporary_files_=();

my $this_timed_data_filename_ = $SPARE_PCF_DIR."/timed_data_pcf_".$indicator_list_filename_base_."_".$datagen_start_yyyymmdd_."_".$datagen_end_yyyymmdd_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_ ; 
if ( -e $this_timed_data_filename_ ) 
{ 
    if ( $debug == 1 )
    {
	print STDOUT "rm -f $this_timed_data_filename_\n"; 
    }
    `rm -f $this_timed_data_filename_`; 
}

my $tradingdate_ = $datagen_start_yyyymmdd_;
while ( $tradingdate_ <= $datagen_end_yyyymmdd_ )
{
    if ( ! ValidDate ( $tradingdate_ ) )
    {
	last;
    }

    my $this_day_timed_data_filename_ = $SPARE_PCF_DIR."/timed_data_pcf_".$indicator_list_filename_base_."_".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_ ; 
    my $exec_cmd="$LIVE_BIN_DIR/datagen $indicator_list_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
    `$exec_cmd`;
    if ( $debug == 1 )
    {
	print STDOUT "$exec_cmd\n";
    }
    my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
    `rm -f $datagen_logfile_`;

    push ( @temporary_files_, $this_day_timed_data_filename_ );

    if ( ( -s $this_day_timed_data_filename_ ) > 0 )
    {
	`cat $this_day_timed_data_filename_ >> $this_timed_data_filename_`;
	if ( $debug == 1 )
	{
	    print STDOUT "cat $this_day_timed_data_filename_ >> $this_timed_data_filename_\n";
	}
	`rm -f $this_day_timed_data_filename_`;
    }
   
    $tradingdate_ = CalcNextDate ( $tradingdate_ );
}

my $startcolindex_ = 4;
my @stdev_values_ = GetStdevOfNonZeroValues ( $this_timed_data_filename_, $startcolindex_ );

`rm -f $this_timed_data_filename_`;
push ( @temporary_files_, $this_timed_data_filename_ );

if ( $debug == 1 )
{
    printf ( STDOUT "%s\n", join ( ' ', @stdev_values_ ) );
}

if ( $#unknown_stdev_const_vec_ != $#stdev_values_ )
{
    printf ( STDERR "number of constituents %d != number of stdevs %d\n", ( $#unknown_stdev_const_vec_ + 1 ), ( $#stdev_values_ + 1 ) );
    exit ( 0 );
}

my $stdev_info_filename_ = $BASESTDEVINFODIR.$stdev_filename_prefix_."_".$yyyymmdd_.".txt";
my $should_print_comment_line_ = 0;
if ( ( -e $stdev_info_filename_ ) && ( ( -s $stdev_info_filename_ ) > 0 ) )
{ 
    $should_print_comment_line_ = 0; 
}
else
{
    $should_print_comment_line_ = 1; 
    CreateEnclosingDirectory ( $stdev_info_filename_ );
}

open STD_INFO_FILE, ">> $stdev_info_filename_ " or PrintStacktraceAndDie ( "Could not open $stdev_info_filename_ for writing\n" );
if ( $should_print_comment_line_ ) 
{ 
    printf STD_INFO_FILE "#SHORTCODE STDEV_%ss\n", $simple_trend_seconds_ ;
}

for ( my $i = 0; $i <= $#unknown_stdev_const_vec_ ; $i ++ )
{
    $const_to_stdev_map_ { $unknown_stdev_const_vec_[$i] } = $stdev_values_[$i];
    printf ( STD_INFO_FILE "%s %f\n", $unknown_stdev_const_vec_[$i], $stdev_values_[$i] ) ;
    if ( $debug == 1 )
    {
	printf ( STDOUT "STD_INFO_FILE %s %f\n", $unknown_stdev_const_vec_[$i], $stdev_values_[$i] ) ;
    }
}
close STD_INFO_FILE;
# sync
if ( ExistsWithSize ( $stdev_info_filename_ ) )
{
    SyncToAllMachines ( $stdev_info_filename_ ) ;
}

my $portfolio_constituent_info_filename_ = $BASEPORTFOLIOCONSTINFODIR.$portfolio_constituent_info_filename_prefix_."_".$yyyymmdd_.".txt";
my $portfolio_constituent_risk_info_filename_ = $BASERISKPORTFOLIOCONSTINFODIR.$risk_portfolio_constituent_info_filename_prefix_."_".$yyyymmdd_.".txt";

CreateEnclosingDirectory ( $portfolio_constituent_info_filename_ );
CreateEnclosingDirectory ( $portfolio_constituent_risk_info_filename_ );

my $portfolio_constituent_info_filename_size_ = 0; 
if ( -e $portfolio_constituent_info_filename_ ) 
{ 
    $portfolio_constituent_info_filename_size_ = -s $portfolio_constituent_info_filename_ ;
}
my $portfolio_constituent_risk_info_filename_size_ = 0;
if ( -e $portfolio_constituent_risk_info_filename_ ) 
{ 
    $portfolio_constituent_risk_info_filename_size_ = -s $portfolio_constituent_risk_info_filename_ ;
}

open PCIFILE, ">> $portfolio_constituent_info_filename_ " or PrintStacktraceAndDie ( "Could not open $portfolio_constituent_info_filename_ for writing\n" );
open RISKPCIFILE, ">> $portfolio_constituent_risk_info_filename_ " or PrintStacktraceAndDie ( "Could not open $portfolio_constituent_risk_info_filename_ for writing\n" );

if ( $portfolio_constituent_info_filename_size_ == 0 )
{ printf PCIFILE "#PORT_SHORTCODE CONST_SH_1 CONST_WT_1 ...\n" ; }

if ( $portfolio_constituent_risk_info_filename_size_ == 0 )
{ printf RISKPCIFILE "#PORT_SHORTCODE CONST_SH_1 CONST_WT_1 ...\n" ; }

foreach my $port_shortcode_ (@port_vec_)
{
    if ( exists ( $port_to_const_list_map_{$port_shortcode_} ) )
    {
	my $const_string_ = $port_to_const_list_map_{$port_shortcode_};
	my @this_const_vec_ = split ( /\^/, $const_string_ );
	my $sum_stdev_ = 0;
	my $sum_inv_stdev_ = 0;
	for ( my $i = 0 ; $i <= $#this_const_vec_; $i ++ )
	{
	    my $this_const_shortcode_ = $this_const_vec_[$i];
	    my $this_const_stdev_ = $const_to_stdev_map_{$this_const_shortcode_};
	    $sum_stdev_ += $this_const_stdev_ ;
	    my $this_inv_stdev_ = 1.0/$this_const_stdev_ ;
	    $sum_inv_stdev_ += $this_inv_stdev_;
	}
	if ( $sum_inv_stdev_ > 0 ) 
	{
	    printf ( PCIFILE "%s", $port_shortcode_ );
	    for ( my $i = 0 ; $i <= $#this_const_vec_; $i ++ )
	    {
		my $this_const_shortcode_ = $this_const_vec_[$i];
		my $this_const_stdev_ = $const_to_stdev_map_{$this_const_shortcode_};
		my $this_inv_stdev_ = 1.0/$this_const_stdev_ ;
		printf ( PCIFILE " %s %f", $this_const_shortcode_, ( $this_inv_stdev_ / $sum_inv_stdev_ ) );
	    }
	    printf ( PCIFILE "\n");
	}
	if ( $sum_stdev_ > 0 )
	{
	    printf ( RISKPCIFILE "%s", $port_shortcode_ );
	    for ( my $i = 0 ; $i <= $#this_const_vec_; $i ++ )
	    {
		my $this_const_shortcode_ = $this_const_vec_[$i];
		my $this_const_stdev_ = $const_to_stdev_map_{$this_const_shortcode_};
		printf ( RISKPCIFILE " %s %f", $this_const_shortcode_, ( $this_const_stdev_ / $sum_stdev_ ) );
	    }
	    printf ( RISKPCIFILE "\n");
	}
    }
}
close PCIFILE;
close RISKPCIFILE;

if ( ExistsWithSize ( $portfolio_constituent_info_filename_ ) )
{
    SyncToAllMachines ( $portfolio_constituent_info_filename_ ) ;
}

if ( ExistsWithSize ( $portfolio_constituent_risk_info_filename_ ) )
{
    SyncToAllMachines ( $portfolio_constituent_risk_info_filename_ ) ;
}

ClearTemporaryFiles ( @temporary_files_ );
