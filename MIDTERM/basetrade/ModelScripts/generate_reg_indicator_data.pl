#!/usr/bin/perl

# \file ModelScripts/generate_timed_indicator_data.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes as input : indicator_list_filename, start_yyyymmdd, end_yyyymmdd, start_time_hhmm, end_time_hhmm [progid] [TIMED_DATA_DAILY_DIR]
# For each of these days, it generates timed data ( just print of all the indicators )

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

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl"; #GetDatesFromNumDays, GetDatesFromStartDate
require "$GENPERLLIB_DIR/sample_data_utils.pl"; #GetFilteredDays

my $USAGE="$0 indicatorlistfilename datagen_start_yyyymmdd datagen_end_yyyymmdd starthhmm endhhmm msecs_timeout_ l1events_timeout_ num_trades_timeout_ horizon_ algo_ outfile_ [datefile_=INVALIDFILE/SD\@Feature_FeatureAux_LBound_UBound/SD\@Feature_FeatureAux_Perc_HIGH, SD\@STDEV_NA_0.3_HIGH/SD\@VOL_NA_300_1300] <filter(fv)=none> <infofilename=\"\"><time=0>";

if ( $#ARGV < 10 ) { print $USAGE."\n"; exit ( 0 ); }
my $indicator_list_filename_ = $ARGV[0];
my $datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[1] );
my $datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[2] );
my $datagen_start_hhmm_ = $ARGV[3];
my $datagen_end_hhmm_ = $ARGV[4];
my $datagen_msecs_timeout_ = $ARGV[5];
my $datagen_l1events_timeout_ = $ARGV[6];
my $datagen_num_trades_timeout_ = $ARGV[7];
my $to_print_on_economic_times_ = 0;
my $td2rd_horizon_secs_ = $ARGV[8];
my $td2rd_algo_ = $ARGV[9] ;
my $this_reg_data_filename_ = $ARGV[10];
my $this_reg_data_filename_unfiltered_ = $ARGV[10]."unfiltered";
my $datefile_ = "INVALIDFILE";
my @filter_flags_;
my $ezone_string_ = "";
my $printtime_ = 0;
if ( $#ARGV > 10 )
{
    $datefile_ = $ARGV[11];
    my @tokens_ = split("@", $datefile_);
    print $tokens_[0];
    if ( scalar ( @tokens_ ) == 2 )
    {
	if ( $tokens_[0] eq "SD" )
	{
	    $datefile_ = "INVALIDFILE";
	    @filter_flags_ = split ( "_", $tokens_[1] );
	    if ( scalar ( @filter_flags_ ) != 4 )
	    {
		print "Expects 4 tokens, when using sample_data filter. Examples: SD\@AvgPrice_NA_0_20 || SD\@STDEV_NA_0.3_HIGH || SD\@CORR_ES_0_0.6_0.9\n";
	    }
	} elsif ( $tokens_[0] eq "EZ" ) {
	    $datefile_ = "INVALIDFILE";
	    $to_print_on_economic_times_ = 3;
	    my @ezone_ = split ( ",", $tokens_[1] );
	    $ezone_string_ = join ( " ", @ezone_);
	    print $ezone_string_."\n";
	}
    }
}
my @valid_date_vec_ = ();
if ( $datefile_ ne "INVALIDFILE" )
{
    open DATEFILE, "< $datefile_" or PrintStacktraceAndDie ( "Could not open $datefile_ for reading\n" );
    @valid_date_vec_ = <DATEFILE>;
    chomp(@valid_date_vec_);
}
my $filter_ = "none";
if ( $#ARGV > 11 )
{
    $filter_ = $ARGV[12];
}
my $infofilename_ = "";
if ( $#ARGV > 12 )
{
    $infofilename_ = $ARGV[13];
    `rm -rf $infofilename_`;
}
if ( $#ARGV > 13 )
{
    $printtime_ = $ARGV[14];
}
my $shortcode_ = `head -n1 $indicator_list_filename_ | awk '{print \$3}'`;chomp($shortcode_);
my $yymmdd_ = `date +%Y%m%d`;chomp($yymmdd_);
my $min_price_increment_ = `$LIVE_BIN_DIR/get_min_price_increment $shortcode_ $yymmdd_` ; chomp ( $min_price_increment_ );

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

my $indicator_list_filename_base_ = basename ( $indicator_list_filename_ ); chomp ($indicator_list_filename_base_);
my $shc=`head -n1 $indicator_list_filename_ | awk '{print \$3;}'`; chomp($shc);
print "SHC : $shc\n";

`rm -f $this_reg_data_filename_`;
`rm -f $this_reg_data_filename_unfiltered_`;

if ( ValidDate ( $datagen_start_yyyymmdd_ ) &&
     ValidDate ( $datagen_end_yyyymmdd_ ) )
{

    my $tradingdate_ = CalcNextWorkingDateMult( $datagen_end_yyyymmdd_, 1 );
    my $max_days_at_a_time_ = 2000; # just a hard limit of studies can be run at most 90 days at a time
    my @dates_ = GetDatesFromStartDate( $shortcode_, $datagen_start_yyyymmdd_, $datagen_end_yyyymmdd_, "INVALIDFILE", $max_days_at_a_time_ );
    my @filter_dates_ ;

    if ( scalar ( @filter_flags_ ) == 4 ) {
	if ( $filter_flags_[3] eq "HIGH" || $filter_flags_[3] eq "LOW" ) {
	    GetFilteredDays ( $shortcode_, \@dates_, $filter_flags_[2], $filter_flags_[3], $filter_flags_[0], $filter_flags_[1], \@filter_dates_, $datagen_start_hhmm_, $datagen_end_hhmm_ );
	} else {
	    GetFilteredDaysOnSampleBounds ( $shortcode_, \@dates_, $filter_flags_[2], $filter_flags_[3], $filter_flags_[0], $filter_flags_[1], \@filter_dates_, $datagen_start_hhmm_, $datagen_end_hhmm_ );
	}
    } else {
	@filter_dates_ = @dates_;
    }

    @filter_dates_ = reverse @filter_dates_;

    my $print_pred_counters_for_this_pred_algo_script = $MODELSCRIPTS_DIR."/print_pred_counters_for_this_pred_algo.pl" ; # replace this SearchScript in future
    my $get_pred_horizon_cmd_ = "$print_pred_counters_for_this_pred_algo_script $shortcode_ $td2rd_horizon_secs_ $td2rd_algo_ INVALIDFILE $datagen_start_hhmm_ $datagen_end_hhmm_";
    my $td2rd_horizon_ = `$get_pred_horizon_cmd_ 2>/dev/null | tail -1`; chomp ( $td2rd_horizon_ );
    if ( $td2rd_horizon_ eq "" ) { $td2rd_horizon_ = $td2rd_horizon_secs_ * 1000; }

    for ( my $i = 0 ; $i < scalar(@filter_dates_); $i ++ )
    {
      $tradingdate_ = $filter_dates_[$i];

      next if ( ($#valid_date_vec_ >= 0) && (not grep {$_ eq $tradingdate_} @valid_date_vec_) ) ;

# name chosen isn't unique to this invokation, so that we can reuse this file if the next day the same model file is chosen
      my $this_day_timed_data_filename_ = $this_reg_data_filename_.".td.".$tradingdate_ ;
      my $this_day_reg_data_filename_ = $this_reg_data_filename_.".rd.".$tradingdate_ ;

      if ( ! ( -e $this_day_timed_data_filename_ ) )
      { # if there is no such file then data for this day needs to be generated


	open(DATA,"< $indicator_list_filename_") or die "Can't open data";

	my @lines = <DATA>;
	my $flag_ilist=0;
	if ($#lines >= 5) {
	    my $string3= $lines[3];
	    my $string4= $lines[4];
	    my @words1 = split / /, $string3;
	    my @words2 = split / /, $string4;	    
	    if (scalar(@words1) >= 3 &&
		$words1[2] eq  "PricePortfolio") {
		if (scalar(@words2) >= 3 &&
		    $words2[2] eq "PricePortfolio") {
		    $flag_ilist=2;
		}
		else {
		    $flag_ilist=1;
		}
	    }
	}
	close(DATA);

	my $indicator_list_filename_modified_ = $indicator_list_filename_;
        $indicator_list_filename_modified_ = $indicator_list_filename_."_modified";

	my $exec_cmd="$LIVE_BIN_DIR/datagen $indicator_list_filename_ $tradingdate_ $datagen_start_hhmm_ $datagen_end_hhmm_ $unique_gsm_id_ $this_day_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";

	`$exec_cmd`;
        print "$exec_cmd\n";



       my $modify_timed_data_exec="~/basetrade/scripts/timed_data_modify.R $this_day_timed_data_filename_ $flag_ilist";
       `$modify_timed_data_exec`;

       my $modify_ilist_cmd="~/basetrade/scripts/modify_ilist.pl $indicator_list_filename_ $flag_ilist";
       `$modify_ilist_cmd`;

        if ( -e $this_day_timed_data_filename_ )
        {
          my $td2rd_exec_cmd="$LIVE_BIN_DIR/timed_data_to_reg_data $indicator_list_filename_modified_ $this_day_timed_data_filename_ $td2rd_horizon_ $td2rd_algo_ $this_day_reg_data_filename_";
          my $trd_per_sec_file_ = "trd_per_sec_".$unique_gsm_id_;
          if ( $filter_ eq "fv" )
          {
            my $t_cmd_ = "$LIVE_BIN_DIR/daily_trade_aggregator $shc $tradingdate_ $trd_per_sec_file_";
            print "$t_cmd_\n";
            `$t_cmd_`;
            $td2rd_exec_cmd=$td2rd_exec_cmd." $trd_per_sec_file_";
          }
          else
          {
            $td2rd_exec_cmd=$td2rd_exec_cmd." INVALIDFILE";
          }
          $td2rd_exec_cmd = $td2rd_exec_cmd." ".$printtime_;
          `$td2rd_exec_cmd`;
          print "$td2rd_exec_cmd\n";
          `cat $this_day_reg_data_filename_ >> $this_reg_data_filename_unfiltered_`;
          `rm -f $this_day_reg_data_filename_`;
          `rm -f $this_day_timed_data_filename_`;
          `rm -f $trd_per_sec_file_`;
	}
      }
    }

    `cp $this_reg_data_filename_unfiltered_ $this_reg_data_filename_`;
    
    if ( ! ($filter_ eq "none"))
      {
	my $apply_dep_filter_script = $MODELSCRIPTS_DIR."/apply_dep_filter.pl" ; # replace this SearchScript in future
    	my $exec_cmd="$apply_dep_filter_script $shortcode_ $this_reg_data_filename_unfiltered_ $filter_ $this_reg_data_filename_ $datagen_end_yyyymmdd_ " ;
    	`$exec_cmd`;
    	if ( ($infofilename_ cmp "" ) != 0 )
    	{
    		my $number_of_entries_ = `wc -l $this_reg_data_filename_| awk '{print \$1}'`;
    		chomp($number_of_entries_);
    		if ( $number_of_entries_ != 0)
    		{
    			`echo $number_of_entries_ $tradingdate_ >> $infofilename_`;
    		}
    	}
    }
    `rm -f $this_reg_data_filename_unfiltered_`
}

print $this_reg_data_filename_;

