#!/usr/bin/perl

# \file ModelScripts/stir_parallel_find_best_params_permute_for_strat.pl
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
use Fcntl qw (:flock);
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

package ResultLine;
use Class::Struct;

# declare the struct
struct ( 'ResultLine', { pnl_ => '$', volume_ => '$', ttc_ => '$' } );

package main;


my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $FBPA_WORK_DIR=$SPARE_HOME."FBPA/";

my $REPO="basetrade";

my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "ankit" || $USER eq "anshul")
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/is_product_holiday.pl"; # IsProductHoliday
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/calc_next_working_date_mult.pl"; # CalcNextWorkingDateMult
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_stir_cat_file.pl"; # GetUniqueSimIdFromStirCatFile
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

require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated
#require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $MAX_STRAT_FILES_IN_ONE_SIM = 40; # please work on optimizing this value
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

# start
my $USAGE="$0 shortcode timeperiod basepx strategyname param_file_with_permutations start_date end_date strategy_file_name [min_volume] [max_ttc] [sort_algo]";

sub GetPriceTypeFromCode 
{
    my $px_code_ = shift;
    my @StandardPxTypes = ( "Midprice", "MktSizeWPrice", "MktSinusoidal", "OrderWPrice", "OfflineMixMMS" , "TradeWPrice" , "ALL" );
    if ( scalar grep $px_code_ eq $_, @StandardPxTypes ) {
	return $px_code_ ;
    }
    my %px_code_map_ = ( "Mkt" => "MktSizeWPrice",
			 "Mid" => "Midprice",
			 "MidPrice" => "Midprice",
			 "Sin" => "MktSinusoidal",
			 "Owp" => "OrderWPrice",
			 "Twp" => "TradeWPrice",
			 "OMix" => "OfflineMixMMS" );
    
    if ( ! $px_code_map_{ $px_code_ } ) {
	print $px_code_map_ { $px_code_ };
	PrintStacktraceAndDie ( "Wrong PxtyPe: $px_code_.\n");
    }
    return $px_code_map_ { $px_code_ } ;
}

sub GetExecLogicFromCode
{
    my $exec_logic_code_ = shift;
    my @StandardExecLogics = ( "DirectionalAggressiveTrading",
			       "DirectionalInterventionAggressiveTrading",
			       "DirectionalInterventionLogisticTrading",
			       "DirectionalLogisticTrading",
			       "DirectionalPairAggressiveTrading",
			       "PriceBasedAggressiveTrading",
			       "PriceBasedInterventionAggressiveTrading",
			       "PriceBasedSecurityAggressiveTrading",
			       "PriceBasedTrading",
			       "PriceBasedVolTrading",
			       "PriceBasedScalper",
			       "PricePairBasedAggressiveTrading", 
                               "ReturnsBasedAggressiveTrading" , 
                               "EquityTrading2",
                               "EquityTrading2-DirectionalAggressiveTrading") ;
    foreach my $st_exec_logic_( @StandardExecLogics ) {
	my $t_ = $st_exec_logic_; 
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
	$t_ =~ s/[a-z]//g ;
	# print "$t_ $st_exec_logic_\n" ;
	if ( $t_ =~ /$exec_logic_code_/ ) { return $st_exec_logic_; }
    }
    PrintStacktraceAndDie ( "Wrong ExecLogic Code: $exec_logic_code_\n" );
}

if ( $#ARGV < 5 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $timeperiod_string_ = $ARGV[1];
my $basepx_ = GetPriceTypeFromCode( $ARGV[2] );
my $strat_name_ = GetExecLogicFromCode( $ARGV[3] );
my $orig_param_filename_ = $ARGV[4];
my $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[5] );
my $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $ARGV[6] );
my $strategy_file_name_ =  ( $ARGV[7] );

my $min_volume_ = -1;
my $max_ttc_ = -1;
my $sort_algo_ = "kCNAPnlAverage";
if ( $#ARGV >= 10 )
{
    $min_volume_ = $ARGV [ 6 ];
    $max_ttc_ = $ARGV [ 7 ];
    $sort_algo_ = $ARGV [ 8 ];
}

my $param_file_list_basename_ = basename ( $orig_param_filename_ );
$FBPA_WORK_DIR = $FBPA_WORK_DIR.$shortcode_."/".$param_file_list_basename_."/";

my $trading_start_hhmm_ = "";
my $trading_end_hhmm_ = "";

my $delete_intermediate_files_ = 1;

my $num_files_to_choose_ = 1;
my $min_pnl_per_contract_to_allow_ = -1.10 ;
my $min_volume_to_allow_ = 100; 
my $max_ttc_to_allow_ = 120;

my %strat2param_;
my @model_filevec_ = ();
my @param_filevec_ = ();
my @strategy_filevec_ = ();
my %param_to_resultvec_;
my %param_to_cmedpnl_;
my %param_to_cmedvolume_;
my $common_line_ = "";
my @prodlist_ = () ;
my %prod_to_strat_string_ = () ;
my @intermediate_files_ = ();

# temporary
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ )
{
    if ( -d $work_dir_ )
    {
	print STDERR "Surprising but this dir exists\n";
	$unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
	$work_dir_ = $FBPA_WORK_DIR.$unique_gsm_id_; 
    }
    else
    {
	last;
    }
}

