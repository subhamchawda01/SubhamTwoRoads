#!/usr/bin/perl

# \file ModelScripts/stir_generate_strategies.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

#format of the input :
#line 1:components of the structure separated by space
#line i : shc_ strategy_

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use Fcntl qw (:flock);
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

package ResultLine;
use Class::Struct;

# declare the struct
struct ( 'ResultLine', { pnl_ => '$', volume_ => '$', ttc_ => '$' } );

package main;

sub LoadConfigFile ;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $GSW_WORK_DIR=$SPARE_HOME."GSW/";

my $REPO="basetrade";
my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STIR_STRATS_DIR="/home/dvctrader/modelling/stir_strats";
my $MODELING_STRATS_DIR="/home/dvctrader/modelling/strats";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $INSTALL_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $GLOBAL_RESULTS_DIR="/NAS1/ec2_globalresults";

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_iso_date_from_str_with_today.pl"; #GetIsoDateFromStrWithToday

my $USAGE = "$0 configname date[TODAY]";
if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $configfile_ = $ARGV[0];
if ( ! ( -e $configfile_ ))
{
	print STDERR "Config file doesnt exist.\n";
}
my $today_ = "";
if ( $#ARGV >= 1 ) { $today_ = $ARGV[1]; }
$GSW_WORK_DIR = $GSW_WORK_DIR."/$configfile_/"; `mkdir -p $GSW_WORK_DIR`;

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $GSW_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
    	print STDERR "Surprising but this dir exists\n";
    	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    	$work_dir_ = $GSW_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}
my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $MODELING_STRATS_DIR ) ) {`mkdir -p $MODELING_STRATS_DIR`;}
if ( ! ( -d $MODELING_STIR_STRATS_DIR ) ) {`mkdir -p $MODELING_STIR_STRATS_DIR`;}

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

my $product_ = "";
my $datagen_start_yyyymmdd_ = "";
my $datagen_end_yyyymmdd_ = "";
my $trading_start_yyyymmdd_ = "";
my $trading_end_yyyymmdd_ = "";
my $datagen_start_hhmm_ = "";
my $datagen_end_hhmm_ = "";
my $trading_start_hhmm_ = "";
my $trading_end_hhmm_ = "";
my $outsample_start_yyyymmdd_ = "";
my $outsample_end_yyyymmdd_ = "";
my $sort_algo_ = "kCNAPnlAverage";

my @input_strat_list_ = () ;
my $mail_address_ = "";
my $template_ilist_ = "";

my %strat_name_to_common_param_name_ = () ;
my %strat_name_to_regress_algo_string_ = () ;
my %strat_name_to_filter_string_ = () ;
my %strat_name_to_predalgo_string_ = () ;
my %strat_name_to_datagen_string_ = ();
my %strat_name_to_pred_duration_ = () ;
my %strat_name_to_prod_list_ = () ;
my %strat_name_to_prod_to_param_name_ = () ;

