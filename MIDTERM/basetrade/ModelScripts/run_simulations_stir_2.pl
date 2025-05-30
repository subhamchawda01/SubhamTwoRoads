#!/usr/bin/perl

# \file ModelScripts/run_simulations_stir_2.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
# This script takes as input :
# directory name ( runs simulations for all files below directory )
# or strategy_list_filename ( a file with lines with just 1 word corresponding to the fullpath of a strategyfile )
# trading_start_yyyymmdd
# trading_end_yyyymmdd
# [ localresults_base_dir ] ( if not specified then use Globalresultsdbdir )
#
# for each valid_date in the period :
#   make a temporary_this_day_strategy_list_file of only the entries which do not have data in the results_base_dir
#   breaks the temporary_this_day_strategy_list_file into blocks of 15 entries
#   for each block of <= 15 entries : create temp_strategy_block_file ( with just the names of files in that block )
#     runs a base_trade_init on this tradingdate and this block and collects the results in a temp_results_block_file
#     then uses add_results_to_local_database ( temp_strategy_block_file, temp_results_block_file, results_base_dir )
#
# At the end looking at the original strategy_list_filename and results_base_dir
# build a summary_results.txt of the following format :
# STRATEGYFILENAME strategy_file_name
# date_1 pnl_1 volume_1
# date_2 pnl_2 volume_2
# .
# .
# date_n pnl_n volume_n
# STATISTICS pnl_average_ pnl_stdev_ volume_average_ pnl_sharpe_ pnl_conservative_average_ pnl_median_average_

use strict;
use warnings;
use POSIX;
use feature "switch";
use List::Util qw/max min/;

sub SanityCheckInputArguments ;
sub LivingEachDay;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/s3_utils.pl"; # S3PutFilePreservePath

if ( $USER ne "dvctrader" )
{
  $LIVE_BIN_DIR = $BIN_DIR;
}

my $TRADELOG_DIR="/spare/local/logs/tradelogs/"; 

my $GLOBALRESULTSTRADESDIR = "/NAS1/ec2_globalresults_trades"; # since this will be synced to S3 anyways


my $hostname_ = `hostname`;
if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
{ # AWS
  $GLOBALRESULTSTRADESDIR = "/NAS1/ec2_globalresults_trades"; # since this will be synced to S3 anyways
}

# flag to control if trades file named as : trades.YYYYMMDD.w_pbt_strat_ilist_OMix_OMix_lmtest4_200_10.pfi_4
# are generated and synced to S3.
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl"; # NoDataDate
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/calc_prev_date.pl"; # CalcPrevDate
require "$GENPERLLIB_DIR/calc_prev_date_mult.pl"; # CalcPrevDateMult # IsDateHoliday
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1 # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_files_pending_sim.pl"; # GetFilesPendingSim
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/get_unique_list.pl"; # GetUniqueList
require "$GENPERLLIB_DIR/get_unique_sim_id_from_stir_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/exists_and_same.pl"; # ExistsAndSame
require "$GENPERLLIB_DIR/make_strat_vec_from_dir_matchbase_stir.pl"; # MakeStratVecFromDirMatchBase
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/check_ilist_data.pl"; # CheckIndicatorData


my $USAGE="$0 shortcode strat_directory/stratlist_file trading_start_yyyymmdd trading_end_yyyymmdd [COMPUTE_INDIVIDUAL_STATS=0] [GLOBALRESULTSDBDIR|DEF_DIR|DB=DB] [MKT_MODEL|DEF=DEF]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $base_dir = $ARGV[1];
my $trading_start_yyyymmdd_ = max ( 20110901, $ARGV[2] );
my $trading_end_yyyymmdd_ = $ARGV[3];
my $MAX_STRAT_FILES_IN_ONE_SIM = 20; # please work on optimizing this value

my $compute_individual_product_stats_  = 0 ;
my $GLOBALRESULTSDBDIR = "DB";
if ( $USER ne "dvctrader" )
{
	$GLOBALRESULTSDBDIR = "DEF_DIR";
}
my $MKT_MODEL = "DEF";

