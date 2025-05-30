#!/usr/bin/perl

# \file ModelScripts/get_stdev_model.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
# modelfile startdate enddate start_hhmm end_hhmm

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

my $GENRSMWORKDIR=$SPARE_HOME."RSM/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $datagen_exec_ = $LIVE_BIN_DIR."/datagen";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $USER eq "rkumar" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}
if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "ankit" || $USER eq "anshul" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

# start 
my $USAGE="$0 modelfile startdate enddate starthhmm endhhmm";

if ( $#ARGV < 4 ) { print $USAGE."\n"; exit ( 0 ); }
my $model_filename_ = $ARGV[0];
my $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[1] );
my $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $datagen_start_hhmm_ = $ARGV[3];
my $datagen_end_hhmm_ = $ARGV[4];
if ( $#ARGV >= 5 )
{
$datagen_exec_ = $ARGV[5];
}

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
my $hhmmss_ = `date +%H%M%S`; chomp ( $hhmmss_ );

my $datagen_msecs_timeout_ = 1000;
my $datagen_l1events_timeout_ = 6;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
#my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );

my @model_coeff_vec_ = ();
my @model_coeff_alpha_vec_ = ();
my @model_coeff_beta_vec_ = ();

my $exec_cmd_ = "cat $model_filename_ | grep MODELMATH | awk '{print \$2}'";
my $model_math_line_ = `$exec_cmd_`;
chomp ( $model_math_line_);
my $model_type_ = $model_math_line_;
my $min_price_increment_ = 1 ;
my $shortcode_ = "NA";
$exec_cmd_ = "cat $model_filename_ | grep MODELINIT | awk '{print \$3}'";
$shortcode_ = `$exec_cmd_`;
chomp ( $shortcode_ );
if ( $shortcode_ ne "NA" )
{
    $exec_cmd_ = "$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $datagen_end_yyyymmdd_";
    $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $datagen_end_yyyymmdd_` ; chomp ( $min_price_increment_ ); # using single date in hope that min_inc doesnt change much
}


my $temp_model_filename_ = $model_filename_."_tmp_stdev_model";
`cp $model_filename_ $temp_model_filename_`;

if ( $model_type_ eq "SIGLR" )
{
    `sed -i 's/SIGLR/LINEAR/g' $temp_model_filename_`;
}

open MFILE, "< $model_filename_" or PrintStacktraceAndDie ( "Could not open model_filename_ $model_filename_ for reading\n" );


while ( my $mline_ = <MFILE> )
{    
    chomp ( $mline_ );
    if ( $mline_ =~ /INDICATOR / )
    {
	my @mwords_ = split ( ' ', $mline_ );
	if ( $#mwords_ >= 2 )
	{
	    if ( $model_type_ eq "SIGLR" )
	    {
		my @t_model_coeffs_ = split ( ':', $mwords_[1] );
		push ( @model_coeff_alpha_vec_, $t_model_coeffs_[0] );
		push ( @model_coeff_beta_vec_, $t_model_coeffs_[1] );
	    }
	    else
	    {
	    	push ( @model_coeff_vec_, $mwords_[1] );
	    }
	}
    }
}
close MFILE;

my $abs_sum_l1 = 0;
my $sum_l1 = 0;
my $sum_l2 = 0;
my $count = 0; 	    

my $tradingdate_ = $datagen_start_yyyymmdd_;
my $max_days_at_a_time_ = 2000;
for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
{
    if ( SkipWeirdDate ( $tradingdate_ ) ||
	 NoDataDate ( $tradingdate_ ) ||
	 IsDateHoliday ( $tradingdate_ ) )
    {
	$tradingdate_ = CalcNextWorkingDateMult ( $tradingdate_, 5 );
	next;
    }
    
    if ( ( ! ValidDate ( $tradingdate_ ) ) ||
	 ( $tradingdate_ > $datagen_end_yyyymmdd_ ) )
    {
	last;
    }
    else 
    {   

	my $this_day_timed_data_filename_ = $GENRSMWORKDIR."timed_data".$tradingdate_."_".$datagen_start_hhmm_."_".$datagen_end_hhmm_."_".$unique_gsm_id_."_".$datagen_msecs_timeout_."_".$datagen_l1events_timeout_."_".$datagen_num_trades_timeout_;
	$exec_cmd_="$datagen_exec_ $temp_model_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
#	print $main_log_file_handle_ "$exec_cmd\n";
	`$exec_cmd_`;
	
	my $datagen_logfile_ = $DATAGEN_LOGDIR."log.".$yyyymmdd_.".".$unique_gsm_id_;
	`rm -f $datagen_logfile_`;

	if ( -e $this_day_timed_data_filename_ )
	{
	    open DFILE, "< $this_day_timed_data_filename_" or PrintStacktraceAndDie ( "Could not open this_day_timed_data_filename_ $this_day_timed_data_filename_ for reading\n" );
	    while ( my $data_line_ = <DFILE> )
	    {
		chomp ( $data_line_ );
		my @dwords_ = split ( ' ', $data_line_ );
		
		if ( $#dwords_ >= $#model_coeff_vec_ + 4 )
		{
		    my $this_model_value_ = 0;

		    if ( $model_type_ eq "LINEAR" )
		    {
		    	for ( my $j = 0; $j <= $#model_coeff_vec_ ; $j ++ )
		  	{
			    $this_model_value_ += ( $dwords_[ $j + 4 ] * $model_coeff_vec_[$j] );
	 		}
		    }
		    
		    if ( $model_type_ eq "SIGLR" )
		    {
			for ( my $j = 0; $j <= $#model_coeff_alpha_vec_ ; $j ++ )
			{
			    $this_model_value_ += ( 1 / ( 1 + exp ( - $dwords_[ $j + 4 ] * $model_coeff_alpha_vec_[$j] ) ) - 0.5 ) * $model_coeff_beta_vec_[$j];
			}		
		    }
		    if ( abs ( $this_model_value_ ) < 10 * $min_price_increment_ )
		    {
			$abs_sum_l1 += abs ($this_model_value_);
			$sum_l1 +=  $this_model_value_ ;
			$sum_l2 += $this_model_value_ * $this_model_value_ ;
			$count ++;
		    }

		}
	    }
	    close DFILE;
	    `rm -f $this_day_timed_data_filename_`;
	}

	$tradingdate_ = CalcNextWorkingDateMult ( $tradingdate_, 1 );
    }
    
}

my $current_model_stdev_ = -1;
my $current_model_l1norm_ = -1;

if ( $count <= 1 )
{
    $current_model_stdev_ = -1;
}
else
{
    $current_model_stdev_ = sqrt ( ( $sum_l2 - ( $sum_l1 * $sum_l1 / $count ) ) / ($count -1) );
    $current_model_l1norm_ = $abs_sum_l1 / $count ;
}
printf "%f %f\n",$current_model_stdev_,$current_model_l1norm_ ;
`rm -f $temp_model_filename_`;
