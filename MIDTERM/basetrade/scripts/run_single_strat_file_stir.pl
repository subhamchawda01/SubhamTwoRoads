#!/usr/bin/perl

# \file ModelScripts/run_single_strat_file.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;
use POSIX;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO = "basetrade";

my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib/";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "$GENPERLLIB_DIR/get_files_pending_sim.pl"; # GetFilesPendingSimAndPnlSamplesFromShcDateDir
require "$GENPERLLIB_DIR/get_seconds_to_prep_for_shortcode.pl"; #GetDataStartTimeForStrat
require "$GENPERLLIB_DIR/get_unique_sim_id_from_cat_file.pl"; # GetUniqueSimIdFromCatFile
require "$GENPERLLIB_DIR/get_unique_sim_id_from_stir_cat_file.pl"; #GetUniqueSimIdFromStirCatFile

my $TRADELOG_DIR = "/spare/local/logs/tradelogs/";
Exceptions();

# start
my $USAGE="$0 shortcode input_strat_list_filename tradingdate_ temp_strategy_list_filenames_ [WORK_DIR] [GLOBALRESULTSDBDIR|DB] [GLOBALPNLSAMPLESDIR|INVALIDDIR] [MKT_MODEL|DEF=DEF] [is_pair_strategy|-1] [custom sim_strategy]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $input_strat_list_filename_ = $ARGV [ 1 ];
my $tradingdate_ = $ARGV [ 2 ];
my $is_pair_strategy_ = -1;
my $MKT_MODEL = "DEF";
my $temp_strategy_list_filenames_ = $ARGV[3];
my $SIM_STRATEGY_EXEC = $LIVE_BIN_DIR."/sim_strategy";


my $GLOBALRESULTSDBDIR = "DB";
my $GLOBALPNLSAMPLESDIR = "INVALIDDIR";
if ( $USER ne "dvctrader" )
{
  $GLOBALRESULTSDBDIR = "DEF_DIR";
}
my $this_day_work_dir_;

