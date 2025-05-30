#!/usr/bin/perl

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$BIN_DIR;
$LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin" if $USER eq "dvctrader";

my $BASETRADEINFODIR="/spare/local/tradeinfo/";
my $REGFILE_BASE_DIR=$BASETRADEINFODIR."/SupervisedPortInfo";

my $SPARE_HOME="/spare/local/".$USER."/";
my $SPARE_REG_DIR=$SPARE_HOME."supervised_portinfo_data";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";

my $LRDBFILE_BASE_DIR =  $BASETRADEINFODIR . "NewLRDBBaseDir";
my $EXCHANGE_SESSIONS_FILE = $LRDBFILE_BASE_DIR . "/exchange_hours_file.txt";

if ( ! -d $SPARE_REG_DIR ) { `mkdir -p $SPARE_REG_DIR`; }

require "$GENPERLLIB_DIR/valid_date.pl";                 # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl";            # IsDateHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl";            # SkipWeirdDate
require "$GENPERLLIB_DIR/no_data_date.pl";               # NoDataDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl";# CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/check_ilist_data.pl";           # GetDateVecWithDataForAllShcs
require "$GENPERLLIB_DIR/lock_utils.pl";                     # for TakeLock, RemoveLock
require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl";    # for IsValidShc
require "$GENPERLLIB_DIR/sync_to_all_machines.pl";       # SyncToAllMachines
require "$GENPERLLIB_DIR/exists_with_size.pl";           # ExistsWithSize
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl";           # GetUTCHHMMStr
require "$GENPERLLIB_DIR/print_stacktrace.pl";           # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1

# start 
my $USAGE="$0 reg_portfolio_inputs_file output_filepath [seconds_to_compare] [end_date] recompute";

