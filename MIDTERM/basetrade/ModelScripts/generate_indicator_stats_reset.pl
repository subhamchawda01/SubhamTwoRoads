#!/usr/bin/perl

# \file ModelScripts/generate_indicator_stats_reset.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes a prod_config
# START
# DEPPARAM na_t1 30 CET_810 EST_800
# SELF FGBL_0 
# SOURCE FGBM_0 FGBS_0 FESX_0 FGBL_0
# NOSELFSOURCE FGBM_0 FGBS_0 FESX_0 
# SOURCECOMBO EBFUT EEQUI
# END

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub LoadInstructionFile ;
sub ReadTemplateFiles ;
sub OpenNewIListFile ;
sub CloseCurrentIListFile ;
sub GenerateIndicatorListFile ;
sub DataGenAndICorrMap ;
sub OutputResults ;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
#my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );

my $SPARE_DATA_DIR=$SPARE_HOME."DataGenOut";
my $IWORK_DIR=$HOME_DIR."/indicatorwork";
my $ITEMPLATES_DIR=$HOME_DIR."/modelling/indicatorwork";

my $CALENDAR_DAYS_FOR_DATAGEN=2000;

my @indicator_list_filename_vec_ = (); # a vector of files in case an individual indicator file becomes too big
my $indicator_list_filename_index_ = 0;
my $MAX_INDICATORS_IN_FILE = 300;
my $num_indicators_added_in_current_file_ = -1; # initial condition signifying no ilist file open
my $main_ilist_filehandle_ = FileHandle->new; # made global to be accessed in the subroutines easily

my $debug = 1;
local $| = 1; # sets autoflush in STDOUT

# if ( $debug == 1 )
# {
#     print STDOUT "$main_log_file_\n";
# }

require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/valid_date.pl";
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl";
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_next_date.pl";
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0 prodconfig [indicators]";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $prodconfig_ = $ARGV[0];

splice ( @ARGV, 0, 1 );
my @indicators_to_focus_on_ = @ARGV ;

my $self_shortcode_ = "FGBL_0";
my $pred_duration_ = 30;
my $predalgo_ = "na_t3";
my $datagen_start_hhmm_ = "CET_810";
my $datagen_end_hhmm_ = "EST_800";
my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 15;
my $datagen_num_trades_timeout_ = 5;
my $to_print_on_economic_times_ = 0;
my $dep_base_pricetype_ = "MktSizeWPrice";
my $dep_pred_pricetype_ = "MktSizeWPrice";
my @source_vec_ = ();
my @noselfsource_vec_ = ();
my @sourcecombo_vec_ = ();

my @self_files_ = (); # Only Self based Indicators
my @source_files_ = (); # Indiactors that take any Shortcode source, could be SELF
my @noselfsource_files_ = (); # Any non-self Shortcode source
my @sourcecombo_files_ = (); # Any Portfolio Source ( Combo and Port indicators )

my %indicator_body_corr_mean_map_ ; # Map from Indicator body to mean correlation
my %indicator_body_corr_stdev_map_ ; # Map from Indicator body to stdev correlation
my %indicator_body_corr_sum_map_ ; # Map from Indicator body to sum of correlation
my %indicator_body_corr_l2_sum_map_ ; # Map from Indicator body to sum of squared correlation
my $num_items_in_corr_vec_ = 0 ; # Number of days of Corr Calculation

my @indicator_body_vec_ = () ;

LoadInstructionFile ( );

my $this_work_dir_ = $SPARE_DATA_DIR."/".$self_shortcode_."_".$pred_duration_."_".$predalgo_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$dep_base_pricetype_."_".$dep_pred_pricetype_."_".$unique_gsm_id_;
my $results_dirname_ = $IWORK_DIR."/".$self_shortcode_."_".$pred_duration_."_".$predalgo_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$dep_base_pricetype_."_".$dep_pred_pricetype_;

my $main_log_file_ = $this_work_dir_."/main_log_file.txt";
CreateEnclosingDirectory ( $main_log_file_ );
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );

ReadTemplateFiles ( );

GenerateIndicatorListFile ( );