LoadConfigFile( $configfile_ );
ParseStrategyFiles();
foreach my $strat_ ( keys %strat_name_to_prod_list_ )
{
	my $strat_basename_ = basename $strat_ ; chomp ( $strat_basename_);
	
	foreach my $prod_ ( @{$strat_name_to_prod_list_ {$strat_} } )
	{
		print $main_log_file_handle_ "Genstratfor $prod_ in strat $strat_ \n";
		my $this_prod_config_name_ = MakeGenStratConfig( $prod_, $strat_ ) ;
		print $main_log_file_handle_ " calling genstrat\n";
		my $genstrat_out_ = `$MODELSCRIPTS_DIR/eq_generate_strategies.pl $this_prod_config_name_ 2>&1`; chomp ( $genstrat_out_ );
		print $main_log_file_handle_ $genstrat_out_."\n";
		my $new_strat_result_dir_ = $genstrat_out_."/local_results_base_dir/";
		my $exec_cmd_ = $LIVE_BIN_DIR."/summarize_strategy_results local_results_base_dir $genstrat_out_/strats_dir $genstrat_out_  $outsample_start_yyyymmdd_ $outsample_end_yyyymmdd_ INVALIDFILE $sort_algo_  2>/dev/null | grep STRAT| head -n1";
		my $chosen_strat_stats_ = `$exec_cmd_`; chomp ( $exec_cmd_);
		my @strat_res_words_ = split ( " ", $chosen_strat_stats_);
		
		my $original_strat_summarize_cmd_ = $INSTALL_BIN_DIR."/summarize_single_strategy_results $prod_ $strat_basename_ $GLOBAL_RESULTS_DIR $outsample_start_yyyymmdd_ $outsample_end_yyyymmdd_ 2>/dev/null";
		#print $original_strat_summarize_cmd_."\n";
		print $main_log_file_handle_ $original_strat_summarize_cmd_."\n";
		my @original_strat_res_ = `$original_strat_summarize_cmd_`; chomp ( @original_strat_res_);
		my $old_strat_results_ = "";
		foreach my $line_ ( @original_strat_res_ )
		{
			if ( index ( $line_, "STATISTICS") >= 0 )
			{
				my @line_words_ = split ( " ", $line_ ) ;
				$old_strat_results_ = sprintf ( "%s", join ( " ", @line_words_[1..($#line_words_)]));
			}
		}
		
		print "NEW $prod_ STRAT STATS: ".join ( " ", @strat_res_words_[2..($#strat_res_words_ -1 )])."\n";
		print "OLD $prod_ STRAT STATS: ".$old_strat_results_."\n";
	}
}

exit ( 0 );

sub MakeGenStratConfig
{
	print $main_log_file_handle_ "MakeGenStratConfig\n";
	
	my $prod_ = shift ;
	my $strat_name_ = shift;
	my $genstrat_config_file_ = $work_dir_."/gen_strat_config_$prod_" ;
	my $prod_list_file_ = $work_dir_."/prod_list_$prod_";
	`echo $prod_ > $prod_list_file_`;
	my $prod_paramlist_ = $work_dir_."/prod_paramlist_$prod_";
	open PARAMLIST, "> $prod_paramlist_ " or PrintStacktraceAndDir( "Could not open the rfile $prod_paramlist_ for writing\n");
	print PARAMLIST "$prod_ $strat_name_to_prod_to_param_name_{$strat_name_}{$prod_}\n"; 
	close PARAMLIST ;
	
	open GS_CONFIG_FILE, "> $genstrat_config_file_" or PrintStacktraceAndDie ( "Could not open file $genstrat_config_file_ for writing..\n");
	print GS_CONFIG_FILE "SHORTCODE\n$prod_\n\n";
	print GS_CONFIG_FILE "DEPENDENT_LIST\n$prod_list_file_\n\n";
	print GS_CONFIG_FILE "TEMPLATE_ILIST\n$template_ilist_\n\n";
	print GS_CONFIG_FILE "TRADING_START_END_YYYYMMDD\n$outsample_start_yyyymmdd_ $outsample_end_yyyymmdd_\n\n";
	print GS_CONFIG_FILE "DATAGEN_START_END_YYYYMMDD\n$datagen_start_yyyymmdd_ $datagen_end_yyyymmdd_\n\n";
	print GS_CONFIG_FILE "TRADING_START_END_HHMM\n$trading_start_hhmm_ $trading_end_hhmm_\n\n";
	print GS_CONFIG_FILE "DATAGEN_START_END_HHMM\n$datagen_start_hhmm_ $datagen_end_hhmm_\n\n";
	
	print GS_CONFIG_FILE "STRATEGY_NAME\nEquityTrading2\n\n";
	print GS_CONFIG_FILE "PARAMLIST\n$prod_paramlist_\n\n";
	print GS_CONFIG_FILE "COMMONPARAMFILE\n$strat_name_to_common_param_name_{$strat_name_}\n\n";
	print GS_CONFIG_FILE "REGRESS_ALGO\n$strat_name_to_regress_algo_string_{$strat_name_}\n\n";
	print GS_CONFIG_FILE "FILTER\n$strat_name_to_filter_string_{$strat_name_}\n\n";
	print GS_CONFIG_FILE "PREDALGO\n$strat_name_to_predalgo_string_{$strat_name_}\n\n";
	print GS_CONFIG_FILE "DATAGEN_TIMEOUTS\n$strat_name_to_datagen_string_{$strat_name_}\n\n";
	print GS_CONFIG_FILE "PRED_DURATION\n$strat_name_to_pred_duration_{$strat_name_}\n\n";
	print GS_CONFIG_FILE "DELETE_INTERMEDIATE_FILES\n1\n\n";
	#print GS_CONFIG_FILE "MAIL_ADDRESS\n$mail_address_\n\n";
	print GS_CONFIG_FILE "INSTALL\n0\n";
	close GS_CONFIG_FILE;
	$genstrat_config_file_;
} 

sub ParseStrategyFiles 
{
	print $main_log_file_handle_ "parseStrategyFiles $#input_strat_list_\n";
	foreach my $strat_ ( @input_strat_list_ )
	{
		my $strat_basename_ = basename $strat_ ;
		my $strat_path_ = `ls $MODELING_STIR_STRATS_DIR/$product_/*/$strat_basename_ 2> /dev/null`; chomp ( $strat_path_ );
		if ( $strat_path_ eq "" )  
		{
			$strat_path_ = `ls $MODELING_STIR_STRATS_DIR/$product_/$strat_basename_ 2> /dev/null`; chomp ( $strat_path_ );
		}
		if ( $strat_path_ eq ""){ print STDERR "Could not find $strat_basename_ in modelling \n"; next; }
		my $strat_im_path_ = `cat $strat_path_ | awk '{print \$2}'`; chomp ( $strat_im_path_);
		my $common_param_ = `cat $strat_im_path_ | grep STRUCTURED_TRADING | awk '{print \$4}'`; chomp ( $common_param_);
		my $common_line_ = `cat $strat_im_path_ | grep STRUCTURED_TRADING `; chomp ( $common_line_);
		$strat_name_to_common_param_name_{$strat_} = $common_param_ ;
		$strat_name_to_regress_algo_string_{$strat_} = "LM";
		$strat_name_to_filter_string_ {$strat_} = "fsg1";
		$strat_name_to_predalgo_string_{$strat_} = "na_t3";
		$strat_name_to_pred_duration_{$strat_} = "96";
		$strat_name_to_datagen_string_{$strat_} = "1000 0 0 ";
		open IM_FILE, "< $strat_im_path_" or PrintStacktraceAndDie ( "Could not open the file $strat_im_path_ for reading\n");
		my @strat_im_lines_ = <IM_FILE>;
		my @prod_list_ = () ;
		foreach my $line_ ( @strat_im_lines_ )
		{
			if ( index ($line_,"STRATEGYLINE") >= 0 )
			{
				my @words_ = split ( " ", $line_ );
				if ( $#words_ >= 0 )
				{
					$strat_name_to_prod_to_param_name_ {$strat_}{$words_[1]} = $words_[3];
				}
				push ( @prod_list_, $words_[1] );
			}
		}
		
		$strat_name_to_prod_list_{$strat_} = \@prod_list_ ;
	}
}

sub LoadConfigFile 
{
	my $cfname_ = shift ;
	open CONFIG, "< $cfname_" or PrintStackTraceAndDie ( "Could not open config file $cfname_ for writing\n");
	my @config_lines_ = <CONFIG>; chomp ( @config_lines_ );
	my $current_param_ = "";
	my $datagen_days_ = 10;
	my $outsample_days_ = 10;
	
	foreach my $config_file_lines_ ( @config_lines_ )
	{
		print $main_log_file_handle_ "$config_file_lines_\n";
		
		if ( index ( $config_file_lines_ , "#" ) == 0 ) # not ignoring lines with # not at the beginning
		{
			next;
		}
		
		my @t_words_ = split ( ' ' , $config_file_lines_ );
		
		if ( $#t_words_ < 0 )
		{
			$current_param_ = "";
			next;
		}
		
		if ( ! $current_param_ )
		{
			$current_param_ = $t_words_ [ 0 ];                                          
			next;
		}
		else    
		{
			given ( $current_param_ )
			{
				when ("SHORTCODE")
				{
					if ( $#t_words_ >=0 ) 
					{
						$product_ = $t_words_[0];
					}
				}
				when ( "DATAGEN_DAYS")
				{
					if ( $#t_words_ >= 0 )
					{
						$datagen_days_ = $t_words_[0];
					}
				}
				when ( "OUTSAMPLE_DAYS")
				{
					if ( $#t_words_ >= 0 )
					{
						$outsample_days_ = $t_words_[0];
					}
				}
				when ("DATAGEN_TRADING_START_END_YYYYMMDD")
				{
					if ( $#t_words_ >= 3 )
					{
						if ( $today_ )
						{
							$datagen_start_yyyymmdd_ =  GetIsoDateFromStrWithToday ( $t_words_[0], $today_ );
							$datagen_end_yyyymmdd_ = GetIsoDateFromStrWithToday ( $t_words_[1], $today_ );
							$trading_start_yyyymmdd_ = GetIsoDateFromStrWithToday ( $t_words_[2], $today_ );
							$trading_end_yyyymmdd_ = GetIsoDateFromStrWithToday ( $t_words_[3], $today_ );
						}
						else
						{
							$datagen_start_yyyymmdd_ =  GetIsoDateFromStrMin1 ( $t_words_[0] );
							$datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[1]);
							$trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[2] );
							$trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[3] );
						}
					}
				}
				when ( "DATAGEN_START_END_YYYYMMDD" )
				{
					if ( $#t_words_ >= 1 )
					{
						if ( $today_ )
						{	
							$datagen_start_yyyymmdd_ = GetIsoDateFromStrWithToday( $t_words_[0], $today_ );
							$datagen_end_yyyymmdd_ = GetIsoDateFromStrWithToday ( $t_words_[1], $today_ );
						}
						else
						{
							$datagen_start_yyyymmdd_ = GetIsoDateFromStrMin1( $t_words_[0] );
							$datagen_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[1] );
						}
					}
				}
				when ( "TRADING_START_END_YYYYMMDD" )
				{
					if ( $#t_words_ >= 1 )
					{
						if ( $today_ )
						{
							$trading_start_yyyymmdd_ = GetIsoDateFromStrWithToday ( $t_words_[0], $today_ );
							$trading_end_yyyymmdd_ = GetIsoDateFromStrWithToday ( $t_words_[1], $today_ );							
						}
						else
						{
							$trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[0] );
							$trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[1] );
						}
					}
				}
				when ( "TRADING_START_END_HHMM" )
				{
					if ( $#t_words_ >= 1 )
					{
						$trading_start_hhmm_ = $t_words_[0];
						$trading_end_hhmm_ = $t_words_[1];
					}
				}
				when ( "DATAGEN_START_END_HHMM" )
				{
					if ( $#t_words_ >= 1 )
					{
						$datagen_start_hhmm_ = $t_words_[0];
						$datagen_end_hhmm_ = $t_words_[1];
					}
				}
				when ( "OUTSAMPLE_START_END_YYYYMMDD")
				{
					if ( $#t_words_ >= 1)
					{
						$outsample_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[0] );
						$outsample_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[1] );
					}
				}
				when ( "TEMPLATE_ILIST")
				{
					if ( $#t_words_ >= 0 )
					{
						$template_ilist_ = $t_words_[0];
					}
				}
				when ( "INPUT_STRAT_LIST_FILE")
				{
					if ( $#t_words_ >= 0 )
					{
						my $input_strat_list_file_ = $t_words_[0];
						if ( -e $input_strat_list_file_ )
						{
							open INPUT_STRAT_LIST, "< $input_strat_list_file_" or PrintStackTraceAndDie ( "Could not open the $input_strat_list_file_ for reading ") ;
							my @out_ = <INPUT_STRAT_LIST>; chomp ( @out_ );
							close INPUT_STRAT_LIST ;
							push ( @input_strat_list_, @out_ );

						}
					}
				}
				when ("MAIL_ADDRESS")
				{
					if ( $#t_words_ >= 0 )
					{
						$mail_address_ = $t_words_[0];
					}
				}
				
			}
		}
	}
}