if ( $#ARGV < 1 ) 
{
  print "USAGE: $USAGE\n";
  exit ( 0 );
}

#my $reg_based_pairs_file_ = $REGFILE_BASE_DIR."/portfolio_inputs";
my $reg_based_pairs_file_ = $ARGV[0]; 

#my $FINAL_REG_FILE = $REGFILE_BASE_DIR."/reg_weights_".$yyyymmdd_;
my $FINAL_REG_FILE = $ARGV[1];

my $seconds_to_compare_ = 120; 
$seconds_to_compare_ = $ARGV[2] if $#ARGV >= 2;

my $yyyymmdd_=`date +%Y%m%d`; chomp ( $yyyymmdd_ );
$yyyymmdd_ = GetIsoDateFromStrMin1($ARGV[3]) if $#ARGV >= 3;

my $numdays_ = 20;

my $recompute =0;
$recompute = $ARGV[4] if $#ARGV >= 4;

my $remove_intermed_files_ = 1;

my $datagen_start_hhmm_utc_;
my $datagen_end_hhmm_utc_;

my $unique_gsm_id_ = `date +%N`; chomp($unique_gsm_id_);
my $work_dir_ = $SPARE_REG_DIR."/".$unique_gsm_id_;
while ( ExistsWithSize($work_dir_) ) {
  $unique_gsm_id_ = `date +%N`; chomp($unique_gsm_id_);
  $work_dir_ = $SPARE_REG_DIR."/".$unique_gsm_id_;
}
`rm -rf $work_dir_`;
`mkdir -p $work_dir_`;

my $log_file_ = $work_dir_."/main_log_file.txt";
print $log_file_. "\n";
open LOGFH, ">", $log_file_ or PrintStacktraceAndDie("cannot open the $log_file_\n");

if ( ! -e $reg_based_pairs_file_ )
{
    printf STDERR "ERROR: $reg_based_pairs_file_  does not exist \n";
    exit ( 1 );
}

my %port_to_const_list_map_ = (); # map from portfolio_shortcode to list of Indep shortcodes
my %time_to_port_map_ =();        # map from start_time^end_time to port_shortcode
my @time_vec = ();                # to store vec of start_time and end_time  

# get portfolios for which weights are already computed if recompute = 0
my @port_dep_to_skip = ();
GetPortfoliosToSkip( ) if $recompute == 0 && -e $FINAL_REG_FILE;

#load the portfolios from portfolio_inputs
LoadPortfolioInputFile( );

my $FINAL_REG_FILE_COPY = $work_dir_."/".basename($FINAL_REG_FILE);
my $datagen_end_yyyymmdd_ = CalcPrevWorkingDateMult ( $yyyymmdd_, 1 );

my @catted_timed_data_vec_ = ( );  # to store all datagen filenames

print LOGFH "Catted Regports Weights File: $FINAL_REG_FILE_COPY\n";

# run the get_weights file for each start time end time pair
foreach my $my_combined_time (keys  %time_to_port_map_) {
  print LOGFH "Generating RegPort Coeffs for $my_combined_time ..\n";

  my @const_vec_ = (); # vector of all constituents that we need to comute simple trend
  my @port_vec_  = (); # vector of all portfolios
  my %const_to_index_map_ = (); # map from constituent_shortcode to index at which it is in the list
  
  foreach my $portname_ ( @{$time_to_port_map_{$my_combined_time}} ) {
    push(@port_vec_,$portname_);
    
    my @my_constituent_vec_t = split ( /\^/, $port_to_const_list_map_{$portname_} );
    
# push dependent
    my $const_ = $my_constituent_vec_t[0];
    if ( ! ( exists $const_to_index_map_{$const_} ) ) {
      push ( @const_vec_, $const_);
      $const_to_index_map_{$const_} = $#const_vec_;
    }

# push indeps
    for ( my $i = 1; $i <= $#my_constituent_vec_t; $i += 2) {
      $const_ = $my_constituent_vec_t[$i];
      if ( ! ( exists $const_to_index_map_{$const_} ) ) {
        push ( @const_vec_, $const_);
        $const_to_index_map_{$const_} = $#const_vec_;
      }
    }
  }
  if ( $#const_vec_ < 0 ) {
    printf STDERR "No constituents found\n";
    next;
  }

  my $date_vec_ref_ = GetDateVecWithDataForAllShcs( $datagen_end_yyyymmdd_, $numdays_, \@const_vec_ );

#generate only ilist_file with simple trend
  my $ilist_ = $work_dir_."/ilist_all_constituent_".$my_combined_time.".txt";
  print LOGFH "Generating Ilist file for $my_combined_time: $ilist_\n";
  GenerateIlistFile($ilist_, \@const_vec_);
  if ( ! -s $ilist_) {
    PrintStacktraceAndDie ( "$ilist_ ilist is not present!" );
  }

  my @time_vec = split(/\^/,$my_combined_time);
  my ($start_time_, $end_time_) = @time_vec[0..1];
  
  my $timed_data_catted_ = $work_dir_."/tdata_Portfolio_All_out.datagen.".$my_combined_time.".txt";
  print LOGFH "Generating Catted TimedData $timed_data_catted_\n";
  GenerateTimedData($ilist_, $start_time_, $end_time_, $date_vec_ref_, $timed_data_catted_);

  my $reg_constituents_index_filename_ = $work_dir_."/reg_constituents_index.txt";
  print LOGFH "Generating Index File $reg_constituents_index_filename_\n";
  GenerateIndexFile(\@port_vec_, $reg_constituents_index_filename_, $timed_data_catted_,\%const_to_index_map_);
 
  my $weights_file_ = $FINAL_REG_FILE_COPY."_".$my_combined_time; 
  print LOGFH "Generating RegPort Weights for $my_combined_time: $weights_file_\n";
  CreateWeightsFile($reg_constituents_index_filename_, $timed_data_catted_, $weights_file_);

  push ( @catted_timed_data_vec_, $timed_data_catted_ );
  `cat $weights_file_ >> $FINAL_REG_FILE_COPY`;
}

print LOGFH "Removing Catted TimedData files\n";
foreach my $tfile_ ( @catted_timed_data_vec_ ) {
  if ( -e $tfile_ ) {
    `rm -f $tfile_ 2>/dev/null`;
  }
}

if (-e $FINAL_REG_FILE_COPY){
  if ($recompute == 0) {
    print LOGFH "Appending $FINAL_REG_FILE_COPY to $FINAL_REG_FILE\n";
    `cat $FINAL_REG_FILE_COPY >> $FINAL_REG_FILE`;
    `rm $FINAL_REG_FILE_COPY`;
  } else {
    print LOGFH "Moving $FINAL_REG_FILE_COPY to $FINAL_REG_FILE\n";
    `mv $FINAL_REG_FILE_COPY  $FINAL_REG_FILE`;
  }
}
close(LOGFH);

sub GetPortfoliosToSkip {
  open(final_reg_file_,"< $FINAL_REG_FILE ") or PrintStacktraceAndDie ( "Could not open $FINAL_REG_FILE\n" );
  while ( my $reg_weights_line = <final_reg_file_> )
  {
    chomp( $reg_weights_line);
    my @reg_weights_words = split( ' ' , $reg_weights_line );
    if ( ($#reg_weights_words >= 2) && ($reg_weights_words[0] eq "PWEIGHTS"))
    {
      my $port_dep_shortcode = $reg_weights_words[1];
      push(@port_dep_to_skip , $port_dep_shortcode);
    }
  }
}

sub LoadPortfolioInputFile
{
#open the portfolio file
  open(REG_PAIRS_FILE, "< $reg_based_pairs_file_ ") or PrintStacktraceAndDie ( "Could not open $reg_based_pairs_file_\n" );

#first collect shortcodes of interest to process only once all the things

  while ( my $reg_based_pairs_file_line_ = <REG_PAIRS_FILE> )
  {
    chomp ( $reg_based_pairs_file_line_ );
    my @pcf_words_ = split( ' ', $reg_based_pairs_file_line_ );
    if ( ($#pcf_words_ >= 2 ) && ( $pcf_words_[0] eq "PLINE" ) ) 
    {  
      my $port_shortcode_ = $pcf_words_[1].":".$pcf_words_[2];
      next if ($port_shortcode_ ~~ @port_dep_to_skip);
      my $p_start_time = $pcf_words_[3];
      my $p_end_time   = $pcf_words_[4];
      my $combined_time = $p_start_time."^".$p_end_time;
      splice ( @pcf_words_, 0, 2 );
      splice ( @pcf_words_, 1, 2 );
#to insert continue for skip
      my $date_to_check = CalcPrevWorkingDateMult ( $yyyymmdd_, $numdays_ );
      next if ( ! IsValidShc($pcf_words_[0], $date_to_check));
      my $to_next = 0;
      for ( my $i = 1; $i <= $#pcf_words_; $i += 2) {
        if ( ! IsValidShc($pcf_words_[$i], $date_to_check)) { 
          $to_next = 1;
          last;
        }
      }
      next if ($to_next == 1);
      $port_to_const_list_map_{$port_shortcode_} = join ( '^', @pcf_words_ ) if ( ! ( exists $port_to_const_list_map_{$port_shortcode_}));
      push(@{$time_to_port_map_{$combined_time}},$port_shortcode_);
    }
  } 
  close REG_PAIRS_FILE ;
}

sub GenerateIlistFile
{
  my ($reg_out_ilist_filename_, $const_vec_ref_) = @_;
  open REG_ILIST_FILE, "> $reg_out_ilist_filename_ " or PrintStacktraceAndDie ( "Could not open $reg_out_ilist_filename_ for writting \n" );

  print REG_ILIST_FILE "MODELINIT DEPBASE NONAME\n";
  print REG_ILIST_FILE "MODELMATH LINEAR CHANGE\n";
  print REG_ILIST_FILE "INDICATORSTART\n";
  print REG_ILIST_FILE "INDICATOR 1.00 SimpleTrend $_ $seconds_to_compare_ MktSizeWPrice\n" foreach @$const_vec_ref_;
  print REG_ILIST_FILE "INDICATOREND\n";
  close REG_ILIST_FILE;
}

sub GenerateTimedData
{
  my ($ilist_, $start_time_, $end_time_, $date_vec_ref_, $timed_data_catted_) = @_;

# # Hardcoded...TODO take args later
  my $dg_msecs_ = 1000;
  my $dg_l1events_ = 0;
  my $dg_num_trades_ = 0;	
  my $print_eco_ = 0;

  foreach my $tradingdate_ ( @$date_vec_ref_ ) {
    my $timed_data_ = $timed_data_catted_."_date.".$tradingdate_;

    my $exec_cmd = "$LIVE_BIN_DIR/datagen $ilist_ $tradingdate_ $start_time_ $end_time_ $unique_gsm_id_ $timed_data_ $dg_msecs_ $dg_l1events_ $dg_num_trades_ $print_eco_ ADD_DBG_CODE -1";
    my @rep_matrix_text_ = `$exec_cmd 2>/dev/null`; 

    print LOGFH $exec_cmd."\n";
    if ( $#rep_matrix_text_ >= 0 ) {
      print LOGFH $_ foreach @rep_matrix_text_;
    }
    
    `cat $timed_data_ >> $timed_data_catted_`;

    if ( -e $timed_data_ ) {
      `rm -f $timed_data_ `;
    }
  }
}

sub GenerateIndexFile
{
  my ( $portvec_ref_, $reg_constituents_index_filename_, $timed_data_catted_, $const_to_index_map_ ) = @_;
  my $REG_CONSTITUENT_INDEX_FH;
  open ($REG_CONSTITUENT_INDEX_FH, "> $reg_constituents_index_filename_");

#Now we have all the datagen, we will make separate files for each portfolio i.e their corresponding datagen 
  foreach my $port_ ( @$portvec_ref_ ) {
    if ( -e $timed_data_catted_ ) {
      my @my_constituent_vec = split ( /\^/, $port_to_const_list_map_{$port_} );

      next if ! exists $$const_to_index_map_{$my_constituent_vec[0]};

      my $indexline_ = "$port_ ";

#Get index of each constuent
      my $dep_index =  $$const_to_index_map_ {$my_constituent_vec[0]} + 2;
      $indexline_ .= "$dep_index";

      for ( my $ii = 1; $ii <= $#my_constituent_vec; $ii += 2 ) {
        if ( exists ( $$const_to_index_map_ {$my_constituent_vec[$ii]} ) ) {
          my $const_indx = $$const_to_index_map_ {$my_constituent_vec[$ii]} + 2;
          $indexline_ .= " $const_indx ".$my_constituent_vec[$ii+1];
        }
        else {
          $indexline_ = "";
          last;
        }
      }
      if ( $indexline_ ne "" ) { 
        print $REG_CONSTITUENT_INDEX_FH "$indexline_\n";
      }
    }
  }
  close( $REG_CONSTITUENT_INDEX_FH);
}

sub CreateWeightsFile {
  my ($reg_constituents_index_filename_, $timed_data_catted_, $weights_file_) = @_;

  my $reg_gen = "$SCRIPTS_DIR/get_regport_coeffs.py  $reg_constituents_index_filename_ $timed_data_catted_ $weights_file_" ;
  my @reg_out_lines_ = `$reg_gen 2>/dev/null`;

  print LOGFH $reg_gen."\n";
  if ( $#reg_out_lines_ >= 0 ) {
    print LOGFH $_ foreach @reg_out_lines_;
  }
}

