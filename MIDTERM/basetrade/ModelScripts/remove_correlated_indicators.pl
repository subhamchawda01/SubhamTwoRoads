#!/usr/bin/perl

# \file ModelScripts/remove_correlated_indicators.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
# modelfile CrossCorrelationThreshold startdate enddate start_hhmm end_hhmm

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $GENRCIWORKDIR=$SPARE_HOME."RCI/";
`mkdir -p $GENRCIWORKDIR`; # won't be needed after a few days

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "rkumar" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}
if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "ankit" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # GetPredCountersForThisPredAlgo
require "$GENPERLLIB_DIR/get_default_timeout_values_for_shortcode.pl"; # GetDefaultTimeoutValuesForShortcode
require "$GENPERLLIB_DIR/get_avg_event_count_per_sec_for_shortcode.pl"; # GetAvgEventCountPerSecForShortcode

# start 
my $USAGE="$0 shortcode=na_shc modelfile cross_correlation_threshold max_num startdate enddate starthhmm endhhmm";

if ( $#ARGV < 7 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $indicator_list_filename_ = $ARGV[1];
my $cross_correlation_threshold_ = $ARGV[2];
my $max_indicators_ = $ARGV[3];
my $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[4] );
my $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[5] );
my $datagen_start_hhmm_ = $ARGV[6];
my $datagen_end_hhmm_ = $ARGV[7];

if ( $shortcode_ eq "na_shc" )
{ $shortcode_ = ""; }

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 10;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;

if ( $shortcode_ ) 
{ 
    ( $datagen_msecs_timeout_, $datagen_l1events_timeout_ ) = GetDefaultTimeoutValuesForShortcode ( $shortcode_ );
    $datagen_l1events_timeout_ = GetAvgEventCountPerSecForShortcode ( $shortcode_, $datagen_start_hhmm_, $datagen_end_hhmm_, $datagen_end_yyyymmdd_ ) / 2.0;
    if ( $datagen_l1events_timeout_ < 1 )
    { # very slow products
	$datagen_msecs_timeout_ = max ( $datagen_msecs_timeout_, (1000/$datagen_l1events_timeout_) );
	$datagen_msecs_timeout_ = min ( 60000, $datagen_msecs_timeout_ );
    }
}

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
#my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );

my @intermediate_files_ = ();

my @pre_indicator_strings_=();
my @indicator_strings_=();
my @post_indicator_strings_=();
{ # TODO : load model file into @pre_indicator_strings_; @indicator_strings_, @post_indicator_strings_
    open (FH, $indicator_list_filename_) or die "Couldn't open location file: $indicator_list_filename_";
    my @all_lines= <FH>;
    close FH;
    my $indicator_reading_ = -1; # pre(-1),at(0),post(1)
    foreach my $line_(@all_lines)
    {
	chomp ( $line_ );
	$line_ =~ s/^\s+//;
	my @words_ = split(/\s+/, $line_);

	given ( $indicator_reading_ )
	{
	    when ( -1 ) 
	    { # pre
		if ( $words_[0] ne "INDICATOR" )
		{ # still pre
		    push ( @pre_indicator_strings_, $line_ );
		}
		else
		{ # moving to INDICATOR lines
		    $indicator_reading_ = 0;
		    push ( @indicator_strings_, $line_ );
		}
	    }
	    when ( 0 )
	    { # at
		if ( $words_[0] ne "INDICATOR" )
		{ # moving to post
		    $indicator_reading_ = 1;
		    push ( @post_indicator_strings_, $line_ );
		}
		else
		{ # INDICATOR lines
		    push ( @indicator_strings_, $line_ );
		}
	    }
	    when ( 1 )
	    { # post
		push ( @post_indicator_strings_, $line_ );
	    }
	}
    }
}

my $combined_reg_data_filename_ = $GENRCIWORKDIR."wmean_reg_data_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$unique_gsm_id_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_;
push ( @intermediate_files_, $combined_reg_data_filename_ );
my $corr_matrix_filename_ = $GENRCIWORKDIR."corr_matrix_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$unique_gsm_id_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_;
push ( @intermediate_files_, $corr_matrix_filename_ );


