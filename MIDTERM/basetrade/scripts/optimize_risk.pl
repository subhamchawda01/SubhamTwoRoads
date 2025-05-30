#!/usr/bin/perl

# \file ModelScripts/optimize_risk.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes an instructionfilename :
# startdate enddate
# constraints

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

sub LoadInstructionFile ; # used to read the instructions
sub CollectData ;
sub ComputeOptimalRisk ;
sub PrintData ; 
sub FillDayVec ;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; #FindItemFromVec


# start 
my $USAGE="$0 instructionfilename <start_date>";
#if start date specified in args, don't use one from instruction file, but use no_of_lookback_days
#also change name of result file

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $instructionfilename_ = $ARGV[0];
my $end_date = "00000000";

my $main_log_file_ = $SPARE_HOME."/RO/optimize_risk_log_file.txt";
my $main_result_file_ = $SPARE_HOME."/RO/optimize_risk_result_file.txt";

if ( $#ARGV == 1 ) { $end_date = $ARGV[1]; $main_result_file_ = $main_result_file_."_".$end_date; }

my $file_1_ = $SPARE_HOME."/RO/optimize_risk_file1";
my $file_2_ = $SPARE_HOME."/RO/optimize_risk_file2";
my $file_3_ = $SPARE_HOME."/RO/optimize_risk_file3";
my $file_4_ = $SPARE_HOME."/RO/optimize_risk_file4";

my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

my $main_result_file_handle_ = FileHandle->new;
$main_result_file_handle_->open ( "> $main_result_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_result_file_ for writing\n" );
$main_result_file_handle_->autoflush(1);

my $cache_file = $SPARE_HOME."/RO/optimize_risk_cache_file";
open FH, "<$cache_file" or die "can't open '$cache_file': $!";
my @data_cache_lines= <FH>;
close FH;
my @new_cache_lines = ();


my $start_yyyymmdd_ = "";
my $end_yyyymmdd_ = "";
my @products_names_ = ();
my @products_list_ = ();
my @products_start_duration_ = ();
my @products_end_duration_ = ();
my @products_maxlosslimits_ = ();
my @day_vec_ = ();
my @product_to_avg_normalized_pnl_map_ = ();
my @product_to_avg_risk_map_ = ();
my @product_to_avg_exch_vol_map_ = ();
my @VAR_limit_percentiles = ();
my @VAR_limit_ntimes = (); #integer 'n' denoting n-times maxloss
my @worst_case_pnl_array = (); #2d
my @normalized_pnl_array = (); #2d
my @max_risk_limits = ();
my @min_risk_limits = ();
my @products_to_lookback_days_map_ = ();
my $result_yyyymmdd = "";
my $global_max_loss = "";
my $lfi_max_loss = "";
my $bax_max_loss = "";
my $di_max_loss = "";
my $risk_measure = 1;
my @product_to_avg_l1sz_map_ = ();
my $real_pnl = 0;
my $optimal_pnl = 0;
my $email_id = "";
my $general_max_v_by_V_limit = 0.15; #use these as limits on % trade volume on exchange
my $general_min_v_by_V_limit = 0.0;
my @product_to_avg_norm_vV_map_ = (); # avg of daily v/V divide by daily uts
my @product_to_max_v_by_V_limit_map_ = ();
#my @product_to_min_v_by_V_limit_map_ = (); #using same min limits for now for all.
my @product_to_final_min_limit_map_ = (); # after considering all limits set by l1sz and volumes % on exchange
my @product_to_final_max_limit_map_ = ();
my @product_to_optimal_maxloss_map_ = ();



# load instruction file
LoadInstructionFile ( );

CollectData ( );

ComputeOptimalRisk ( );

PrintData ( );


open FH, "+>>$cache_file" or die "can't open '$cache_file': $!";
print FH @new_cache_lines;
close FH;

# end script
$main_log_file_handle_->close;
$main_result_file_handle_->close;

exit ( 0 );


sub LoadInstructionFile 
{
    print $main_log_file_handle_ "LoadInstructionFile $instructionfilename_\n";

    open INSTRUCTIONFILEHANDLE, "< $instructionfilename_ " or PrintStacktraceAndDie ( "$0 Could not open $instructionfilename_\n" );
    
    my $current_instruction_="";
    my $current_instruction_set_ = 0;
    my $t_ = 0;
    while ( my $thisline_ = <INSTRUCTIONFILEHANDLE> ) 
    {
	chomp ( $thisline_ ); # remove newline
	print $main_log_file_handle_ "$thisline_\n"; #logging for later

	my @instruction_line_words_ = split ( ' ', $thisline_ );
	# {} remove empty space at the beginning

	if ( $#instruction_line_words_ < 0 ) 
	{ # empty line hence set $current_instruction_set_ 0
	    $current_instruction_ = "";
	    $current_instruction_set_ = 0;
	    next;
	} 
	else 
	{ # $#instruction_line_words_ >= 0

	    if ( substr ( $instruction_line_words_[0], 0, 1) ne '#' )
	    {

		if ( $current_instruction_set_ == 0 ) 
		{ # no instruction set currently being processed
		    $current_instruction_ = $instruction_line_words_[0];
		    $current_instruction_set_ = 1;
		    # print $main_log_file_handle_ "Setting instruction to $current_instruction_\n";
		} 
		else 
		{ # $current_instruction_ is valid
		    # print $main_log_file_handle_ "DEBUG current_instruction is $current_instruction_\n";
		    given ( $current_instruction_ ) 
		    {
			when ("START_END_YYYYMMDD") 
			{
			    if ( ( $#instruction_line_words_ >= 1 ) && 
				 ( ( ! ( $start_yyyymmdd_ ) ) || ( rand(1) > 0.50 ) ) )
			    {
				$start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[0] );
				$end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $instruction_line_words_[1] );
			    }
			    # print $main_log_file_handle_ $current_instruction_," is ",join (' ', @instruction_line_words_),"\n";
			}
			when ("PRODUCTS_STARTTIME_ENDTIME_MAXLOSS_MAXRISK_MINRISK_LOOKBACKDAYS")
			{
			    push ( @products_names_, $instruction_line_words_[0] );
			    push ( @products_list_, $instruction_line_words_[1] );
			    push ( @products_start_duration_, $instruction_line_words_[2] );
			    push ( @products_end_duration_, $instruction_line_words_[3] );
			    push ( @products_maxlosslimits_, $instruction_line_words_[4] );
			    push ( @max_risk_limits, $instruction_line_words_[5] );
			    push ( @min_risk_limits, $instruction_line_words_[6] );
			    push ( @products_to_lookback_days_map_, $instruction_line_words_[7] );
			    if ( $#instruction_line_words_ > 7 )
                                {
                                        push ( @product_to_max_v_by_V_limit_map_, $instruction_line_words_[8] );
                                }
                                else
                                {
                                        push ( @product_to_max_v_by_V_limit_map_, $general_max_v_by_V_limit );
                                }

			}
			when ( "CONSTRAINTS" )
			{
				push ( @VAR_limit_percentiles, $instruction_line_words_[0] );
			  	push ( @VAR_limit_ntimes , $instruction_line_words_[1] );
			}
			when ( "RESULT_YYYYMMDD" )
			{
				if ( $end_date == "00000000" ) { $result_yyyymmdd = $instruction_line_words_[0] ; }
				else 			      { $result_yyyymmdd = $end_date ; }
			}
			when ( "GLOBALMAXLOSS" )
            {
                $global_max_loss = $instruction_line_words_[0] ;
            }
			when ( "LFIMAXLOSS" )
			{
				$lfi_max_loss = $instruction_line_words_[0] ;
			}
			when ( "BAXMAXLOSS" )
			{
				$bax_max_loss = $instruction_line_words_[0] ;
			}
			when ( "DIMAXLOSS" )
			{
				$di_max_loss = $instruction_line_words_[0] ;
			}
            when ( "RISKMEASURE" )
            {
                $risk_measure = $instruction_line_words_[0] ;
                #1=>volume_ratio 2=>traded_volume 3=>uts
            }
			when ("MAIL_ADDRESS")
                        {
                            $email_id = $instruction_line_words_[0];
                        }

			default
			{
			}
		    }
		}
	    }
	}
    }
    
    close ( INSTRUCTIONFILEHANDLE );

	
	# fill up datagen_day_vec_
	@day_vec_ = ();
	my $datagen_date_ = $end_yyyymmdd_;
	my $max_days_at_a_time_ = 2000;
	
	if ( $end_date == "00000000" )
	{	
	for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
	{
	    #print $main_log_file_handle_ "Considering datagen day $datagen_date_ \n";

	    if ( ( ! ValidDate ( $datagen_date_ ) ) ||
		 ( $datagen_date_ < $start_yyyymmdd_ ) )
	    {
		last;
	    }

	    if ( SkipWeirdDate ( $datagen_date_ ) )
	    {
		print $main_log_file_handle_ "Skipping day $datagen_date_ \n";
		$datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
		next;
	    }
	    
	    {   
		print $main_log_file_handle_ "Taking day $datagen_date_\n";
		push ( @day_vec_, $datagen_date_ ) ;
	    }

	    $datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
	}
	}
	
}

sub FillDayVec
{
	my $working_index = $_[0];
	my $shc_ = $products_list_[$working_index];
	# fill up datagen_day_vec_
	@day_vec_ = ();
	my $datagen_date_ = CalcPrevWorkingDateMult ( $end_date, 1 ); #end_date is result date, look for no_of_lookback_days
	my $max_days_at_a_time_ = 2000;
	
	for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ ) 
	{
	    #print $main_log_file_handle_ "Considering datagen day $datagen_date_ \n";

	    if ( ! ValidDate ( $datagen_date_ ) )
	    {
		last;
	    }

	    if ( SkipWeirdDate ( $datagen_date_ ) ||
                 NoDataDateForShortcode ( $datagen_date_ , $shc_ ) ||
                 ( IsDateHoliday ( $datagen_date_ ) || ( ( $shc_ ) && ( IsProductHoliday ( $datagen_date_, $shc_ ) ) ) ) )
	    {
		print $main_log_file_handle_ "Skipping day $datagen_date_ \n";
		$datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );
		next;
	    }
	    
	    {   
		print $main_log_file_handle_ "Taking day $datagen_date_\n";
		push ( @day_vec_, $datagen_date_ ) ;
	    }

	    $datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );

	    if ( $#day_vec_ >= $products_to_lookback_days_map_[$working_index] )
	    {
		last;
	    }

	}
	
	# if day_vec is empty... fill it with the first 20 days.... just to make sure that script runs for other prods
	if ( $#day_vec_ < 0 )
	{
	my $datagen_date_ = CalcPrevWorkingDateMult ( $end_date, 1 );
        my $max_days_at_a_time_ = 2000;
        for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_at_a_time_ ; $t_day_index_ ++ )
        {
            #print $main_log_file_handle_ "Considering datagen day $datagen_date_ \n";
            if ( ! ValidDate ( $datagen_date_ ) )
            {
                 last;
            }
            {
                print $main_log_file_handle_ "Forcefully Taking day $datagen_date_\n";
                push ( @day_vec_, $datagen_date_ ) ;
            }
            $datagen_date_ = CalcPrevWorkingDateMult ( $datagen_date_, 1 );

            if ( $#day_vec_ >= $products_to_lookback_days_map_[$working_index] )
            {
                last;
            }
	}
	}
}

sub CollectData
{
	my $t_iter = -1;
	my $no_of_days_ = 0 ;
	my @products_average_normalized_pnl_ = ();
	
	
	for ( my $index_ = 0; $index_ <= $#products_list_; $index_ ++ )
	{
		if ( $end_date == "00000000" ) { }
		else { FillDayVec($index_); }
		my @min_normalised_pnl = () ;
		my $avg_normalized_pnl = 0 ;
		my $avg_risk = 0 ;
		my $avg_exch_vol = 0 ;
		my $avg_l1_size = 0 ;
		my $avg_norm_vV = 0;
		$no_of_days_ = 0 ;
	
		my $norm_pnl = 0;	
		my $iter = -1;
		foreach my $dt ( @day_vec_ )
		{ 
		my $exec_cmd_ = "$BIN_DIR/get_pnl_for_shortcode_bytime_stats $products_list_[$index_] $dt 1 UTC_$products_start_duration_[$index_] UTC_$products_end_duration_[$index_] DETAILS";
		$norm_pnl = 0;
		$iter = $iter + 1;
		print $main_log_file_handle_ "$exec_cmd_\n" ;
		
		my $cache_hit = 0 ;
		my $result_lines = "";
		for (my $i = 0; $i <= $#data_cache_lines; $i++)
		{
			if ( $data_cache_lines[$i] =~ m/$exec_cmd_/ )
			{
				$cache_hit = 1;
			#	print $main_log_file_handle_ "cachehit\n";
				$result_lines = $data_cache_lines[$i+1];
				last;
			}
		}
		if ( $cache_hit == 0 )
		{
			$result_lines = `$exec_cmd_`;
			push ( @new_cache_lines, $exec_cmd_."\n" );
			push ( @new_cache_lines, $result_lines."\n" );
		}
		print $main_log_file_handle_ "$result_lines\n" ;
		my @rwords_ = split ( ' ', $result_lines ) ;
		if ( ( $#rwords_ > -1 ) && ( $rwords_[5] > 0 ) )
		{
			my $exec_cmd_2_ = "$BIN_DIR/get_volume_on_day $products_list_[$index_] $dt $products_start_duration_[$index_] $products_end_duration_[$index_]";
			print $main_log_file_handle_ "$exec_cmd_2_\n" ;
			my $cache_hit_2 = 0 ;
			my $result_lines_2 = "";
			for (my $i = 0; $i <= $#data_cache_lines; $i++)
			{
				if ( $data_cache_lines[$i] =~ m/$exec_cmd_2_/ )
				{
					$cache_hit_2 = 1;
				#	print $main_log_file_handle_ "cachehit2\n";
					$result_lines_2 = $data_cache_lines[$i+1];
					last;
				}
			}
			if ( $cache_hit_2 == 0 )
			{
				$result_lines_2 = `$exec_cmd_2_`;
				push ( @new_cache_lines, $exec_cmd_2_."\n" );
				push ( @new_cache_lines, $result_lines_2."\n" );
			}
			print $main_log_file_handle_ "$result_lines_2\n" ;
			my @rwords_2 = split ( ' ', $result_lines_2 ) ;
			
			my $exec_cmd_3_ = "$BIN_DIR/get_avg_l1sz_on_day $products_list_[$index_] $dt $products_start_duration_[$index_] $products_end_duration_[$index_]";
			print $main_log_file_handle_ "$exec_cmd_3_\n" ;
			my $cache_hit_3 = 0 ;
			my $result_lines_3 = "";
			for (my $i = 0; $i <= $#data_cache_lines; $i++)
			{
				if ( $data_cache_lines[$i] =~ m/$exec_cmd_3_/ )
				{
					$cache_hit_3 = 1;
				#	print $main_log_file_handle_ "cachehit3\n";
					$result_lines_3 = $data_cache_lines[$i+1];
					last;
				}
			}
			if ( $cache_hit_3 == 0 )
			{
				$result_lines_3 = `$exec_cmd_3_`;
				push ( @new_cache_lines, $exec_cmd_3_."\n" );
				push ( @new_cache_lines, $result_lines_3."\n" );
			}
			print $main_log_file_handle_ "$result_lines_3\n" ;
			my @rwords_3 = split ( ' ', $result_lines_3 ) ;
			
			if ( ( $#rwords_2 > -1 ) && ( $rwords_2[$#rwords_2] > 0 ) )
			{
				my $exch_vol = $rwords_2[$#rwords_2] ;
				$no_of_days_ = $no_of_days_ + 1 ;
				my $vol = $rwords_[5];
				my $pnl = $rwords_[8];
				my $uts = $rwords_[24];
				if ( $risk_measure == 3 )
				{
					if ( $uts == 0 ) { $uts = 1 ; }
					$norm_pnl = $pnl/$uts ;
					$avg_risk = $avg_risk + $uts ;
                                        $avg_norm_vV = $avg_norm_vV + (($vol/$exch_vol)/$uts) ;
					push( @min_normalised_pnl, $rwords_[18]/$uts );
				}
				if ( $risk_measure == 2 )
				{
					$norm_pnl = $pnl/$vol ;
					$avg_risk = $avg_risk + $vol;
					push( @min_normalised_pnl, $rwords_[18]/$vol );
				}
				if ( $risk_measure == 1 ) 
				{
					$norm_pnl = $pnl/($vol/$exch_vol) ;
					$avg_risk = $avg_risk + ($vol/$exch_vol);
					push( @min_normalised_pnl, $rwords_[18]/($vol/$exch_vol) )
				}
				$avg_normalized_pnl = $avg_normalized_pnl + $norm_pnl;
				$avg_exch_vol = $avg_exch_vol + $exch_vol ;
				$avg_l1_size = $avg_l1_size + $rwords_3[$#rwords_3];
			}			
		}
		push ( @{ $normalized_pnl_array[$iter] }  , $norm_pnl ) ;
		}
		
		print $main_log_file_handle_ "$products_list_[$index_] $avg_normalized_pnl $avg_risk $no_of_days_\n";
		if ( $no_of_days_ > 0 )
		{	
		push ( @product_to_avg_normalized_pnl_map_, $avg_normalized_pnl/$no_of_days_ );
		push ( @product_to_avg_risk_map_, $avg_risk/$no_of_days_ );
		push ( @product_to_avg_exch_vol_map_ , $avg_exch_vol/$no_of_days_ );
		push ( @product_to_avg_l1sz_map_ , $avg_l1_size/$no_of_days_  );
                push ( @product_to_avg_norm_vV_map_ , $avg_norm_vV/$no_of_days_ );
		}
		else
		{
		push ( @product_to_avg_normalized_pnl_map_, 0 );
                push ( @product_to_avg_risk_map_, 0 );
                push ( @product_to_avg_exch_vol_map_ , 0 );
                push ( @product_to_avg_l1sz_map_ , 0  );
		push ( @product_to_avg_norm_vV_map_ , 0 );
		}
		@min_normalised_pnl = sort {$a <=> $b} @min_normalised_pnl;
		
		for( my $iter = 0; $iter <= $#VAR_limit_percentiles; $iter ++ )
		{
			my $percentile_to_index = $VAR_limit_percentiles[$iter]*($#min_normalised_pnl) ;
			if ( $#min_normalised_pnl < 0 )
			{
			push ( @{ $worst_case_pnl_array[$iter] }, 0 ) ;
			}
			else
			{
			push ( @{ $worst_case_pnl_array[$iter] }, $min_normalised_pnl[ $percentile_to_index ] ) ;
			}
		}
	}
}

sub PrintData
{
	print $main_log_file_handle_ "DATA\n";
	print $main_log_file_handle_ "PRODS:\n @products_list_\n";
	print $main_log_file_handle_ "AVG_NORMALIZED_PNLS:\n @product_to_avg_normalized_pnl_map_\n";
	print $main_log_file_handle_ "AVG_RISKS:\n @product_to_avg_risk_map_\n";
	print $main_log_file_handle_ "AVG_EXCHG_VOL:\n @product_to_avg_exch_vol_map_\n";
	print $main_log_file_handle_ "AVG_L1SZS:\n @product_to_avg_l1sz_map_\n";
	print $main_log_file_handle_ "MAX_LOSS:\n @products_maxlosslimits_\n";
	print $main_log_file_handle_ "AVG_NORM_vV:\n @product_to_avg_norm_vV_map_\n";
	print $main_log_file_handle_ "VAR_PNLS:\n";
	for ( my $i = 0 ; $i <= $#VAR_limit_percentiles; $i ++)
	{
	print $main_log_file_handle_ "@{$worst_case_pnl_array[$i]}\n";
	}

	#print to result file also
	print $main_result_file_handle_ "DATA\n";
	print $main_result_file_handle_ "PRODS:\n @products_list_\n";
	print $main_result_file_handle_ "AVG_NORMALIZED_PNLS:\n @product_to_avg_normalized_pnl_map_\n";
	print $main_result_file_handle_ "AVG_RISKS:\n @product_to_avg_risk_map_\n";
	print $main_result_file_handle_ "AVG_EXCHG_VOL:\n @product_to_avg_exch_vol_map_\n";
	print $main_result_file_handle_ "AVG_L1SZS:\n @product_to_avg_l1sz_map_\n";
	print $main_result_file_handle_ "MAX_LOSS:\n @products_maxlosslimits_\n";
	print $main_result_file_handle_ "AVG_NORM_vV:\n @product_to_avg_norm_vV_map_\n";
	print $main_result_file_handle_ "VAR_PNLS:\n";
	for ( my $i = 0 ; $i <= $#VAR_limit_percentiles; $i ++)
	{
	print $main_result_file_handle_ "@{$worst_case_pnl_array[$i]}\n";
	}
	
	#final output for script
	
	print "ResultFile: $main_result_file_ RealPnL: $real_pnl OptimalPnL: $optimal_pnl \n";
}

sub ComputeOptimalRisk
{
	my $file1_handle = FileHandle->new;
	$file1_handle->open ( "> $file_1_ " ) or PrintStacktraceAndDie ( "Could not open $file_1_ for writing\n" );
	$file1_handle->autoflush(1);
	my $file2_handle = FileHandle->new;
	$file2_handle->open ( "> $file_2_ " ) or PrintStacktraceAndDie ( "Could not open $file_2_ for writing\n" );
	$file2_handle->autoflush(1);
	my $file3_handle = FileHandle->new;
	$file3_handle->open ( "> $file_3_ " ) or PrintStacktraceAndDie ( "Could not open $file_3_ for writing\n" );
	$file3_handle->autoflush(1);
	my $file4_handle = FileHandle->new;
	$file4_handle->open ( "> $file_4_ " ) or PrintStacktraceAndDie ( "Could not open $file_4_ for writing\n" );
	$file4_handle->autoflush(1);
	
	print $main_log_file_handle_ "risk_measure: $risk_measure\n";
	if ( $risk_measure == 3 )
	{
		for ( my $i = 0; $i <= $#max_risk_limits; $i++ )
		{
			$max_risk_limits[$i] = $max_risk_limits[$i] * $product_to_avg_l1sz_map_ [$i] ;
			$min_risk_limits[$i] = $min_risk_limits[$i] * $product_to_avg_l1sz_map_ [$i] ;
			if ( $product_to_avg_norm_vV_map_[$i] == 0 )
                        {
                                $product_to_max_v_by_V_limit_map_[$i] = 0;
                        }
                        else
                        {
                                $product_to_max_v_by_V_limit_map_[$i] = $product_to_max_v_by_V_limit_map_[$i] / $product_to_avg_norm_vV_map_[$i] ; #convert from % vV limit to uts limit
                        }

		}
	}
	if ( $risk_measure == 2 )
	{
		for ( my $i = 0; $i <= $#max_risk_limits; $i++ )
		{
			$max_risk_limits[$i] = $max_risk_limits[$i] * $product_to_avg_exch_vol_map_[$i] ;
			$min_risk_limits[$i] = $min_risk_limits[$i] * $product_to_avg_exch_vol_map_[$i] ;
		}
	}
	if ( $risk_measure == 1 )
	{
		#( $risk_measure == 1 ) #~ m/"volume_ratio"/ )
	}
	
	#This part of the code is to avoid inconsistency in constraints, else R messes up optimization
	# r_min <= r_optim <= r_max
	# r_optim <= alpha (determined by VaR limits)
	# if alpha < r_min , inconsistent state
	# Set r_min = alpha
	
	my @alpha = @worst_case_pnl_array ;
	my @min_var_lim_for_prod = ();
	for ( my $i = 0; $i <= $#max_risk_limits; $i ++ )
	{
		push( @min_var_lim_for_prod, 1000000 ) ;
	}

	print $main_log_file_handle_ "VaR_limits issues:\n ";
	for ( my $i = 0; $i <= $#worst_case_pnl_array; $i ++ )
	{
		for( my $j = 0; $j <= $#{ $worst_case_pnl_array[$i] }; $j ++ )
		{
			my $temp = 0;
			if ( $worst_case_pnl_array[$i][$j] == 0 )
			{ #avoid bad case of prod being absent
			$temp = 1000000;
			}
			else
			{
			$temp = -$VAR_limit_ntimes[$i]*$products_maxlosslimits_[$j]/$worst_case_pnl_array[$i][$j] ;
			}
			$alpha[$i][$j] = $temp ;
			#push ( @{ $alpha[$j] }, $temp ) ;
			#print "$alpha[$i][$j] ";
			#if ( $temp < $min_risk_limits[$j] )
			#{
			#	$min_risk_limits[$j] = $alpha[$i][$j] ;
			#	print $main_log_file_handle_ "incosistent constraint : $i $j \n"
			#}
			if ( $temp < $min_var_lim_for_prod[$j] )
			{
				$min_var_lim_for_prod[$j] = $temp;
			}
		}
		#print "\n";
	}
        for ( my $i = 0; $i <= $#max_risk_limits; $i ++ )
        {
                if ( $min_var_lim_for_prod[$i] == 1000000 )
                {
                        $min_var_lim_for_prod[$i] = 0; # implies product absent
                }
        }
        for ( my $i = 0; $i <= $#max_risk_limits; $i ++ )
        {
                if ( $max_risk_limits[$i] < $product_to_max_v_by_V_limit_map_[$i] )
                {
                        push ( @product_to_final_max_limit_map_, $max_risk_limits[$i] );
                }
                else
                {
                        push ( @product_to_final_max_limit_map_, $product_to_max_v_by_V_limit_map_[$i] );
                }

                if ( ( $product_to_final_max_limit_map_[$i] > $min_var_lim_for_prod[$i] ) && ( $min_var_lim_for_prod[$i] > $min_risk_limits[$i] ) )
                {
                        $product_to_final_max_limit_map_[$i] = $min_var_lim_for_prod[$i] ;
                }

                if ( $min_var_lim_for_prod[$i] > $min_risk_limits[$i] )
                {
                        push ( @product_to_final_min_limit_map_, $min_risk_limits[$i] );
                }
                else
                {
                        push ( @product_to_final_min_limit_map_, $min_var_lim_for_prod[$i] );
                }
        }

	print $file1_handle "@product_to_avg_normalized_pnl_map_" ;
	
	for ( my $i = 0; $i <= $#products_list_; $i ++ )
	{
		for ( my $j = 0; $j <= $#products_list_; $j ++ )
		{
			if ( $i == $j )
			{
				print $file2_handle " 1 ";
			}
			else
			{
				print $file2_handle " 0 ";
			}
		}
		print $file2_handle "\n";
		print $file3_handle "<= ";
		print $file4_handle "$product_to_final_max_limit_map_[$i] ";	

		for ( my $j = 0; $j <= $#products_list_; $j ++ )
		{
			if ( $i == $j )
			{
				print $file2_handle " 1 ";
			}
			else
			{
				print $file2_handle " 0 ";
			}
		}
		print $file2_handle "\n";
		print $file3_handle ">= ";
		print $file4_handle "$product_to_final_min_limit_map_[$i] ";
	}
	
# Taken care of by using final_min/max_limit_maps
#	for ( my $i = 0; $i <= $#worst_case_pnl_array; $i ++ )
#	{
#		for ( my $j = 0; $j <= $#{ $worst_case_pnl_array[$i] }; $j ++ )
#		{
#			for ( my $k = 0; $k <= $#products_list_; $k ++ )
#			{
#				if ( $k == $j )
#				{
#					#$worst_case_pnl_array[$i][$j] = -$worst_case_pnl_array[$i][$j];
#					#print $file2_handle "$worst_case_pnl_array[$i][$j]";
#					print $file2_handle " 1 ";
#				}
#				else
#				{
#					print $file2_handle " 0 ";
#				}
#			}
#			print $file2_handle "\n";
#			print $file3_handle "<= ";
#			#my $temp = $VAR_limit_ntimes[$i]*$products_maxlosslimits_[$j]/$product_to_avg_risk_map_[$j] ;
#			#my $temp = $VAR_limit_ntimes[$i]*$products_maxlosslimits_[$j] ;
#			#my $temp = $VAR_limit_ntimes[$i]*$products_maxlosslimits_[$j]/$worst_case_pnl_array[$i][$j] ;
#			#print $file4_handle "$temp ";
#			print $file4_handle "$alpha[$i][$j] ";
#		}
#	}
	
	my $n_r = $#normalized_pnl_array ;
	my $n_c = $#{ $normalized_pnl_array[0] } ;
	for ( my $i = 0; $i <= $n_r; $i ++ )
	{
		if ( $#{ $normalized_pnl_array[$i] } < $n_c )
		{
			$n_r = $i - 1;
			last;	# use global max_loss limits only on min no of lookback days 
		}
	}

	for ( my $i = 0; $i <= $n_r; $i ++ )
    {
        for ( my $j = 0; $j <= $n_c; $j ++ )
        {
			print $file2_handle "$normalized_pnl_array[$i][$j] ";
		}
		print $file2_handle "\n";
		print $file3_handle ">= ";
		print $file4_handle "-$global_max_loss ";
	}

	for ( my $i = 0; $i <= $n_r; $i ++ )
	{
		for ( my $j = 0; $j <= $n_c; $j ++ )
		{
			if ( $products_list_[$j] =~ m/LFI/ )
			{
				print $file2_handle "$normalized_pnl_array[$i][$j] ";
			}
			else
			{
				print $file2_handle "0 ";
			}
		}
		print $file2_handle "\n";
		print $file3_handle ">= ";
		print $file4_handle "-$lfi_max_loss ";
	}
	
	for ( my $i = 0; $i <= $n_r; $i ++ )
	{
		for ( my $j = 0; $j <= $n_c; $j ++ )
		{
			if ( $products_list_[$j] =~ m/BAX/ )
			{
				print $file2_handle "$normalized_pnl_array[$i][$j] ";
			}
			else
			{
				print $file2_handle "0 ";
			}
		}
		print $file2_handle "\n";
		print $file3_handle ">= ";
		print $file4_handle "-$bax_max_loss ";
	}

	for ( my $i = 0; $i <= $n_r; $i ++ )
	{
		for ( my $j = 0; $j <= $n_c; $j ++ )
		{
			if ( $products_list_[$j] =~ m/DI1/ )
			{
				print $file2_handle "$normalized_pnl_array[$i][$j] ";
			}
			else
			{
				print $file2_handle "0 ";
			}
		}
		print $file2_handle "\n";
		print $file3_handle ">= ";
		print $file4_handle "-$di_max_loss ";
	}

	$file1_handle->close;
	$file2_handle->close;
	$file3_handle->close;
	$file4_handle->close;	
	
	my $exec_cmd = "$SCRIPTS_DIR/linear_optimization.R $file_1_ $file_2_ $file_3_ $file_4_";
	print $main_log_file_handle_ "$exec_cmd\n";
	my @res_lines_ = `$exec_cmd`;
	print $main_log_file_handle_ "@res_lines_\n" ;
	#print "@products_list_\n";
	print $main_result_file_handle_ "@res_lines_\n" ;
	my $t_iter = 0;
	my @optimal_risk =();
	foreach my $line_ ( @res_lines_ )
	{
		if ( $line_ =~ m/solution/)
		{
			$t_iter = $t_iter+1;
			my $solution_line = $res_lines_[$t_iter];
			my @solution_words = split ( ' ', $solution_line );
			print $main_log_file_handle_ "solution: @solution_words\n" ;
			my $i = 1;
			for ( ; $i <= $#solution_words; $i ++ )
			{
				push (@optimal_risk,$solution_words[$i]);
			}
			while ( $#optimal_risk < $#products_list_ )
			{
				print $main_log_file_handle_ "@optimal_risk";
				$t_iter = $t_iter+1;
				$solution_line = $res_lines_[$t_iter];
        	    my @solution_words = split ( ' ', $solution_line );
	            print $main_log_file_handle_ "solution: @solution_words\n" ;
				for ( my $t = 1; $t <= $#solution_words; $t ++ )
	            {
        	        push (@optimal_risk,$solution_words[$t]);
					$i = $i+1;
                }

			}
		}
		$t_iter = $t_iter + 1;
	}

for ( my $i = 0; $i <= $#products_maxlosslimits_ ; $i++ )
{
	if ( $min_var_lim_for_prod[$i] == 0 )
	{ push ( @product_to_optimal_maxloss_map_, 0 ); }
	else
	{ push ( @product_to_optimal_maxloss_map_, ($products_maxlosslimits_[$i]/$min_var_lim_for_prod[$i])*$optimal_risk[$i] ) ; }
}

	if ( $email_id )
	{
		my $mail_body = "";
		$mail_body = $mail_body."prod\t avg_real_risk\t optimal_risk\t optimal_maxloss\t avg_pnl_per_unit_risk\t max_risk\t min_risk\t var_constraint\t exch_vol_constraint\n";
        	for (my $i = 0; $i<=$#optimal_risk; $i++)
        	{
                my $temp_line = sprintf "$products_list_[$i]\t\t %.2f\t\t %.2f\t\t %.2f\t\t %.2f\t\t %.2f\t\t %.2f\t\t %.2f\t\t %.2f\n", $product_to_avg_risk_map_[$i], $optimal_risk[$i], $product_to_optimal_maxloss_map_[$i] , $product_to_avg_normalized_pnl_map_[$i], $max_risk_limits[$i], $min_risk_limits[$i], $min_var_lim_for_prod[$i], $product_to_max_v_by_V_limit_map_[$i];
		$mail_body = $mail_body."$temp_line";
        	}
		open(MAIL, "|/usr/sbin/sendmail -t");
		my $hostname_=`hostname`;
		print MAIL "To: $email_id\n";
        	print MAIL "From: $email_id\n";
	        print MAIL "Subject: OptimalRisk $hostname_\n\n";
		print MAIL $mail_body ;
	        close(MAIL);
        	print $main_log_file_handle_ "Mail Sent to $email_id\n$mail_body\n";

	}
	else
	{#gather data for result date
	my $total_pnl = 0 ;
	my $optimal_total_pnl = 0;
	my @prod_pnls = ();
	my @optimal_pnls = ();
	for ( my $index_ = 0; $index_ <= $#products_list_; $index_ ++ )
	{		
		my $exec_cmd_ = "$BIN_DIR/get_pnl_for_shortcode_bytime_stats $products_list_[$index_] $result_yyyymmdd 1 UTC_$products_start_duration_[$index_] UTC_$products_end_duration_[$index_] DETAILS";
        print $main_log_file_handle_ "$exec_cmd_\n" ;
        my $cache_hit = 0 ;
		my $result_lines = "";
		for (my $i = 0; $i <= $#data_cache_lines; $i++)
		{
			if ( $data_cache_lines[$i] =~ m/$exec_cmd_/ )
			{
				$cache_hit = 1;
			#	print $main_log_file_handle_ "cachehit\n";
				$result_lines = $data_cache_lines[$i+1];
				last;
			}
		}
		if ( $cache_hit == 0 )
		{
			$result_lines = `$exec_cmd_`;
			push ( @new_cache_lines, $exec_cmd_."\n" );
			push ( @new_cache_lines, $result_lines."\n" );
		}
		print $main_log_file_handle_ "$result_lines\n" ;
		my @rwords_ = split ( ' ', $result_lines ) ;
		if ( ( $#rwords_ > -1 ) && ( $rwords_[5] > 0 ) )
		{
			push (@prod_pnls, $rwords_[8]) ;
			my $x = 0;
			#my $x = $rwords_[8]*$optimal_risk[$index_]/$product_to_avg_risk_map_[$index_] ;
			if ( $risk_measure == 3)
			{
				my $uts = 1;
				if ( $rwords_[24] > 0 ) { $uts = $rwords_[24]; } else { $uts = 1; }
				$x = $rwords_[8]*$optimal_risk[$index_]/$uts;
			}
			else
			{
				PrintStacktraceAndDie ( "Not completely implemented for 1 and 2 risk measures. \n" )
			}
			push (@optimal_pnls, $x );
			$total_pnl = $total_pnl + $rwords_[8];
			$optimal_total_pnl = $optimal_total_pnl + $x;
		}
		else
		{
			push (@prod_pnls, 0) ;
			push (@optimal_pnls, 0 );
		}
	}
	#print "@products_list_\n";
	#print "real risk:@product_to_avg_risk_map_\n";
	#print "optimal risk:@optimal_risk\n";
	print $main_result_file_handle_ "prod\t avg_real_risk\t optimal_risk\t optimal_maxloss\t avg_pnl_per_unit_risk\t max_risk\t min_risk\t var_constraint\t exch_vol_constraint\n";
	for (my $i = 0; $i<=$#optimal_risk; $i++)
	{
		printf $main_result_file_handle_ "$products_list_[$i]\t\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\n", $product_to_avg_risk_map_[$i], $optimal_risk[$i], $product_to_optimal_maxloss_map_[$i], $product_to_avg_normalized_pnl_map_[$i], $max_risk_limits[$i], $min_risk_limits[$i], $min_var_lim_for_prod[$i], $product_to_max_v_by_V_limit_map_[$i];
	}
	print $main_result_file_handle_ "REAL:\n@prod_pnls\ntotal on $result_yyyymmdd: $total_pnl\n";
	print $main_result_file_handle_ "OPTIMAL:\n@optimal_pnls\ntotal on $result_yyyymmdd: $optimal_total_pnl\n";
	$optimal_pnl = $optimal_total_pnl;
	$real_pnl = $total_pnl;
	}	
}
