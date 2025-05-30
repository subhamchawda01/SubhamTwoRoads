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
no if ($] >= 5.018), 'warnings' => 'experimental';

package ResultLine;
use Class::Struct;

# declare the struct
struct ( 'ResultLine', { pnl_ => '$', volume_ => '$', ttc_ => '$' } );

package main;

sub MakeStrategyFiles ; # takes input_stratfile and makes strategyfiles corresponding to different scale constants
sub RunSimulationOnCandidates ; # for the strategyfiles generated finds results in local database
sub SummarizeLocalResultsAndChoose ; # from the files created in the local_results_base_dir choose the best ones to send to pool
sub SendMail ;
sub ReadTemplateIlist;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 
my $GSW_WORK_DIR=$SPARE_HOME."GSW/";

my $REPO="basetrade";
my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_STIR_STRATS_DIR=$MODELING_BASE_DIR."/stir_strats";
my $MODELING_STRATS_DIR=$MODELING_BASE_DIR."/strats";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $INSTALL_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "ankit" || $USER eq "anshul" || $USER eq "rkumar")
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
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/file_name_in_new_dir.pl"; #FileNameInNewDir
require "$GENPERLLIB_DIR/list_file_to_vec.pl"; #ListFileToVec
require "$GENPERLLIB_DIR/find_item_from_vec_with_base.pl"; #FindItemFromVecWithBase
require "$GENPERLLIB_DIR/get_model_and_param_file_names.pl"; #GetModelAndParamFileNames
require "$GENPERLLIB_DIR/get_strat_start_end_hhmm.pl"; # GetStratStartEndHHMM
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_in_tp_match_strat_base_excluding_sets.pl"; # MakeStratVecFromDirInTpMatchStratBaseExcludingSets
require "$GENPERLLIB_DIR/make_filename_vec_from_list.pl"; # MakeFilenameVecFromList
require "$GENPERLLIB_DIR/array_ops.pl"; # GetConsMedianAndSort
require "$GENPERLLIB_DIR/get_pred_counters_for_this_pred_algo.pl"; # for GetPredCountersForThisPredAlgo
require "$GENPERLLIB_DIR/get_cs_temp_file_name.pl"; # GetCSTempFileName
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_and_tt.pl"; # MakeStratVecFromDirAndTT
require "$GENPERLLIB_DIR/get_sector_port_for_stocks.pl"; #for GetSectorPortForStocks 
require "$GENPERLLIB_DIR/parallel_sim_utils.pl"; # GetGlobalUniqueId , AllOutputFilesPopulated
require "$GENPERLLIB_DIR/sample_data_utils.pl"; #GetFeatureAverageDays
require "$GENPERLLIB_DIR/install_stir_strategy_modelling.pl"; #InstallStirStrategyModelling
require "$GENPERLLIB_DIR/strat_utils.pl"; # IsModelScalable , AddL1NormToModel ,GetModelL1NormVec 
require "$GENPERLLIB_DIR/genstrat_utils.pl"; # GenerateDaysVec

my $MAX_STRAT_FILES_IN_ONE_SIM = 40; # please work on optimizing this value
my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( );

# start
my $USAGE="$0 configname_ ";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $configname_ = $ARGV[0];

my $shortcode_ = "";
my $prod_list_file_ = "" ;
my $template_ilist_name_ = "";
my @prodlist_ = () ;

my $start_date_yyyymmdd_ = "";
my $end_date_yyyymmdd_ = "" ;
my $datagen_start_time_ = "" ;
my $datagen_end_time_ = "" ;
my $datagen_day_filter_start_yyyymmdd_ = "";
my $datagen_day_filter_max_days_ = 200;
my @datagen_day_filter_choices_ = ();
my %prod_to_datagen_exclude_days_ = ();
my %prod_to_datagen_days_ = ();
my $datagen_day_inclusion_prob_ = 1;
my $use_insample_days_for_trading_ = 0;

my $trading_start_hhmm_  = "" ;
my $trading_end_hhmm_ = "" ;
my $trading_start_yyyymmdd_ = "" ; 
my $trading_end_yyyymmdd_ = "" ;
my @trading_days_ = ();
my @trading_exclude_days_ = ();

my $filter_ = "" ;
my $predalgo_ = "" ;
my @predalgo_vec_ = () ;
my $regress_algo_string_ = "" ;
my @regress_algo_string_vec_ = () ;
my $datagen_timeouts_ = "" ;
my @datagen_timeout_vec_ = () ;

my $pred_duration_ = "" ;
my @pred_duration_vec_ = () ;

my $strategy_name_ = "" ;
my $common_param_filename_ = "";
my $min_volume_ = -1;
my $min_sharpe_ = -1;
my $min_pnl_per_vol_ = -1;
my $sort_algo_ = "kCNAPnlAverage";

my $number_of_strats_to_install_ = 1;
my $use_median_cutoff_ = "1";
my $add_modelinfo_to_model_ = 0; 
my $name_ = "";
my $author_ = "dvctrader_p";
my $generate_individual_data_ = 1;
my $apply_trade_vol_filter_ = 0;
my $fsudm_level_ = 0;

my $cut_off_rank_ = 30 ;
my %prod_to_reg_data_file_ = () ;
my %prod_to_filtered_data_file_ = () ;
my %prod_to_ilist_filename_ = () ;
my %prod_to_model_filename_ = () ;
my %prod_to_param_filename_ = () ;
my $template_shc_ = () ;
my $template_file_read_ = "";
my $dep_baseprice_type_ = "";
my $dep_pred_price_ = "" ;
my $num_prices_in_ilist_ = "" ;

my @common_indicators_ = () ;
my @template_indicators_ = () ;

my $delete_intermediate_files_ = 1;

my $num_files_to_choose_ = 1;
my $min_pnl_per_contract_to_allow_ = -1.10 ;
my $min_volume_to_allow_ = 100; 
my $max_ttc_to_allow_ = 120;
my $add_prod_name_in_strat_name_ = 0 ;

my @model_filevec_ = ();
my @strategy_filevec_ = ();

my @intermediate_files_ = ();
my $paramname_ = "";
my $paramlistfilename_ = "" ;

my %structure_strats_ = ();
my %structure_models_ = ();
my %structure_params_ = ();
my %idx_to_shc_map_ = ();
my $unique_shc_ = 0;
my @offset_array = ();
my @permutation_index_array = ();
my %prod_to_ilist_name_ = () ;
my $mail_content_ = "" ;
my $mail_id_ = "" ;
my $install_ = 1;

LoadConfigFile ( $configname_ ) ;

my $pool_file_list_basename_ = basename ( $prod_list_file_ );
$GSW_WORK_DIR = $GSW_WORK_DIR.$shortcode_."/".$pool_file_list_basename_."/";
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $work_dir_ = $GSW_WORK_DIR.$unique_gsm_id_; 
for ( my $i = 0 ; $i < 30 ; $i ++ ) {
  if ( -d $work_dir_ ) {
    print STDERR "Surprising but this dir exists\n";
    $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
    $work_dir_ = $GSW_WORK_DIR.$unique_gsm_id_; 
  }
  else {
    last;
  }
}
print "WORKDIR = $work_dir_\n";

my $num_params_ = -1 ; # number of params per product, should be same across all products
my $ilist_filename_ = $work_dir_."/temp_ilist.txt";
my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my $local_strats_dir_ = $work_dir_."/strats_dir";
my $temp_strat_filename_ = $local_strats_dir_."/temp_strat_file";
my $full_strategy_filename_ = "";

my $main_log_file_ = $work_dir_."/main_log_file.txt";
my $main_log_file_handle_ = FileHandle->new;
my @unique_results_filevec_ = (); # used in RunSimulationOnCandidates and SummarizeLocalResultsAndChoose
# start
if ( ! ( -d $work_dir_ ) ) { `mkdir -p $work_dir_`; }
if ( ! ( -d $local_results_base_dir ) ) { `mkdir -p $local_results_base_dir`; }
if ( ! ( -d $local_strats_dir_ ) ) { `mkdir -p $local_strats_dir_`; }
if ( ! ( -d $MODELING_STRATS_DIR ) ) {`mkdir -p $MODELING_STRATS_DIR`;}
if ( ! ( -d $MODELING_STIR_STRATS_DIR ) ) {`mkdir -p $MODELING_STIR_STRATS_DIR`;}

$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

my $process_start_time_ = `date`; chomp ( $process_start_time_ ) ;
my $server_ = `hostname`; chomp ( $server_ ) ;

GetProductsToTrade ( $prod_list_file_ );

if ( ! $generate_individual_data_ ) {
 MakeIndicatorList ( $template_ilist_name_ ) ;
}
MakeIndividualIndicatorList($template_ilist_name_ ) ;

GetParamFilenames ( ) ;
CollectDates () ;

if ( $generate_individual_data_ ) {
  GenerateIndividualData() ;
} else {
  GenerateData () ;
}

RunRegressionMakeModels ( ) ;
MakeStrategyFiles ( );

if ( $shortcode_ eq "NSE_FO" ) {
  `mkdir -p $work_dir_/strats_dir1/strat/`;
  `mkdir -p $work_dir_/strats_dir1/results/`;
  my $strats_ = `ls  $work_dir_/strats_dir/\* | grep -v _im_`;
  my @strats_array_ = split '\n', $strats_;
  
  foreach my $stt_ ( @strats_array_ ) {
    `cp $stt_ $work_dir_/strats_dir1/strat/`;
  }
  my $date_ = `date +%Y%m%d`; chomp( $date_ );
  my $date1_ = `$INSTALL_BIN_DIR/calc_prev_week_day $date_ 22`;
  my $date2_ = `$INSTALL_BIN_DIR/calc_prev_week_day $date1_ 22`;  
  my $date3_ = `$INSTALL_BIN_DIR/calc_prev_week_day $date2_ 22`;  
  `$HOME_DIR."/".$REPO/ModelScripts/run_simulations_stir_2.pl NSE_FO $work_dir_/strats_dir1/strat/ $date1_ $date_ 1 $work_dir_/strats_dir1/results/ 1>/dev/null 2>&1 &`;
  `$HOME_DIR."/".$REPO/ModelScripts/run_simulations_stir_2.pl NSE_FO $work_dir_/strats_dir1/strat/ $date2_ $date1_ 1 $work_dir_/strats_dir1/results/ 1>/dev/null 2>&1 &`;
  `$HOME_DIR."/".$REPO/ModelScripts/run_simulations_stir_2.pl NSE_FO $work_dir_/strats_dir1/strat/ $date3_ $date2_ 1 $work_dir_/strats_dir1/results/ 1>/dev/null 2>&1 &`;
}
else {
  # Run simulations on the created strategy files
  RunSimulationOnCandidates ( );

  # among the candidates choose the best
  SummarizeLocalResultsAndChoose();
}
$main_log_file_handle_->close;
print $work_dir_."\n";
exit ( 0 );


sub GetProductsToTrade 
{
  my $filename_ = shift ;
  open PROD_FILE, " < $filename_ " or PrintStacktraceAndDie (  " Could not open file $filename_ for reading\n" ) ;
  @prodlist_ = <PROD_FILE>; chomp ( @prodlist_ ) ;
  $_ =~ s/^\s+|\s+$//g foreach @prodlist_;
  close PROD_FILE;
}