my $indicator_corr_record_file_ = $results_dirname_."/indicator_corr_record_file.txt";
CreateEnclosingDirectory ( $indicator_corr_record_file_ );
my $indicator_corr_record_file_handle_ = FileHandle->new;
if ( $#indicators_to_focus_on_ >= 0 )
{ # if only a few indicators then open in append mode 
$indicator_corr_record_file_handle_->open ( ">> $indicator_corr_record_file_ " ) or PrintStacktraceAndDie ( "Could not open $indicator_corr_record_file_ for app-writing\n" );
}
else
{
$indicator_corr_record_file_handle_->open ( "> $indicator_corr_record_file_ " ) or PrintStacktraceAndDie ( "Could not open $indicator_corr_record_file_ for writing\n" );
}

DataGenAndICorrMap ( );

$indicator_corr_record_file_handle_->close;

OutputResults ( );

$main_log_file_handle_->close;

{
    printf ( STDOUT "Summary in $results_dirname_ Full Record in $indicator_corr_record_file_\n" );
}
exit ( 0 );

sub LoadInstructionFile 
{
    open PRODCONFIGHANDLE, "< $prodconfig_ " or PrintStacktraceAndDie ( "Could not open $prodconfig_\n" );
    while ( my $thisline_ = <PRODCONFIGHANDLE> ) 
    {
	chomp ( $thisline_ );
	my @this_words_ = split ( ' ', $thisline_ );
	if ( $#this_words_ >= 1 )
	{
	    given ( $this_words_[0] )
	    {
		when ("DEPPARAM")
		{
		    if ( $#this_words_ < 9 )
		    {
			PrintStacktraceAndDie ( "DEPPARAM line should be like DEPPARAM na_e3 30 CET_810 EST_800 1000 15 1 MktSizeWPrice MktSizeWPrice" );
		    }
		    $predalgo_ = $this_words_[1];
		    $pred_duration_ = $this_words_[2];
		    $datagen_start_hhmm_ = $this_words_[3];
		    $datagen_end_hhmm_ = $this_words_[4];
		    $datagen_msecs_timeout_ = $this_words_[5];
		    $datagen_l1events_timeout_ = $this_words_[6];
		    $datagen_num_trades_timeout_ = $this_words_[7];
		    $dep_base_pricetype_ = $this_words_[8];
		    $dep_pred_pricetype_ = $this_words_[9];
		}
		when ("SELF")
		{
		    $self_shortcode_ = $this_words_[1];
		}
		when ("SOURCE")
		{
		    for ( my $i = 1; $i <= $#this_words_; $i ++ )
		    {
			my $found_this_ = 0;
			for ( my $j = 0; $j <= $#source_vec_ ; $j++ )
			{
			    if ( $source_vec_[$j] eq $this_words_[$i] )
			    {
				$found_this_ = 1;
				last;
			    }
			}
			if ( $found_this_ == 0 )
			{
			    push ( @source_vec_, $this_words_[$i] );
			    if ( $self_shortcode_ ne $this_words_[$i] )
			    { # all sources that aren't the dependant are automatically added 
				# to noselfsource_vec_
				push ( @noselfsource_vec_, $this_words_[$i] );
			    }
			}
		    }
		}
		when ("NOSELFSOURCE")
		{
		    for ( my $i = 1; $i <= $#this_words_; $i ++ )
		    {
			push ( @noselfsource_vec_, $this_words_[$i] );
		    }
		}
		when ("SOURCECOMBO")
		{
		    for ( my $i = 1; $i <= $#this_words_; $i ++ )
		    {
			push ( @sourcecombo_vec_, $this_words_[$i] );
		    }
		}
	    }
	}
    }
}

sub ReadTemplateFiles
{
    if ( $#indicators_to_focus_on_ >= 0 ) 
    {
	for ( my $itfo_idx_ = 0 ; $itfo_idx_ <= $#indicators_to_focus_on_ ; $itfo_idx_ ++ )
	{
	    my $try_name_ = "";
	    $try_name_ = "indicator_list_".$indicators_to_focus_on_[$itfo_idx_]."_SELF";
	    if ( -e $ITEMPLATES_DIR."/".$try_name_ )
	    {
		push ( @self_files_, $try_name_ );
	    }
	    $try_name_ = "indicator_list_".$indicators_to_focus_on_[$itfo_idx_]."_NOSELFSOURCE";
	    if ( -e $ITEMPLATES_DIR."/".$try_name_ )
	    {
		push ( @noselfsource_files_, $try_name_ );
	    }
	    $try_name_ = "indicator_list_".$indicators_to_focus_on_[$itfo_idx_]."_SOURCECOMBO";
	    if ( -e $ITEMPLATES_DIR."/".$try_name_ )
	    {
		push ( @sourcecombo_files_, $try_name_ );
	    }
	    $try_name_ = "indicator_list_".$indicators_to_focus_on_[$itfo_idx_]."_SOURCE";
	    if ( -e $ITEMPLATES_DIR."/".$try_name_ )
	    {
		push ( @source_files_, $try_name_ );
	    }
	}

     	# print STDERR "SELF_FILES: ".join ( ' ', @self_files_ )."\n";
      	# print STDERR "NOSELFSOURCE_FILES: ".join ( ' ', @noselfsource_files_ )."\n";
      	# print STDERR "SOURCECOMBO_FILES: ".join ( ' ', @sourcecombo_files_ )."\n";
     	# print STDERR "SOURCE_FILES: ".join ( ' ', @source_files_ )."\n";
    }
    else
    {
	@self_files_ = `ls -l $ITEMPLATES_DIR/ | grep indicator_list_ | grep _SELF | awk '{print \$9}'`;
	chomp ( @self_files_ );
	# if ( $debug == 1 )
	# {
	# 	print $main_log_file_handle_ "SELF_FILES: ".join ( ' ', @self_files_ )."\n";
	# }

	@noselfsource_files_ = `ls -l $ITEMPLATES_DIR/ | grep indicator_list_ | grep _NOSELFSOURCE | awk '{print \$9}'`;
	chomp ( @noselfsource_files_ );
	# if ( $debug == 1 )
	# {
	#  	print $main_log_file_handle_ "NOSELFSOURCE_FILES: ".join ( ' ', @noselfsource_files_ )."\n";
	# }

	@sourcecombo_files_ = `ls -l $ITEMPLATES_DIR/ | grep indicator_list_ | grep _SOURCECOMBO | awk '{print \$9}'`;
	chomp ( @sourcecombo_files_ );
	# if ( $debug == 1 )
	# {
	#  	print $main_log_file_handle_ "SOURCECOMBO_FILES: ".join ( ' ', @sourcecombo_files_ )."\n";
	# }

	@source_files_ = `ls -l $ITEMPLATES_DIR/ | grep indicator_list_ | grep _SOURCE | grep -v _SOURCECOMBO | awk '{print \$9}'`;
	chomp ( @source_files_ );
	# if ( $debug == 1 )
	# {
	# 	print $main_log_file_handle_ "SOURCE_FILES: ".join ( ' ', @source_files_ )."\n";
	# }
    }    
}

sub OpenNewIListFile
{
    # add header and open next file 
    my $new_indicator_list_filename_ = $this_work_dir_."/ilist_file_".$indicator_list_filename_index_.".txt"; 
    $indicator_list_filename_index_ ++; # for next time

    # push file in vec
    push ( @indicator_list_filename_vec_, $new_indicator_list_filename_ ) ;

    $main_ilist_filehandle_->open ( ">> $new_indicator_list_filename_ " ) or PrintStacktraceAndDie ( "Could not open $new_indicator_list_filename_\n" );
    $main_ilist_filehandle_->autoflush(1);

    print $main_ilist_filehandle_ "MODELINIT DEPBASE $self_shortcode_ $dep_base_pricetype_ $dep_pred_pricetype_\n";
    print $main_ilist_filehandle_ "MODELMATH LINEAR CHANGE\n";
    print $main_ilist_filehandle_ "INDICATORSTART\n";

    $num_indicators_added_in_current_file_ = 0;
}

sub CloseCurrentIListFile
{
    if ( $num_indicators_added_in_current_file_ >= 0 )
    {
        # add trailer 
	print $main_ilist_filehandle_ "INDICATOREND\n";
	# close current file handle
	close $main_ilist_filehandle_ ;
	$num_indicators_added_in_current_file_ = -1;
    }
}

sub GenerateIndicatorListFile
{
    
    `mkdir -p $this_work_dir_`;

    # SELF indicators
    foreach my $this_self_indicator_filename_ ( @self_files_ ) # it seems foreach returns by reference
    {
	$this_self_indicator_filename_ = $ITEMPLATES_DIR."/".$this_self_indicator_filename_;

	next if ( ! ExistsWithSize ( $this_self_indicator_filename_ ) ) ;

	open THIS_ILIST_FILEHANDLE, "< $this_self_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_self_indicator_filename_\n" );
	my @ilines_ = <THIS_ILIST_FILEHANDLE>;
	chomp ( @ilines_ );
	for ( my $i = 0 ; $i <= $#ilines_; $i ++ )
	{
	    my $this_indicator_line_ = $ilines_[$i];
	    if ( $this_indicator_line_ =~ m/INDICATOR 1.00 / )
	    {
		my $this_indicator_body_ = $this_indicator_line_;
		$this_indicator_body_ =~ s/INDICATOR 1.00 //;

		$this_indicator_body_ =~ s/SELF/$self_shortcode_/;
		$this_indicator_line_ =~ s/SELF/$self_shortcode_/;

		if ( ! exists $indicator_body_corr_mean_map_{$this_indicator_body_} )
		{ # this indicator has never been seen hence we 
		    # can add it to the map with value 0 and add a 
		    # line to the main indicator file
		    $indicator_body_corr_mean_map_{$this_indicator_body_} = 0;
		    $indicator_body_corr_stdev_map_{$this_indicator_body_} = 0;
		    $indicator_body_corr_sum_map_{$this_indicator_body_} = 0;
		    $indicator_body_corr_l2_sum_map_{$this_indicator_body_} = 0;

		    if ( $num_indicators_added_in_current_file_ < 0 )
		    { OpenNewIListFile ( ); }

		    printf ( $main_ilist_filehandle_ "%s\n", $this_indicator_line_ );
		    push ( @indicator_body_vec_, $this_indicator_body_ );

		    $num_indicators_added_in_current_file_ ++;
		    if ( $num_indicators_added_in_current_file_ == $MAX_INDICATORS_IN_FILE )
		    { CloseCurrentIListFile ( ); }
		}
	    }
	}
	close THIS_ILIST_FILEHANDLE;
    }

    # SOURCE indicators
    foreach my $this_source_indicator_filename_ ( @source_files_ )
    {
	$this_source_indicator_filename_ = $ITEMPLATES_DIR."/".$this_source_indicator_filename_;

	next if ( ! ExistsWithSize ( $this_source_indicator_filename_ ) ) ;

	open THIS_ILIST_FILEHANDLE, "< $this_source_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_source_indicator_filename_\n" );
	my @ilines_ = <THIS_ILIST_FILEHANDLE>;
	chomp ( @ilines_ );
	for ( my $i = 0 ; $i <= $#ilines_; $i ++ )
	{
	    my $orig_this_indicator_line_ = $ilines_[$i];
	    if ( $orig_this_indicator_line_ =~ m/INDICATOR 1.00 / )
	    {
		my $orig_this_indicator_body_ = $orig_this_indicator_line_;
		$orig_this_indicator_body_ =~ s/INDICATOR 1.00 //;

		$orig_this_indicator_body_ =~ s/ SELF/ $self_shortcode_/g;
		$orig_this_indicator_line_ =~ s/ SELF/ $self_shortcode_/g;

		foreach my $this_source_shortcode_ ( @source_vec_ )
		{
		    my $this_indicator_body_ = $orig_this_indicator_body_ ;
		    my $this_indicator_line_ = $orig_this_indicator_line_ ;

		    $this_indicator_body_ =~ s/SOURCE/$this_source_shortcode_/g;
		    $this_indicator_line_ =~ s/SOURCE/$this_source_shortcode_/g;

		    if ( ! exists $indicator_body_corr_mean_map_{$this_indicator_body_} )
		    { # this indicator has never been seen hence we 
			# can add it to the map with value 0 and add a 
			# line to the main indicator file
			$indicator_body_corr_mean_map_{$this_indicator_body_} = 0;
			$indicator_body_corr_stdev_map_{$this_indicator_body_} = 0;
			$indicator_body_corr_sum_map_{$this_indicator_body_} = 0;
			$indicator_body_corr_l2_sum_map_{$this_indicator_body_} = 0;
			
			if ( $num_indicators_added_in_current_file_ < 0 )
			{ OpenNewIListFile ( ); }
			
			printf ( $main_ilist_filehandle_ "%s\n", $this_indicator_line_ );
			push ( @indicator_body_vec_, $this_indicator_body_ );
			
			$num_indicators_added_in_current_file_ ++;
			if ( $num_indicators_added_in_current_file_ == $MAX_INDICATORS_IN_FILE )
			{ CloseCurrentIListFile ( ); }
		    }
		}
	    }
	}
	close THIS_ILIST_FILEHANDLE;
    }

    # NOSELFSOURCE indicators
    foreach my $this_noselfsource_indicator_filename_ ( @noselfsource_files_ )
    {
	$this_noselfsource_indicator_filename_ = $ITEMPLATES_DIR."/".$this_noselfsource_indicator_filename_;

	next if ( ! ExistsWithSize ( $this_noselfsource_indicator_filename_ ) ) ;

	open THIS_ILIST_FILEHANDLE, "< $this_noselfsource_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_noselfsource_indicator_filename_\n" );
	my @ilines_ = <THIS_ILIST_FILEHANDLE>;
	chomp ( @ilines_ );
	for ( my $i = 0 ; $i <= $#ilines_; $i ++ )
	{
	    my $orig_this_indicator_line_ = $ilines_[$i];
	    if ( $orig_this_indicator_line_ =~ m/INDICATOR 1.00 / )
	    {
		my $orig_this_indicator_body_ = $orig_this_indicator_line_;
		$orig_this_indicator_body_ =~ s/INDICATOR 1.00 //;

		$orig_this_indicator_body_ =~ s/ SELF/ $self_shortcode_/g;
		$orig_this_indicator_line_ =~ s/ SELF/ $self_shortcode_/g;

#		if ( $debug == 1 ) { print $main_log_file_handle_ "BEFORE LOOPING ON $orig_this_indicator_body_ , noselfsource_vec_ = ".join ( ' ', @noselfsource_vec_ )."\n"; }
		foreach my $this_source_shortcode_ ( @noselfsource_vec_ )
		{
		    my $this_indicator_body_ = $orig_this_indicator_body_ ;
		    my $this_indicator_line_ = $orig_this_indicator_line_ ;

		    $this_indicator_body_ =~ s/NOSELFSOURCE/$this_source_shortcode_/;
		    $this_indicator_line_ =~ s/NOSELFSOURCE/$this_source_shortcode_/;

#		    if ( $debug == 1 ) { print $main_log_file_handle_ "For $this_source_shortcode_ : $this_indicator_body_ $this_indicator_line_\n"; }

		    if ( ! exists $indicator_body_corr_mean_map_{$this_indicator_body_} )
		    {   # this indicator has never been seen hence we 
			# can add it to the map with value 0 and add a 
			# line to the main indicator file
			$indicator_body_corr_mean_map_{$this_indicator_body_} = 0;
			$indicator_body_corr_stdev_map_{$this_indicator_body_} = 0;
			$indicator_body_corr_sum_map_{$this_indicator_body_} = 0;
			$indicator_body_corr_l2_sum_map_{$this_indicator_body_} = 0;

			if ( $num_indicators_added_in_current_file_ < 0 )
			{ OpenNewIListFile ( ); }
			
			printf ( $main_ilist_filehandle_ "%s\n", $this_indicator_line_ );
			push ( @indicator_body_vec_, $this_indicator_body_ );
#			if ( $debug == 1 ) { print $main_log_file_handle_ "Print to $main_ilist_filehandle_ $this_indicator_line_\n"; }
			
			$num_indicators_added_in_current_file_ ++;
			if ( $num_indicators_added_in_current_file_ == $MAX_INDICATORS_IN_FILE )
			{ CloseCurrentIListFile ( ); }
		    }
		}
	    }
	}
	close THIS_ILIST_FILEHANDLE;
    }

    # SOURCECOMBO indicators
    foreach my $this_sourcecombo_indicator_filename_ ( @sourcecombo_files_ )
    {
	$this_sourcecombo_indicator_filename_ = $ITEMPLATES_DIR."/".$this_sourcecombo_indicator_filename_;

	next if ( ! ExistsWithSize ( $this_sourcecombo_indicator_filename_ ) ) ;

	open THIS_ILIST_FILEHANDLE, "< $this_sourcecombo_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_sourcecombo_indicator_filename_\n" );
	my @ilines_ = <THIS_ILIST_FILEHANDLE>;
	chomp ( @ilines_ );
	for ( my $i = 0 ; $i <= $#ilines_; $i ++ )
	{
	    my $orig_this_indicator_line_ = $ilines_[$i];
	    if ( $orig_this_indicator_line_ =~ m/INDICATOR 1.00 / )
	    {
		my $orig_this_indicator_body_ = $orig_this_indicator_line_;
		$orig_this_indicator_body_ =~ s/INDICATOR 1.00 //;

		$orig_this_indicator_body_ =~ s/ SELF/ $self_shortcode_/g;
		$orig_this_indicator_line_ =~ s/ SELF/ $self_shortcode_/g;

		foreach my $this_sourcecombo_shortcode_ ( @sourcecombo_vec_ )
		{
		    my $this_indicator_body_ = $orig_this_indicator_body_ ;
		    my $this_indicator_line_ = $orig_this_indicator_line_ ;

		    $this_indicator_body_ =~ s/SOURCECOMBO/$this_sourcecombo_shortcode_/;
		    $this_indicator_line_ =~ s/SOURCECOMBO/$this_sourcecombo_shortcode_/;

		    if ( ! exists $indicator_body_corr_mean_map_{$this_indicator_body_} )
		    {   # this indicator has never been seen hence we 
			# can add it to the map with value 0 and add a 
			# line to the main indicator file
			$indicator_body_corr_mean_map_{$this_indicator_body_} = 0;
			$indicator_body_corr_stdev_map_{$this_indicator_body_} = 0;
			$indicator_body_corr_sum_map_{$this_indicator_body_} = 0;
			$indicator_body_corr_l2_sum_map_{$this_indicator_body_} = 0;
			
			if ( $num_indicators_added_in_current_file_ < 0 )
			{ OpenNewIListFile ( ); }
			
			printf ( $main_ilist_filehandle_ "%s\n", $this_indicator_line_ );
			push ( @indicator_body_vec_, $this_indicator_body_ );
			
			$num_indicators_added_in_current_file_ ++;
			if ( $num_indicators_added_in_current_file_ == $MAX_INDICATORS_IN_FILE )
			{ CloseCurrentIListFile ( ); }
		    }
		}
	    }
	}
	close THIS_ILIST_FILEHANDLE;
    }

    if ( $num_indicators_added_in_current_file_ >= 0 )
    {
	CloseCurrentIListFile ( );
    }
}

sub DataGenAndICorrMap
{
    my $tradingdate_ = GetIsoDateFromStrMin1 ( "TODAY-1" );
    my $max_days_at_a_time_ = $CALENDAR_DAYS_FOR_DATAGEN;
    for ( my $i = 0 ; $i < $max_days_at_a_time_ ; $i ++ ) 
    {
 	if ( ! ValidDate ( $tradingdate_ ) )
 	{
 	    last;
 	}
 	else 
 	{   # for this trading date generate the reg_data_file
 	    my $this_day_file_extension_ = $pred_duration_."_".$predalgo_."_".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_ ; 
	    my $this_day_timed_data_filename_ = $this_work_dir_."/timed_data_".$this_day_file_extension_;
	    my $this_day_wmean_reg_data_filename_ = $this_work_dir_."/wmean_reg_data_".$this_day_file_extension_;
 	    my $this_day_reg_data_filename_ = $this_work_dir_."/reg_data_".$this_day_file_extension_;
 	    my $this_day_filtered_reg_data_filename_ = $this_work_dir_."/filtered_reg_data_".$this_day_file_extension_;

	    my @indicator_corr_vec_ = ();
	
	    for ( my $j = 0 ; $j <= $#indicator_list_filename_vec_ ; $j ++ )
	    {
		my $indicator_list_filename_ = $indicator_list_filename_vec_[$j];

		{		
		    my $exec_cmd="$BIN_DIR/datagen $indicator_list_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
		    print $main_log_file_handle_ "$exec_cmd\n";
		    `$exec_cmd`;
		}

		if ( ! ExistsWithSize ( $this_day_timed_data_filename_ ) )
		{
		    print $main_log_file_handle_ "Does not exist. rm -f $this_day_timed_data_filename_\n";
		    `rm -f $this_day_timed_data_filename_`;
		    last; # next date
		}
		
		# generate reg data file which is not mean zero
		{
		    my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( "" , $pred_duration_, $predalgo_, $this_day_timed_data_filename_ );
		    my $exec_cmd="$BIN_DIR/timed_data_to_reg_data $indicator_list_filename_ $this_day_timed_data_filename_ $this_pred_counters_ $predalgo_ $this_day_wmean_reg_data_filename_";
		    print $main_log_file_handle_ "$exec_cmd\n";
		    `$exec_cmd`;
		    print $main_log_file_handle_ "rm -f $this_day_timed_data_filename_\n" ;
		    `rm -f $this_day_timed_data_filename_`;
		}
		
		# generate reg data file which is mean zero
		if ( ! ExistsWithSize ( $this_day_wmean_reg_data_filename_ ) )
		{
		    print $main_log_file_handle_ "Does not exist. rm -f $this_day_wmean_reg_data_filename_\n";
		    `rm -f $this_day_wmean_reg_data_filename_`;
		    last; # next date
		}

		
		{
		    my $this_day_stats_reg_data_filename_ = $this_work_dir_."/stats_reg_data_".$this_day_file_extension_;
		    my $exec_cmd="$BIN_DIR/remove_mean_reg_data  $this_day_wmean_reg_data_filename_ $this_day_reg_data_filename_ $this_day_stats_reg_data_filename_";
		    print $main_log_file_handle_ "$exec_cmd\n";
		    `$exec_cmd`;
		    print $main_log_file_handle_ "rm -f $this_day_stats_reg_data_filename_\n" ;
		    `rm -f $this_day_stats_reg_data_filename_`;
		}
		print $main_log_file_handle_ "rm -f $this_day_wmean_reg_data_filename_\n" ;
		`rm -f $this_day_wmean_reg_data_filename_`;

		{
		    my $exec_cmd="$MODELSCRIPTS_DIR/select_rows_with_dep_in_range_2.pl $this_day_reg_data_filename_ 1 1.0 -1 $this_day_filtered_reg_data_filename_" ;
		    print $main_log_file_handle_ "$exec_cmd\n";
		    `$exec_cmd`;

		    print $main_log_file_handle_ "rm -f $this_day_reg_data_filename_\n" ;
		    `rm -f $this_day_reg_data_filename_`;
		}
		
		if ( ExistsWithSize ( $this_day_filtered_reg_data_filename_ ) )
		{
		    my $exec_cmd="$BIN_DIR/get_dep_corr $this_day_filtered_reg_data_filename_";
		    print $main_log_file_handle_ "$exec_cmd\n";
		    my $dcf_line_ = `$exec_cmd`; chomp ( $dcf_line_ );
		    
		    print $main_log_file_handle_ "rm -f $this_day_filtered_reg_data_filename_\n" ;
		    `rm -f $this_day_filtered_reg_data_filename_`;

		    my @dcf_vec_ = split ( ' ', $dcf_line_ );
		    
		    push ( @indicator_corr_vec_, @dcf_vec_ ) ;
		}
		else
		{
		    print $main_log_file_handle_ "removing 0sized $this_day_filtered_reg_data_filename_\n" ;
		    `rm -f $this_day_filtered_reg_data_filename_`;

		    last; # if one file was not successful rest of it is useless
		}
	    }
	    
	    if ( $#indicator_corr_vec_ == $#indicator_body_vec_ )
	    {
		print $main_log_file_handle_ "For $tradingdate_ num_indicators ( $#indicator_body_vec_ ) == num_corr ( $#indicator_corr_vec_ ) \n";
		$num_items_in_corr_vec_ ++;
		for ( my $i = 0 ; $i < $#indicator_corr_vec_ ; $i ++ )
		{
		    my $this_indicator_body_ = $indicator_body_vec_[$i];
		    $indicator_body_corr_sum_map_{$this_indicator_body_} += $indicator_corr_vec_[$i];
		    $indicator_body_corr_l2_sum_map_{$this_indicator_body_} += $indicator_corr_vec_[$i] * $indicator_corr_vec_[$i];
		    printf ( $indicator_corr_record_file_handle_ "%d INDICATOR %s %s\n", $tradingdate_, $indicator_corr_vec_[$i], $this_indicator_body_ ) ;
		}
	    }
	    else
	    {
		print $main_log_file_handle_ "For $tradingdate_ num_indicators ( $#indicator_body_vec_ ) ! num_corr ( $#indicator_corr_vec_ ) \n";
	    }
	}
	$tradingdate_ = CalcPrevDate ( $tradingdate_ );
    }

    for my $this_indicator_body_ ( keys %indicator_body_corr_l2_sum_map_ )
    {
	my $this_count = $num_items_in_corr_vec_ ;
	my $this_sum_l1 = $indicator_body_corr_sum_map_{$this_indicator_body_};
	my $this_sum_l2 = $indicator_body_corr_l2_sum_map_{$this_indicator_body_};

	if ( $this_count > 0 )
	{
	    $indicator_body_corr_mean_map_{$this_indicator_body_} = $this_sum_l1/$this_count;
	}
	else
	{
	    $indicator_body_corr_mean_map_{$this_indicator_body_} = 0;
	}
	if ( $this_count > 1 )
	{
	    $indicator_body_corr_stdev_map_{$this_indicator_body_} = sqrt ( ( $this_sum_l2 - ( $this_sum_l1 * $this_sum_l1 / $this_count ) ) / ($this_count -1) ) ;
	}
	else
	{
	    $indicator_body_corr_stdev_map_{$this_indicator_body_} = 1;
	}
#	if ( $debug == 1 ) { printf $main_log_file_handle_ "INDICATOR %.3f %.3f %s\n", $indicator_body_corr_mean_map_{$this_indicator_body_}, $indicator_body_corr_stdev_map_{$this_indicator_body_}, $this_indicator_body_; }
    }
}

sub OutputResults 
{
    my $indicator_list_prefix_ = $IWORK_DIR."/indicator_list_";
    my $unsorted_results_prefix_ = $results_dirname_."/unsorted_results_";
    my $sorted_results_prefix_ = $results_dirname_."/sorted_results_";
    CreateEnclosingDirectory ( $unsorted_results_prefix_ );
    
    # SELF indicators
    foreach my $this_self_indicator_filename_ ( @self_files_ )
    {
	# $this_self_indicator_filename_ = $IWORK_DIR."/".$this_self_indicator_filename_; # don't need to this here since it seems the prev call did it by reference

	next if ( ! ExistsWithSize ( $this_self_indicator_filename_ ) ) ;

	open THIS_ILIST_FILEHANDLE, "< $this_self_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_self_indicator_filename_\n" );
	my @ilines_ = <THIS_ILIST_FILEHANDLE>;
	chomp ( @ilines_ );
	close THIS_ILIST_FILEHANDLE;

	my $this_results_filename_ = $this_self_indicator_filename_;
	$this_results_filename_ =~ s#$indicator_list_prefix_#$unsorted_results_prefix_#g ;
	my $this_sorted_results_filename_ = $this_self_indicator_filename_;
	$this_sorted_results_filename_ =~ s#$indicator_list_prefix_#$sorted_results_prefix_#g ;
	open THIS_ILIST_RESULT_FILEHANDLE, "> $this_results_filename_ " or PrintStacktraceAndDie ( "Could not open $this_results_filename_\n" );
	for ( my $i = 0 ; $i <= $#ilines_; $i ++ )
	{
	    my $this_indicator_line_ = $ilines_[$i];
	    if ( $this_indicator_line_ =~ m/INDICATOR 1.00 / )
	    {
		my $this_indicator_body_ = $this_indicator_line_;
		$this_indicator_body_ =~ s/INDICATOR 1.00 //;

		$this_indicator_body_ =~ s/SELF/$self_shortcode_/g;
		$this_indicator_line_ =~ s/SELF/$self_shortcode_/g;

		printf ( THIS_ILIST_RESULT_FILEHANDLE "INDICATOR %.3f %.3f %s\n", $indicator_body_corr_mean_map_{$this_indicator_body_}, $indicator_body_corr_stdev_map_{$this_indicator_body_}, $this_indicator_body_ ) ;
#		if ( $debug == 1 ) { printf $main_log_file_handle_ "INDICATOR %.3f %.3f %s\n", $indicator_body_corr_mean_map_{$this_indicator_body_}, $indicator_body_corr_stdev_map_{$this_indicator_body_}, $this_indicator_body_; }
	    }
	}
	close THIS_ILIST_RESULT_FILEHANDLE ;
	
	`$SCRIPTS_DIR/sort_abs.pl 2 $this_results_filename_ > $this_sorted_results_filename_`;
    }

    # SOURCE indicators
    foreach my $this_source_indicator_filename_ ( @source_files_ )
    {
#	$this_source_indicator_filename_ = $IWORK_DIR."/".$this_source_indicator_filename_;

	next if ( ! ExistsWithSize ( $this_source_indicator_filename_ ) ) ;

	open THIS_ILIST_FILEHANDLE, "< $this_source_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_source_indicator_filename_\n" );
	my @ilines_ = <THIS_ILIST_FILEHANDLE>;
	chomp ( @ilines_ );
	close THIS_ILIST_FILEHANDLE;

	my $this_results_filename_ = $this_source_indicator_filename_;
	$this_results_filename_ =~ s#$indicator_list_prefix_#$unsorted_results_prefix_# ;
	my $this_sorted_results_filename_ = $this_source_indicator_filename_;
	$this_sorted_results_filename_ =~ s#$indicator_list_prefix_#$sorted_results_prefix_#g ;
	open THIS_ILIST_RESULT_FILEHANDLE, "> $this_results_filename_ " or PrintStacktraceAndDie ( "Could not open $this_results_filename_\n" );
	for ( my $i = 0 ; $i <= $#ilines_; $i ++ )
	{
	    my $orig_this_indicator_line_ = $ilines_[$i];
	    if ( $orig_this_indicator_line_ =~ m/INDICATOR 1.00 / )
	    {
		my $orig_this_indicator_body_ = $orig_this_indicator_line_;
		$orig_this_indicator_body_ =~ s/INDICATOR 1.00 //;

		$orig_this_indicator_body_ =~ s/ SELF/ $self_shortcode_/g;
		$orig_this_indicator_line_ =~ s/ SELF/ $self_shortcode_/g;

		foreach my $this_source_shortcode_ ( @source_vec_ )
		{
		    my $this_indicator_body_ = $orig_this_indicator_body_ ;
		    my $this_indicator_line_ = $orig_this_indicator_line_ ;

		    $this_indicator_body_ =~ s/SOURCE/$this_source_shortcode_/;
		    $this_indicator_line_ =~ s/SOURCE/$this_source_shortcode_/;

		    printf ( THIS_ILIST_RESULT_FILEHANDLE "INDICATOR %.3f %.3f %s\n", $indicator_body_corr_mean_map_{$this_indicator_body_}, $indicator_body_corr_stdev_map_{$this_indicator_body_}, $this_indicator_body_ ) ;
#		    if ( $debug == 1 ) { printf ( $main_log_file_handle_ "INDICATOR %.3f %.3f %s\n", $indicator_body_corr_mean_map_{$this_indicator_body_}, $indicator_body_corr_stdev_map_{$this_indicator_body_}, $this_indicator_body_ ) ; }
		}
	    }
	}
	close THIS_ILIST_RESULT_FILEHANDLE ;

	`$SCRIPTS_DIR/sort_abs.pl 2 $this_results_filename_ > $this_sorted_results_filename_`;
    }

    # NOSELFSOURCE indicators
    foreach my $this_noselfsource_indicator_filename_ ( @noselfsource_files_ )
    {
#	$this_noselfsource_indicator_filename_ = $IWORK_DIR."/".$this_noselfsource_indicator_filename_;

	next if ( ! ExistsWithSize ( $this_noselfsource_indicator_filename_ ) ) ;

	open THIS_ILIST_FILEHANDLE, "< $this_noselfsource_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_noselfsource_indicator_filename_\n" );
	my @ilines_ = <THIS_ILIST_FILEHANDLE>;
	chomp ( @ilines_ );
	close THIS_ILIST_FILEHANDLE;

	my $this_results_filename_ = $this_noselfsource_indicator_filename_;
	$this_results_filename_ =~ s#$indicator_list_prefix_#$unsorted_results_prefix_#g ;
	my $this_sorted_results_filename_ = $this_noselfsource_indicator_filename_;
	$this_sorted_results_filename_ =~ s#$indicator_list_prefix_#$sorted_results_prefix_#g ;
	open THIS_ILIST_RESULT_FILEHANDLE, "> $this_results_filename_ " or PrintStacktraceAndDie ( "Could not open $this_results_filename_\n" );
	for ( my $i = 0 ; $i <= $#ilines_; $i ++ )
	{
	    my $orig_this_indicator_line_ = $ilines_[$i];
	    if ( $orig_this_indicator_line_ =~ m/INDICATOR 1.00 / )
	    {
		my $orig_this_indicator_body_ = $orig_this_indicator_line_;
		$orig_this_indicator_body_ =~ s/INDICATOR 1.00 //;

		$orig_this_indicator_body_ =~ s/ SELF/ $self_shortcode_/g;
		$orig_this_indicator_line_ =~ s/ SELF/ $self_shortcode_/g;

		foreach my $this_source_shortcode_ ( @noselfsource_vec_ )
		{
		    my $this_indicator_body_ = $orig_this_indicator_body_ ;
		    my $this_indicator_line_ = $orig_this_indicator_line_ ;

		    $this_indicator_body_ =~ s/NOSELFSOURCE/$this_source_shortcode_/;
		    $this_indicator_line_ =~ s/NOSELFSOURCE/$this_source_shortcode_/;

		    printf ( THIS_ILIST_RESULT_FILEHANDLE "INDICATOR %.3f %.3f %s\n", $indicator_body_corr_mean_map_{$this_indicator_body_}, $indicator_body_corr_stdev_map_{$this_indicator_body_}, $this_indicator_body_ ) ;
		}
	    }
	}
	close THIS_ILIST_RESULT_FILEHANDLE ;

	`$SCRIPTS_DIR/sort_abs.pl 2 $this_results_filename_ > $this_sorted_results_filename_`;
    }

    # SOURCECOMBO indicators
    foreach my $this_sourcecombo_indicator_filename_ ( @sourcecombo_files_ )
    {
#	$this_sourcecombo_indicator_filename_ = $IWORK_DIR."/".$this_sourcecombo_indicator_filename_;

	next if ( ! ExistsWithSize ( $this_sourcecombo_indicator_filename_ ) ) ;

	open THIS_ILIST_FILEHANDLE, "< $this_sourcecombo_indicator_filename_ " or PrintStacktraceAndDie ( "Could not open $this_sourcecombo_indicator_filename_\n" );
	my @ilines_ = <THIS_ILIST_FILEHANDLE>;
	chomp ( @ilines_ );
	close THIS_ILIST_FILEHANDLE;

	my $this_results_filename_ = $this_sourcecombo_indicator_filename_;
	$this_results_filename_ =~ s#$indicator_list_prefix_#$unsorted_results_prefix_#g ;
	my $this_sorted_results_filename_ = $this_sourcecombo_indicator_filename_;
	$this_sorted_results_filename_ =~ s#$indicator_list_prefix_#$sorted_results_prefix_#g ;
	open THIS_ILIST_RESULT_FILEHANDLE, "> $this_results_filename_ " or PrintStacktraceAndDie ( "Could not open $this_results_filename_\n" );
	for ( my $i = 0 ; $i <= $#ilines_; $i ++ )
	{
	    my $orig_this_indicator_line_ = $ilines_[$i];
	    if ( $orig_this_indicator_line_ =~ m/INDICATOR 1.00 / )
	    {
		my $orig_this_indicator_body_ = $orig_this_indicator_line_;
		$orig_this_indicator_body_ =~ s/INDICATOR 1.00 //;

		$orig_this_indicator_body_ =~ s/ SELF/ $self_shortcode_/g;
		$orig_this_indicator_line_ =~ s/ SELF/ $self_shortcode_/g;

		foreach my $this_sourcecombo_shortcode_ ( @sourcecombo_vec_ )
		{
		    my $this_indicator_body_ = $orig_this_indicator_body_ ;
		    my $this_indicator_line_ = $orig_this_indicator_line_ ;

		    $this_indicator_body_ =~ s/SOURCECOMBO/$this_sourcecombo_shortcode_/;
		    $this_indicator_line_ =~ s/SOURCECOMBO/$this_sourcecombo_shortcode_/;

		    printf ( THIS_ILIST_RESULT_FILEHANDLE "INDICATOR %.3f %.3f %s\n", $indicator_body_corr_mean_map_{$this_indicator_body_}, $indicator_body_corr_stdev_map_{$this_indicator_body_}, $this_indicator_body_ ) ;
		}
	    }
	}
	close THIS_ILIST_RESULT_FILEHANDLE ;

	`$SCRIPTS_DIR/sort_abs.pl 2 $this_results_filename_ > $this_sorted_results_filename_`;
    }

}