if ( $#ARGV >= 4 ) { $compute_individual_product_stats_ = $ARGV[4]; }
if ( $#ARGV >= 5 ) { $GLOBALRESULTSDBDIR = $ARGV[5]; }
if ( $#ARGV >= 6 ) { $MKT_MODEL = $ARGV[6]; }

#setting default directories
if ( $GLOBALRESULTSDBDIR eq "DEF_DIR" )
{
	$GLOBALRESULTSDBDIR = $HOME_DIR."/globalresults";
	if ( index ( $hostname_ , "ip-10-0" ) >= 0 )
	{ # AWS
  		$GLOBALRESULTSDBDIR = $HOME_DIR."/ec2_globalresults";
  }
}

if ( $MKT_MODEL eq "DEF" )
{
  $MKT_MODEL = GetMarketModelForShortcode ( $shortcode_ );
}

my $is_db_ = int($GLOBALRESULTSDBDIR eq "DB");

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $debug_ = 1;

my $shift_di_symbols_ = -1;

SanityCheckInputArguments( );

my @strategy_filevec_ = ();

LocalMakeStratVecFromDirMatchBaseStir ( );

LivingEachDay ( );

exit( 0 );


###### subs ######
sub LivingEachDay
{
  my @resultsfiles_=();
  my $tradingdate_ = $trading_end_yyyymmdd_;
  my $max_days_at_a_time_ = 2000;
  for ( my $j = 0 ; $j < $max_days_at_a_time_ ; $j ++ ) 
  {
START_OF_SIM_FOR_DAY:
    if ( SkipWeirdDate ( $tradingdate_ ) || IsDateHoliday ( $tradingdate_ ) )
    {
      $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
      next;
    }
    if ( ( ! ValidDate ( $tradingdate_ ) ) ||
        ( $tradingdate_ < $trading_start_yyyymmdd_ ) ) 
    {
      last;
    }

    {
      my @this_day_strategy_filevec_ = ();
      GetFilesPendingSimFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, $GLOBALRESULTSDBDIR ); # only the files that do not have results in the global database

        if ( $#this_day_strategy_filevec_ >= 0 )
        {
          my $this_day_work_dir_ = $SPARE_HOME."RS/".$shortcode_."/".$tradingdate_."/".$unique_gsm_id_;
          if ( -d $this_day_work_dir_ ) { `rm -rf $this_day_work_dir_`; }
          `mkdir -p $this_day_work_dir_`;

          print $this_day_work_dir_."\n";

          my $main_log_file_ = $this_day_work_dir_."/main_log_file.txt";
          my $main_log_file_handle_ = FileHandle->new;
          $main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" ); 
          $main_log_file_handle_->autoflush(1);
          print $main_log_file_handle_ "Logfile for $tradingdate_\n";


# for this trading date break the @this_day_strategy_filevec_ into blocks of size MAX_STRAT_FILES_IN_ONE_SIM
          my $this_day_strategy_filevec_front_ = 0;
          my $temp_strategy_list_file_index_ = 0; # only a counter for differnet stratlistfilenames
            my $this_strat_bunch_next_strat_id_ = 10011; # start from beginnin
            while ( $this_day_strategy_filevec_front_ <= $#this_day_strategy_filevec_ )
            {
              my $this_day_strategy_filevec_back_ = min ( ( $this_day_strategy_filevec_front_ + $MAX_STRAT_FILES_IN_ONE_SIM - 1 ), $#this_day_strategy_filevec_ ) ;
              my $temp_strategy_cat_file_ = $this_day_work_dir_."/temp_strategy_list_file_".$temp_strategy_list_file_index_.".txt" ; 
              my $temp_strategy_list_filenames_ = $this_day_work_dir_."/temp_strategy_list_filenames_".$temp_strategy_list_file_index_.".txt" ;
              open TSLF, "> $temp_strategy_list_filenames_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_ for writing\n" );
              open TSCF, "> $temp_strategy_cat_file_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_cat_file_ for writing\n" );
              my @strat_index_vec_ = () ;
              for ( my $this_day_strategy_filevec_idx_ = $this_day_strategy_filevec_front_; $this_day_strategy_filevec_idx_ <= $this_day_strategy_filevec_back_; $this_day_strategy_filevec_idx_ ++ )
              {
                my $this_strat_filename_ = $this_day_strategy_filevec_[$this_day_strategy_filevec_idx_] ;
                print $main_log_file_handle_ "Processing $this_strat_filename_ assigning stratid $this_strat_bunch_next_strat_id_\n";
                open THIS_STRAT_FILEHANDLE, "< $this_strat_filename_ " or PrintStacktraceAndDie ( "Could not open $this_strat_filename_\n" );
                my @this_strat_lines_ = <THIS_STRAT_FILEHANDLE>;
                close THIS_STRAT_FILEHANDLE ;
                my $added_one_strat_ = 0;

                for ( my $tsl_index_ = 0 ; $tsl_index_ <= $#this_strat_lines_; $tsl_index_ ++ )
                {
                  my @tsl_words_ = split ( ' ', $this_strat_lines_[$tsl_index_] );
                  $tsl_words_[2] = $this_strat_bunch_next_strat_id_ ++;
                  push ( @strat_index_vec_, $tsl_words_[2] ) ;
                  print TSCF join ( ' ', @tsl_words_ )."\n";
                  $added_one_strat_ ++ ;
                  print $main_log_file_handle_ "Adding to TSCF ".join ( ' ', @tsl_words_ )."\n";
                }
                if( $added_one_strat_ > 0 ) { print TSLF "$this_strat_filename_\n";}
              }
              close TSCF;
              close TSLF;
              my @sim_strategy_output_lines_=(); # stored to seive out the SIMRESULT lines
                my %unique_id_to_pnlstats_map_=(); # stored to write extended results to the database
                my %prod_to_unique_id_pnlstats_map_=();
              my %unique_id_to_trades_filename_ = ( ); # to store trades.yyyymmdd.strat_filename on S3
              {
                my $market_model_index_ = 0 ;
                if ( $MKT_MODEL >= 0 )
                { # override
                  $market_model_index_ = $MKT_MODEL ;
                }
                my $exec_cmd="$BIN_DIR/sim_strategy SIM $temp_strategy_cat_file_ $unique_gsm_id_ $tradingdate_ $market_model_index_ 0 0.0 0 ADD_DBG_CODE -1 2>/dev/null"; 
                @sim_strategy_output_lines_=`$exec_cmd` ;
                if ( $debug_ == 1 ) 
                { 
                  print $main_log_file_handle_ "$exec_cmd\n"; 
                  print $main_log_file_handle_ "OutputLines: ".join ( "", @sim_strategy_output_lines_ )."\n";
                }
                my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
                if ( ExistsWithSize ( $this_tradesfilename_ ) )
                {
                  my $strategy_trades_files_dir_ = $this_day_work_dir_."/strategy_trades_files";
                  `mkdir -p $strategy_trades_files_dir_`;
                  my @pnlstats_output_lines_ = ( ) ;
                  $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_stir_2.pl $this_tradesfilename_";
                  @pnlstats_output_lines_ = `$exec_cmd`;

                  if ( $debug_ == 1 ) 
                  { 
                    print $main_log_file_handle_ "$exec_cmd\n"; 
                    print $main_log_file_handle_ "PNLstatsOutputLines: ".join ( "", @pnlstats_output_lines_ )."\n";
                  }
                  for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
                  {
                    my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
                    if ( $#rwords_ >= 1 )
                    {
                      my $unique_sim_id_ = $rwords_[0];
                      splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_sim_id_
                        $unique_id_to_pnlstats_map_{$unique_sim_id_} = join ( ' ', @rwords_ );				   
#print $unique_sim_id_."\n";
                      if ( $debug_ == 1 )
                      {
                        print $main_log_file_handle_ "For $unique_sim_id_ pnlstatsmappedto: ".$unique_id_to_pnlstats_map_{$unique_sim_id_}."\n";
                      }
                    }
                  }

                  if ( $compute_individual_product_stats_ == 1 )
                  {
                    $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_stir_2.pl $this_tradesfilename_ I 1 ";
                    @pnlstats_output_lines_ = `$exec_cmd`;
                    if ( $debug_ == 1 ) 
                    { 
                      print $main_log_file_handle_ "$exec_cmd\n"; 
                      print $main_log_file_handle_ "PNLstatsOutputLines: ".join ( "", @pnlstats_output_lines_ )."\n";
                    }

                    for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
                    {
                      my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
                      if ( $#rwords_ >= 1 )
                      {
                        my $prod_id_ = $rwords_[0];
                        my @prod_and_id_ = split ( /\./, $prod_id_ );
                        if ( $#prod_and_id_ >= 1 )
                        {
                          my $prod_name_ = $prod_and_id_[0];
                          my $id_name_ = $prod_and_id_[1];
                          splice ( @rwords_, 0, 1);
                          $prod_to_unique_id_pnlstats_map_{$prod_name_}{$id_name_} = join ( ' ', @rwords_ );
                          if ( $debug_ == 1 )
                          {
                            print $main_log_file_handle_ "For $id_name_ and prod $prod_name_ pnlstatsmappedto: ".$prod_to_unique_id_pnlstats_map_{$prod_name_}{$id_name_}."\n";
                          }
                        }
                      }
                    }
                  }
                }
                { 
                  `rm -f $this_tradesfilename_`; 
                  my $this_logfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int($unique_gsm_id_);
                  `rm -f $this_logfilename_`;
                }
              }
              my $temp_results_list_file_ = $this_day_work_dir_."/temp_results_list_file_".$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
              my @list_of_unique_ids_in_TRLF = ( );

              open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
              for ( my $t_sim_strategy_output_lines_index_ = 0, my $psindex_ = 0; $t_sim_strategy_output_lines_index_ <= $#sim_strategy_output_lines_; $t_sim_strategy_output_lines_index_ ++ )
              {
                if ( $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] =~ /SIMRESULT/ )
                { # SIMRESULT pnl volume
                  my @rwords_ = split ( ' ', $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] );
                  if ( $#rwords_ >= 2 ) 
                  {
                    splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc
                      my $remaining_simresult_line_ = join ( ' ', @rwords_ );
                    my $unique_sim_id_ = GetUniqueSimIdFromStirCatFile ( $temp_strategy_cat_file_, $psindex_ );
                    if ( ! exists $unique_id_to_pnlstats_map_{$unique_sim_id_} )
                    {
                      $unique_id_to_pnlstats_map_{$unique_sim_id_} = "0 0 0 0 0 0 0 0 0 0 0 0 0";
#				PrintStacktraceAndDie ( "unique_id_to_pnlstats_map_ missing $unique_sim_id_ for listfile: $temp_results_list_file_ catfile: $temp_strategy_cat_file_ rline: $remaining_simresult_line_\n" );
                    }
                    printf TRLF "%s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_sim_id_};
                    push ( @list_of_unique_ids_in_TRLF , $unique_sim_id_ );

                    if ( $debug_ == 1 )
                    {
                      printf $main_log_file_handle_ "Adding to TRLF %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_sim_id_};
                    }
                  }
                }
                else
                {
                  PrintStacktraceAndDie ( "SIMRESULT line has less than 3 words\n" ); 
                }
                $psindex_ ++;
              }
              close TRLF;
              
              if ( ( ExistsWithSize ( $temp_results_list_file_ ) ) &&
                  ( ExistsWithSize ( $temp_strategy_list_filenames_ ) ) )
              {
          		my $results_database_dir_ = $is_db_ ? "INVALIDDIR" : $GLOBALRESULTSDBDIR."/".$shortcode_;
			    my $add_to_SQLDB_ = $is_db_ ? $shortcode_ : 0; #passing shortcode when inserting into DB
                my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_global_database.pl $temp_strategy_list_filenames_ $temp_results_list_file_ $tradingdate_ $results_database_dir_ $add_to_SQLDB_"; 
                my $output_ = `$exec_cmd`;
                #print "$output_";
                if ( $debug_ == 1 ) 
                { 
                  print $main_log_file_handle_ "$exec_cmd\n"; 
                  print $main_log_file_handle_ "Output: $output_\n"; 
                }
                if ( $output_ =~ /FAILED/ )
                { 
#do not rm these files
                  print STDERR "AR2GDb.pl FAILED on $tradingdate_. output :\n $output_\n";
                  if ( $debug_ == 1 )
                  {
                    print $main_log_file_handle_ "AR2GDb.pl FAILED on $tradingdate_. output :\n $output_\n";
                  }
                  open TEMP_STRATEGY_LIST_FILEHANDLE, "< $temp_strategy_list_filenames_ " or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_\n" );
                  my @temp_strat_list_lines_ = <TEMP_STRATEGY_LIST_FILEHANDLE>;
                  chomp ( @temp_strat_list_lines_ );
                  close TEMP_STRATEGY_LIST_FILEHANDLE ;

                  for ( my $tslf_idx_ = 0 ; $tslf_idx_ <= $#temp_strat_list_lines_ ; $tslf_idx_ ++ )
                  {
                    printf STDERR "AR2GDb.pl failed list file item : %s\n", $temp_strat_list_lines_[$tslf_idx_] ;
                    if ( $debug_ == 1 )
                    {
                      printf $main_log_file_handle_ "AR2GDb.pl failed list file item : %s\n", $temp_strat_list_lines_[$tslf_idx_] ;
                    }
                  }
                  exit ( 0 );
                }
                else {}
              }
              else
              {
                if ( ! ExistsWithSize ( $temp_results_list_file_ ) )
                {
                  print STDERR "Date: $tradingdate_ ResultsFile $temp_results_list_file_ missing for catfile: $temp_strategy_cat_file_ listfile: $temp_strategy_list_filenames_ \n" ;
                  if ( $debug_ == 1 )
                  {
                    print $main_log_file_handle_ "Date: $tradingdate_ ResultsFile $temp_results_list_file_ missing for catfile: $temp_strategy_cat_file_ listfile: $temp_strategy_list_filenames_ \n" ;
                  }
                  open TEMP_STRATEGY_LIST_FILEHANDLE, "< $temp_strategy_list_filenames_ " or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_\n" );
                  my @temp_strat_list_lines_ = <TEMP_STRATEGY_LIST_FILEHANDLE>;
                  chomp ( @temp_strat_list_lines_ );
                  close TEMP_STRATEGY_LIST_FILEHANDLE ;

                  for ( my $tslf_idx_ = 0 ; $tslf_idx_ <= $#temp_strat_list_lines_ ; $tslf_idx_ ++ )
                  {
                    printf STDERR "failed list file item : %s\n", $temp_strat_list_lines_[$tslf_idx_] ;
                    if ( $debug_ == 1 )
                    {
                      printf $main_log_file_handle_ "failed list file item : %s\n", $temp_strat_list_lines_[$tslf_idx_] ;
                    }
                  }
                }
                if ( ! ExistsWithSize ( $temp_strategy_list_filenames_ ) )
                {
                  print STDERR "StrategyListFile $temp_strategy_list_filenames_ missing\n" ;
                  if ( $debug_ == 1 )
                  { 
                    print $main_log_file_handle_ "StrategyListFile $temp_strategy_list_filenames_ missing\n" ;
                  }
                }
              }

			# print ("COMPUTING INDIVIDUAL PRODUCTS $compute_individual_product_stats_\n");

# adds the results to respective constituent products
              if ( $compute_individual_product_stats_ == 1)
              {
                foreach my $prod_ ( keys %prod_to_unique_id_pnlstats_map_) 
                {
                  my %id_to_stats_map_ = %{$prod_to_unique_id_pnlstats_map_{$prod_}};
                  my $temp_results_list_file_ = $this_day_work_dir_."/temp_results_list_file_".$prod_.$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
                  my @list_of_unique_ids_in_TRLF = ( );
                  open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
                  foreach my $id1_ ( @strat_index_vec_ )
                  {
                    if ( exists ( $id_to_stats_map_{$id1_} )  ) 
                    {
                      printf TRLF "%s\n",$id_to_stats_map_{$id1_};
                      push ( @list_of_unique_ids_in_TRLF , $id1_ );
                      if ( $debug_ == 1 )
                      {
                        printf $main_log_file_handle_ "Adding to TRLF %s\n",$id_to_stats_map_{$id1_};
                      }								
                    }
                    else
                    {
                      printf TRLF "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n";
                    }
                  }
                  close TRLF;
                  
                  # print (" FILENAMES $temp_results_list_file_ $temp_strategy_list_filenames_ $prod_");
                  
                  if ( ( ExistsWithSize ( $temp_results_list_file_ ) ) &&
                      ( ExistsWithSize ( $temp_strategy_list_filenames_ ) ) )
                  {
                    my $shc_nse_ = $prod_;
                    if ( $shc_nse_ !~ /NSE|BSE/ ) {
                      my $ec="$BIN_DIR/get_shortcode_for_symbol $prod_ $tradingdate_";
                      $shc_nse_ = `$ec`; chomp ( $shc_nse_);
                    }

                    my $exch_line_ = `$BIN_DIR/get_contract_specs $prod_ $tradingdate_ EXCHANGE 2>/dev/null`;
# if $shc_nse_ is Valid Shortcode
                    #if ( defined $exch_line_ && $exch_line_ ne "" ) {
                      my $this_prod_list_filenames_ = $temp_strategy_list_filenames_."_".$shc_nse_;
                      my $this_prod_list_statistics_ = $temp_strategy_list_filenames_."_".$shc_nse_."_result";
                      open TPLF, "> $this_prod_list_filenames_" or PrintStacktraceAndDie ( "Could not open $this_prod_list_filenames_ for writing\n" );
                      open TPLS, "> $this_prod_list_statistics_" or PrintStacktraceAndDie ( "Could not open $this_prod_list_statistics_ for writing\n" );

                      my $strats_ = `cat $temp_strategy_list_filenames_`;
                      my $results_of_this_product_ = `cat $temp_results_list_file_`;

                      my @strat_array_ = split '\n', $strats_;
                      my @results_of_this_product_array_ = split '\n', $results_of_this_product_;

                      for( my $i = 0; $i<=$#strat_array_; $i ++ )
                      {
                        my $temp_result_ = $results_of_this_product_array_[$i];
                        my $temp_strat_ = $strat_array_[$i];
                        chomp( $temp_result_ ); chomp( $temp_strat_ );
                        my @statistics_ = split ' ', $temp_result_;
                        my $this_volume_ = $statistics_[1] + 0;
                        if ($this_volume_ != 0)
                        {
                          printf TPLS "$temp_result_\n";
                          printf TPLF $temp_strat_."_"."$shc_nse_\n";
                        }
                      }
                      close TPLS; close TPLF;

                      my $results_database_dir_ = $is_db_ ? "INVALIDDIR" : $GLOBALRESULTSDBDIR."/".$shc_nse_;
                      my $add_to_SQLDB_ = $is_db_ ? $shc_nse_ : 0; #passing shortcode when inserting into DB
                      my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_global_database.pl $this_prod_list_filenames_ $this_prod_list_statistics_ $tradingdate_ $results_database_dir_ $add_to_SQLDB_";
                      my $output_ = `$exec_cmd`;
                      
                    #  `rm -f $this_prod_list_filenames_`;
                    #  `rm -f $this_prod_list_statistics_`;
                      if ( $debug_ == 1 )
                      { 
                        print $main_log_file_handle_ "$exec_cmd\n"; 
                        print $main_log_file_handle_ "Output: $output_\n";
                      }
                   # }
                  }
                }
              }

              $this_day_strategy_filevec_front_ = $this_day_strategy_filevec_back_ + 1;
              $temp_strategy_list_file_index_ ++;
              $this_strat_bunch_next_strat_id_ = 10011; # start from beginning again
            } # while
          $main_log_file_handle_->close;
        } # if
    }

    $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
  }    
}

sub LocalMakeStratVecFromDirMatchBaseStir
{
  my $top_directory_ = $base_dir;
  $top_directory_ = File::Spec->rel2abs ( $top_directory_ );
  my @ret_abs_file_paths_ = ();
  if ( -d $top_directory_ )
  {
    if (opendir my $dh, $top_directory_)
    {
      my @t_list_=();
      while ( my $t_item_ = readdir $dh)
      {
        push @t_list_, $t_item_;
      }
      closedir $dh;

      for my $dir_item_ (@t_list_)
      {
        next if $dir_item_ eq '.' || $dir_item_ eq '..';

        my $strat_file_path_ = $top_directory_."/".$dir_item_;
        if ( -f $strat_file_path_ )
        {
          push @ret_abs_file_paths_, $strat_file_path_;
        }
        if ( -d $strat_file_path_ )
        {
          push @ret_abs_file_paths_, MakeStratVecFromDirMatchBaseStir ( $strat_file_path_ ) ;
        }
      }
    }
  }
  @strategy_filevec_ = @ret_abs_file_paths_;
}


sub SanityCheckInputArguments
{
  if ( ! ( ( -d $base_dir ) || ( -f $base_dir ) ) )
  { 
    print STDERR "$base_dir isn't a file or directory";
    exit( 0 );
  }

  if ( ! ( $trading_start_yyyymmdd_ ) )
  {
    print STDERR "TRADING_START_YYYYMMDD missing\n";
    exit ( 0 );
  }

  if ( ! ( ValidDate ( $trading_start_yyyymmdd_ ) ) )
  {
    print STDERR "TRADING_START_YYYYMMDD not Valid\n";
    exit ( 0 );
  }

  if ( ! ( $trading_end_yyyymmdd_ ) )
  {
    print STDERR "TRADING_END_YYYYMMDD missing\n";
    exit ( 0 );
  }

  if ( ! ( ValidDate ( $trading_end_yyyymmdd_ ) ) )
  {
    print STDERR "TRADING_END_YYYYMMDD not Valid\n";
    exit ( 0 );
  }

}