sub ReadTemplateIlist
{
  my $templatename_ = shift ;
  return if ( $template_file_read_);
  open TEMPLATE_FILE, "< $templatename_ " or PrintStacktraceAndDie ( " Could not open template file for reading\n" ) ; 
  my @list_ = <TEMPLATE_FILE>;

  my $model_init_read_ = "";
  foreach my $line_ ( @list_ )  {
    $line_ =~ s/^\s+|\s+$//g ;
    my @line_words_ = split ( ' ', $line_ ) ;
    next if ( $#line_words_ < 1 );
    
    if ( index ( $line_, "MODELINIT") >= 0 ) {
      $template_shc_ = $line_words_[2] ; 
      $dep_baseprice_type_ = $line_words_[3] ;
      $dep_pred_price_ = $line_words_[4];
      $model_init_read_ = "true" ;
    }

    if ( $line_words_[0] eq "INDICATOR" && $model_init_read_ )  {
      if ( index (  $line_, $template_shc_) >= 0 ||
          index ( $line_, "SECTORPORT" ) >= 0 ||
          index ( $line_, "SECTORFRAC" ) >= 0 || 
          index ( $line_, "NEXTMAJORSTOCK" ) >= 0 ) {
        push ( @template_indicators_, $line_ )  ;
      }
      else {
        push ( @common_indicators_ , $line_ ) ;
      }
    }
    $template_file_read_ = "true";
  }
}

sub MakeIndividualIndicatorList
{
  my $templatename_ = shift ;
  ReadTemplateIlist($templatename_);

  my $template_shc_base = $template_shc_;
  if ( index ($template_shc_base, "NSE_") >= 0 ) {
    $template_shc_base =~ s/_FUT0|_FUT1|_C0_O1|_P0_O1//g;
  }


  foreach my $prod_ ( @prodlist_ ) {
    my $ilist_ = $work_dir_."/".$prod_."_ilist_filename.txt";
    open THIS_ILIST_FILE, "> $ilist_" or PrintStackTraceAndDie ( "Could not open file to write\n" ) ;
    print THIS_ILIST_FILE "MODELINIT DEPBASE $prod_ $dep_baseprice_type_ $dep_pred_price_\n";
    print THIS_ILIST_FILE "MODELMATH LINEAR CHANGE\n";
    print THIS_ILIST_FILE "INDICATORSTART\n";

    my $prod_base = $prod_;
    if ( index ($prod_base, "NSE_") >= 0 ) {
      $prod_base =~ s/_FUT0|_FUT1|_C0_O1|_P0_O1//g;
    }

    print $main_log_file_handle_ "Template $template_shc_ $template_shc_base Prod: $prod_ $prod_base\n"; 

    foreach my $common_line_ ( @common_indicators_ ) {
      my $t_ind_ = $common_line_ ;
      $t_ind_ =~ s/$template_shc_base/$prod_base/g;
      $t_ind_ = AdjustIndicatorParameters ( $t_ind_, $prod_ ); 
      print THIS_ILIST_FILE $t_ind_."\n" ;
    }

    foreach my $template_indicator_ ( @template_indicators_ ) {
      my $t_ind_ = $template_indicator_ ;
      $t_ind_ =~ s/$template_shc_base/$prod_base/g;
      $t_ind_ = AdjustIndicatorParameters ( $t_ind_, $prod_ ); 
      print  THIS_ILIST_FILE $t_ind_."\n" ;
    }
    print THIS_ILIST_FILE "INDICATOREND\n";

    close THIS_ILIST_FILE ;
    $prod_to_ilist_name_{$prod_} = $ilist_;
  }
}

sub MakeIndicatorList 
{
  my $templatename_ = shift ;
  ReadTemplateIlist($templatename_);

  open ILIST_FILE, "> $ilist_filename_ " or PrintStacktraceAndDie ( " Could not open ilist file to write\n" ) ;
  my $dep_shortcode_ = "BR_IND_0";
  $dep_shortcode_ = "NSE_NIFTY_FUT0" if ( index ( $template_shc_, "NSE_") >= 0 );
  
  print ILIST_FILE "MODELINIT DEPBASE " .$dep_shortcode_." ".$dep_baseprice_type_." ".$dep_pred_price_ ."\n";
  print ILIST_FILE "MODELMATH LINEAR CHANGE\n" ; 
  print ILIST_FILE "INDICATORSTART\n" ;

  foreach my $prod_ ( @prodlist_ ) {
    print ILIST_FILE "INDICATOR 1.00 L1Price $prod_ $dep_baseprice_type_\n" ;
    $num_prices_in_ilist_ = 1 ;
    
    if ( $dep_baseprice_type_ ne $dep_pred_price_ ) {
      print ILIST_FILE "INDICATOR 1.00 L1Price $prod_ $dep_pred_price_\n" ;
      $num_prices_in_ilist_++;
    }
  }

  foreach my $common_line_ ( @common_indicators_ ) {
    print ILIST_FILE $common_line_."\n" ;
  }

  foreach my $prod_ ( @prodlist_ ) {
    foreach my $template_indicator_ ( @template_indicators_ ) {
      my $template_shc_base = $template_shc_;
      if ( index ($template_shc_base, "NSE_") >= 0 ) {
        $template_shc_base =~ s/_FUT0|_FUT1|_C0_O1|_P0_O1//g;
      }

      my $prod_base = $prod_;
      if ( index ($prod_base, "NSE_") >= 0 ) {
        $prod_base =~ s/_FUT0|_FUT1|_C0_O1|_P0_O1//g;
      }
      print $main_log_file_handle_ "Template $template_shc_ $template_shc_base Prod: $prod_ $prod_base\n"; 

      my $t_ind_ = $template_indicator_ ;
      $t_ind_ =~ s/$template_shc_base/$prod_base/g;
      $t_ind_ = AdjustIndicatorParameters ( $t_ind_, $prod_ ); 
      print  ILIST_FILE $t_ind_."\n" ;
    }
  }
  printf ( ILIST_FILE "INDICATOREND\n" ) ;
}

sub CollectDates 
{
  my $seed_ = int(rand()*100);

  foreach my $prod_ ( @prodlist_ ) {
    my %t_dates_map_ = ();
    if ( defined $prod_to_datagen_days_{ $prod_ } ) { 
      $t_dates_map_{ $_ } = 1 foreach @{ $prod_to_datagen_days_{ $prod_ } };
    }
    if ( defined $prod_to_datagen_days_{ "ALL" } ) {
      $t_dates_map_{ $_ } = 1 foreach @{ $prod_to_datagen_days_{ "ALL" } };
    }
    my @t_dates_vec_ = keys %t_dates_map_;

    my %t_exclude_map_ = ();
    if ( defined $prod_to_datagen_exclude_days_{ $prod_ } ) {
      $t_exclude_map_{ $_ } = 1 foreach @{ $prod_to_datagen_exclude_days_{ $prod_ } };
    }
    if ( defined $prod_to_datagen_exclude_days_{ "ALL" } ) {
      $t_exclude_map_{ $_ } = 1 foreach @{ $prod_to_datagen_exclude_days_{ "ALL" } };
    }
    my @t_exclude_vec_ = keys %t_exclude_map_;

    srand($seed_);
    GenerateDaysVec ( $prod_, $start_date_yyyymmdd_, $end_date_yyyymmdd_, $datagen_start_time_, $datagen_end_time_, \@t_dates_vec_, \@datagen_day_filter_choices_, $datagen_day_filter_start_yyyymmdd_, $datagen_day_filter_max_days_, \@t_exclude_vec_, "datagen", $main_log_file_handle_, "" );
    $prod_to_datagen_days_{ $prod_ } = \@t_dates_vec_;
    print $main_log_file_handle_ "datagen dates for $prod_ : ".join(" ", @t_dates_vec_)."\n";
  }

  my @ttrading_days_ = @trading_days_;
  GenerateDaysVec ( $shortcode_, $trading_start_yyyymmdd_, $trading_end_yyyymmdd_, $trading_start_hhmm_, $trading_end_hhmm_, \@ttrading_days_, [], undef, undef, \@trading_exclude_days_, "trading", $main_log_file_handle_, "" );

  if ( $use_insample_days_for_trading_ == 0 ) {
    foreach my $date_ ( @trading_days_ ) {
      if ( scalar grep { FindItemFromVec ( $date_, @{ $prod_to_datagen_days_{ $_ } } ) } @prodlist_ > ($#prodlist_+1)/2 ) {
        print $main_log_file_handle_ "Skipping $date_ from trading days as it's present in most of the datagen days\n";
      }
      else {
        push ( @trading_days_, $date_ );
      }
    }
  }
  print $main_log_file_handle_ "trading dates: ".join(" ", @trading_days_)."\n";
}

sub GenerateIndividualData 
{
  foreach my $prod_ ( @prodlist_ ) {
    my $datagen_timeout_string_ = $datagen_timeouts_;
    $datagen_timeout_string_ =~ s/ /_/g;
    my $this_common_string_ = join ( "_", $start_date_yyyymmdd_, $end_date_yyyymmdd_, 
        $datagen_start_time_, $datagen_end_time_, $datagen_timeout_string_, $pred_duration_, $predalgo_ );
    $prod_to_reg_data_file_ { $prod_ } = $work_dir_."/reg_data_".$prod_."_".$this_common_string_ ;

    foreach my $tradingdate_ ( @{ $prod_to_datagen_days_{ $prod_ } } ) {
      my $this_day_string_ = join ( "_", $tradingdate_, $datagen_start_time_, $datagen_end_time_, $datagen_timeout_string_ );

      my $model_name_ = $prod_to_ilist_name_ { $prod_ } ;
      my $this_day_out_file_prod_base_ = $work_dir_."/prod_timed_data_".$this_day_string_;
      my $this_day_reg_data_file_base_ = $work_dir_."/prod_reg_data_".$this_day_string_;
      my $datagen_cout_cerr_ = $work_dir_."/datagen_cout_cerr$tradingdate_.txt ";
      `echo "$prod_ " >> $datagen_cout_cerr_`;

      if ( $model_name_ ) {

        my $exec_cmd_ = $INSTALL_BIN_DIR."/datagen $model_name_ $tradingdate_ $datagen_start_time_ $datagen_end_time_ 1 $this_day_out_file_prod_base_ $datagen_timeouts_ ADD_DBG_CODE -1 
          1>>$datagen_cout_cerr_ 2>&1";
        print $main_log_file_handle_ "$exec_cmd_\n";
        `$exec_cmd_`;
        if ( ! ExistsWithSize ( $this_day_out_file_prod_base_ ) ) { 
          print $main_log_file_handle_ " Could not find the timed data..\n"; 
          next ; 
        }

        my $trade_per_sec_file = $work_dir_."/".$prod_."_trade_per_sec_";
        if ( $filter_ eq "fv" ) {
          $apply_trade_vol_filter_ = 1 ;
        }
        elsif ( $filter_ eq "fsudm" || $filter_ eq "fsudm1" ) {
          $fsudm_level_ = 1;
        }
        elsif ( $filter_ eq "fsudm2" ) {
          $fsudm_level_ = 2;
        }

        if ( $apply_trade_vol_filter_ == 1 ) {
          my $exec_cmd="$LIVE_BIN_DIR/daily_trade_aggregator $prod_ $tradingdate_ $trade_per_sec_file";
          print $main_log_file_handle_ "$exec_cmd\n";
          my @daily_trade_agg_output_lines_ = `$exec_cmd`;
          print $main_log_file_handle_ @daily_trade_agg_output_lines_."\n";
        }

        my $this_day_this_prod_timed_data_file_ = $this_day_out_file_prod_base_;
        my $this_prod_reg_data_file_ = $prod_to_reg_data_file_{$prod_};
        my $this_day_this_prod_reg_data_file_ = $this_day_reg_data_file_base_;
        my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( $prod_, $pred_duration_, $predalgo_, $this_day_this_prod_timed_data_file_ );
        $exec_cmd_ = $INSTALL_BIN_DIR."/timed_data_to_reg_data $model_name_ $this_day_this_prod_timed_data_file_ $this_pred_counters_ $predalgo_ $this_day_this_prod_reg_data_file_" ;
        $exec_cmd_ .= $apply_trade_vol_filter_ == 1 ? " $trade_per_sec_file" : " INVALIDFILE";
        $exec_cmd_ .= " 0 $fsudm_level_";
        print $main_log_file_handle_ "$exec_cmd_\n";
        `$exec_cmd_`; 

        $exec_cmd_ = "cat $this_day_this_prod_reg_data_file_ >> $this_prod_reg_data_file_";
        print $main_log_file_handle_ "$exec_cmd_\n"; `$exec_cmd_`;

        if ( $delete_intermediate_files_ ) {
          print $main_log_file_handle_ " rm -rf $this_day_this_prod_reg_data_file_ "."\n";
          print $main_log_file_handle_ "rm -rf $this_day_this_prod_timed_data_file_\n";
          print $main_log_file_handle_ "rm -rf $trade_per_sec_file\n";
          `rm -rf $this_day_this_prod_reg_data_file_`;
          `rm -rf $this_day_this_prod_timed_data_file_` ;
          `rm -rf $trade_per_sec_file`;
        }
      }

      if ( $delete_intermediate_files_ ) {
        print $main_log_file_handle_ " rm -rf $this_day_out_file_prod_base_\n";
        `rm -rf $this_day_out_file_prod_base_`;
      }
    }

#apply filters here
    my $this_filtered_reg_data_filename_  = $prod_to_reg_data_file_ { $prod_ }."_".$filter_;
    my $this_reg_data_filename_ = $prod_to_reg_data_file_{$prod_};
    ApplyFilters ( $prod_, $this_reg_data_filename_, $this_filtered_reg_data_filename_ ) ;
    $prod_to_filtered_data_file_{ $prod_ } = $this_filtered_reg_data_filename_ ;
  }
}

sub GenerateData
{
  my %date_map_ = ( );

  foreach my $prod_ (  @prodlist_ ) {
    my $datagen_timeout_string_ = $datagen_timeouts_;
    $datagen_timeout_string_ =~ s/ /_/g;
    my $this_common_string_ = join ( "_", $start_date_yyyymmdd_, $end_date_yyyymmdd_, 
        $datagen_start_time_, $datagen_end_time_, $datagen_timeout_string_, $pred_duration_, $predalgo_ );
    $prod_to_reg_data_file_ { $prod_ } = $work_dir_."/reg_data_".$prod_."_".$this_common_string_ ;
    $date_map_{ $_ } = 1 foreach @{ $prod_to_datagen_days_{ $prod_ } };
  }
  my @date_vec_ = keys %date_map_;

  foreach my $tradingdate_ ( @date_vec_) {

    my $datagen_timeout_string_ = $datagen_timeouts_;
    $datagen_timeout_string_ =~ s/ /_/g;
    my $this_day_string_ = join ( "_", $tradingdate_, $datagen_start_time_, $datagen_end_time_, $datagen_timeout_string_ );
    my $this_day_out_file_ = $work_dir_."/price_data_$this_day_string_";
    my $this_day_out_file_prod_base_ = $work_dir_."/prod_timed_data_".$this_day_string_;
    my $this_day_reg_data_file_base_ = $work_dir_."/prod_reg_data_".$this_day_string_;
    my $model_name_ = $ilist_filename_ ;
    my $datagen_cout_cerr_ = $work_dir_."/datagen_cout_cerr$tradingdate_.txt ";
    
    if ( $model_name_ ) {

      my $exec_cmd_ = $INSTALL_BIN_DIR."/datagen $model_name_ $tradingdate_ $datagen_start_time_ $datagen_end_time_ 1 $this_day_out_file_ $datagen_timeouts_ ADD_DBG_CODE -1 1>$datagen_cout_cerr_ 2>&1";
      print $main_log_file_handle_ "$exec_cmd_\n";
      `$exec_cmd_`;
      if ( ! ExistsWithSize ( $this_day_out_file_ ) ) { 
        print $main_log_file_handle_ " Could not find the timed data..\n"; 
        next ; 
      }

      my $total_prods_ = $#prodlist_ + 1 ;
      my $num_common_indicators_ = $#common_indicators_ + 1;
      my $num_template_indicators_ = $#template_indicators_ + 1 ;
      $exec_cmd_ = $MODELSCRIPTS_DIR."/separate_data.R $model_name_ $this_day_out_file_ $total_prods_ $num_common_indicators_ $num_template_indicators_ $this_day_out_file_prod_base_ $num_prices_in_ilist_";
      print $main_log_file_handle_ "$exec_cmd_\n";
      `$exec_cmd_`;
      
      if ( $delete_intermediate_files_ ) {
        print $main_log_file_handle_ "rm -rf $this_day_out_file_\n" ; `rm -rf $this_day_out_file_ `;
      }

      for ( my $index_ = 0 ; $index_ <= $#prodlist_; $index_ ++ ) {
        my $td_index_ = $index_ + 1 ;
        my $this_day_this_prod_timed_data_file_ = $this_day_out_file_prod_base_."_".$td_index_;
        my $this_prod_reg_data_file_ = $prod_to_reg_data_file_{$prodlist_[$index_]};
        my $this_day_this_prod_reg_data_file_ = $this_day_reg_data_file_base_."_".$index_;
        my $this_pred_counters_ = GetPredCountersForThisPredAlgo ( $prodlist_[$index_] , $pred_duration_, $predalgo_, $this_day_this_prod_timed_data_file_ );
        $exec_cmd_ = $INSTALL_BIN_DIR."/timed_data_to_reg_data $model_name_ $this_day_this_prod_timed_data_file_ $this_pred_counters_ $predalgo_ $this_day_this_prod_reg_data_file_" ;
        print $main_log_file_handle_ "$exec_cmd_\n";
        `$exec_cmd_`; 
        $exec_cmd_ = "cat $this_day_this_prod_reg_data_file_ >> $this_prod_reg_data_file_";
        print $main_log_file_handle_ "$exec_cmd_\n"; `$exec_cmd_`;

        if ( $delete_intermediate_files_ ) {
          print $main_log_file_handle_ " rm -rf $this_day_this_prod_reg_data_file_ "."\n";
          print $main_log_file_handle_ "rm -rf $this_day_this_prod_timed_data_file_\n";
          `rm -rf $this_day_this_prod_reg_data_file_`;
          `rm -rf $this_day_this_prod_timed_data_file_` ;
        }
      }
      if ( $delete_intermediate_files_ ) {
        print $main_log_file_handle_ " rm -rf $this_day_out_file_\n";
        `rm -rf $this_day_out_file_`;
      }
    }
  }    
#apply filters here

  foreach my $prod_ (  @prodlist_ ) {
    my $this_filtered_reg_data_filename_  = $prod_to_reg_data_file_ { $prod_ }."_".$filter_;
    my $this_reg_data_filename_ = $prod_to_reg_data_file_{$prod_};
    ApplyFilters ( $prod_, $this_reg_data_filename_, $this_filtered_reg_data_filename_ ) ;
    $prod_to_filtered_data_file_{ $prod_ } = $this_filtered_reg_data_filename_ ;
  }
}

sub ApplyFilters 
{
  my ( $prod_, $this_reg_data_filename_, $this_filtered_reg_data_filename_ ) = @_ ;

  my $exec_cmd="$MODELSCRIPTS_DIR/apply_dep_filter.pl $prod_ $this_reg_data_filename_ $filter_ $this_filtered_reg_data_filename_ $trading_end_yyyymmdd_"; 
  print $main_log_file_handle_ "$exec_cmd\n";
  `$exec_cmd`;

  if ( $delete_intermediate_files_ ) {
    print $main_log_file_handle_ " rm -rf $this_reg_data_filename_ \n" ;
#`rm -rf $this_reg_data_filename_`;
  }
}

sub RunRegressionMakeModels 
{
  print $main_log_file_handle_ " RunRegressionMakeModels\n";

  foreach my $prod_ ( @prodlist_ ) 
  {
    my $datagen_timeout_string_ = $datagen_timeouts_;
    $datagen_timeout_string_ =~ s/ /_/g ;
    my $regress_algo_print_string_ = $regress_algo_string_;
    $regress_algo_print_string_ =~ s/ /_/g;
    my $this_common_string_ = join ( "_", $start_date_yyyymmdd_, $end_date_yyyymmdd_, 
        $datagen_start_time_, $datagen_end_time_, $datagen_timeout_string_, 
        $pred_duration_, $predalgo_, $regress_algo_print_string_);

    my $reg_data_file_ = $prod_to_filtered_data_file_{ $prod_};
    my $this_prod_reg_out_filename_ = $work_dir_."/".$prod_."_".$this_common_string_."_reg_out_file" ;
    my $this_prod_model_filename_ = $work_dir_."/w_model_".$prod_."_".$this_common_string_;
    my $this_prod_ilist_filename_ = $work_dir_."/".$prod_."_ilist_filename.txt";
    $prod_to_ilist_filename_ { $prod_} = $this_prod_ilist_filename_ ;
    $prod_to_model_filename_{ $prod_ } = $this_prod_model_filename_ ;

    my $avoid_sharpe_check_file_ = $work_dir_."/avoid_sharpe_check_file_.txt";
    `>$avoid_sharpe_check_file_`;

    my @regress_algo_words_ = split ( ' ', $regress_algo_string_ ) ;

    given ( $regress_algo_words_[0] ) { 
      when ( "L1_REGRESSION")
      {
        print $main_log_file_handle_ "L1_REGRESSION\n";
        my $tvalue_cutoff_l1_reg_ = 5;
        my $tvalue_change_ratio_ = .5;
        if ( $#regress_algo_words_ >= 1) {
          my $tvalue_cutoff_l1_reg_ = $regress_algo_words_[0];
          my $tvalue_change_ratio_ = $regress_algo_words_[1];
        }
        my $exec_cmd= "$MODELSCRIPTS_DIR/lad_regression.R -i $reg_data_file_ -o $this_prod_reg_out_filename_ "." -c $tvalue_cutoff_l1_reg_ -r $tvalue_change_ratio_ ";
        print $main_log_file_handle_ "$exec_cmd \n ";
        `$exec_cmd`;
        if ( ExistsWithSize ( $this_prod_reg_out_filename_ ) )
        {
          my $exec_cmd_="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_ " ;
          print $main_log_file_handle_ "$exec_cmd_";
          `$exec_cmd_ `;
        }
        else
        {
          print $main_log_file_handle_ "Error Missing RegDataFile $this_prod_reg_out_filename_ \n";
        }

      }
      when ( "FSLR" ) 
      {
        print $main_log_file_handle_ "FSLR\n";
        my $max_model_size_ = 20 ;
        if ( $#regress_algo_words_ > 4 )  
        { 
          $max_model_size_ = $regress_algo_words_[5] ; 
        }
        my $this_string_ = join ( " ", @regress_algo_words_[1..4] ) ;
        my $exec_cmd_ = "$INSTALL_BIN_DIR/callFSLR $reg_data_file_ ".$this_string_." ".$this_prod_reg_out_filename_. " ".$max_model_size_." ".$avoid_sharpe_check_file_." ".$this_prod_ilist_filename_." N";
        print $main_log_file_handle_ "$exec_cmd_\n";
        `$exec_cmd_`;
        if ( ExistsWithSize ( $this_prod_reg_out_filename_ ) )
        {
          my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_ ";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
        }
        else
        {
          print $main_log_file_handle_ "ERROR missing $this_prod_reg_out_filename_\n";
        }
      }
      when ( "SIGLR" ) 
      {
        print $main_log_file_handle_ "SIGLR\n";
        if ( $#regress_algo_words_ >= 3 ) 
        {
          my $max_model_size_ = $regress_algo_words_[1] ;
          my $domination_penalty_ = $regress_algo_words_[2] ;
          my $penalty_ = $regress_algo_words_[3] ;
          my $p_norm_ = 0.5;
          if ( $penalty_ eq "LP" ) 
          {
            if ( $#regress_algo_words_ > 3 ) 
            {
              $p_norm_ = $regress_algo_words_[4] ;
            }
          }

          my $exec_cmd= "$MODELSCRIPTS_DIR/SIGLR_grad_descent_2.R $reg_data_file_ $this_prod_reg_out_filename_ $max_model_size_ $domination_penalty_ $penalty_ ";
          if ( $penalty_ eq "LP" )
          {
            $exec_cmd= "$MODELSCRIPTS_DIR/SIGLR_grad_descent_2.R $reg_data_file_ $this_prod_reg_out_filename_  $max_model_size_ $domination_penalty_ $penalty_ $p_norm_ ";
          }
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
          if ( ExistsWithSize  ( $this_prod_reg_out_filename_ ) )
          {
            my $exec_cmd_ = $MODELSCRIPTS_DIR."/place_coeffs_in_siglr_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_ ";
            print $main_log_file_handle_ "$exec_cmd_\n";
            `$exec_cmd_`;
          }
          else
          {
            print $main_log_file_handle_ "Error Missing regression out file $this_prod_reg_out_filename_\n"; 
          }
        }
      } 
      when ( "FSHLR" )
      {
        print $main_log_file_handle_ " Regress algo FSHLR \n";
        my $max_model_size_ = 20 ;
        if ( $#regress_algo_words_ > 4 )  
        { 
          $max_model_size_ = $regress_algo_words_[5] ; 
        }
        my $this_string_ = join ( " ", @regress_algo_words_[1..4] ) ;
        my $exec_cmd_ = "$INSTALL_BIN_DIR/callFSHLR $reg_data_file_ ".$this_string_." ".$this_prod_reg_out_filename_. " ".$max_model_size_." ".$avoid_sharpe_check_file_;
        print $main_log_file_handle_ "$exec_cmd_\n";
        `$exec_cmd_`;
        if ( ExistsWithSize ( $this_prod_reg_out_filename_ ) )
        {
          my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_ ";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
        }
        else
        {
          print $main_log_file_handle_ "ERROR missing $this_prod_reg_out_filename_\n";
        }
      }
      when ("FSVLR" ) 
      {
        print $main_log_file_handle_ " Regress algo FSVLR \n";
        my $max_model_size_ = 20 ;
        if ( $#regress_algo_words_ > 5 )  
        { 
          $max_model_size_ = $regress_algo_words_[6] ; 
        }
        my $this_string_ = join ( " ", @regress_algo_words_[1..5] ) ;
        my $exec_cmd_ = "$INSTALL_BIN_DIR/callFSVLR $reg_data_file_ ".$this_string_." ".$this_prod_reg_out_filename_. " ".$max_model_size_." ".$avoid_sharpe_check_file_;
        print $main_log_file_handle_ "$exec_cmd_\n";
        `$exec_cmd_`;
        if ( ExistsWithSize ( $this_prod_reg_out_filename_ ) )
        {
          my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_ ";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
        }
        else
        {
          print $main_log_file_handle_ "ERROR missing $this_prod_reg_out_filename_\n";
        }
      }
      when ("FSHDVLR")
      {
        print $main_log_file_handle_ " Regress algo FSHDVLR \n";
        my $max_model_size_ = 20 ;
        if ( $#regress_algo_words_ > 4 )  
        { 
          $max_model_size_ = $regress_algo_words_[5] ; 
        }
        my $this_string_ = join ( " ", @regress_algo_words_[1..4] ) ;
        my $exec_cmd_ = "$INSTALL_BIN_DIR/callFSHDVLR $reg_data_file_ ".$this_string_." ".$this_prod_reg_out_filename_. " ".$max_model_size_." ".$avoid_sharpe_check_file_;
        print $main_log_file_handle_ "$exec_cmd_\n";
        `$exec_cmd_`;
        if ( ExistsWithSize ( $this_prod_reg_out_filename_ ) )
        {
          my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_ ";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
        }
        else
        {
          print $main_log_file_handle_ "ERROR missing $this_prod_reg_out_filename_\n";
        }
      }
      when ( "FSRR" ) 
      {
        print $main_log_file_handle_ " Regress algo FSRR\n";
        my $max_model_size_ = 20 ;
        if ( $#regress_algo_words_ > 5 )  
        { 
          $max_model_size_ = $regress_algo_words_[6] ; 
        }
        my $this_string_ = join ( " ", @regress_algo_words_[1..5] ) ;
        my $exec_cmd_ = "$INSTALL_BIN_DIR/callFSRR $reg_data_file_ ".$this_string_." ".$this_prod_reg_out_filename_. " ".$max_model_size_." ".$avoid_sharpe_check_file_;
        print $main_log_file_handle_ "$exec_cmd_\n";
        `$exec_cmd_`;
        if ( ExistsWithSize ( $this_prod_reg_out_filename_ ) )
        {
          my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_ ";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
        }
        else
        {
          print $main_log_file_handle_ "ERROR missing $this_prod_reg_out_filename_\n";
        }
      }
      when ( "FSLRM" ) 
      {
        print $main_log_file_handle_ " Regress algo FSRR\n";
        my $max_model_size_ = 20 ;
        if ( $#regress_algo_words_ > 7 )  
        { 
          $max_model_size_ = $regress_algo_words_[7] ; 
        }
        my $this_string_ = join ( " ", @regress_algo_words_[1..5] ) ;
        my @high_sharpe_indep_index_ = GetIndexOfHighSharpeIndep ( $this_prod_ilist_filename_, $avoid_sharpe_check_file_ );
        print $main_log_file_handle_ "high_sharpe_indep_index_: @high_sharpe_indep_index_";

        my $exec_cmd_ = "$INSTALL_BIN_DIR/callFSRR $reg_data_file_ ".$this_string_." ".$this_prod_reg_out_filename_. " ".$max_model_size_." ".$avoid_sharpe_check_file_;
        print $main_log_file_handle_ "$exec_cmd_\n";
        `$exec_cmd_`;
        if ( ExistsWithSize ( $this_prod_reg_out_filename_ ) )
        {
          my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_ ";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
        }
        else
        {
          print $main_log_file_handle_ "ERROR missing $this_prod_reg_out_filename_\n";
        }
      }
      when ( "LM")
      {
        print $main_log_file_handle_ "Regress algo LM \n";
        my $max_model_size_ = 20 ;
        my $exec_cmd_ = "$MODELSCRIPTS_DIR/build_linear_model.R  $reg_data_file_ $this_prod_reg_out_filename_ ";
        print $main_log_file_handle_  "$exec_cmd_ \n"; `$exec_cmd_`;
        if ( ExistsWithSize ( $this_prod_reg_out_filename_) )
        {
          my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_lasso_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
        }
        else
        {
          print $main_log_file_handle_ "Errod mising $this_prod_reg_out_filename_\n";
        }
      }
      when ( "LR" )
      {
        my $lasso_or_ridge_ = 1 ;
        if ( $#regress_algo_words_ >= 1 )
        {
          $lasso_or_ridge_ = $regress_algo_words_[1];
        }
        my $max_model_size_ = 30 ;
        my $exec_cmd_ = "$MODELSCRIPTS_DIR/build_unconstrained_lasso_model.R $reg_data_file_ $lasso_or_ridge_ $this_prod_reg_out_filename_ ";
        print $main_log_file_handle_ "$exec_cmd_\n";  `$exec_cmd_`;
        if ( ExistsWithSize( $this_prod_reg_out_filename_ ) ) 
        {
          my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_lasso_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
        }
        else 
        {
          print $main_log_file_handle_ " Error missing $this_prod_reg_out_filename_ \n";
        }
      }
      when ("BOOSTING" )
      {
        print $main_log_file_handle_ " Regress Algo Boosting\n";
        my $reg_coeff_ = $regress_algo_words_[1] ;
        my $loss_function_ = $regress_algo_words_[2] ;
        my $max_model_size_ = $regress_algo_words_[3] ;
        my $exec_cmd_ = "$MODELSCRIPTS_DIR/adaboost.py BOOSTING $reg_data_file_ $reg_coeff_ $loss_function_ $max_model_size_ $this_prod_ilist_filename_  > $this_prod_reg_out_filename_";
        print $main_log_file_handle_ "$exec_cmd_\n";`$exec_cmd_`;
        if ( ExistsWithSize( $this_prod_reg_out_filename_ ) ) 
        {
          my $exec_cmd="cp $this_prod_reg_out_filename_ $this_prod_model_filename_";
          print $main_log_file_handle_ "$exec_cmd\n";
          `$exec_cmd`;
        }
      }
      when ("TREEBOOSTING" )
      {
        print $main_log_file_handle_ " Regress Algo Tree Boosting\n";
        if ( $#regress_algo_words_ >= 3 )
        {
          my $reg_coeff_ = $regress_algo_words_[1] ;
          my $loss_function_ = $regress_algo_words_[2] ;
          my $max_model_size_ = $regress_algo_words_[3] ;
          my $exec_cmd_ = "$MODELSCRIPTS_DIR/adaboost.py TREEBOOSTING $reg_data_file_ $reg_coeff_ $loss_function_ $max_model_size_ $this_prod_ilist_filename_  > $this_prod_reg_out_filename_";
          print $main_log_file_handle_ "$exec_cmd_\n";`$exec_cmd_`;
          if ( ExistsWithSize( $this_prod_reg_out_filename_ ) ) 
          {
            my $exec_cmd="cp $this_prod_reg_out_filename_ $this_prod_model_filename_";
            print $main_log_file_handle_ "$exec_cmd\n";
            `$exec_cmd`;
          }
        }
      }
      when ("RANDOMFOREST" )
      {
        print $main_log_file_handle_ "Regress Algo Randomforest\n";
        if ( $#regress_algo_words_ >= 4 ) 
        {
          my $max_model_size_ = $regress_algo_words_[1];
          my $num_iters_ = $regress_algo_words_[2];
          my $num_trees_ = $regress_algo_words_[3];
          my $max_nodes_ = $regress_algo_words_[4];
          my $num_quantiles_ = 9;
          if ( $#regress_algo_words_ >= 5 ) 
          {
            $num_quantiles_ = $regress_algo_words_[5];
          }
          my $exec_cmd_ = "$MODELSCRIPTS_DIR/randomForest_model.R $reg_data_file_ $this_prod_reg_out_filename_ $max_model_size_ $num_iters_ $num_trees_ $max_nodes_ $num_quantiles_ ";
          print $main_log_file_handle_ "$exec_cmd_\n";
          `$exec_cmd_`;
          if ( ExistsWithSize( $this_prod_reg_out_filename_ ) ) 
          {
            my $exec_cmd="$MODELSCRIPTS_DIR/place_coeffs_in_random_forest_model.pl $this_prod_model_filename_ $this_prod_ilist_filename_ $this_prod_reg_out_filename_";
            print $main_log_file_handle_ "$exec_cmd_\n";
            `$exec_cmd`;
          }
          else 
          {
            print $main_log_file_handle_ "ERROR COuld not generate random forest model for ilist $this_prod_ilist_filename_\n";
          }
        }
      }
    }

    if ( IsModelScalable($this_prod_model_filename_) && ( $add_modelinfo_to_model_ == 1 ) ) {
      my @t_l1norm_vec_ = ();
      GetModelL1NormVec($this_prod_model_filename_, "TODAY-80", "TODAY-1", $trading_start_hhmm_, $trading_end_hhmm_, \@t_l1norm_vec_);
      AddL1NormToModel($this_prod_model_filename_, \@t_l1norm_vec_);
    }

    if ( $delete_intermediate_files_ ) {
      print $main_log_file_handle_  " rm -rf $reg_data_file_ \n" ;
      `rm -rf $reg_data_file_ `;
    }
    if ( ExistsWithSize ( $this_prod_model_filename_ ) ) {
      $idx_to_shc_map_ { $unique_shc_ } = $prod_ ;
      $unique_shc_ ++ ;
    }
  }
}

sub GetParamFilenames 
{
  if ( $paramname_ ) {
    push ( @{$prod_to_param_filename_{$_}}, $paramname_ ) foreach @prodlist_;
  }

  if ( $paramlistfilename_ )
  {
    open PARAMLIST, "< $paramlistfilename_" or PrintStacktraceAndDie ( " Could not open file $paramlistfilename_ for reading data.. \n" );
    my @paramlistlines_ = <PARAMLIST>; chomp ( @paramlistlines_ );
    close PARAMLIST ;
    
    foreach my $line_  ( @paramlistlines_ ) {
      my @paramlistline_words_ = split ( " ", $line_ );
      next if ( $#paramlistline_words_ < 1 );
      
      if ( FindItemFromVec ( $paramlistline_words_[0], @prodlist_ ) ) {
        print $main_log_file_handle_ "Adding param : ".$paramlistline_words_[1]. " for product: " . $paramlistline_words_[0]."\n";
        push ( @{$prod_to_param_filename_{$paramlistline_words_[0]} }, $paramlistline_words_[1] ) ;
      }
    }
  }

  foreach my $key_ ( keys %prod_to_param_filename_ ) 
  {
    print $main_log_file_handle_ " NumParam:  $num_params_ . This prod params : @{$prod_to_param_filename_{$key_ } }   \n";
    if ( $num_params_ < @{$prod_to_param_filename_{$key_ } } ) {
      $num_params_ = @{$prod_to_param_filename_{$key_ }};
    }
  }
  print $main_log_file_handle_ " NumParam:  $num_params_ . This prod params :  \n";

# fill the rest of paramvecs with their last values till this lenghte
  foreach my $key_ ( keys %prod_to_param_filename_ ) {
    while ( $num_params_ - @{$prod_to_param_filename_{$key_}} > 0 ) {
      my $len_ = @{$prod_to_param_filename_{$key_}} -1 ;
      push ( @{$prod_to_param_filename_{$key_}}, $prod_to_param_filename_{$key_}[$len_] ) ;
    }
    print $main_log_file_handle_ " NumParamL:  $num_params_ . This prod params : @{$prod_to_param_filename_{$key_ } }   \n";
  }
  $num_params_--;
}

sub MakeStrategyFiles 
{
  my $strategy_progid_ = 1001;
  my $ind_ = 0 ;

  my $datagen_timeout_string_ = $datagen_timeouts_;
  $datagen_timeout_string_ =~ s/ /_/g ;
  my $regress_algo_print_string_ = $regress_algo_string_;
  $regress_algo_print_string_ =~ s/ /_/g;

  my $this_common_string_ = join ( "_", $start_date_yyyymmdd_, $end_date_yyyymmdd_, 
      $datagen_start_time_, $datagen_end_time_, $datagen_timeout_string_, 
      $pred_duration_, $predalgo_, $regress_algo_print_string_);

  my $stir_strat_name_common_string_ = "_".$this_common_string_; 
  if ( $add_prod_name_in_strat_name_ != 0 ) {
    foreach my $key_ ( keys %idx_to_shc_map_ ) {
      $stir_strat_name_common_string_ = $stir_strat_name_common_string_."_".$idx_to_shc_map_{$key_};
      $ind_++ ;
      if ( $ind_ > 20 ) { last ; } 
    }
  }
  else {
    $ind_++ foreach keys %idx_to_shc_map_;
    $stir_strat_name_common_string_ = $stir_strat_name_common_string_."_".$ind_;
  }

  for (my $idx_ = 0; $idx_ <= $num_params_ ; $idx_++)  # just one strat created
  { 
    my $cmbname_ = basename ( $common_param_filename_ ) ; chomp ( $cmbname_ ) ;
    my $this_strategy_im_filebase_ = "w_strat_im_".$name_.$stir_strat_name_common_string_."_".$cmbname_."_".$idx_ ;
    my $this_strategy_im_filename_ = $local_strats_dir_."/".$this_strategy_im_filebase_;

    my $this_strategy_filebase_ = "w_strat_".$name_.$stir_strat_name_common_string_."_".$cmbname_."_".$idx_;
    my $this_strategy_filename_ = $local_strats_dir_."/".$this_strategy_filebase_;

    open my $im_file_handle_, ">", $this_strategy_im_filename_ or PrintStacktraceAndDie ( "Could not open $this_strategy_im_filename_ for writing\n" );
    print $im_file_handle_ "STRUCTURED_TRADING ".$shortcode_." ".$strategy_name_." ".$common_param_filename_." ".$trading_start_hhmm_." ".$trading_end_hhmm_." ".$strategy_progid_."\n";

    if ( $unique_shc_ <= 0 ) {
      print $main_log_file_handle_ " No modelfile created for any of the shortcodes. Exiting ";
      exit ( 1 ) ;
    }

    for (my $shc_idx_ = 0;$shc_idx_ < $unique_shc_ ; $shc_idx_ ++) {
      print $im_file_handle_ "STRATEGYLINE ".$idx_to_shc_map_{$shc_idx_}." ".$prod_to_model_filename_{$idx_to_shc_map_{$shc_idx_}}." ".$prod_to_param_filename_{$idx_to_shc_map_{$shc_idx_}}[$idx_]."\n";
    }
    close($im_file_handle_);

    my $exec_cmd_ = "echo STRUCTURED_STRATEGYLINE $this_strategy_im_filename_ $strategy_progid_ > $this_strategy_filename_";
    `$exec_cmd_`;
    if ( ExistsWithSize (  $this_strategy_filename_ ) ) {
      push ( @strategy_filevec_, $this_strategy_filename_ ); 
    }
    ++$strategy_progid_;
  }
}


sub RunSimulationOnCandidates
{
  print $main_log_file_handle_ "\n\n RunSimulationOnCandidates\n\n";

  my @non_unique_results_filevec_ = ( );

  my @unique_sim_id_list_ = ( );
  my @independent_parallel_commands_ = ( );
  my @tradingdate_list_ = ( );
  my @temp_strategy_list_file_index_list_ = ( );
  my @temp_strategy_list_file_list_ = ( );
  my @temp_strategy_cat_file_list_ = ( );
  my @temp_strategy_output_file_list_ = ( );

# generate a list of commands which are unique , 
# independent from each other and can be safely run in parallel.

  foreach my $tradingdate_ ( @trading_days_ ) {
    my $temp_strategy_list_file_index_ = 0;

    for ( my $strategy_filevec_index_ = 0 ; $strategy_filevec_index_ <= $#strategy_filevec_ ; ) {
      my $temp_strategy_list_file_ = $work_dir_."/temp_strategy_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
      my $temp_strategy_cat_file_ = $work_dir_."/temp_strategy_cat_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";
      my $temp_strategy_output_file_ = $work_dir_."/temp_strategy_output_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt";

      open ( TSLF , ">" , $temp_strategy_list_file_ ) or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_file_\n" );
      for ( my $num_files_ = 0 ; $num_files_ < $MAX_STRAT_FILES_IN_ONE_SIM && $strategy_filevec_index_ <= $#strategy_filevec_ ; $num_files_ ++ ) {
        my $this_strategy_filename_ = $strategy_filevec_ [ $strategy_filevec_index_ ];
        $strategy_filevec_index_ ++ ;
        print TSLF $this_strategy_filename_."\n";
        `cat $this_strategy_filename_ >> $temp_strategy_cat_file_`;
      }
      close ( TSLF );

      my $unique_sim_id_ = GetGlobalUniqueId ( );
      my $market_model_index_ = GetMarketModelForShortcode ( $shortcode_ );
      my $exec_cmd_ = $LIVE_BIN_DIR."/sim_strategy SIM ".$temp_strategy_cat_file_." ".$unique_sim_id_." ".$tradingdate_." ".$market_model_index_." ADD_DBG_CODE -1 > ".$temp_strategy_output_file_." 2>/dev/null";
      print $main_log_file_handle_ "$exec_cmd_\n";

      push ( @unique_sim_id_list_ , $unique_sim_id_ );
      push ( @independent_parallel_commands_ , $exec_cmd_ );

      push ( @tradingdate_list_ ,$tradingdate_ );
      push ( @temp_strategy_list_file_index_list_ , $temp_strategy_list_file_index_ ); $temp_strategy_list_file_index_ ++;

      push ( @temp_strategy_list_file_list_ , $temp_strategy_list_file_ );
      push ( @temp_strategy_cat_file_list_ , $temp_strategy_cat_file_ );
      push ( @temp_strategy_output_file_list_ , $temp_strategy_output_file_ );
    }
  }

# process the list of commands , processing MAX_CORES_TO_USE_IN_PARALLEL at once
  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; ) {
    my @output_files_to_poll_this_run_ = ( );

    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#independent_parallel_commands_ ; $num_parallel_ ++ ) {
      push ( @output_files_to_poll_this_run_ , $temp_strategy_output_file_list_ [ $command_index_ ] );

      my $exec_cmd_ = "> ".$temp_strategy_output_file_list_ [ $command_index_ ];
      `$exec_cmd_`;

      print $main_log_file_handle_ $independent_parallel_commands_ [ $command_index_ ]."\n";
      $exec_cmd_ = $independent_parallel_commands_ [ $command_index_ ]." &";
      `$exec_cmd_`;

      $command_index_ ++;
      sleep ( 1 );
    }
    while ( ! AllOutputFilesPopulatedLocal ( @output_files_to_poll_this_run_ ) )
    { # there are still some sim-strats which haven't output SIMRESULT lines
      my $result_ = AllOutputFilesPopulatedLocal ( @output_files_to_poll_this_run_ );	
      sleep ( 1 );
    }
  }

  for ( my $command_index_ = 0 ; $command_index_ <= $#independent_parallel_commands_ ; $command_index_ ++ )
  {
    my %unique_id_to_pnlstats_map_ = ( );

    my $unique_sim_id_ = $unique_sim_id_list_ [ $command_index_ ];
    my $tradingdate_ = $tradingdate_list_ [ $command_index_ ];

    my $temp_strategy_cat_file_ = $temp_strategy_cat_file_list_ [ $command_index_ ];
    my $temp_strategy_list_file_ = $temp_strategy_list_file_list_ [ $command_index_ ];
    my $exec_cmd_ = "cat ".$temp_strategy_cat_file_;
    my $strategy_cat_file_lines_ = `$exec_cmd_`; chomp($strategy_cat_file_lines_);
    my @strategy_cat_file_line_vec_ = split('\n', $strategy_cat_file_lines_);


    my $temp_strategy_output_file_ = $temp_strategy_output_file_list_ [ $command_index_ ];
    $exec_cmd_ = "cat ".$temp_strategy_output_file_;
    my $tradeinit_output_lines_ = `$exec_cmd_`; chomp($tradeinit_output_lines_);
    my @tradeinit_output_line_vec_ = split('\n', $tradeinit_output_lines_);
    
    my $this_trades_filename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".$unique_sim_id_;
    my @pnlstats_output_line_vec_ = ();
    if ( ExistsWithSize ( $this_trades_filename_ ) ) {
      my $exec_cmd_ = $MODELSCRIPTS_DIR."/get_pnl_stats_stir_2.pl $this_trades_filename_";
      print $main_log_file_handle_ $exec_cmd_."\n";
      my $pnlstats_output_lines_ = `$exec_cmd_`; chomp ( $pnlstats_output_lines_ );
      @pnlstats_output_line_vec_ = split('\n', $pnlstats_output_lines_);
    }

    if ( $delete_intermediate_files_ ) {
      my $this_tradeslogfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int ( $unique_sim_id_ );
      `rm -f $this_tradeslogfilename_`;
    }

#to check if pnlstats_output_line_vec_, tradeinit_output_line_vec_, strategy_cat_file_line_vec_ have same size

    if ( ($#tradeinit_output_line_vec_ != $#pnlstats_output_line_vec_) || ($#pnlstats_output_line_vec_ != $#strategy_cat_file_line_vec_) ) {
      print $main_log_file_handle_ "somehow pnlstats_output_line_vec_, tradeinit_output_line_vec_, strategy_cat_file_line_vec_ don't have same size,\n";
      print $main_log_file_handle_ "check files $temp_strategy_output_file_, $temp_strategy_cat_file_, $this_trades_filename_\n";
      next;
    }


    my $temp_results_list_file_ = `echo $temp_strategy_output_file_ | sed 's\/temp_strategy_output_file_\/temp_results_list_file_\/g'`; chomp($temp_results_list_file_);
    open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );

    for (my $t_strat_idx_ = 0 ; $t_strat_idx_ <= $#tradeinit_output_line_vec_ ; $t_strat_idx_++ ) {

      my $strategy_cat_file_line_ = $strategy_cat_file_line_vec_[$t_strat_idx_]; chomp($strategy_cat_file_line_);
      my @strategy_cat_file_line_words_ = split(' ', $strategy_cat_file_line_);
      my $unique_strat_id_ = 	$strategy_cat_file_line_words_[2];

      my $pnlstats_output_line_ = $pnlstats_output_line_vec_[$t_strat_idx_]; chomp($pnlstats_output_line_);
      my $tradeinit_output_line_ = $tradeinit_output_line_vec_[$t_strat_idx_]; chomp($tradeinit_output_line_);

      if ( $tradeinit_output_line_ =~ /SIMRESULT/ ) {
# SIMRESULT pnl volume S B A 
        my @rwords_ = split ( ' ', $tradeinit_output_line_ );
        splice ( @rwords_, 0, 1 ); 
        my $remaining_simresult_line_ = join ( ' ', @rwords_ );
        
        if ( ( $rwords_[1] > 0 ) || # volume > 0                                                                                                                                                  
            ( ( $shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all
        {
          printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $pnlstats_output_line_, $unique_strat_id_ ;
          printf TRLF "%s %s %s\n",$remaining_simresult_line_,$pnlstats_output_line_, $unique_strat_id_;
        }
      }
    }

    close TRLF;		

    if ( ExistsWithSize ( $temp_results_list_file_ ) ) {
      my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_local_database.pl $temp_strategy_list_file_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir";
      print $main_log_file_handle_ "$exec_cmd\n";
      my $this_local_results_database_file_ = `$exec_cmd`;
      push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
    }
  }

  @unique_results_filevec_ = GetUniqueList ( @non_unique_results_filevec_ );
}

sub GetPriceForShortcode {
  my $shortcode = shift;
  my $date = $start_date_yyyymmdd_;
  if ( index ( $shortcode, "NSE_") < 0 ) { return 1.0;}
  my $exec_cmd_ = "$INSTALL_BIN_DIR/get_contract_specs $shortcode $date LAST_CLOSE_PRICE | awk '{print \$2}'";
  my $price = `$exec_cmd_`; chomp ( $price);
  return $price;
}

sub AdjustIndicatorParameters 
{
  my $this_indicator_string_ = shift ;
  my $dependant_  = shift ;
  my @words_ = split ( " ", $this_indicator_string_ )  ;
  
  if ( $#words_>= 3 ) {
    for ( my $ind_ = 3; $ind_ <= $#words_;$ind_++ ) {
      
      if ( index ( $words_[$ind_], "L1Size" ) >= 0 ) {
        my @words_words_ = split ( /\*/, $words_[$ind_] ); chomp (@words_words_);
      
        if  ( $#words_words_  >= 1 ) {
          my $date_ = `date +%Y%m%d`; chomp ( $date_ );
          $date_ = $start_date_yyyymmdd_ if $start_date_yyyymmdd_ ne "";
         
          my $l1_size_ =  int ( GetFeatureAverageDays ( $dependant_, $start_date_yyyymmdd_ , 30, "L1SZ" ) ) ;
          if ( $words_words_[0] eq "Price") {
            $words_words_[0] = GetPriceForShortcode($dependant_);
          }
          print $main_log_file_handle_ "\n".$words_[$ind_]. " ".@words_words_." $words_[3] retVal: $l1_size_ fact: $words_words_[0]\n";
          $words_[$ind_] = $words_words_[0] * $l1_size_ ;
        }
      }
      
      if ( index ( $words_[$ind_], "L1Order" ) >= 0 ) {
        my @words_words_ = split ( /\*/, $words_[$ind_] ); chomp (@words_words_);
      
        if  ( $#words_words_  >= 1 ) {
          my $date_ = `date +%Y%m%d`; chomp ( $date_ );
          $date_ = $start_date_yyyymmdd_ if $start_date_yyyymmdd_ ne "";
        
          my $l1_size_ =  int ( GetFeatureAverageDays ( $dependant_, $start_date_yyyymmdd_ , 30, "ORDSZ" ) ) ;

          if ( $words_words_[0] eq "Price") {
            $words_words_[0] = GetPriceForShortcode($dependant_);
          }
          print $main_log_file_handle_ "\n".$words_[$ind_]. " ".@words_words_." $words_[3] retVal: $l1_size_ fact: $words_words_[0]\n";
          $words_[$ind_] = $words_words_[0] * $l1_size_ ;
        }
      }

      if ( index ( $words_[$ind_], "SECTORPORT") >= 0 ) {
        my @words_words_ = split ( /\*/, $words_[$ind_] );

        if ( $#words_words_  >= 0 ) {
          my @portfolios_ = GetSectorPortForStocks ( $dependant_, 0 );
          
          if ( $#portfolios_ >= 0 ) {
            print $main_log_file_handle_ "\n".$words_[$ind_]. " ".@words_words_." $words_[3] , RetVal: $portfolios_[0]  \n"; 
            $words_[$ind_ ] = $portfolios_[0]; 
          }
          else {
            print $main_log_file_handle_ "Could not find sectir portfolio for this stock.going with DeFAULT BREQ3";
            $words_[$ind_] = "BREQ4";
          }
        }
      }

      elsif ( index ( $words_ [$ind_], "SECTORFRAC") >= 0 ) {
        my @words_words_ = split ( /\*/, $words_[$ind_] ) ;
        
        if ( $#words_words_  >= 0 ) {
          my @portfolios_ = GetSectorPortForStocks ( $dependant_, 1 );
          
          if ( $#portfolios_ >= 0 ) { 
            print $main_log_file_handle_ "\n".$words_[$ind_]. " ".@words_words_." $words_[3] , RetVal: $portfolios_[0]  \n"; 
            $words_[$ind_ ] = $portfolios_[0]; 
          }
          else {
            print $main_log_file_handle_ "Could not find sectir portfolio for this stock.going with DeFAULT BREQ3";
            $words_[$ind_] = "BREQ3";
          }
        }
      }

      elsif ( index ( $words_ [$ind_] , "NEXTMAJORSTOCK" ) >= 0 ) {
        my @words_words_ = split ( /\*/, $words_[$ind_] ) ; 
        
        if ( $#words_words_ >= 0 ) {
          my @stock_list_ = GetNextMajorStockInSector ( $dependant_) ;
          
          if ( $#stock_list_ >= 0 ) {
            print $main_log_file_handle_ "\n".$words_[$ind_]." ".@words_words_."$words_[3], RETVAL: $stock_list_[0] \n ";
            $words_[ $ind_ ] = $stock_list_ [0];
          }
          else {
            print $main_log_file_handle_ " Could not find next major stock for $dependant_\n";
            $words_[$ind_] = "PETR4";
          }
        }
      }
    }
    $this_indicator_string_ = join ( " ", @words_  ) ;
  }
  return $this_indicator_string_ ;
}

sub AllOutputFilesPopulatedLocal 
{
  print $main_log_file_handle_ " AllOutputFilesPopulated \n";  
  my ( @output_files_to_poll_this_run_ ) = @_;

  foreach my $outfile_ ( @output_files_to_poll_this_run_ ) {
    my $exec_cmd_ = "grep SIMRESULT $outfile_ 2>/dev/null | wc -l 2>/dev/null";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );
    return 0 if $#exec_cmd_output_ < 0;

    my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ 0 ] );
    return 0 if $#exec_cmd_output_words_ < 0;

    my $simresult_count_ = $exec_cmd_output_words_[ 0 ];
    return 0 if $simresult_count_ <= 0;
  }
  return 1;
}

sub SummarizeLocalResultsAndChoose
{
  print $main_log_file_handle_ "SummarizeLocalResultsAndChoose\n";
  
  my $exec_cmd_ =  $LIVE_BIN_DIR."/summarize_local_results_dir_and_choose_by_algo ".$sort_algo_." 1 $num_files_to_choose_ -1.85 10000 5000 -1 ".$local_results_base_dir." 2>/dev/null ";
  print $main_log_file_handle_ $exec_cmd_."\n";
  my @results = `$exec_cmd_`; chomp(@results);

  my $num_strats_in_global_results_=0;
  my @strat_files_selected_ = ();
  my %strat_indices_ = ();

  my @small_results_ = grep { index($_, "STRATEGYFILEBASE") >= 0 || index($_, "STATISTICS") >= 0 } @results;

  if ( ! ( -d $MODELING_STIR_STRATS_DIR."/".$shortcode_) ) {
    my $ec_ = "mkdir -p $MODELING_STIR_STRATS_DIR/$shortcode_" ; 
    print $main_log_file_handle_ "$ec_\n";
    `$ec_`;
  }

  if ( ! ( -d $MODELING_STRATS_DIR."/".$shortcode_) ) {
    my $ec_ = "mkdir -p $MODELING_STRATS_DIR/$shortcode_";
    print $main_log_file_handle_ "$ec_\n";
    `$ec_`;
  }

  if ($use_median_cutoff_) {
#adding global result check
    print $main_log_file_handle_ "Using Median Cutoff\n";
    my $global_results_dir_path = "/NAS1/ec2_globalresults/$shortcode_/";
    
    my $srv_name_=`hostname | cut -d'-' -f2`; chomp($srv_name_);
    $global_results_dir_path = $HOME_DIR."/ec2_globalresults/$shortcode_/" if $srv_name_ =~ "crt";

    (my $localresults_dir_path = $unique_results_filevec_[0]) =~ s/201[0-9]\/.*//;
    
    my @global_results_files = @unique_results_filevec_;
    foreach my $fl_(@global_results_files) {
      my $global_fl_ = $fl_; $global_fl_ =~ s/$localresults_dir_path/$global_results_dir_path/g;
      if ( ! -e $global_fl_ ) { next;  }
      print $main_log_file_handle_ "cat $global_fl_ >> $fl_\n";
      `cat $global_fl_ >> $fl_`;
    }

    my $timeperiod_ = "$trading_start_hhmm_-$trading_end_hhmm_";
    print $main_log_file_handle_ "TimePeriod: $timeperiod_\n";

    my $cstempfile_ = GetCSTempFileName ( $work_dir_."/cstemp" );
    open CSTF, "> $cstempfile_" or PrintStacktraceAndDie ( "Could not open $cstempfile_ for writing\n" );

    my @all_strats_in_dir_ = MakeStratVecFromDirAndTT ( $MODELING_STRATS_DIR."/".$shortcode_, $timeperiod_);
    print $main_log_file_handle_ "All global strats: ".join("\n", @all_strats_in_dir_)."\n";
    print CSTF basename($_)."\n" foreach @all_strats_in_dir_;

    $num_strats_in_global_results_ = $#all_strats_in_dir_ + 1;
    print CSTF (split(' ', $_))[1]."\n" foreach grep { $_ =~ /STRATEGYFILEBASE/ } @results;
    close CSTF;

    if( "$num_strats_in_global_results_" eq "") { 
      print STDERR "No previous strats in the pool";
      $num_strats_in_global_results_ = 0; 
    }

    print $main_log_file_handle_ "Num of strats in global result: $num_strats_in_global_results_\n\n";

    my $exec_cmd_="$LIVE_BIN_DIR/summarize_strategy_results local_results_base_dir $cstempfile_ $work_dir_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_ INVALIDFILE $sort_algo_";
    print $main_log_file_handle_ $exec_cmd_."\n";
    my @global_res_out_ = `$exec_cmd_`; 
    print $main_log_file_handle_ "@global_res_out_\n";chomp(@global_res_out_);

    if ( $num_strats_in_global_results_ <= 0 ) {
      $mail_content_ .= "No previous strat in the pool to compare\n";
    }
    else {
      $mail_content_ .= "=============================  STRAT-POOL STATISTICS (Size: $num_strats_in_global_results_) =======================\n";
      $mail_content_ .= "     pnl  pnl_stdev  volume  pnl_shrp  pnl_cons  pnl_median  ttc  pnl_zs  avg_min_max_pnl  PPT  S B A  MAXDD\n";
      for (my $i=0; $i<$num_strats_in_global_results_ * 0.75 + 5; $i += int($cut_off_rank_/2.0)){
        if ( $i > $#global_res_out_ ) { next; }
        my @t_line_ = split(/ /, $global_res_out_[$i]);
        $mail_content_ .= "[$i]: @t_line_[2 .. $#t_line_]\n";
      }
      $mail_content_ .= "==========================\n\n";
    }

    foreach ( @results ) {
      if ( $_ =~ /STRATEGYFILEBASE/ ) {
        my $strat_line = $_;
        my $strat_ = (split(' ', $strat_line))[1];
        print $main_log_file_handle_ "strat:".$strat_."\n";
        my @index_list = grep {$global_res_out_[$_] =~ $strat_} 0 .. $#global_res_out_;

        if ($#index_list >= 0 && $index_list[0] <= $cut_off_rank_) {
          push(@strat_files_selected_, $strat_);
        }
        if($#index_list >= 0 ) {
          $strat_indices_{$strat_} = int($index_list[0]);
        }
      }
    }

    print $main_log_file_handle_ "\n====================================\nAccepted Strats via global_result_analysis:\n@strat_files_selected_\n";
    while (my ($key, $value) = each(%strat_indices_) ){
      print $main_log_file_handle_ "$key: $value \n";
    }
    print $main_log_file_handle_ "=======================================\n";
  }

  foreach my $strat_ ( @strat_files_selected_ ) {
    $mail_content_ = $mail_content_."For strat: ".$strat_."\n";
    print $main_log_file_handle_ "For strat: ".$strat_."\n";

    my $exec_cmd = "$LIVE_BIN_DIR/summarize_single_strategy_results local_results_base_dir $strat_ $work_dir_ $trading_start_yyyymmdd_ $trading_end_yyyymmdd_";
    print $main_log_file_handle_ "$exec_cmd\n";
    my @t_insample_text_ = `$exec_cmd`;
    my $t_temp_strategy_filename_ = FindItemFromVecWithBase ( $strat_ , @strategy_filevec_  ) ;

    if  ( $install_ == 1 ) {
      print $main_log_file_handle_ " Installing strat $t_temp_strategy_filename_ for $shortcode_, datagen-time: $datagen_start_time_-$datagen_end_time_ , trading-time : $trading_start_hhmm_-$trading_end_hhmm_ " ;
      InstallStirStrategyModelling ( $t_temp_strategy_filename_, $shortcode_, "$datagen_start_time_-$datagen_end_time_", "$trading_start_hhmm_-$trading_end_hhmm_" ) ;
    }

    $mail_content_ = $mail_content_." Installing strat : ".$strat_."\n" ;
    $mail_content_ = $mail_content_." Performance: \n". join ( "", @t_insample_text_ ) . "\n";
  }

  SendMail () if ( $#strat_files_selected_ >= 0 );
}

sub SendMail 
{
  if ( $mail_id_ && $mail_content_ ) {
    open ( MAIL, "|/usr/sbin/sendmail -t" ) ;
    print MAIL "To: $mail_id_\n";
    print MAIL "From: $mail_id_\n";
    print MAIL "Subject: StirGenstrat( $configname_ ) $process_start_time_ $server_ \n\n";
    print MAIL $mail_content_;

    close(MAIL);
  }
}

sub LoadConfigFile
{
  my $config_file_ = shift ;
  open ( CONFIG_FILE , "<" , $config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );
  my $current_param_ = "";
  foreach my $config_file_lines_ ( @config_file_lines_ ) 
  {	
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
        when ( "SHORTCODE" )
        {
          $shortcode_ = $t_words_[0] ;
        }
        when ( "DEPENDENT_LIST" )
        {
          $prod_list_file_ = $t_words_[0] ;
        }
        when ("TEMPLATE_ILIST")
        {
          $template_ilist_name_ = $t_words_[0] ;
        }
        when ("DATAGEN_TRADING_START_END_YYYYMMDD")
        {
          if ( $#t_words_ >= 3 )
          {
            $start_date_yyyymmdd_ =  GetIsoDateFromStrMin1 ( $t_words_[0] );
            $end_date_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[1]);
            $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[2] );
            $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[3] );
          }
        }
        when ( "DATAGEN_START_END_YYYYMMDD" )
        {
          if ( $#t_words_ >= 1 )
          {
            $start_date_yyyymmdd_ = GetIsoDateFromStrMin1( $t_words_[0] );
            $end_date_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[1] );
          }
        }
        when ( "DATAGEN_START_DATE" )
        {
          $start_date_yyyymmdd_ = $t_words_[0];
        }
        when ( "DATAGEN_END_DATE" )
        {
          $end_date_yyyymmdd_  = $t_words_[0];
        }
        when ("DATAGEN_DAYS")
        {
          push ( @{ $prod_to_datagen_days_{ "ALL" } }, @t_words_ );
        }
        when ("DATAGEN_DAYS_FILE")
        {
          if ( $#t_words_ > 0 ) {
            my $prod_ = $t_words_[0];
            if ( ExistsWithSize ( $t_words_ [0] ) )
            {
              my $days_file_ = $t_words_ [0];
              open DFHANDLE, "< $days_file_" or PrintStacktraceAndDie ( "$0 Could not open $days_file_\n" );
              my @day_vec_ = <DFHANDLE>; chomp ( @day_vec_ );
              push ( @{ $prod_to_datagen_days_{ $prod_ } }, @day_vec_ );
              close DFHANDLE;
            }
          }
        }
        when ("DATAGEN_EVENT")
        {
          my $event_name_ = $t_words_[0] ;
          my $ex_cmd_ = "grep $event_name_ $HOME_DIR/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201?_processed.txt  | awk '{print \$5}' | cut -d'_' -f1" ;
          my @day_vec_ = `$ex_cmd_`; chomp ( @day_vec_ );
          push ( @{ $prod_to_datagen_days_{ "ALL" } }, @day_vec_ );
        }
        when ("DATAGEN_BAD_DAYS")
        {
          @datagen_day_filter_choices_ = ( "bd" );
        }
        when ("DATAGEN_POOL_BAD_DAYS")
        {
          @datagen_day_filter_choices_ = ( "pbd ".join(" ", @t_words_) );
        }
        when ("DATAGEN_VERY_BAD_DAYS")
        {
          @datagen_day_filter_choices_ = ( "vbd" );
        }
        when ("DATAGEN_HIGH_VOLUME_DAYS")
        {
          @datagen_day_filter_choices_ = ( "hv" );
        }
        when ("DATAGEN_LOW_VOLUME_DAYS")
        {
          @datagen_day_filter_choices_ = ( "lv" );
        }
        when ("DATAGEN_HIGH_STDEV_DAYS")
        {
          @datagen_day_filter_choices_ = ( "hsd ".join(" ", @t_words_) );
        }
        when ("DATAGEN_DAY_FILTER")
        {
          push ( @datagen_day_filter_choices_, join(" ", @t_words_) );
        }
        when ("DATAGEN_DAY_FILTER_START_DATE")
        {
          $datagen_day_filter_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[0] );
        }
        when ("DATAGEN_DAY_FILTER_MAX_DAYS")
        {
          $datagen_day_filter_max_days_ = int($t_words_[0]);
        }
        when ("DATAGEN_DAY_INCLUSION_PROB")
        {
          $datagen_day_inclusion_prob_ = max ( 0.00, min ( 1.00, $t_words_[0] ) ) ;
        }
        when ("DATAGEN_EXCLUDE_DAYS")
        {
          push ( @{ $prod_to_datagen_exclude_days_{ "ALL" } }, @t_words_ );
        }
        when ("DATAGEN_EXCLUDE_DAYS_FILE")
        {
          if ( $#t_words_ > 0 ) {
            my $prod_ = $t_words_[0];
            if ( ExistsWithSize ( $t_words_ [1] ) )
            {
              my $exclude_days_file_ = $t_words_ [1];
              open DFHANDLE, "< $exclude_days_file_" or PrintStacktraceAndDie ( "$0 Could not open $exclude_days_file_\n" );
              my @exclude_days_ = <DFHANDLE>; chomp ( @exclude_days_ );
              push ( @{ $prod_to_datagen_exclude_days_{ $prod_ } }, @exclude_days_ );
              close DFHANDLE;
            }
          }
        }
        when ( "DATAGEN_START_END_HHMM" )
        {
          if ( $#t_words_ >= 1 )
          {
            $datagen_start_time_ = $t_words_[0] ;
            $datagen_end_time_ = $t_words_[1] ;
          }
        }
        when ( "DATAGEN_START_TIME" )
        {
          $datagen_start_time_ = $t_words_[0]
        }
        when ( "DATAGEN_END_TIME" )
        {
          $datagen_end_time_ = $t_words_[0] ; 
        }
        when ( "TRADING_START_END_YYYYMMDD" )
        {
          if ( $#t_words_ >= 1 )
          {
            $trading_start_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[0] );
            $trading_end_yyyymmdd_ = GetIsoDateFromStrMin1 ( $t_words_[1] );
          }
        }
        when ( "TRADING_START_DATE" )
        {
          $trading_start_yyyymmdd_ = $t_words_[0];
        }
        when ( "TRADING_END_DATE" )
        {
          $trading_end_yyyymmdd_ = $t_words_[0];
        }
        when ( "TRADING_START_END_HHMM" )
        {
          if ( $#t_words_ >= 1 )
          {
            $trading_start_hhmm_ = $t_words_[0];
            $trading_end_hhmm_ = $t_words_[1];
          }
        }
        when ("TRADING_DAYS")
        {
          push ( @trading_days_, @t_words_ );
        }
        when ("TRADING_DAYS_FILE")
        {
          if ( ExistsWithSize ( $t_words_ [0] ) )
          {
            my $days_file_ = $t_words_ [0];
            open DFHANDLE, "< $days_file_" or PrintStacktraceAndDie ( "$0 Could not open $days_file_\n" );
            my @day_vec_ = <DFHANDLE>; chomp ( @day_vec_ );
            push ( @trading_days_, @day_vec_ );
            close DFHANDLE;
          }
        }
        when ("TRADING_EVENT")
        {
          my $event_name_ = $t_words_[0] ;
          my $ex_cmd_ = "grep $event_name_ $HOME_DIR/infracore_install/SysInfo/BloombergEcoReports/merged_eco_201?_processed.txt  | awk '{print \$5}' | cut -d'_' -f1" ;
          my @day_vec_ = `$ex_cmd_`; chomp ( @day_vec_ );
          push ( @trading_days_, @day_vec_ );
        }
        when ("TRADING_EXCLUDE_DAYS")
        {
          push ( @trading_exclude_days_, @t_words_ );
        }
        when ("TRADING_EXCLUDE_DAYS_FILE")
        {
          if ( ExistsWithSize ( $t_words_ [0] ) )
          {
            my $exclude_days_file_ = $t_words_ [0];
            open DFHANDLE, "< $exclude_days_file_" or PrintStacktraceAndDie ( "$0 Could not open $exclude_days_file_\n" );
            my @exclude_days_ = <DFHANDLE>; chomp ( @exclude_days_ );
            push ( @trading_exclude_days_, @exclude_days_ );
            close DFHANDLE;
          }
        }
        when ( "TRADING_START_TIME" )
        {
          $trading_start_hhmm_ = $t_words_[0]
        }
        when ( "TRADING_END_TIME" )
        {
          $trading_end_hhmm_ = $t_words_[0] ; 
        }
        when ( "STRATEGY_NAME" ) 
        {
          if ( $#t_words_ >= 0 )
          {
            $strategy_name_ = $t_words_[0] ;
          }
        }
        when ( "ADD_PROD_NAME_IN_STRAT_NAME" )
        {
          if ( $#t_words_ >= 0 )
          {
            $add_prod_name_in_strat_name_ = $t_words_[0] ;
          }
        }
        when ( "DATAGEN_DAY_INCLUSION_PROB" )
        {
          if ( $#t_words_ >= 0 )
          {
            $datagen_day_inclusion_prob_ = max ( 0.0 , min ( 1.00 , $t_words_[0] ) ) ;
          }
        }
        when ( "USE_INSAMPLE_DAYS_FOR_TRADING")
        {
          if ( $#t_words_ >= 0 )
          {
            $use_insample_days_for_trading_ = $t_words_[0];
          }
        }
        when ( "COMMONPARAMFILE" ) 
        {
          $common_param_filename_ = $t_words_[0] ;
        }
        when ( "REGRESS_ALGO" ) 
        {
          $regress_algo_string_ = join ( ' ', @t_words_[0..$#t_words_] ) ; 
          push ( @regress_algo_string_vec_, $regress_algo_string_ ); 
        }
        when ( "REGRESS_EXEC" ) 
        {
          $regress_algo_string_ = join ( ' ', @t_words_[0..$#t_words_] ) ; 
          push ( @regress_algo_string_vec_, $regress_algo_string_ ); 
        }
        when ( "FILTER" ) 
        {
          $filter_ = $t_words_[0] ; 
        }
        when ( "PREDALGO" )
        {
          $predalgo_ = $t_words_[0] ;
          push ( @predalgo_vec_, $predalgo_ ) ;
        }
        when ( "PARAMNAME" ) 
        {
          $paramname_ = $t_words_[0] ;
        }
        when ( "PARAMLIST" ) 
        {
          $paramlistfilename_ = $t_words_[0] ;
        }
        when ( "DATAGEN_TIMEOUTS" )
        {
          $datagen_timeouts_ = join ( ' ', @t_words_[0..$#t_words_] );
          push ( @datagen_timeout_vec_ , $datagen_timeouts_ );
        }
        when ( "DATAGEN_TIMEOUT" )
        {
          $datagen_timeouts_ = join ( ' ', @t_words_[0..$#t_words_] );
          push ( @datagen_timeout_vec_ , $datagen_timeouts_ );
        }
        when ( "PRED_DURATION" ) 
        {
          $pred_duration_ = $t_words_[0] ;
          push ( @pred_duration_vec_ , $pred_duration_ ) ;
        }
        when ( "DELETE_INTERMEDIATE_FILES")
        {
          if ( $t_words_[0] eq "0" ) { $delete_intermediate_files_ = "" ; } 
          else
          {
            $delete_intermediate_files_ = "1";
          }
        }
        when ( "ADD_MODELINFO_TO_MODEL" )
        {
          if ( $#t_words_ >= 0 )
          {
            $add_modelinfo_to_model_ = $t_words_[0] ;
          }
        }
        when ("MAIL_ADDRESS")
        {
          if ( $#t_words_ >= 0 ) 
          {
            $mail_id_ = $t_words_[0] ;
          }
        }
        when ( "INSTALL")
        {
          if ( $#t_words_ >= 0 )
          {
            $install_ = $t_words_[0];
          }
        }
        when ( "NAME" )
        {
          if ( $#t_words_ >= 0 )
          {
            $name_ = $t_words_[0] ;
          }
        }
        when ( "AUTHOR" )
        {
          if ( $#t_words_ >= 0 ) 
          {
            $author_ = $t_words_[0] ;
          }
        }
        when ( "GENERATE_COMBINED_DATA" )
        {
          $generate_individual_data_ = 0 if ( $#t_words_ >= 0 && $t_words_[0] );
        }
      }
    }
  }

  if ( $#regress_algo_string_vec_ > 0 )
  {
# more than 1 algo string specified, pick randomly
    $regress_algo_string_ = $regress_algo_string_vec_ [int ( rand ( $#regress_algo_string_vec_ + 1 ) ) ]
  } 

  if ( $#predalgo_vec_ > 0 ) 
  {
    $predalgo_ = $predalgo_vec_ [ int ( rand ( $#predalgo_vec_ + 1 ) ) ];
  }

  if ( $#pred_duration_vec_ > 0 ) 
  {
    $pred_duration_ = $pred_duration_vec_ [ int ( rand ( $#pred_duration_vec_ + 1 ) ) ];
  }
  if ( $#datagen_timeout_vec_ > 0 )
  {
    $datagen_timeouts_ = $datagen_timeout_vec_[ int ( rand ( $#datagen_timeout_vec_ + 1 ))] ;
  }

  if ( index ( $datagen_timeouts_, "EVT" ) >= 0 || index ( $predalgo_, "e" ) >= 0 ) {
    $generate_individual_data_ = 1;
  }
  elsif ( index ( $filter_ , "fv" ) >= 0 || index ( $filter_, "fdsum" ) >= 0 ) {
    $generate_individual_data_ = 0;
  }

  my @timeout_string_words_ = split ( ' ', $datagen_timeouts_ ) ;
  my ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 4000, 10, 0 );
  my $timeout_set_already_ = 0;

  if ( index ( $timeout_string_words_[0] , "EVT" ) != -1 ) 
  {
    ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 4000, 10, 0 );
    if ( $timeout_string_words_[0] eq "EVT2" )
    {
      $datagen_l1events_timeout_ = "c1" ;  # msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
    }
    if ( $timeout_string_words_[0] eq "EVT3" )
    {
      $datagen_l1events_timeout_ = "c2" ;  # msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
    }
    if ( $timeout_string_words_[0] eq "EVT4" )
    {
      $datagen_l1events_timeout_ = "c3" ;  # msecs_timeout & num_trades_timeout still using dep ( not a bad idea )
    }
    $timeout_set_already_ = 1;
  }
  elsif ( $timeout_string_words_[0] eq "TRD1" )
  {
    ( $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) = ( 8000, 20, 1 );
    $timeout_set_already_ = 1;
  }
  if ( $#timeout_string_words_ >= 2 )
  {
    if ( $timeout_set_already_ == 0 )
    {
      $datagen_msecs_timeout_ = $timeout_string_words_[0];
      $datagen_l1events_timeout_ = $timeout_string_words_[1];
      $datagen_num_trades_timeout_ = $timeout_string_words_[2];
    }
  }
  $datagen_timeouts_ = join ( " ", $datagen_msecs_timeout_, $datagen_l1events_timeout_, $datagen_num_trades_timeout_ ) ;
}