if ( $#ARGV >= 4 ) { $this_day_work_dir_ = $ARGV[4]; }
if ( $#ARGV >= 5 ) { $GLOBALRESULTSDBDIR = $ARGV[5]; }
if ( $#ARGV >= 6 ) { $GLOBALPNLSAMPLESDIR = $ARGV[6]; }


if ( $#ARGV >= 7 ) { $MKT_MODEL = $ARGV[7]; }
if ( $#ARGV >= 8 ) { $is_pair_strategy_ = $ARGV[8]; }
if ( $#ARGV >= 9 ) { $SIM_STRATEGY_EXEC = $ARGV[9]; }

my $is_db_ = int($GLOBALRESULTSDBDIR eq "DB") ;


my $debug_ = 1;
my $market_model_index_ = 0;
if ( $MKT_MODEL eq "DEF" )
{
  $MKT_MODEL = GetMarketModelForShortcode ( $shortcode_ );
}

if ( $MKT_MODEL >= 0 )
{
  $market_model_index_ = $MKT_MODEL ;
}

if ( IsDateHoliday ( $tradingdate_ )   ) {
  print "$tradingdate_ is Holiday!";
  exit 0;
}                

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);
my $pnl_stats_command = "$MODELSCRIPTS_DIR/get_pnl_stats_2.pl $this_tradesfilename_ SX";
my $compute_individual_product_stats_ = 0;
my $exec_cmd = "";

if ( $is_pair_strategy_ > 0 ) {
  $pnl_stats_command = "$MODELSCRIPTS_DIR/get_pnl_stats_stir_2.pl $this_tradesfilename_";
  if ( $is_pair_strategy_ > 2 ) {
    $compute_individual_product_stats_ = 1;
  }
}

my $temp_output_log = $TRADELOG_DIR."/output_log.".$tradingdate_.".".int($unique_gsm_id_);
my @strategy_filevec_ = ($input_strat_list_filename_);
my @this_day_strategy_filevec_ = ();


GetFilesPendingSimFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, $GLOBALRESULTSDBDIR );   
my $sim_with_no_db_results = @this_day_strategy_filevec_;
if ( $sim_with_no_db_results  == 0 ) {
  print "DB result exists! \n";
  exit ( 0 );
}

my $data_start_time_ = 0.0;
my $USE_DEFAULT_START_TIME = 0;
if ( $USE_DEFAULT_START_TIME == 0) {
  $data_start_time_ = GetDataStartTimeForStrat( $input_strat_list_filename_, $tradingdate_ );
}
my $sim_cmd_ = "$SIM_STRATEGY_EXEC SIM $input_strat_list_filename_ $unique_gsm_id_ $tradingdate_ $market_model_index_ 0 $data_start_time_ 0 ADD_DBG_CODE -1 2>&1";
# This line was printed by sim_strategy (stdout/stderr)

my @exec_output_lines_=`$sim_cmd_`;
my @pnlstats_output_lines_=`$pnl_stats_command 2>&1`; chomp ( @pnlstats_output_lines_ );

my %unique_id_to_pnlstats_map_ = ( );
my %prod_to_unique_id_pnlstats_map_= ( );
if ( ! defined $this_day_work_dir_ ) {
  $this_day_work_dir_ = $SPARE_HOME."RS/".$shortcode_."/".$tradingdate_;
}
$this_day_work_dir_ .= "/".$unique_gsm_id_;
if ( -d $this_day_work_dir_ ) { `rm -rf $this_day_work_dir_`; }
`mkdir -p $this_day_work_dir_`;


my $main_log_file_ = $this_day_work_dir_."/main_log_file.txt";
#print "Log file:  $main_log_file_ \n";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);


for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
{
  my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
  if( $#rwords_ >= 1 )
  {
    my $unique_sim_id_ = $rwords_[0];
    splice ( @rwords_, 0, 1 ); # remove the first two words since it is unique_sim_id_
        $unique_id_to_pnlstats_map_ { $unique_sim_id_ } = join ( ' ', @rwords_ );

    if ( $debug_ == 1 )
    {
      print $main_log_file_handle_ "For $unique_sim_id_ pnlstatsmappedto: ".$unique_id_to_pnlstats_map_ { $unique_sim_id_ }."\n";
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


my $temp_results_list_file_ = $this_day_work_dir_."/temp_results_list_file_".$tradingdate_.".txt" ;
#print "$temp_results_list_file_  \n";
my @list_of_unique_ids_in_TRLF = ( );
open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );

my $psindex_ = 0;
foreach my $outline_ ( @exec_output_lines_ )
{
  if ( $outline_ =~ /SIMRESULT/ )
  { # SIMRESULT pnl volume                                                                                                                                                                                               
    my @rwords_ = split ( ' ', $outline_ );
    if ( $#rwords_ >= 2 ) 
    {
      splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc                                                                                     
          my $remaining_simresult_line_ = join ( ' ', @rwords_ );
      my $unique_sim_id_ = GetUniqueSimIdFromStirCatFile( $input_strat_list_filename_, $psindex_ );
      if ( ! exists $unique_id_to_pnlstats_map_ { $unique_sim_id_ } )
      {
        $unique_id_to_pnlstats_map_ { $unique_sim_id_ } = "0 0 0 0 0 0 0 0 0 0 0 0 0";
      }
      printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_ { $unique_sim_id_ }, $unique_sim_id_  ;
      printf TRLF "%s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_ { $unique_sim_id_ };
      push ( @list_of_unique_ids_in_TRLF , $unique_sim_id_ );

      if ( $debug_ == 1 )
      {
        printf $main_log_file_handle_ "Adding to TRLF %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_ { $unique_sim_id_ };
      }
    }
    else
    {
      PrintStacktraceAndDie ( "SIMRESULT line has less than 3 words\n" ); 
    }
    $psindex_++;
  }
}
close TRLF;

#my $temp_strategy_list_filenames_ = $this_day_work_dir_."/temp_strategy_list_file_".$tradingdate_.".txt" ;
#open TSLF, "> $temp_strategy_list_filenames_" or PrintStacktraceAndDie ( "Could not open $temp_strategy_list_filenames_ for writing\n" );
#printf TSLF "%s \n", $input_strat_list_filename_;
#close TSLF;

#Consider result only if the run was fine
if ( ( ExistsWithSize ( $temp_results_list_file_ ) ) &&
    ( ExistsWithSize ( $temp_strategy_list_filenames_ ) ) )
{
  my $results_database_dir_ = $is_db_ ? "INVALIDDIR" : $GLOBALRESULTSDBDIR."/".$shortcode_;

  my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_global_database.pl $temp_strategy_list_filenames_ $temp_results_list_file_ $tradingdate_ $results_database_dir_ $is_db_"; 

  my $output_results_ = `$exec_cmd`;

  if ( $debug_ == 1 ) 
  { 
    print $main_log_file_handle_ "$exec_cmd\n"; 
    print $main_log_file_handle_ "Output: $output_results_\n"; 
  }

  if ( $output_results_ =~ /FAILED/ )
  {
# do not rm these files
    print STDERR "AR2GDb.pl FAILED on $tradingdate_. output :\n $output_results_\n";
    if ( $debug_ == 1 )
    {
      print $main_log_file_handle_ "AR2GDb.pl FAILED on $tradingdate_. output :\n $output_results_\n";
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
  else
  {
# probably not needed after this
#	print "$input_strat_list_filename_ $temp_strategy_list_filenames_ $this_tradesfilename_ \n";
#	`rm -f $input_strat_list_filename_`;
#	`rm -f $temp_strategy_list_filenames_`;
#	`rm -f $this_tradesfilename_`;
  }
}
else
{
  if ( ! ExistsWithSize ( $temp_strategy_list_filenames_ ) )
  {
    print STDERR "StrategyListFile $temp_strategy_list_filenames_ missing\n" ;
    if ( $debug_ == 1 )
    { 
      print $main_log_file_handle_ "StrategyListFile $temp_strategy_list_filenames_ missing\n" ;
    }
  }
  else
  {
    if ( ! ExistsWithSize ( $temp_results_list_file_ ) )
    {
      print STDERR "Date: $tradingdate_ ResultsFile $temp_results_list_file_ missing for: $input_strat_list_filename_ listfile: $temp_strategy_list_filenames_ \n" ;
      if ( $debug_ == 1 )
      {
        print $main_log_file_handle_ "Date: $tradingdate_ ResultsFile $temp_results_list_file_ missing for: $input_strat_list_filename_  listfile: $temp_strategy_list_filenames_ \n" ;
      }
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
}

if ( $compute_individual_product_stats_ == 1)
{
  my $results_database_dir_ = $is_db_ ? "INVALIDDIR" : $GLOBALRESULTSDBDIR."/".$shortcode_;
  foreach my $prod_ ( keys %prod_to_unique_id_pnlstats_map_) 
  {
    my %id_to_stats_map_ = %{$prod_to_unique_id_pnlstats_map_{$prod_}};
#TODO: Check why this list needed
    my $temp_strategy_list_file_index_ = `date +%N`;
    chomp($temp_strategy_list_file_index_);

    my $temp_results_list_file_ = $this_day_work_dir_."/temp_results_list_file_".$prod_.$tradingdate_."_".$temp_strategy_list_file_index_.".txt" ;
    my @list_of_unique_ids_in_TRLF = ( );
    open TRLF, "> $temp_results_list_file_" or PrintStacktraceAndDie ( "Could not open $temp_results_list_file_ for writing\n" );
    foreach my $id1_ ( keys %id_to_stats_map_ )
    {
      printf TRLF "%s\n",$id_to_stats_map_{$id1_};
      push ( @list_of_unique_ids_in_TRLF , $id1_ );
      if ( $debug_ == 1 )
      {
        printf $main_log_file_handle_ "Adding to TRLF %s\n",$id_to_stats_map_{$id1_};
      }								
    }
    close TRLF;
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
      if ( defined $exch_line_ && $exch_line_ ne "" ) {
        my $this_prod_list_filenames_ = $temp_strategy_list_filenames_."_".$prod_;
        my $this_prod_list_statistics_ = $temp_strategy_list_filenames_."_".$prod_."_result";
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

        my $results_database_dir_ = $is_db_ ? "INVALIDDIR" : $GLOBALRESULTSDBDIR."/".$prod_;
        my $add_to_SQLDB_ = $is_db_ ? $shc_nse_ : 0; #passing shortcode when inserting into DB
            my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_global_database.pl $this_prod_list_filenames_ $this_prod_list_statistics_ $tradingdate_ $results_database_dir_ $add_to_SQLDB_";
        my $output_ = `$exec_cmd`;
        `rm -f $this_prod_list_filenames_`;
        `rm -f $this_prod_list_statistics_`;
        if ( $debug_ == 1 )
        { 
          print $main_log_file_handle_ "$exec_cmd\n"; 
          print $main_log_file_handle_ "Output: $output_\n";
        }
      }
    }
    else {
      print "Error: $temp_results_list_file_ $temp_strategy_list_filenames_ files not present/empty \n";
    }
  }
}

##subs
sub Exceptions {
  if ( $USER ne "dvctrader" )
  {
    $LIVE_BIN_DIR = $BIN_DIR;
  }
}