# GENERATE DATA FOR THE DAYS ... END RESULT IS REG_DATA
my $datagen_date_ = $datagen_end_yyyymmdd_;
my $max_days_at_a_time_ = 2000;
for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
{
    if ( SkipWeirdDate ( $datagen_date_ ) ||
	 ( ( $shortcode_ ) && ( NoDataDateForShortcode ( $datagen_date_ , $shortcode_ ) ) ) ||
	 ( IsDateHoliday ( $datagen_date_ ) || ( ( $shortcode_ ) && ( IsProductHoliday ( $datagen_date_, $shortcode_ ) ) ) ) )
#	 NoDataDate ( $datagen_date_ ) ||
#	 IsDateHoliday ( $datagen_date_ ) )
    {
	$datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
	next;
    }
    
    if ( ( ! ValidDate ( $datagen_date_ ) ) ||
	 ( $datagen_date_ < $datagen_start_yyyymmdd_ ) )
    {
	last;
    }
    else 
    {   

	my $this_day_timed_data_filename_ = $GENRCIWORKDIR."timed_data_".$datagen_date_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$unique_gsm_id_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_;
	{
	    my $exec_cmd="$LIVE_BIN_DIR/datagen $indicator_list_filename_ $datagen_date_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
	    `$exec_cmd`;
	    push ( @intermediate_files_, $this_day_timed_data_filename_ ) ;
	}
	{ # just clean up 
	    my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
	    `rm -f $datagen_logfile_`;
	}

	if ( ExistsWithSize ( $this_day_timed_data_filename_ ) )
	{
	    my $this_pred_duration_ = 32;
	    my $this_predalgo_ = "na_e3";

	    my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( $shortcode_ , $this_pred_duration_, $this_predalgo_, $this_day_timed_data_filename_ );

	    my $this_day_wmean_reg_data_filename_ = $GENRCIWORKDIR."wmean_reg_data_".$this_pred_duration_."_".$this_predalgo_."_".$datagen_date_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$unique_gsm_id_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_;

	    {
		my $exec_cmd="$LIVE_BIN_DIR/timed_data_to_reg_data $indicator_list_filename_ $this_day_timed_data_filename_ $this_pred_counters_ $this_predalgo_ $this_day_wmean_reg_data_filename_";
		print STDERR $exec_cmd ,"\n" ;
		`$exec_cmd`;
		push ( @intermediate_files_, $this_day_wmean_reg_data_filename_ ) ;
	    }
	    if ( ExistsWithSize ( $this_day_wmean_reg_data_filename_ ) )
	    {
		`cat $this_day_wmean_reg_data_filename_ >> $combined_reg_data_filename_`;
	    }
	    else
	    {
		print STDERR "$this_day_wmean_reg_data_filename_ is empty : no reg data for $datagen_date_ \n";
	    }
	}
	else
	{
	    print STDERR "$this_day_timed_data_filename_ : no timed data created for $datagen_date_ \n";
	}

	$datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
    }
}

# Get Correlation Matrix
# Process to Select the indicator strings that meet the cut
my @selected_indicator_strings_ = ();
if ( ExistsWithSize ( $combined_reg_data_filename_ ) )
{
    {
	my $exec_cmd="$LIVE_BIN_DIR/get_correlation_matrix $combined_reg_data_filename_ > $corr_matrix_filename_";
	`$exec_cmd`;
    }
    
    my @corr_matrix_ = ();
    # load corr_matrix into a 2D array
    if ( ExistsWithSize ( $corr_matrix_filename_ ) )
    {
	open (CMFH, $corr_matrix_filename_) or die "Couldn't open corr_matrix_filename $corr_matrix_filename_";
	my @all_lines_= <CMFH>;
	close CMFH;
	if ( $#all_lines_ > 0 ) 
	{ # error check
	    my $num_cols_ = 0;
	    my $this_line_ = $all_lines_[0];
	    chomp ( $this_line_ );
	    $this_line_ =~ s/^\s+//;
	    my @this_words_ = split (/\s+/, $this_line_);
	    $num_cols_ = $#this_words_ + 1;
	    push ( @corr_matrix_, \@this_words_ );
	    for ( my $all_lines_idx_ = 1; $all_lines_idx_ <= $#all_lines_ ; $all_lines_idx_ ++ )
	    {
		$this_line_ = $all_lines_[$all_lines_idx_];
		chomp ( $this_line_ );
		$this_line_ =~ s/^\s+//;
		my @this_line_words_ = split (/\s+/, $this_line_);
		if ( $num_cols_ == ( $#this_line_words_ + 1 ) ) 
		{ # matches cols in first line
		    push ( @corr_matrix_, \@this_line_words_ );
		}
	    }
	}
    }

    # check ( $#corr_matrix_ == $#indicator_strings_ + 1 )
    if ( $#corr_matrix_ == ( $#indicator_strings_ + 1 ) )
    { # santiy check
    
	my @indicator_eligibility_ = (0) x ( 1 + $#indicator_strings_ ) ; # all eligible
	for ( my $i = 0 ; $i <= $#indicator_strings_ ; $i ++ )
	{ 
	    if ( $indicator_eligibility_ [ $i ] == 0 )
	    { # still eligible
		push ( @selected_indicator_strings_, $indicator_strings_[$i] );
		
		for ( my $j = $i + 1 ; $j <= $#indicator_strings_ ; $j ++ )
		{
		    if ( $indicator_eligibility_ [ $j ] == 0 )
		    { # still eligible
			if ( abs ( $corr_matrix_[$i + 1][$j + 1] ) >= $cross_correlation_threshold_ )
			{ # highly cross-correlated
			    $indicator_eligibility_ [ $j ] = 1;
#			    print "Elim $j for $i\n";
			}
		    }
		}
	    }
	}
    }
}
else
{
    print STDERR "no reg file created, there is nothing to do\n";
}

# print the selected indicators
for ( my $i = 0 ; $i <= $#pre_indicator_strings_ ; $i ++ )
{
    print $pre_indicator_strings_[$i]."\n";
}
for ( my $i = 0 ; $i <= $#selected_indicator_strings_ && $i < $max_indicators_ ; $i ++ )
{
    print $selected_indicator_strings_[$i]."\n";
}
for ( my $i = 0 ; $i <= $#post_indicator_strings_ ; $i ++ )
{
    print $post_indicator_strings_[$i]."\n";
}

# clean intermediate files
for ( my $i = 0 ; $i <= $#intermediate_files_; $i ++ )
{
    if ( -e $intermediate_files_[$i] )
    {
	`rm -f $intermediate_files_[$i]`;
    }
}
