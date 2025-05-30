#!/usr/bin/perl

# \file ModelScripts/find_best_params_permute_for_strat.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
# This script takes :
#
# SHORTCODE
# TIMEPERIOD
# BASEPX
# PARAMFILE_WITH_PERMUTATIONS
# TRADING_START_YYYYMMDD 
# TRADING_END_YYYYMMDD

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use List::Util qw(first); # for index

use Class::Struct;

sub ComparisonFunction ;
sub PickProducts;
sub CreateStrats;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPA_WORK_DIR=$SPARE_HOME."FBPA/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "ankit" || $USER eq "anshul")
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats"; # this directory is used to s
my $MODELING_STIR_STRATS_DIR=$MODELING_BASE_DIR."/stir_strats"; # this directory is used to s

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
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/permute_params.pl"; # PermuteParams
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_start_end_hhmm.pl"; # GetStratStartEndHHMM
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_base_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratBaseExcludingSets
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $USAGE="<script> const_prod_list_file product_ globalresults start_date end_date [sort_algo=kCNAPnlAverage] [num_strats_to_choose=1]";

if ( $#ARGV < 4 )
{
	print $USAGE."\n";
	exit ( 0 );
}

my $prod_filename_ = $ARGV[0];
my $product_ = $ARGV[1];
my $globalresults_dir_ = $ARGV[2];
my $start_date_ = $ARGV[3];
my $end_date_ = $ARGV[4];

my $sort_algo_ = "kCNAPnlAdjAverage";
my $num_strats_to_choose_ = "1";
my $strats_dir_ = "/tmp/";
my $unique_id_ = `date +%N`; chomp ( $unique_id_ );
$strats_dir_ = $strats_dir_."/".$unique_id_ ; chomp ( $strats_dir_);
`mkdir -p $strats_dir_`;
my $summarize_ = 1;

if ( $#ARGV >= 5 ) { $sort_algo_ = $ARGV[5]; }
if ( $#ARGV >= 6 ) { $num_strats_to_choose_ = $ARGV[6] ; }
if ( $#ARGV >= 7 ) { $summarize_ = $ARGV[7];}
if ( $#ARGV >= 8 ) { $strats_dir_ = $ARGV[8] ; }


open PROD_LIST, "< $prod_filename_" or PrintStacktraceAndDie ( "Could not open the file $prod_filename_\n");
my @prod_list_ = <PROD_LIST>; chomp ( @prod_list_);
my @strategy_vec_vec_ = () ;
for my $prod_ ( @prod_list_ )
{
	chomp ( $prod_);
  my $tmp_file_list_ = "/tmp/tmp_filelist_".$unique_id_;
  `$SCRIPTS_DIR/get_eq_strats.sh $prod_ $MODELING_STIR_STRATS_DIR/$product_ > $tmp_file_list_`;
	my $exec_cmd_ = "$LIVE_BIN_DIR/summarize_strategy_results $prod_ $tmp_file_list_ $globalresults_dir_ $start_date_ $end_date_ INVALIDFILE $sort_algo_ 2>/dev/null | head -n$num_strats_to_choose_";
	my @strategy_out_ =`$exec_cmd_`; chomp ( @strategy_out_);
	push ( @strategy_vec_vec_ , \@strategy_out_);
  `rm -f $tmp_file_list_`;
	#print "@strategy_out_ \n";
}

my %common_param_to_prod_line_ = () ;
my %common_param_to_common_line_ = () ;
	
my @best_res_out_line_ = () ;
for ( my $index_ = 0; $index_ < $num_strats_to_choose_; $index_++ ) 
{
	my @result_out_line_ = () ;
	for ( my $prod_index_ = 0 ; $prod_index_ <= $#prod_list_ ; $prod_index_++ )
	{
		my @vec_ = @{$strategy_vec_vec_[$prod_index_] };
		#print "@vec_ and $#vec_\n";
		if ( $#vec_ < $index_ ) { next ; }
		my $strat_out_line_ = $vec_[$index_];
		#print "SOLINE ".$strat_out_line_."\n";
		my @strat_out_words_ = split ( " ", $strat_out_line_);
		
		if ( $#strat_out_words_ >= 8 )
		{
			my @this_result_line_ = ($prod_list_[$prod_index_]);
			push ( @this_result_line_, @strat_out_words_[2..$#strat_out_words_] );
			push ( @this_result_line_, $strat_out_words_[1] );
			#print "STR: ".$this_result_line_[1]."\n";
			push ( @result_out_line_ , \@this_result_line_ );
			#print "$prod_list_[$prod_index_] ".join ( " ",@strat_out_words_[2..$#strat_out_words_] )." ".substr( $strat_out_words_[1], 0 , 100 )." \n";
		}
	}
	@result_out_line_ = sort ComparisonFunction  @result_out_line_ ;
	if ( $summarize_ == 1 )
	{
		printf "%dth BEST STRAT \n", $index_ + 1;
		print "==================================================================================================================================================\n";
		foreach my $line_ ( @result_out_line_ )
		{
			print join ( " ", @{$line_}[0..(scalar(@{$line_})-2)] )." ".substr( @{$line_}[scalar(@{$line_})-1], 0 , 100)."\n" ;
		}
		print "\n\n";
	}
	
	if ( $summarize_ == 0 && $index_ == 0 )
	{ 
		@best_res_out_line_ = @result_out_line_; 
		PickProducts ( ); 
	}	
} 

CreateStrats ( );

exit (0);

sub PickProducts 
{
	foreach my $line_ ( @best_res_out_line_ )
	{
		my @line_arr_ = @{$line_ };
		my $prod_ = $line_arr_[0];
		my $strat_basename_ = $line_arr_[$#line_arr_];
		if ( $line_arr_[1] > 0 ) # currently having check for just pnl 
		{
			print "PICKING product $prod_ from strat $strat_basename_\n";
			my $strat_path_ = `ls $MODELING_STIR_STRATS_DIR/$product_/*/$strat_basename_ 2> /dev/null`; chomp ( $strat_path_ );
			if ( $strat_path_ eq "" )  
			{
				$strat_path_ = `ls $MODELING_STIR_STRATS_DIR/$product_/$strat_basename_ 2> /dev/null`; chomp ( $strat_path_ );
			}
			if ( $strat_path_ eq ""){ print STDERR "Could not find $strat_basename_ in modelling \n"; }
			
			my $strat_im_path_ = `cat $strat_path_ | awk '{print \$2}'`; chomp ( $strat_im_path_);
			my $common_param_ = `cat $strat_im_path_ | grep STRUCTURED_TRADING | awk '{print \$4}'`; chomp ( $common_param_);
			my $common_line_ = `cat $strat_im_path_ | grep STRUCTURED_TRADING `; chomp ( $common_line_);
			$common_param_to_common_line_{ $common_param_} = $common_line_ ; 
			my $prod_line_ = `cat $strat_im_path_ | grep STRATEGYLINE | grep $prod_ `; chomp( $prod_line_ ) ;
			$common_param_to_prod_line_{$common_param_}{$prod_} = $prod_line_ ;
		} 
	}
}

sub CreateStrats 
{
	my $idx_ = 1001 ; 
	foreach my $key_ ( keys %common_param_to_prod_line_ )
	{
		my $strat_name_ = $strats_dir_."/"."strat_".$idx_ ;
		my $strat_im_name_ = $strats_dir_."/"."im_strat_".$idx_ ;
		print $strat_name_." ".$strat_im_name_."\n";
		open STRAT, "> $strat_name_" or PrintStacktraceAndDie ( "Could not open strat $strat_name_\n");
		print STRAT "STRUCTURED_STRATEGYLINE $strat_im_name_ $idx_\n";
		close STRAT;
		
		open IM_STRAT, "> $strat_im_name_" or PrintStacktraceAndDie ( " Could not open im_strat $strat_im_name_ for writing\n" );
		my @common_line_words_ = split ( " ", $common_param_to_common_line_{$key_} );
		print IM_STRAT join (" ",@common_line_words_[0..($#common_line_words_-1)] )." ".$idx_."\n";
		foreach my $prod_ ( keys %{$common_param_to_prod_line_{$key_}} )
		{
			print IM_STRAT $common_param_to_prod_line_{$key_}{$prod_}."\n";
		}
		close IM_STRAT ;
		print "STRAT ".$strat_name_."\n";
		$idx_++;
	}
}

sub ComparisonFunction 
{
	#my ( $a , $b) = @_ ;
	if ( $sort_algo_ eq "kCNAPnlAverage" )
	{
		return ( $b->[1] <=> $a->[1] );
	}
	elsif ( $sort_algo_ eq "kCNAPnlSharpe" )
	{
		return ( $b->[4] <=> $a->[4] );
	}
	return 0 ;
}