my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $temp_strat_filename_ = $local_strats_dir_."/temp_strat_file";
my $local_params_dir_ = $work_dir_."/params_dir";
my $full_strategy_filename_ = "";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose

# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir ) ) { `mkdir -p $local_results_base_dir`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $local_params_dir_ ) ) { `mkdir -p $local_params_dir_`; }

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

# Get param files and model files
ProcessListArgs ( );
if ( $#param_filevec_ < 0 )
{
    print "Exiting due to 0 param_filevec_\n";
    print $main_log_file_handle_ "Exiting due to 0 param_filevec_\n";
}
else
{
# From the given arguments create the strategy files for simulations

}
# end script
$main_log_file_handle_->close;

exit ( 0 );

sub ProcessListArgs
{
    print $main_log_file_handle_ "\nProcessListArgs\n\n";

    print $main_log_file_handle_ "\n$strategy_file_name_\n";
    #models
    my $full_stir_strategy_filename_ = "";
    if ( ExistsWithSize ( $strategy_file_name_ ) )
    {
	$full_stir_strategy_filename_ = $strategy_file_name_;
    }   
    else
    {
	print $main_log_file_handle_ "Strategy not found\n";	
    }

    print $main_log_file_handle_ "\n$full_stir_strategy_filename_\n";  
	
    
    my $exec_cmd_ = "cat $full_stir_strategy_filename_ | awk '{ print \$2}'";
    $full_strategy_filename_ = `$exec_cmd_`;
    chomp( $full_strategy_filename_ ); 

    open STRAT_FILE, "< $full_strategy_filename_" or PrintStacktraceAndDie ( "Could not open $full_strategy_filename_ for reading\n" ) ;
    my @strat_lines_ = <STRAT_FILE>;
    foreach my $strat_ ( @strat_lines_ ) 
    {
      $strat_ =~ s/^\s+|\s+$//g;
      if ( index ( $strat_ , "STRUCTURED_TRADING" ) >= 0 ) 
      {
        $common_line_ = $strat_ ;
        my @common_words_ = split ( ' ', $common_line_ ) ; chomp ( @common_words_ );
        if ( $#common_words_ >= 2 ) 
        {
          $common_words_[2] = $strat_name_
        }
        $common_line_ = join ( ' ', @common_words_ ) ; 
      }
      if ( index ( $strat_, "STRATEGYLINE" ) >= 0 ) 
      {
        my @strat_line_words_ = split (" ",$strat_ );
        if ( $#strat_line_words_ >= 3 ) 
        {
          push ( @prodlist_, $strat_line_words_[1] ) ;
          $prod_to_strat_string_ {$strat_line_words_[1]} = $strat_ ;
        }
      }
    }
    foreach my $prod_ ( @prodlist_ ) 
    {
      my $paramname_for_this_prod_ = MakeParamForThisProduct ( $prod_ ) ;
      my $this_prod_strat_content_name_ = $work_dir_."/fbpa_strat_$prod_";
      my $this_prod_strat_name_ = $work_dir_."/fbpta_strat_im_$prod_";
      open THIS_PROD_STRAT, "> $this_prod_strat_content_name_ " or PrintStacktraceAndDie ( "Could not open file $this_prod_strat_content_name_ for writing \n" ) ;
      print THIS_PROD_STRAT $common_line_."\n";
      print THIS_PROD_STRAT $prod_to_strat_string_ { $prod_ }."\n";
      close THIS_PROD_STRAT ;
      my $exec_cmd_ = " echo STRUCTURED_STRATEGYLINE $this_prod_strat_content_name_ 1001 > $this_prod_strat_name_ "; `$exec_cmd_`;
      $exec_cmd_ = "$MODELSCRIPTS_DIR/stir_parallel_find_best_param_permute_for_strat.pl $prod_  $paramname_for_this_prod_ 1 $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ $this_prod_strat_name_ 0 100000 kCNAPnlAdjAverage 1 ";
      print $main_log_file_handle_ $exec_cmd_ ."\n";
      my $val_ = `$exec_cmd_`; chomp ( $val_ ) ;
      print $main_log_file_handle_ $val_."\n";
    }

}

sub MakeParamForThisProduct 
{
  my $product_ = shift ;
# Currently template param has everything except unit tradsize

  open PARAM , "< $orig_param_filename_ " or PrintStacktraceAndDie ( "could not open the file" ) ;
  my @lines_= <PARAM>; chomp ( @lines_ ) ;
  foreach my $line_ ( @lines_ )
  {
    if ( index ( $line_, "UNIT_TRADE_SIZE" ) >= 0 ) 
    {
      my $this_uts_ = GetUTSForThisProduct ( $product_ ) ;
      $line_ = "PARAMVALUE UNIT_TRADE_SIZE $this_uts_\n";
    }
  }
  my $this_prod_param_name_ = $work_dir_."/$product_"."_param";
  open THIS_PROD_PARAM , "> $this_prod_param_name_ " or PrintStacktraceAndDie ( " COuld not open file $this_prod_param_name_ of writing \n" ) ;
  print THIS_PROD_PARAM join ( "\n", @lines_ ) ;
  close THIS_PROD_PARAM ;
  $this_prod_param_name_;
}

sub GetUTSForThisProduct 
{
  my $prod_ = shift ;
  given ( $prod_ )
  {
    when("SBSP3"){return 200;}
    when("HYPE3"){return 300;}
    when("CPLE6"){return 100;}
    when("TIMP3"){return 300;}
    when("ENBR3"){return 300;}
    when("BRKM5"){return 200;}
    when("MRVE3"){return 300;}
    when("GFSA3"){return 2000;}
    when("POMO4"){return 1200;}
    when("MRFG3"){return 400;}
    when("TBLE3"){return 100;}
    when("SANB11"){return 300;}
    when("EVEN3"){return 500;}
    when("CYRE3"){return 300;}
    when("BBDC4"){return 300;}
    when("KLBN11"){return 300;}
    when("VIVT4"){return 100;}
    when("ELET6"){return 300;} 
    when("JBSS3"){return 400;}
    when("BVMF3"){return 700;}
    when("ELET3"){return 600;}
    when("BBDC3"){return 100;}
    when("CTIP3"){return 200;}
    when("CMIG4"){return 400;}
    when("LREN3"){return 100;}
    when("EMBR3"){return 200;}
    when("GOLL4"){return 200;}
    when("PCAR4"){return 100;}
    when("CRUZ3"){return 200;}
    when("CCRO3"){return 400;}
    when("KROT3"){return 500;}
    when("ABEV3"){return 700;}
    when("QUAL3"){return 200;}
    when("ITUB4"){return 300;}
    when("HGTX3"){return 200;}
    when("ITSA4"){return 1000;}
    when("BRFS3"){return 100;}
    when("BRAP4"){return 200;}
    when("ESTC3"){return 300;}
    when("NATU3"){return 100;}
    when("LIGT3"){return 200;}
    when("DTEX3"){return 300;}
    when("PDGR3"){return 0;}
    when("UGPA3"){return 100;}
    when("USIM5"){return 1500;}
    when("ECOR3"){return 200;}
    when("CPFE3"){return 100;}
    when("RSID3"){return 1500;}
    when("LAME4"){return 200;}
    when("GOAU4"){return 200;}
    when("RLOG3"){return 200;}
    when("GGBR4"){return 700;}
    when("FIBR3"){return 100;}
    when("CSAN3"){return 100;}
    when("ELPL4"){return 200;}
    when("PETR4"){return 2000;}
    when("BBAS3"){return 300;}
    when("BRML3"){return 200;}
    when("CESP6"){return 100;}
    when("VALE5"){return 800;}
    when("PETR3"){return 1000;}
    when("VALE3"){return 400;}
    when("BRPR3"){return 200;}
    when("CSNA3"){return 700;}
    when("SUZB5"){return 500;}
    when("OIBR4"){return 500;}
    when("ALLL3"){return 500;}
    when("RUMO3"){return 500;}
    when("RENT3"){return 100;}
    when("BBSE3"){return 200;}
    when("CIEL3"){return 200;}
    when("MULT3"){return 10;}
    when("BEEF3"){return 500;}
    when("SULA11"){return 500;}
    when("BBTG11"){return 300;}
    when("EQTL3"){return 200;}
    when("MPLU3"){return 100;}
    when("BRSR3"){return 500;}
    when("PSSA3"){return 100;}
    when("RADL3"){return 300;}
    when("TOTS3"){return 100;}
    when("ODPV3"){return 600;}
    when("MDIA3"){return 100;}
    when("TAEE11"){return 300;}
    when("MYPK3"){return 300;}
    when("SEER3"){return 300;}
    when("GETI3"){return 300;}
    when("ANIM3"){return 200;}
    when("WEGE3"){return 400;}
    when("IGTA3"){return 300;}
    when("QGEP3"){return 700;}
    when("VLID3"){return 200;}
  }
}
# For each param file & for each model file, we create a strategy file
