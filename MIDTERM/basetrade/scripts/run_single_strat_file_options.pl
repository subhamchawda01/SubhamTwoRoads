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
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_files_pending_sim.pl"; # GetFilesPendingSimAndPnlSamplesFromShcDateDir
require "$GENPERLLIB_DIR/get_seconds_to_prep_for_shortcode.pl"; #GetDataStartTimeForStrat
require "$GENPERLLIB_DIR/get_unique_sim_id_from_options_cat_file.pl"; # GetUniqueSimIdFromOptionsCatFile

my $TRADELOG_DIR = "/spare/local/logs/tradelogs/";
Exceptions();

# start
my $USAGE="$0 shortcode input_strat_file tradingdate_ temp_strategy_list_filenames_ [GLOBALRESULTSDBDIR|DB] [GLOBALPNLSAMPLESDIR|INVALIDDIR] [SPECIFIC_SHC=NONE]";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }

my $shortcode_ = $ARGV [ 0 ];
my $input_strat_file_ = $ARGV [ 1 ];
my $tradingdate_ = $ARGV [ 2 ];
my $is_pair_strategy_ = -1;
my $MKT_MODEL = "DEF";
my $temp_strategy_list_filenames_ = $ARGV[3];
my $SIM_STRATEGY_EXEC = $BIN_DIR."/sim_strategy";

my $GLOBALRESULTSDBDIR = "DB";
my $GLOBALPNLSAMPLESDIR = "INVALIDDIR";
if ( $USER ne "dvctrader" )
{
	$GLOBALRESULTSDBDIR = "DEF_DIR";
}

if ( $#ARGV >= 4 ) { $GLOBALRESULTSDBDIR = $ARGV[4]; } 
if ( $#ARGV >= 5 ) { $GLOBALPNLSAMPLESDIR = $ARGV[5]; }

my $debug_ = 1;
my $is_db_ = int($GLOBALRESULTSDBDIR eq "DB") ;
my $dump_pnl_samples_into_dir_ = int($GLOBALPNLSAMPLESDIR ne "INVALIDDIR") ;
my $check_pnl_samples_ = int($is_db_ || $dump_pnl_samples_into_dir_); #check for pnl_samples only if they will be dumped into db or dir
my $is_specific_shc_ = 0;
my $specific_shc_ = "NONE";

if (( $#ARGV >= 6 ) && ! ($ARGV[6] eq "NONE"))
{
  $is_specific_shc_ = 1;
  $specific_shc_ = $ARGV[6]; 
}

my $reference_shc_ = "NSE_NIFTY_FUT0";

if(($shortcode_ == "CURStrats") || (index($shortcode_, "USDINR") != -1) )
{
  $reference_shc_ = "NSE_USDINR_FUT0";
}

if ( IsDateHoliday ( $tradingdate_ ) || ( IsProductHoliday ( $tradingdate_, $reference_shc_ ) ) ) {
  print "$tradingdate_ is Holiday!";
  exit 0;
}                

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ );
my $this_tradesfilename_ = $TRADELOG_DIR."/trades.".$tradingdate_.".".int($unique_gsm_id_);

my $temp_output_log = $TRADELOG_DIR."/output_log.".$tradingdate_.".".int($unique_gsm_id_);
my @strategy_filevec_ = ($input_strat_file_);
my @this_day_strategy_filevec_ = ();

GetFilesPendingSimFromShcDateDir ( $shortcode_, $tradingdate_, \@strategy_filevec_ , \@this_day_strategy_filevec_, $GLOBALRESULTSDBDIR );   
my $sim_with_no_db_results = @this_day_strategy_filevec_;
if ( $sim_with_no_db_results  == 0 ) {
	print "DB result exists! \n";
	exit ( 0 );
}

my $pnl_samples_command = "$MODELSCRIPTS_DIR/get_pnl_stats_options.pl $this_tradesfilename_ P 2>&1";
my @pnlsamples_output_lines_=""; 

my $this_day_work_dir_ = $SPARE_HOME."RS/".$shortcode_."/".$tradingdate_."/".$unique_gsm_id_;
if ( -d $this_day_work_dir_ ) { `rm -rf $this_day_work_dir_`; }
	`mkdir -p $this_day_work_dir_`;

my $main_log_file_ = $this_day_work_dir_."/main_log_file.txt";
print  $this_day_work_dir_."\n";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);

my @sim_strategy_output_lines_=(); # stored to seive out the SIMRESULT lines
my @pnlstats_output_lines_=(); # stored to seive out the SIMRESULT lines
my %unique_id_to_pnlstats_map_=(); # stored to write extended results to the database
my %prod_to_unique_id_pnlstats_map_=();
my %prod_to_results_file_map_=();
my %prod_to_pnlsamples_file_map_=();
my %prod_to_stratlist_file_map_=();
my %unique_id_to_trades_filename_ = ( ); # to store trades.yyyymmdd.strat_filename on S3
{
  my $exec_cmd="$BIN_DIR/sim_strategy_options SIM $input_strat_file_ $unique_gsm_id_ $tradingdate_ ADD_DBG_CODE -1 2>/dev/null"; 
  @sim_strategy_output_lines_=`$exec_cmd` ;
  $exec_cmd="$MODELSCRIPTS_DIR/get_pnl_stats_options.pl $this_tradesfilename_ X 2>&1"; 
  @pnlstats_output_lines_=`$exec_cmd` ;
  if ( $debug_ == 1 ) 
  { 
    print $main_log_file_handle_ "$exec_cmd\n"; 
    print $main_log_file_handle_ "OutputLines: \n".join ( "", @sim_strategy_output_lines_ )."\n";
  }
  if ( ExistsWithSize ( $this_tradesfilename_ ) )
  {

    #PNLSAMPLES 
    @pnlsamples_output_lines_ = `$pnl_samples_command`;
    chomp(@pnlsamples_output_lines_);
        
    { 
      `rm -f $this_tradesfilename_`; 
      my $this_logfilename_ = $TRADELOG_DIR."/log.".$tradingdate_.".".int($unique_gsm_id_);
      `rm -f $this_logfilename_`;
    }
  }
 
  my $all_results_list_file_ = $this_day_work_dir_."/temp_results_list_file_".$tradingdate_.".txt" ;
  my $all_strategy_list_filenames_ = $temp_strategy_list_filenames_."_ALL";

  if (-e $all_results_list_file_ ) { unlink $all_results_list_file_ ; }   
  if (-e $all_strategy_list_filenames_ ) { unlink $all_strategy_list_filenames_ ; }   

 
  for ( my $t_pnlstats_output_lines_index_ = 0, my $psindex_ = 0; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
  {
    my $suffix_ = "";
    my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );

    if (index($rwords_[1], "_DELTA") != -1)
    {
      $suffix_ = "_DELTA";
    }

    if (index($rwords_[1], "_VEGA") != -1)
    {
      $suffix_ = "_VEGA";
    }

    $rwords_[1] =~ s/$suffix_//g; 

    if ($rwords_[1] eq "ALL")
    { 
      open TPLF, ">> $all_strategy_list_filenames_" or PrintStacktraceAndDie ( "Could not open $all_strategy_list_filenames_ for writing\n" );
      my $strats_ = `cat $temp_strategy_list_filenames_`;
      my @strat_array_ = split '\n', $strats_;

      for( my $i = 0; $i<=$#strat_array_; $i ++ )
      {
        my $temp_strat_ = $strat_array_[$i];
        chomp( $temp_strat_ );
        printf TPLF $temp_strat_.$suffix_."\n";
      }
      close TPLF;
      if ( $#rwords_ >= 2 ) 
      {
        open TRLF, ">> $all_results_list_file_" or PrintStacktraceAndDie ( "Could not open $all_results_list_file_ for writing\n" );               
        splice ( @rwords_, 0, 2 ); # remove the first two words
        my $remaining_pnlstats_line_ = join ( ' ', @rwords_ );
        printf TRLF "%s\n",$remaining_pnlstats_line_;

        if ( $debug_ == 1 )
        {
          printf $main_log_file_handle_ "Adding to ALL TRLF %s\n",$remaining_pnlstats_line_;
        }
      }
      else
      {
        PrintStacktraceAndDie ( "PNLStats line has less than 3 words\n" ); 
      }
      $psindex_ ++;
    }
     
    else {
      # adds the results to respective constituent products
      my $prod_ = $rwords_[1];
      
      if(($is_specific_shc_ == 1) && ((index($prod_, $specific_shc_) == -1))) 
      {
        next;   
      }

      if(! exists $prod_to_stratlist_file_map_{$prod_} )
      {
        $prod_to_stratlist_file_map_{$prod_} = $temp_strategy_list_filenames_."_".$prod_;
        $prod_to_results_file_map_{$prod_}  = $temp_strategy_list_filenames_."_".$prod_."_result";
        $prod_to_pnlsamples_file_map_{$prod_}  = $temp_strategy_list_filenames_."_".$prod_."_pnlsamples";
        if (-e $prod_to_stratlist_file_map_{$prod_} ) { unlink $prod_to_stratlist_file_map_{$prod_} ; }
        if (-e $prod_to_results_file_map_{$prod_} ) { unlink $prod_to_results_file_map_{$prod_} ; }
        if (-e $prod_to_pnlsamples_file_map_{$prod_} ) { unlink $prod_to_pnlsamples_file_map_{$prod_} ; }
      }

      my $this_prod_to_stratlist_file_ = $prod_to_stratlist_file_map_{$prod_};
      open TPLF, ">> $this_prod_to_stratlist_file_" or PrintStacktraceAndDie ( "Could not open $this_prod_to_stratlist_file_ for writing\n" );

      my $strats_ = `cat $temp_strategy_list_filenames_`;
      my @strat_array_ = split '\n', $strats_;

      for( my $i = 0; $i<=$#strat_array_; $i ++ )
      {
        my $temp_strat_ = $strat_array_[$i];
        chomp( $temp_strat_ );
        printf TPLF $temp_strat_."_".$prod_.$suffix_."\n";
      }
      close TPLF;
      
      my $this_results_list_file_ = $prod_to_results_file_map_{$prod_}  ;
      my $this_strategy_list_filenames_ =  $prod_to_stratlist_file_map_{$prod_};
      open TRLF, ">> $this_results_list_file_" or PrintStacktraceAndDie ( "Could not open $this_results_list_file_ for writing\n" );               
      splice ( @rwords_, 0, 2); # remove the first two words
      my $remaining_pnlstats_line_ = join ( ' ', @rwords_ );

      if ( $#rwords_ >= 2 ) 
      {
        printf TRLF "%s\n",$remaining_pnlstats_line_;
        if ( $debug_ == 1 )
        {
          printf $main_log_file_handle_ "Adding to %s TRLF %s\n",$prod_,$remaining_pnlstats_line_;
        }
      }
      else
      {
        PrintStacktraceAndDie ( "Output line has less than 3 words : $pnlstats_output_lines_[$t_pnlstats_output_lines_index_]\n" ); 
      } 
    }
    close TRLF;
  }

  foreach my $pnlsample_line_ ( @pnlsamples_output_lines_ )
  {
    my @rwords_ = split ( ' ', $pnlsample_line_ );
    if ( $#rwords_ >= 2 )
    {
      if ($rwords_[0] eq "ALL")
      {
        my $this_pnlsamples_list_file_ = $this_day_work_dir_."/temp_pnl_samples_list_file_".$tradingdate_.".txt";
        open PNLSAMPLES, "> $this_pnlsamples_list_file_" or PrintStacktraceAndDie ( "Could not open $this_pnlsamples_list_file_ for writing\n" );
        splice ( @rwords_, 0, 1 ); # remove the first word since it is shc_
        # Writing three times because we have delta and vega strats name also now in file
        print PNLSAMPLES join ( ' ', @rwords_ )."\n";
        print PNLSAMPLES join ( ' ', @rwords_ )."\n";
        print PNLSAMPLES join ( ' ', @rwords_ )."\n";
        close PNLSAMPLES;
      }
      else 
      {
        if(($is_specific_shc_ == 1) && ((index($rwords_[0], $specific_shc_) == -1))) 
        {
          next;   
        }
        my $this_pnlsamples_list_file_ = $prod_to_pnlsamples_file_map_{$rwords_[0]};
        open PNLSAMPLES, "> $this_pnlsamples_list_file_" or PrintStacktraceAndDie ( "Could not open $this_pnlsamples_list_file_ for writing\n" );
        splice ( @rwords_, 0, 1 ); # remove the first word since it is shc_
        print PNLSAMPLES join ( ' ', @rwords_ )."\n";
        print PNLSAMPLES join ( ' ', @rwords_ )."\n";
        print PNLSAMPLES join ( ' ', @rwords_ )."\n";
        close PNLSAMPLES;
      }
    }
  }
}

# Adding global results 

my $temp_results_list_file_= $this_day_work_dir_."/temp_results_list_file_".$tradingdate_.".txt" ;
my $temp_pnlsamples_list_file_ = $this_day_work_dir_."/temp_pnl_samples_list_file_".$tradingdate_.".txt";
my $this_strategy_list_filenames_ = $temp_strategy_list_filenames_."_ALL";
if ( ( ExistsWithSize ( $temp_results_list_file_ ) ) && ( ExistsWithSize ( $this_strategy_list_filenames_ ) ) && ( ExistsWithSize ( $temp_pnlsamples_list_file_ ) ) )
{
  my $results_database_dir_ = $is_db_ ? "INVALIDDIR" : $GLOBALRESULTSDBDIR."/".$shortcode_; 
  my $add_to_SQLDB_ = $is_db_ ? $shortcode_ : 0; #passing shortcode when inserting into DB
  my $pnlsamples_database_dir_ = !$dump_pnl_samples_into_dir_ ? "INVALIDDIR" : $GLOBALPNLSAMPLESDIR."/".$shortcode_;
  my $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_global_database.pl $this_strategy_list_filenames_ $temp_results_list_file_ $tradingdate_ $results_database_dir_ $add_to_SQLDB_ $temp_pnlsamples_list_file_ $pnlsamples_database_dir_"; 
  my $output_ = `$exec_cmd`;
  if ( $debug_ == 1 ) 
  { 
    print $main_log_file_handle_ "$exec_cmd\n"; 
    print $main_log_file_handle_ "Output: $output_\n"; 
  }
  if ( $output_ =~ /FAILED/ )
  { 
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
    print STDERR "Date: $tradingdate_ ResultsFile $temp_results_list_file_ missing for catfile: $input_strat_file_ listfile: $temp_strategy_list_filenames_ \n" ;
    if ( $debug_ == 1 )
    {
      print $main_log_file_handle_ "Date: $tradingdate_ ResultsFile $temp_results_list_file_ missing for catfile: $input_strat_file_ listfile: $temp_strategy_list_filenames_ \n" ;
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


#Adding individual results

foreach my $prod_ ( keys %prod_to_results_file_map_ ) 
{
  my $this_results_list_file_ = $prod_to_results_file_map_{$prod_}  ;
  my $this_pnlsamples_list_file_ = $prod_to_pnlsamples_file_map_{$prod_}  ;
  my $this_strategy_list_filenames_ =  $prod_to_stratlist_file_map_{$prod_};
  if ( ( ExistsWithSize ( $this_results_list_file_ ) ) && ( ExistsWithSize ( $this_strategy_list_filenames_ ) ) )
  {
    my $results_database_dir_ = $is_db_ ? "INVALIDDIR" : $GLOBALRESULTSDBDIR."/".$prod_; 
    my $add_to_SQLDB_ = $is_db_ ? $prod_ : 0; #passing shortcode when inserting into DB
    my $pnlsamples_database_dir_ = !$dump_pnl_samples_into_dir_ ? "INVALIDDIR" : $GLOBALPNLSAMPLESDIR."/".$prod_;
    my $exec_cmd="";
    if ( ExistsWithSize ( $this_pnlsamples_list_file_ )  )
    {
      $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_global_database.pl $this_strategy_list_filenames_ $this_results_list_file_ $tradingdate_ $results_database_dir_ $add_to_SQLDB_ $this_pnlsamples_list_file_ $pnlsamples_database_dir_"; 
    } else 
    {
      $exec_cmd="$MODELSCRIPTS_DIR/add_results_to_global_database.pl $this_strategy_list_filenames_ $this_results_list_file_ $tradingdate_ $results_database_dir_ $add_to_SQLDB_";
    }

      my $output_ = `$exec_cmd`;
    if ( $debug_ == 1 ) 
    { 
      print $main_log_file_handle_ "$exec_cmd\n"; 
      print $main_log_file_handle_ "Output: $output_\n"; 
    }
    if ( $output_ =~ /FAILED/ )
    { 
      print STDERR "AR2GDb.pl FAILED on $tradingdate_. output :\n $output_\n";
      if ( $debug_ == 1 )
      {
        print $main_log_file_handle_ "AR2GDb.pl FAILED on $tradingdate_. output :\n $output_\n";
      }
      open THIS_STRATEGY_LIST_FILEHANDLE, "< $this_strategy_list_filenames_ " or PrintStacktraceAndDie ( "Could not open $this_strategy_list_filenames_\n" );
      my @this_strat_list_lines_ = <THIS_STRATEGY_LIST_FILEHANDLE>;
      chomp ( @this_strat_list_lines_ );
      close THIS_STRATEGY_LIST_FILEHANDLE ;

      for ( my $tslf_idx_ = 0 ; $tslf_idx_ <= $#this_strat_list_lines_ ; $tslf_idx_ ++ )
      {
        printf STDERR "AR2GDb.pl failed list file item : %s\n", $this_strat_list_lines_[$tslf_idx_] ;
        if ( $debug_ == 1 )
        {
          printf $main_log_file_handle_ "AR2GDb.pl failed list file item : %s\n", $this_strat_list_lines_[$tslf_idx_] ;
        }
      }
      exit ( 0 );
    }
    else {}
  }
  else
  {
    if ( ! ExistsWithSize ( $this_results_list_file_ ) )
    {
      print STDERR "Date: $tradingdate_ ResultsFile $this_results_list_file_ missing for catfile: $input_strat_file_ listfile: $this_strategy_list_filenames_ \n" ;
      if ( $debug_ == 1 )
      { 
        print $main_log_file_handle_ "Date: $tradingdate_ ResultsFile $this_results_list_file_ missing for catfile: $input_strat_file_ listfile: $this_strategy_list_filenames_ \n" ;
      }
      open THIS_STRATEGY_LIST_FILEHANDLE, "< $this_strategy_list_filenames_ " or PrintStacktraceAndDie ( "Could not open $this_strategy_list_filenames_\n" );
      my @this_strat_list_lines_ = <THIS_STRATEGY_LIST_FILEHANDLE>;
      chomp ( @this_strat_list_lines_ );
      close THIS_STRATEGY_LIST_FILEHANDLE ;

      for ( my $tslf_idx_ = 0 ; $tslf_idx_ <= $#this_strat_list_lines_ ; $tslf_idx_ ++ )
      {
        printf STDERR "failed list file item : %s\n", $this_strat_list_lines_[$tslf_idx_] ;
        if ( $debug_ == 1 )
        {
          printf $main_log_file_handle_ "failed list file item : %s\n", $this_strat_list_lines_[$tslf_idx_] ;
        }
      }
    }
    if ( ! ExistsWithSize ( $this_strategy_list_filenames_ ) )
    {  
      print STDERR "StrategyListFile $this_strategy_list_filenames_ missing\n" ;
      if ( $debug_ == 1 )
      { 
        print $main_log_file_handle_ "StrategyListFile $this_strategy_list_filenames_ missing\n" ;
      }
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


