#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;    # for basename and dirname
use sigtrap qw(handler signal_handler normal-signals error-signals);
use List::Util qw[min max sum];    # max , min

push (@INC, "/home/ec2-user/perl5/lib/perl5/x86_64-linux-thread-multi");

my $USER             = $ENV{'USER'};
my $HOME_DIR         = $ENV{'HOME'};
my $REPO             = "basetrade";
my $GENPERLLIB_DIR   = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";
my $SCRIPTS_DIR      = $HOME_DIR . "/" . $REPO . "_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR . "/" . $REPO . "/ModelScripts";
my $LIVE_BIN_DIR     = $HOME_DIR . "/LiveExec/bin";
if ( !( -d $LIVE_BIN_DIR ) && !( -e $LIVE_BIN_DIR ) ) { $LIVE_BIN_DIR = $HOME_DIR . "/" . $REPO . "_install/bin"; }

my $BASETRADEINFODIR       = "/spare/local/tradeinfo/";
my $SPARE_HOME="/spare/local/".$USER."/";
my $SPARE_LRDB_DIR=$SPARE_HOME."lrdbdata";
my $DATAGEN_LOGDIR="/spare/local/logs/datalogs/";
my $PORTFOLIO_INPUTS="/spare/local/tradeinfo/PCAInfo/portfolio_inputs";
my $SHARED_DIR="/media/shared/ephemeral16/LRDB_logs";

require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl";    # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/load_portfolio_names.pl";
require "$GENPERLLIB_DIR/get_kth_word.pl";
require "$GENPERLLIB_DIR/clear_temporary_files.pl";          # for ClearTemporaryFiles
require "$GENPERLLIB_DIR/sync_to_all_machines.pl";           # for SyncToAllMachines
require "$GENPERLLIB_DIR/lock_utils.pl";                     # for TakeLock, RemoveLock
require "$GENPERLLIB_DIR/print_stacktrace.pl";               # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/parallel_sim_utils.pl";
require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl";     # GetIsoDateFromStrMin1
#require "$GENPERLLIB_DIR/mail_utils.pl";                     # SendMail
require "$GENPERLLIB_DIR/check_ilist_data.pl";               # GetDateVecWithDataForAllShcs
require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl";        # IsValidShc GetExchFromSHC
require "$GENPERLLIB_DIR/get_port_constituents.pl";          # IsValidPort
require "$GENPERLLIB_DIR/string_utils.pl";                   # trim
require "$GENPERLLIB_DIR/di1_utils.pl";                      # ExpiredDIContract
require "$GENPERLLIB_DIR/date_utils.pl";                     # GetOverlapFraction, GetUTCTime
require "$GENPERLLIB_DIR/trade_time_utils.pl";               # GetShcTradeTimeUTC
require "$GENPERLLIB_DIR/gen_ind_utils.pl";                  # ShcHasExchSource
require "$GENPERLLIB_DIR/ec2_utils.pl";                      # IsEc2Worker
require "$GENPERLLIB_DIR/lrdb_db_access_manager.pl";

#Main code begins.

my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel();
my $hostname_ = `hostname`; chomp($hostname_);

# Mode = 0 : No files will be changed.
# Mode = 1 : If ALL ALL and no file for the month, then a new file will be created, otherwise the old file will be changed.
# Mode = 2 : A new file with the current date will be created but only when ALL-ALL is present

my $USAGE = "$0 date dep|ALL|filename indep|ALL|filename [TYPE(CHANGE|RETURNS)=CHANGE] [mode = 1] [recompute = 0]\n";
$USAGE = $USAGE . "# Mode = 0 : No files will be changed. New Values will be printed on stdout.\n";
$USAGE = $USAGE . "# Mode = 1 : Input date will be changes to 1st of specified month. Eg. 20150510 => 20150501. This is to keep only one dated file for each month.\n";
$USAGE = $USAGE . "# Mode = 2 : A new file with the specified date will be created. Should be used in special circumstances only.\n";

if ( $#ARGV < 2 ) {
  print $USAGE. "\n";
  exit(0);
}

#reading args and initializing
my $seconds_to_compare_ = 300;
my $yyyymmdd_           = GetIsoDateFromStrMin1(shift);
my $lookback_           = 60; 
my $input_dep_   = shift;    # if all is specified we use all the *indep pairs from the lrdb_pairs_ file
my $input_indep_ = shift;    # if all is specified we use all the dep* paris from the lrdb_pairs_ file

my $lrdb_type_str_    = "CHANGE";
my $mode_             = 1;
my $recompute_        = 0;
if (@ARGV) { $lrdb_type_str_ = shift; }

if (!($lrdb_type_str_ eq "RETURNS" || $lrdb_type_str_ eq "CHANGE")) {
  print $USAGE. "\n";
  print "Type should be CHANGE or RETURNS \n";
  exit(0);
}


if (@ARGV) { $mode_          = int(shift) + 0; }
if (@ARGV) { $recompute_     = int(shift) + 0; }

my $is_ret_lrdb_ = $lrdb_type_str_ eq "RETURNS" ? 1 : 0;

if ( $mode_ == 1 ) {
  #20150510 => 20150501, to avoid too many dated files.
  $yyyymmdd_ = int( int( $yyyymmdd_ / 100 ) * 100 + 1 );
}
my $datagen_end_yyyymmdd_ = CalcPrevWorkingDateMult( $yyyymmdd_, 1 );

my @exchanges_ = ();
FetchExchanges(\@exchanges_);

my @input_dep_vec_ = ();
if ( $input_dep_ ne "ALL" ) {
  if ( IsValidShc($input_dep_) ) {
    @input_dep_vec_ = ($input_dep_);
  }
  elsif ( $input_dep_ ~~ @exchanges_ ) {
    GetDepsForExchange($input_dep_, \@input_dep_vec_, $is_ret_lrdb_);
  }
  elsif ( ExistsWithSize($input_dep_) ) {
    #dependents specified as a file
    open DEPFILE, "< $input_dep_"
      or PrintStacktraceAndDie(
      "$input_dep_ is not a valid SHC, so interpreting it as a file but can't open $input_dep_ for reading\n");
    @input_dep_vec_ = <DEPFILE>;
    chomp(@input_dep_vec_);
    @input_dep_vec_ = map { trim($_) } @input_dep_vec_;
  }

  if ( !@input_dep_vec_ ) {
    print "invalid input_dep $input_dep_\n";
    exit(0);
  }
}

my @input_indep_vec_ = ();
if ( $input_indep_ ne "ALL" ) {
  if ( IsValidShc($input_indep_) || IsValidPort($input_indep_) ) {
    @input_indep_vec_ = ($input_indep_);
  }
  elsif ( $input_indep_ ~~ @exchanges_ ) {
    GetDepsForExchange($input_indep_, \@input_indep_vec_, $is_ret_lrdb_);
  }
  elsif ( ExistsWithSize($input_indep_) ) {
    #independents specified as a file
    open INDEPFILE, "< $input_indep_"
      or PrintStacktraceAndDie(
      "$input_indep_ is not a valid SHC/PORT, so interpreting it as a file but can't open $input_indep_ for reading\n");
    @input_indep_vec_ = <INDEPFILE>;
    chomp(@input_indep_vec_);
    @input_indep_vec_ = map { trim($_) } @input_indep_vec_;
  }

  if ( !@input_indep_vec_ ) {
    print "invalid input_indep $input_indep_\n";
    exit(0);
  }
}

my $skip_shc_in_comb_run_file_ = $BASETRADEINFODIR . "/NewLRDBBaseDir/skip_shc_in_comb_run.txt";
open SKIP_SHC_FILE, "< $skip_shc_in_comb_run_file_" or print "Can't open $skip_shc_in_comb_run_file_ for reading\n";
my @skip_shc_in_comb_run_vec_ = <SKIP_SHC_FILE>;
chomp(@skip_shc_in_comb_run_vec_);
close SKIP_SHC_FILE;

my $skip_exch_in_comb_run_file_ = $BASETRADEINFODIR . "/NewLRDBBaseDir/skip_exch_in_comb_run.txt";
open SKIP_EXCH_FILE, "< $skip_exch_in_comb_run_file_" or print "Can't open $skip_exch_in_comb_run_file_ for reading\n";
my @skip_exch_in_comb_run_vec_ = <SKIP_EXCH_FILE>;
chomp(@skip_exch_in_comb_run_vec_);
close SKIP_EXCH_FILE;

my $unique_gsm_id_ = `date +%N`;
chomp($unique_gsm_id_);
$unique_gsm_id_ = int($unique_gsm_id_) + 0;
my $work_dir_ = $SPARE_LRDB_DIR . "/" . $unique_gsm_id_;
while ( ExistsWithSize($work_dir_) ) {
  $unique_gsm_id_ = `date +%N`;
  chomp($unique_gsm_id_);
  $unique_gsm_id_ = int($unique_gsm_id_) + 0;
  $work_dir_      = $SPARE_LRDB_DIR . "/" . $unique_gsm_id_;
}
my $LOGFILE_DIR = $SHARED_DIR . "/" . $unique_gsm_id_;
my $shared_ilist_dir = $LOGFILE_DIR . "/ilists/";
`rm -rf $work_dir_`;
`mkdir -p $work_dir_`;
`rm -rf $LOGFILE_DIR`;
`mkdir -p $LOGFILE_DIR`;
`mkdir -p $shared_ilist_dir`;

my $log_file_ = $LOGFILE_DIR . "/main_log_file.txt";
my $log_file_pairs = $LOGFILE_DIR . "/lrdb_pairs.txt";
my $log_file_parallel_process = $work_dir_ . "/parallel_process.txt";

print $log_file_. "\n";
open LOGFH, ">", $log_file_ or PrintStacktraceAndDie("cannot open the $log_file_\n");
open LOGFHP, ">", $log_file_pairs or PrintStacktraceAndDie("cannot open the $log_file_pairs\n");
open LOGFHPA, ">", $log_file_parallel_process or PrintStacktraceAndDie("cannot open the $log_file_parallel_process\n");

#print LOGFH "seconds_to_compare = " . $seconds_to_compare_ . "\n";
#print LOGFH "date = " . $yyyymmdd_ . "\n";
#print LOGFH "datagen_end_date_ = " . $datagen_end_yyyymmdd_ . "\n";
#print LOGFH "lookback = " . $lookback_ . "\n";
#print LOGFH "dependent = " . $input_dep_ . "\n";
#print LOGFH "independent = " . $input_indep_ . "\n";
#print LOGFH "mode = " . $mode_ . "\n";
#print LOGFH "recompute = " . $recompute_ . "\n";

# For mails
my $mail_address_ = "nseall@tworoads.co.in";

my $lrdb_ilist_file_prefix_ = $work_dir_ . "/ilist_temp";    # /spare/local/dvctrader/lrdbdata/2334555/ilist_temp

my %exchange_sessions_               = ();
my %shortcode_start_end_to_l1events_ = ();

my $datagen_msecs_timeout_      = 10000;
my $datagen_l1events_timeout_   = 0;
my $datagen_num_trades_timeout_ = 0;
my $to_print_on_economic_times_ = 0;

my @temporary_files_ = ();

my %lrdb_cache_                      = ();
my %dep_indep_pairs_to_datagen_days_ = ();

#This is to keep filenames readable
my %actual_to_pseudo_name_ = ();

#done reading/initializing

my @input_dep_exchanges_ = @exchanges_;

if ( $input_dep_ ne "ALL" ) {
  my %dep_exchs_map_ = map { GetExchFromSHC($_) => 1 } @input_dep_vec_;
  @input_dep_exchanges_ = keys %dep_exchs_map_;
}

print LOGFH "Date " . $yyyymmdd_ . "\n\n";
my $summ_email_str_ = "Date " . $yyyymmdd_ . "\nLOGFILE " . $log_file_ . "\n\n\nExchange Session \#Successful_pairs \#Failed_pairs log_file \n";

my @mail_table_ = ();
my @header_ = ( "Exchange", "Session", "#Successful_pairs", "#Failed_pairs", "#Large_changes","log_file");
push( @mail_table_, \@header_ );


my $lrdb_dump_str_ = "";
my %dep_shortcodes_ = ();
my $big_change_count = 0;
my %total_dep_shortcodes = ();
my %exch_session_count = ();

my @indep_ilist_vec_ref_ = ( );
foreach my $this_exchange_ (@input_dep_exchanges_) {
  #print LOGFH "computing lrdb coefficients for exchange " . $this_exchange_ . "...\n";

  my $elog_file_ = $LOGFILE_DIR."/log_file_".$this_exchange_.".txt";
  open ELOGFH, "> $elog_file_" or PrintStacktraceAndDie( "Could not open $elog_file_ for writing" );
  my $exchange_info_str_ = "Exchange    Session      #Successful_pairs       #Failed_pairs       #Large_Changes\n";
  my $suc_failed_str_     = "dep_shortcode Start_end_time #succesful_pairs   #failed_pairs   #large_changes Log_file \n";
  my @sessions_ = ();
  GetExchangeSessions($this_exchange_, \@sessions_);
  $exch_session_count{$this_exchange_} = $#sessions_ + 1;
  my %sessionids_to_start_end_times_ = map { $$_[0] => $$_[1]."-".$$_[2] } @sessions_;

  foreach my $sessionid_ ( keys %sessionids_to_start_end_times_ ) {
    my $total_sucess_pairs_ = 0;
    my $total_failed_pairs_ = 0;
    my $total_large_changes_= 0;
    my $t_time_         = `date +"%Y%m%d %H:%M UTC"`;
    chomp($t_time_);
    #$summ_email_str_ = $summ_email_str_ . "STARTED: $t_time_\n\n";

    my $last_date_ = FetchLastDateForSession ( $sessionid_, $yyyymmdd_, $is_ret_lrdb_ );

    my @lrdb_pairs_vec_ = ();
    FetchLRDBPairsForSession ( $sessionid_, \@lrdb_pairs_vec_ , $is_ret_lrdb_);

    my %start_end_time_pairs_to_dep_indep_pairs_ = ();
    my %dep_indep_pairs_to_start_end_time_       = ();
    %dep_shortcodes_ = ();

#process LRDBPAIRS file
    foreach my $lrdb_pair_ref_ ( @lrdb_pairs_vec_ ) {

      next if $#$lrdb_pair_ref_ != 3;
      my ($dep_shortcode_, $indep_shortcode_, $start_time_, $end_time_) = @$lrdb_pair_ref_;

      $actual_to_pseudo_name_{$dep_shortcode_}   = GetPseudoName($dep_shortcode_);
      $actual_to_pseudo_name_{$indep_shortcode_} = GetPseudoName($indep_shortcode_);

      next if ! SanityCheckForPair($dep_shortcode_, $indep_shortcode_);

      if ( $recompute_ == 0 ) {
        my ($old_beta_, $old_corr_) = FetchLRDBForPair($dep_shortcode_, $indep_shortcode_, $sessionid_, $yyyymmdd_, $is_ret_lrdb_);
        if ( defined $old_beta_ ) {
          #print LOGFH "skipping $dep_shortcode_, $indep_shortcode_ [ pair already present ]\n";
          next;
        }
      }
      if ( $recompute_ == -1 ) {
        my ($old_beta_, $old_corr_) = FetchLRDBForPairwithoutdirtybit($dep_shortcode_, $indep_shortcode_, $sessionid_, $yyyymmdd_, $is_ret_lrdb_);
        if ( defined $old_beta_ ) {
          #print LOGFH "skipping $dep_shortcode_, $indep_shortcode_ [ pair already present ]\n";
          next;
        }
      }

      my $dep_indep_string_ = $dep_shortcode_."^".$indep_shortcode_;
      my $start_end_string_ = $start_time_."-".$end_time_;
      push( @{ $start_end_time_pairs_to_dep_indep_pairs_{$start_end_string_} }, $dep_indep_string_ );
      $dep_indep_pairs_to_start_end_time_{$dep_indep_string_} = $start_end_string_;
      $dep_shortcodes_{$dep_shortcode_} = 1;
    }

    if ( ! %dep_indep_pairs_to_start_end_time_ ) {
      #print "No Match for $input_dep_ $input_indep_ in lrdb pairs table for $this_exchange_ ".$sessionids_to_start_end_times_{ $sessionid_ }."\n";
      #print LOGFH "No Match for $input_dep_ $input_indep_ in lrdb pairs table for $this_exchange_ ".$sessionids_to_start_end_times_{ $sessionid_ }."\n";
      #corner case which you might forget recompute = 0 and pair already present;
      $exch_session_count{$this_exchange_} -= 1;
      next;
    }




    for my $dep_shortcode_ ( keys %dep_shortcodes_ ) {

      $total_dep_shortcodes{$dep_shortcode_}= 1;

      my @global_answers_ = ();
      my @failed_pairs_   = ();
      my $this_dep_proc_start_date_str_ = `date`;
      chomp($this_dep_proc_start_date_str_);
      #print LOGFH "computing for dep_shortcode_ " . $dep_shortcode_ . "...\n";
      #print LOGFH "time : $this_dep_proc_start_date_str_\n";



      for my $start_end_string_ ( keys %start_end_time_pairs_to_dep_indep_pairs_ ) {
        my @dep_indep_pairs_ = @{ $start_end_time_pairs_to_dep_indep_pairs_{$start_end_string_} };
        my @indeps_          = ();
        for my $dep_indep_pair_ (@dep_indep_pairs_) {
          my ($tdep_, $tindep_) = split( /\^/, $dep_indep_pair_ );
          push(@indeps_, $tindep_) if ( $dep_shortcode_ eq $tdep_ );
        }
        next if ( $#indeps_ < 0 );

        #print LOGFH "computing for start_end_time " . $start_end_string_ . "\n";
        my ($start_time_, $end_time_) = split( '-', $start_end_string_ );
        #print LOGFH "creating ilist for indeps ".join(" ", @indeps_)."\n";

        my @filtered_indeps_ = ();
        my @answers_         = ();

        for my $indep_ (@indeps_) {
          my $key_ = $dep_shortcode_ . "^" . $indep_ . "^" . $start_end_string_;
          # Pairs that need to be computed.
          print LOGFHP $dep_shortcode_ . " " . $indep_ . " " . $start_end_string_ . "\n";
          if ( !exists $lrdb_cache_{$key_} ) {
            push( @filtered_indeps_, $indep_ );
          }
          else {
            print LOGFHP "lrdb already computed in previous iteration for " . $key_ . "\n";
            my $value_ = $lrdb_cache_{$key_};
            push( @answers_, $key_ . "^" . $value_ );
          }
        }

        if ( $#filtered_indeps_ >= 0 ) {
          @indep_ilist_vec_ref_ =
          CreateIlist( $this_exchange_, $dep_shortcode_, $start_end_string_, \@filtered_indeps_,
            $datagen_end_yyyymmdd_ );

          my $lrdb_ilist_vec_ref_     = $indep_ilist_vec_ref_[0];
          my $combined_indep_vec_ref_ = $indep_ilist_vec_ref_[1];
          my $combined_indep_lowl1_vec_ref_ = $indep_ilist_vec_ref_[2];
          my $combined_indep_highl1_vec_ref_ = $indep_ilist_vec_ref_[3];

          my @dep_indep_vec_                 = ();
          my @independent_parallel_commands_ = ();
          my $pseudo_dep_shortcode_          = $actual_to_pseudo_name_{$dep_shortcode_};
          my $date_vec_ref_;
          if ( $#{$combined_indep_lowl1_vec_ref_} >= 0 ) {
#            print "Number of indeps with lower l1 evemts ".( $#$combined_indep_lowl1_vec_ref_ + 1 )."\n";
            @dep_indep_vec_ = @{$combined_indep_lowl1_vec_ref_};
            push( @dep_indep_vec_, $dep_shortcode_ );
            $date_vec_ref_ = GetDateVecWithDataForAllShcs( $datagen_end_yyyymmdd_, $lookback_, \@dep_indep_vec_ );
#            print "@$date_vec_ref_"."\n";
              foreach my $lrdb_ilist_file_ (@$lrdb_ilist_vec_ref_) {
                if ( index( $lrdb_ilist_file_, $pseudo_dep_shortcode_ . "_COMBINED_LOWL1_" . $start_end_string_ ) != -1 ) {
                  ComputeCommands( $this_exchange_,   $dep_shortcode_, "COMBINED_LOWL1", $start_end_string_, $start_time_, $end_time_, $lrdb_ilist_file_, $date_vec_ref_,\@independent_parallel_commands_);
                }
              }
              RunParallelProcesses( \@independent_parallel_commands_, $MAX_CORES_TO_USE_IN_PARALLEL, *LOGFHPA );
              my $this_matrix_data_filename_ = CombineDatagens( $this_exchange_, $dep_shortcode_, $combined_indep_lowl1_vec_ref_, $start_end_string_, $date_vec_ref_, "COMBINED_LOWL1" );
              ComputeLRDB( $dep_shortcode_, $combined_indep_lowl1_vec_ref_, $start_end_string_, $this_matrix_data_filename_, \@answers_, \@failed_pairs_ );
          }
          if ( $#{$combined_indep_vec_ref_} >= 0 ) {
#            print "Number of indeps with similar l1 evemts ". ($#$combined_indep_vec_ref_ + 1) ."\n";
            @dep_indep_vec_ = @{$combined_indep_vec_ref_};
            push( @dep_indep_vec_, $dep_shortcode_ );
            $date_vec_ref_ = GetDateVecWithDataForAllShcs( $datagen_end_yyyymmdd_, $lookback_, \@dep_indep_vec_ );
              foreach my $lrdb_ilist_file_ (@$lrdb_ilist_vec_ref_) {
                if ( index( $lrdb_ilist_file_, $pseudo_dep_shortcode_ . "_COMBINED_SIMILARL1_" . $start_end_string_ ) != -1 ) {
                  ComputeCommands(
                    $this_exchange_,   $dep_shortcode_,
                    "COMBINED_SIMILARL1",        $start_end_string_,
                    $start_time_,      $end_time_,
                    $lrdb_ilist_file_, $date_vec_ref_,
                    \@independent_parallel_commands_
                  );
                }
              }
              RunParallelProcesses( \@independent_parallel_commands_, $MAX_CORES_TO_USE_IN_PARALLEL, *LOGFHPA );
              my $this_matrix_data_filename_ = CombineDatagens( $this_exchange_, $dep_shortcode_, $combined_indep_vec_ref_, $start_end_string_, $date_vec_ref_, "COMBINED_SIMILARL1" );
              ComputeLRDB( $dep_shortcode_, $combined_indep_vec_ref_, $start_end_string_, $this_matrix_data_filename_, \@answers_, \@failed_pairs_ );
          }
          if ( $#{$combined_indep_highl1_vec_ref_} >= 0 ) {
#            print "Number of indeps with higher l1 evemts ". ($#$combined_indep_highl1_vec_ref_ + 1 ) ."\n";
            @dep_indep_vec_ = @{$combined_indep_highl1_vec_ref_};
            push( @dep_indep_vec_, $dep_shortcode_ );
#            print "@dep_indep_vec_\n";
            $date_vec_ref_ = GetDateVecWithDataForAllShcs( $datagen_end_yyyymmdd_, $lookback_, \@dep_indep_vec_ );
#            print "@$date_vec_ref_\n";
              foreach my $lrdb_ilist_file_ (@$lrdb_ilist_vec_ref_) {
#                print "$lrdb_ilist_file_\n";
                if ( index( $lrdb_ilist_file_, $pseudo_dep_shortcode_ . "_COMBINED_HIGHL1_" . $start_end_string_ ) != -1 ) {
                  ComputeCommands( $this_exchange_,   $dep_shortcode_, "COMBINED_HIGHL1", $start_end_string_, $start_time_, $end_time_, $lrdb_ilist_file_, $date_vec_ref_,\@independent_parallel_commands_);
                }
              }

              RunParallelProcesses( \@independent_parallel_commands_, $MAX_CORES_TO_USE_IN_PARALLEL, *LOGFHPA );
              my $this_matrix_data_filename_ = CombineDatagens( $this_exchange_, $dep_shortcode_, $combined_indep_highl1_vec_ref_, $start_end_string_, $date_vec_ref_, "COMBINED_HIGHL1" );
            ComputeLRDB( $dep_shortcode_, $combined_indep_highl1_vec_ref_, $start_end_string_, $this_matrix_data_filename_, \@answers_, \@failed_pairs_ );
          }
          push( @global_answers_, @answers_ );
          #print LOGFH "Number of independent datagen commands".( $#independent_parallel_commands_ + 1 )."\n";
        }
      }

      $total_sucess_pairs_ += ( $#global_answers_ + 1 );
      $total_failed_pairs_ += ( $#failed_pairs_ + 1 );



      #$lrdb_dump_str_ .= DumpLRDBAndSendMail( $dep_shortcode_, $sessionid_, \@global_answers_, \@failed_pairs_, $this_dep_proc_start_date_str_ );
      ($lrdb_dump_str_, $big_change_count) = DumpLRDBAndSendMail( $dep_shortcode_, $sessionid_, \@global_answers_, \@failed_pairs_, $this_dep_proc_start_date_str_ );
      $total_large_changes_ += $big_change_count;
      $suc_failed_str_ .= $dep_shortcode_ . " " x (14 - length ($dep_shortcode_)) . $sessionids_to_start_end_times_{$sessionid_}  .
          " " x (25 - length ($sessionids_to_start_end_times_{$sessionid_})) . ( $#global_answers_ + 1 ) . " " x (10 - length ($#global_answers_ + 1)).
          ( $#failed_pairs_ + 1 ) . " " x (15 - length ( $#failed_pairs_ + 1 ) ). $lrdb_dump_str_ . "\n";
      ClearTemporaryFiles ( @temporary_files_ );
      @temporary_files_ = ();
    }

    #$summ_email_str_ =
    #$summ_email_str_ . "# of Sucessful Pairs: $total_sucess_pairs_\n# of Failed Pairs: $total_failed_pairs_\n\n";
    $t_time_ = `date +"%Y%m%d %H:%M UTC"`;
    chomp($t_time_);
    #$summ_email_str_ = $summ_email_str_ . "COMPLETED: $t_time_\n\n";

    #$summ_email_str_ = $summ_email_str_ . "\n\nIndividualRunLogs:\n" . $lrdb_dump_str_;
    my @row_ = ();
    push( @row_, $this_exchange_ );
    push( @row_, $sessionids_to_start_end_times_{$sessionid_});
    push( @row_, $total_sucess_pairs_);
    push( @row_, $total_failed_pairs_);
    push( @row_, $total_large_changes_);
    push( @row_, $elog_file_);
    push( @mail_table_, \@row_ );

      $summ_email_str_ = $summ_email_str_ . $this_exchange_ . " " . $sessionids_to_start_end_times_{$sessionid_} . " " . $total_sucess_pairs_ . " "
          . $total_failed_pairs_ . " " . $elog_file_ . "\n";
      $exchange_info_str_ = $exchange_info_str_ . $this_exchange_ . " " x (7 - length ($this_exchange_) ) . $sessionids_to_start_end_times_{$sessionid_} .
          " " x (25 - length ($sessionids_to_start_end_times_{$sessionid_}) ) . $total_sucess_pairs_ . " " x (25 -length ($total_sucess_pairs_))
          . $total_failed_pairs_ . " " x (15 -length ($total_failed_pairs_)) . $total_large_changes_ . "\n";
  }
  printf
  print ELOGFH "Exchange Summary\n" . $exchange_info_str_ . "\n \n";
  print ELOGFH "Individual Summary\n" . $suc_failed_str_ ."\n \n";
}
print LOGFH  "\n \n \nMail Sent : \n" . $summ_email_str_;
#Sendmail only in case of combined run, to avoid spamming from individual runs;
if ( keys %total_dep_shortcodes > 1 ) {
  SendMail(
      $mail_address_,
      $mail_address_,
      "$lrdb_type_str_ LRDB Summary $yyyymmdd_",
      \@mail_table_
   );
}

sub SanityCheckForPair {
  my ($dep_shortcode_, $indep_shortcode_) = @_;
  my $t_line_ = $dep_shortcode_." ".$indep_shortcode_;

  if ( $dep_shortcode_ eq $indep_shortcode_ ) {
# Skipping self self pair.
    #print LOGFH "skipping $t_line_ [ self self pair ]\n";
    return 0;
  }

  if ( $input_dep_ ne "ALL" && !grep { $_ eq $dep_shortcode_ } @input_dep_vec_ ) {
# This pair should not be computed (as per user inputs).
    #print LOGFH "skipping $t_line_ [ as per dep user inputs ]\n";
    return 0;
  }

  if ( !IsValidShc($dep_shortcode_) ) {
    #print LOGFH "skipping $t_line_ [ invalid dep ]\n";
    return 0;
  }

  if ( $input_indep_ ne "ALL" && !grep { $_ eq $indep_shortcode_ } @input_indep_vec_ ) {
# This pair should not be computed (as per user inputs).
    #print LOGFH "skipping $t_line_ [ as per indep user inputs ]\n";
    return 0;
  }

  if ( !IsValidShc($indep_shortcode_) && !IsValidPort($indep_shortcode_) ) {
    #print LOGFH "skipping $t_line_ [ invalid indep ]\n";
    return 0;
  }

  if ( ExpiredDIContract( $dep_shortcode_, $datagen_end_yyyymmdd_ ) ) {
    #print LOGFH "skipping $t_line_ [ dep is an expired DI1 contract ]\n";
    return 0;
  }

  if ( ExpiredDIContract( $indep_shortcode_, $datagen_end_yyyymmdd_ ) ) {
    #print LOGFH "skipping $t_line_ [ indep is an expired DI1 contract ]\n";
    return 0;
  }

#skip shc which have weird volume pattern in a combined run, eg. BR_DOL_1
  if ( $input_dep_ eq "ALL" ) {
    if ( grep { $_ eq $dep_shortcode_ } @skip_shc_in_comb_run_vec_ ) {
      #print LOGFH "skipping $t_line_ [ dep is in $skip_shc_in_comb_run_file_, please run for $dep_shortcode_ as input_dep separately ]\n";
      return 0;
    }
    my $dep_exch_ = GetExchFromSHC($dep_shortcode_);
    if ( grep { $_ eq $dep_exch_ } @skip_exch_in_comb_run_vec_ ) {
      #print LOGFH "skipping $t_line_ [ $dep_exch_ is in $skip_exch_in_comb_run_file_, please run for $dep_shortcode_ as input_dep separately ]\n";
      return 0;
    }
  }

  if ( $input_indep_ eq "ALL" ) {
    if ( grep { $_ eq $indep_shortcode_ } @skip_shc_in_comb_run_vec_ ) {
      #print LOGFH
       # "skipping $t_line_ [ indep is in $skip_shc_in_comb_run_file_, please run for $indep_shortcode_ as input_indep separately ]\n";
      return 0;
    }

    foreach my $t_skip_exch_ (@skip_exch_in_comb_run_vec_) {
      if ( ShcHasExchSource( $indep_shortcode_, $t_skip_exch_ ) ) {
        #print LOGFH
         # "skipping $t_line_ [ $t_skip_exch_ is in $skip_exch_in_comb_run_file_, please run for $indep_shortcode_ as input_dep separately ]\n";
        return 0;
      }
    }
  }

  return 1;
}

sub GetPseudoName {
  my $shc = shift;
  ( my $new_shc = $shc ) =~ s/&/~/;
  return $new_shc;
}

sub GetL1Events {
  my $shc_        = shift;
  my $start_time_ = shift;
  my $end_time_   = shift;
  my $lookback_ = shift;
  my $end_date_   = shift;
  my $exec_cmd_;
  if ( IsValidPort($shc_) ) {
    $exec_cmd_ = "grep -w $shc_ $PORTFOLIO_INPUTS | head -1";
    my $portfolio_line_ = `$exec_cmd_`;chomp($portfolio_line_);
    my @portfolio_shcs_ = split(" ",$portfolio_line_);chomp(@portfolio_shcs_);
    @portfolio_shcs_ = @portfolio_shcs_[2..$#portfolio_shcs_];
    my @portfolio_l1events_ = ( );
    my $l1events_;

    foreach my $port_shc_ (@portfolio_shcs_) {
      if ( !exists( $shortcode_start_end_to_l1events_{ $port_shc_ . "_" . $start_time_ . "-" . $end_time_ } ) ) {
          $shortcode_start_end_to_l1events_{ $port_shc_ . "_" . $start_time_ . "-" . $end_time_ } = GetL1Events( $port_shc_, $start_time_, $end_time_, $lookback_, $end_date_ );
      }
      push( @portfolio_l1events_, $shortcode_start_end_to_l1events_{ $port_shc_ . "_" . $start_time_ . "-" . $end_time_ } );
    }
    my @sorted_portfolio_events_ = sort @portfolio_l1events_;
    my $port_l1events_           = sum(@portfolio_l1events_) / @portfolio_l1events_;
    return $port_l1events_;
  }
  else {
#    $exec_cmd_ = $SCRIPTS_DIR
#      . "/get_avg_l1_events_in_timeperiod.pl $shc_ $start_date_ $end_date_ $start_time_ $end_time_ | tail -2 | head -1";
    $exec_cmd_ = $SCRIPTS_DIR."/get_avg_samples.pl $shc_ $end_date_ $lookback_ $start_time_ $end_time_ 0 L1EVPerSec 2>/dev/null | tail -1";
    my $line_ = `$exec_cmd_`;
    my @words_ = split( " ", $line_ );
    chomp(@words_);
    #printf LOGFH "L1Events for $shc_ $start_time_ $end_time_ $words_[-1]\n";
    return $words_[-1];
  }
}


sub CreateIlist {

  #  my $this_lrdbtimezonestr_ = shift;
  my $this_exchange_ = shift;
  my $dep_shortcode_        = shift;
  my $start_end_string_     = shift;
  my $indeps_               = shift;
  my $date_                 = shift;
  my $pseudo_indep_shortcode_;
  my @lrdb_ilist_file_vec_;
  my @combined_indeps_lowl1_ = ();
  my @combined_indeps_ = ();
  my @combined_indeps_highl1_ = ();
  my @seperate_indeps_ = ();
  my @start_end_times_ = split( '-', $start_end_string_ );

  #  our %shortcode_start_end_to_l1events_ = ( );
  if ( !exists( $shortcode_start_end_to_l1events_{ $dep_shortcode_ . "_" . $start_end_string_ } ) ) {
    $shortcode_start_end_to_l1events_{ $dep_shortcode_ . "_" . $start_end_string_ } =
      GetL1Events( $dep_shortcode_, $start_end_times_[0], $start_end_times_[1],
      60, $date_ );
  }

  for my $indep_ (@$indeps_) {
    if ( !exists( $shortcode_start_end_to_l1events_{ $indep_ . "_" . $start_end_string_ } ) ) {
      $shortcode_start_end_to_l1events_{ $indep_ . "_" . $start_end_string_ } =
        GetL1Events( $indep_, $start_end_times_[0], $start_end_times_[1], 60,
        $date_ );
    }
    if ( $shortcode_start_end_to_l1events_{ $indep_ . "_" . $start_end_string_ } < 0.5 * $shortcode_start_end_to_l1events_{ $dep_shortcode_ . "_" . $start_end_string_ } )
    {
      push ( @combined_indeps_lowl1_, $indep_ );
      #printf LOGFH "combining pair with low l1 events $dep_shortcode_ $indep_\n";
    } 
    elsif ( $shortcode_start_end_to_l1events_{ $indep_ . "_" . $start_end_string_ } >
      0.5 * $shortcode_start_end_to_l1events_{ $dep_shortcode_ . "_" . $start_end_string_ }
      and $shortcode_start_end_to_l1events_{ $indep_ . "_" . $start_end_string_ } <
      1.5 * $shortcode_start_end_to_l1events_{ $dep_shortcode_ . "_" . $start_end_string_ } )
    {
      #printf LOGFH "combined pair $dep_shortcode_ $indep_\n";
      push( @combined_indeps_, $indep_ );
    }
    else {
      #printf LOGFH "combining pair with high l1 events $dep_shortcode_ $indep_\n";
      push( @combined_indeps_highl1_, $indep_ );
    }

  }
  CreateCombinedIlist ( $this_exchange_, $dep_shortcode_, $start_end_string_, \@combined_indeps_lowl1_, "COMBINED_LOWL1", \@lrdb_ilist_file_vec_ );
  CreateCombinedIlist ( $this_exchange_, $dep_shortcode_, $start_end_string_, \@combined_indeps_, "COMBINED_SIMILARL1", \@lrdb_ilist_file_vec_ );
  CreateCombinedIlist ( $this_exchange_, $dep_shortcode_, $start_end_string_, \@combined_indeps_highl1_, "COMBINED_HIGHL1", \@lrdb_ilist_file_vec_ );
  return ( \@lrdb_ilist_file_vec_, \@combined_indeps_, \@combined_indeps_lowl1_, \@combined_indeps_highl1_ );
}

sub CreateCombinedIlist {
  my $this_exchange_ = shift;
  my $dep_shortcode_ = shift;
  my $start_end_string_ = shift;
  my $combined_indeps_ref_ = shift;
  my $ilist_tag_ = shift;
  my $lrdb_ilist_file_vec_ref_ = shift;
  my $pseudo_dep_shortcode_ = $actual_to_pseudo_name_{$dep_shortcode_};

  if ( $#$combined_indeps_ref_ + 1 > 0 )
  {
    my $lrdb_ilist_file_ = "$lrdb_ilist_file_prefix_" . "_" . $this_exchange_ . "_" . $pseudo_dep_shortcode_ ."_" . $ilist_tag_ . "_" . $start_end_string_ . ".txt";
    if ( -e $lrdb_ilist_file_ ) { `rm -rf $lrdb_ilist_file_` }
    #print LOGFH "ilist path " . $lrdb_ilist_file_ . "\n";
    push( @temporary_files_, $lrdb_ilist_file_ );
    open LRDB_ILIST_FILE, "> $lrdb_ilist_file_ " or PrintStacktraceAndDie("could not open $lrdb_ilist_file_ for writing\n");
    printf LRDB_ILIST_FILE "MODELINIT DEPBASE %s MktSizeWPrice MktSizeWPrice\n", $dep_shortcode_;
    print LRDB_ILIST_FILE "MODELMATH LINEAR CHANGE\n";
    print LRDB_ILIST_FILE "INDICATORSTART\n";
    if ($is_ret_lrdb_) {
      if ( IsValidPort($dep_shortcode_) ) {
        printf LRDB_ILIST_FILE "INDICATOR 1.00 SimpleReturnsPort %s %d MktSizeWPrice\n", $dep_shortcode_,
               $seconds_to_compare_;
      }
      else {
        printf LRDB_ILIST_FILE "INDICATOR 1.00 SimpleReturns %s %d MktSizeWPrice\n", $dep_shortcode_,
               $seconds_to_compare_;
      }
    }
    else {
      if ( IsValidPort($dep_shortcode_) ) {
        printf LRDB_ILIST_FILE "INDICATOR 1.00 SimpleTrendPort %s %d MktSizeWPrice\n", $dep_shortcode_,
               $seconds_to_compare_;
      }
      else {
        printf LRDB_ILIST_FILE "INDICATOR 1.00 SimpleTrend %s %d MktSizeWPrice\n", $dep_shortcode_,
               $seconds_to_compare_;
      }
    }
    for my $indep_ (@$combined_indeps_ref_) {
      #printf LOGFH "Adding indep to combined ilist for $dep_shortcode_ - $indep_\n";
      if ($is_ret_lrdb_) {
        if ( IsValidPort($indep_) ) {
          printf LRDB_ILIST_FILE "INDICATOR 1.00 SimpleReturnsPort %s %d MktSizeWPrice\n", $indep_,
                 $seconds_to_compare_;
        }
        else {
          printf LRDB_ILIST_FILE "INDICATOR 1.00 SimpleReturns %s %d MktSizeWPrice\n", $indep_, $seconds_to_compare_;
        }
      }
      else {
        if ( IsValidPort($indep_) ) {
          printf LRDB_ILIST_FILE "INDICATOR 1.00 SimpleTrendPort %s %d MktSizeWPrice\n", $indep_, $seconds_to_compare_;
        }
        else {
            printf LRDB_ILIST_FILE "INDICATOR 1.00 SimpleTrend %s %d MktSizeWPrice\n", $indep_, $seconds_to_compare_;
        }
      }
    }
    print LRDB_ILIST_FILE "INDICATOREND\n";
    close LRDB_ILIST_FILE;
    `scp $lrdb_ilist_file_ $shared_ilist_dir`;
    push( @$lrdb_ilist_file_vec_ref_, $lrdb_ilist_file_ );
  }
}


sub ComputeCommands {

  #  my $this_lrdbtimezonestr_ = shift;
  my $this_exchange_                 = shift;
  my $dep_shortcode_                 = shift;
  my $indep_shortcode_               = shift;
  my $start_end_string_              = shift;
  my $datagen_start_time_            = shift;
  my $datagen_end_time_              = shift;
  my $lrdb_ilist_file_               = shift;
  my $date_vec_ref_                  = shift;
  my $independent_parallel_commands_ = shift;
  my $pseudo_dep_shortcode_          = $actual_to_pseudo_name_{$dep_shortcode_};
  my $pseudo_indep_shortcode_;

  if ( index ($indep_shortcode_ , "COMBINED" ) == -1 ) {
    $pseudo_indep_shortcode_ = $actual_to_pseudo_name_{$indep_shortcode_};
#    print "$indep_shortcode_\n";
#    print index ($indep_shortcode_ , "COMBINED" )."\n";
  }
  else {
    $pseudo_indep_shortcode_ = $indep_shortcode_;
#    print "Combined ilist\n";
  }

  foreach my $tradingdate_ (@$date_vec_ref_) {
#    print "Date to run datagen". $tradingdate_ ."\n";
#    my $this_timed_data_filename_ = "$work_dir_"."/lrdb_timed_data_file_".$this_lrdbtimezonestr_."_".$pseudo_dep_shortcode_."_".$pseudo_indep_shortcode_."_".$start_end_string_."_".$tradingdate_.".txt";
    my $this_timed_data_filename_ =
        "$work_dir_"
      . "/lrdb_timed_data_file_"
      . $this_exchange_ . "_"
      . $pseudo_dep_shortcode_ . "_"
      . $pseudo_indep_shortcode_ . "_"
      . $start_end_string_ . "_"
      . $tradingdate_ . ".txt";
    if ( -e $this_timed_data_filename_ ) { `rm -rf $this_timed_data_filename_`; }
    push( @temporary_files_, $this_timed_data_filename_ );

    my $exec_cmd_ =
        "$LIVE_BIN_DIR/datagen $lrdb_ilist_file_ $tradingdate_ "
      . $datagen_start_time_ . " "
      . $datagen_end_time_
      . " $unique_gsm_id_ $this_timed_data_filename_ $datagen_msecs_timeout_ $datagen_l1events_timeout_ $datagen_num_trades_timeout_ $to_print_on_economic_times_ ADD_DBG_CODE -1";
    push( @$independent_parallel_commands_, $exec_cmd_ );
    #print LOGFH $exec_cmd_ . "\n";
  }
}

sub CombineDatagens {
  my $this_exchange_        = shift;
  my $dep_shortcode_        = shift;
  my $indeps_               = shift;
  my $start_end_string_     = shift;
  my $date_vec_ref_         = shift;
  my $ilist_tag_          = shift;
  my $pseudo_dep_shortcode_ = $actual_to_pseudo_name_{$dep_shortcode_};
  my $pseudo_indep_shortcode_;

  if ( index( $ilist_tag_, "COMBINE" ) != -1 or $#{$indeps_} > 0 ) {
    $pseudo_indep_shortcode_ = $ilist_tag_;
  }
  else {
    $pseudo_indep_shortcode_ = $actual_to_pseudo_name_{ ${$indeps_}[0] };
  }

# We now have to run parallel datagens for 60 days.
  my $this_matrix_data_filename_ =
      "$work_dir_"
    . "/lrdb_matrix_data_file_"
    . $this_exchange_ . "_"
    . $pseudo_dep_shortcode_ . "_"
    . $pseudo_indep_shortcode_ . "_"
    . $start_end_string_ . ".txt";
  #print LOGFH "combining datagens into " . $this_matrix_data_filename_ . "\n";
  if ( -e $this_matrix_data_filename_ ) { `rm -f $this_matrix_data_filename_`; }
  push( @temporary_files_, $this_matrix_data_filename_ );

  #Combine
  my $good_days_ = 0;
  foreach my $tradingdate_ (@$date_vec_ref_) {

    my $this_timed_data_filename_ =
        "$work_dir_"
      . "/lrdb_timed_data_file_"
      . $this_exchange_ . "_"
      . $pseudo_dep_shortcode_ . "_"
      . $pseudo_indep_shortcode_ . "_"
      . $start_end_string_ . "_"
      . $tradingdate_ . ".txt";
    if ( ExistsWithSize($this_timed_data_filename_) && SanityChecks($this_timed_data_filename_) ) {
      my $exec_cmd =
"cat $this_timed_data_filename_ | awk \'{ i=5; while ( i <= NF ) { printf \"%f \", \$i; i++ } ; printf \"\\n\"; }\' >> $this_matrix_data_filename_";
      #print LOGFH "$exec_cmd\n";
      `$exec_cmd`;
      $good_days_++;
    }
    else {
      #print LOGFH "$this_timed_data_filename_ missing after datagen\n";
    }
    `rm -f $this_timed_data_filename_`;
  }

  #print LOGFH "number of good days (no issues with datagen) for " . $this_exchange_ . "_"
  #  . $pseudo_dep_shortcode_ . "_"
  #  . $pseudo_indep_shortcode_ . "_"
  #  . $start_end_string_ . ".txt" ." is " . $good_days_ . "\n";
  #print LOGFH $this_exchange_ ." " . $good_days_ . "\n";
  for my $indep_ (@$indeps_) {
    $dep_indep_pairs_to_datagen_days_{ $dep_shortcode_ . "^" . $indep_ . "^" . $start_end_string_ } = $good_days_;
  }
  if ( $good_days_ < $lookback_ /3){
    print LOGFH "ERROR! number of good days (no issues with datagen) for " . $this_exchange_ . "_"
    . $pseudo_dep_shortcode_ . "_"
    . $pseudo_indep_shortcode_ . "_"
    . $start_end_string_ . ".txt" ." is " . $good_days_
        . " which is less than one third of lookback days so removing the matrix data file \n";
    `rm $this_matrix_data_filename_`;
  }
  return $this_matrix_data_filename_;
}

sub SanityChecks {
  my $this_timed_data_filename_ = shift;
  my $nan_present_              = `grep nan $this_timed_data_filename_| wc -l`;
  if ( $nan_present_ > 0 ) {
    return 0;
  }
  my $line_count_ = `wc -l $this_timed_data_filename_ | cut -d' ' -f1`;
  #print LOGFH "Line count after datagen = $line_count_\n";
  if ( $line_count_ == 0 ) {
    return 0;
  }
  return 1;
}

sub ComputeLRDB {
  my $dep_shortcode_             = shift;
  my $indeps_                    = shift;
  my $start_end_string_          = shift;
  my $this_matrix_data_filename_ = shift;
  my $answers_                   = shift;
  my $failed_pairs_              = shift;

  if ( ExistsWithSize($this_matrix_data_filename_) ) {
    my $exec_cmd = "$MODELSCRIPTS_DIR/get_stdev_correlation_matrix.py $this_matrix_data_filename_";
    #print LOGFH "$exec_cmd\n";
    my @stdev_corr_matrix_text_ = `$exec_cmd`;
    if ( $#stdev_corr_matrix_text_ >= 0 ) {
      my @stdev_vec_ = split( ' ', $stdev_corr_matrix_text_[0] );
      my $dep_stdev_ = $stdev_vec_[0];
      
      for ( my $indep_idx_ = 0 ; $indep_idx_ <= $#$indeps_ ; $indep_idx_++ ) {
        my $indep_shortcode_ = $$indeps_[$indep_idx_];
        my $indep_stdev_     = $stdev_vec_[ $indep_idx_ + 1 ];

        my $lr_correlation_ = GetKthWord( $stdev_corr_matrix_text_[1], $indep_idx_ + 1 );
        if ( $indep_stdev_ == 0 ) {
          print LOGFH "ERROR! ignoring as indep_stdev_ is 0 " . $dep_shortcode_ . " " . $indep_shortcode_ . " " . $start_end_string_ . "\n\n";
          push( @$failed_pairs_, $dep_shortcode_ . "^" . $indep_shortcode_ . "^" . $start_end_string_ );
          next;
        }
        my $lr_coeff_ = ( $dep_stdev_ / $indep_stdev_ ) * $lr_correlation_;

        my $t_dep_indep_string_ = $dep_shortcode_ . "^" . $indep_shortcode_;
        my $key_ = $dep_shortcode_ . "^" . $indep_shortcode_ . "^" . $start_end_string_;
        AddToCache( $dep_shortcode_, $indep_shortcode_, $start_end_string_, $lr_coeff_, $lr_correlation_ );

        my $answer_ = $key_ . "^" . $lr_coeff_ . "^" . $lr_correlation_ . "^" . $dep_stdev_ . "^" . $indep_stdev_ . "\n";
        push( @$answers_, $answer_ );
      }
    }
  }
  else {
    printf LOGFH "ERROR! %s matrix_data_file missing \n", $this_matrix_data_filename_;
    for my $indep_ (@$indeps_) {
      push( @$failed_pairs_, $dep_shortcode_ . "^" . $indep_ . "^" . $start_end_string_ );
    }
  }
  `rm -f $this_matrix_data_filename_`;
}

sub AddToCache {
  my $dep_shortcode_    = shift;
  my $indep_shortcode_  = shift;
  my $start_end_string_ = shift;
  my $lr_coeff_         = shift;
  my $lr_correlation_   = shift;
  my $key_              = $dep_shortcode_ . "^" . $indep_shortcode_ . "^" . $start_end_string_;
  my $value_            = $lr_coeff_ . "^" . $lr_correlation_;
  #print LOGFH "adding $key_ to cache\n";
  $lrdb_cache_{$key_} = $value_;
}

sub DumpLRDBAndSendMail {
  my $dep_shortcode_ = shift;
  my $sessionid_ = shift;
  my $global_answers_ = shift;
  my $failed_pairs_ = shift;

  my $tlog_file_ = $LOGFILE_DIR."/log_file_".$dep_shortcode_."_".$sessionid_.".txt";
  open TLOGFH, "> $tlog_file_" or PrintStacktraceAndDie( "Could not open $tlog_file_ for writing" );

  my $process_end_date_str_ = `date +"%Y%m%d %H:%M UTC"`;
  chomp($process_end_date_str_);
  #my $mail_str_ = "DEP: $dep_shortcode_, TIME: $process_end_date_str_\nLOGFILE: $tlog_file_\n";

  my @bigchange_vec_ = ();
  my $bigchange_count_ = 0;

  print TLOGFH "\n\nSuccessful Pairs\n\nDEP^INDEP" . " " x (10) . "beta       correlation     Session_Timings\n";
  foreach my $answer_ (@$global_answers_) {
    my @temp_array_       = split( /\^/, $answer_ );
    $dep_shortcode_       = $temp_array_[0];
    my $indep_shortcode_  = $temp_array_[1];
    my $start_end_string_ = $temp_array_[2];
    my $lr_coeff_         = $temp_array_[3];
    my $lr_correlation_   = $temp_array_[4];
    my $dep_stdev_        = $temp_array_[5];
    my $indep_stdev_      = $temp_array_[6];
    my $datagen_days_ =
      $dep_indep_pairs_to_datagen_days_{ $dep_shortcode_ . "^" . $indep_shortcode_ . "^" . $start_end_string_ };



    printf TLOGFH "%s^%s" . " " x (19- length($indep_shortcode_) - length ($dep_shortcode_))."%.10f %.6f          %s\n", $dep_shortcode_, $indep_shortcode_, $lr_coeff_, $lr_correlation_,
      $start_end_string_;
#$mail_str_ = $mail_str_
#. sprintf( "%s^%s %.10f %.6f %s [%d]\n",
#$dep_shortcode_, $indep_shortcode_, $lr_coeff_, $lr_correlation_, $start_end_string_, $datagen_days_ );

    my $dirty_bit_ = 0;
    my $bigchange_str_ = CheckLRDBPair($dep_shortcode_, $indep_shortcode_, $sessionid_, $lr_coeff_, $lr_correlation_, $is_ret_lrdb_, $dep_stdev_, $indep_stdev_);
    if ( $bigchange_str_ ne "" ) {
      $dirty_bit_ = 1;
      push ( @bigchange_vec_, $bigchange_str_ );
      $bigchange_count_++;
    }

    InsertLRDBPairValue($dep_shortcode_, $indep_shortcode_, $sessionid_, $yyyymmdd_, $lr_coeff_, $lr_correlation_, $dirty_bit_, $is_ret_lrdb_, 1);
  }
  print TLOGFH "\n";

  if ($#$failed_pairs_ >= 0 ){
    print TLOGFH "Failed Pairs \n\n";
    print TLOGFH "Dep^Indep^Session_Timings\n"
  }
  foreach my $failed_pair_ (@$failed_pairs_) {
    print TLOGFH "FAILED: $failed_pair_\n";
  }
  print TLOGFH "\n\n";

  if ($bigchange_count_ > 0) {
    print TLOGFH "Pairs With Large Beta/Correlation change:\n\n";
    print TLOGFH "DEP" . " " x ((15)) ."INDEP" . " " x (10). " New_beta New_correlation Dep_stdev Indep_stdev   Prev_beta  Prev_correlation   Date       Corr_change% Beta_change%\n";
  }
  foreach my $bigch_ ( @bigchange_vec_ ) {
    print TLOGFH "$bigch_\n";
  }
  print TLOGFH "\n";

  #$mail_str_ .= "# of Large Changes: ".$bigchange_count_."\n\n";
  my $mail_str_ = $bigchange_count_ . " " x ( 16 - length ($bigchange_count_)) . $tlog_file_;
  return ($mail_str_,$bigchange_count_ );
}

sub SendMail
{
  my ( $to_, $from_ , $subject_, $table_) = @_;
  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $to_\n";
  print MAIL "From: $from_\n";
  print MAIL "Subject: $subject_ \n";

  my $exchange_ = "";
  my $prev_exchange_ = "";
  printf MAIL "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n";
  printf MAIL "<html><body>\n";
  printf MAIL "<table border = \"2\">" ;
  printf MAIL "\n";
  for( my $k = 0; $k<=$#$table_; $k ++ )
  {
    printf MAIL "<tr>\n";
    my $row_ = $$table_[$k];
    $exchange_ = $$row_[0] if $k != 0;

    for ( my $j = 0; $j <= $#$row_ ; $j ++)

    {
      if($k == 0)
      {
        printf MAIL "<td align=\"center\"><font font-weight = \"bold\" size = \"2\" color=\"Red\">%s</font></td>\n", $$row_[$j];
        next;
      }

      if (($j == 0 or $j == $#$row_) and ($exchange_ ne $prev_exchange_)) {

        printf MAIL
            "<td align=\"center\" rowspan=\"$exch_session_count{$exchange_}\"><font font-weight = \"bold\" size = \"2\" color=\"darkblue\">%s</font></td>\n"
            , $$row_[$j];

        if ($j == $#$row_) {
          $prev_exchange_ = $exchange_;
        }
      }
      elsif ($j != 0 and $j != $#$row_)

      {
          printf MAIL "<td align=\"center\"><font font-weight = \"bold\" size = \"2\" color=\"darkblue\">%s</font></td>\n", $$row_[$j];
      }

    }
    printf MAIL "</tr>\n";
  }

  printf MAIL "</table>\n";
  printf MAIL "</body></html>\n";

  close(MAIL);
}


sub CheckLRDBPair
{
  my ($dep, $indep, $sessionid, $beta, $correlation, $is_ret_lrdb_, $dep_stdev_, $indep_stdev_) = @_;


  my @row_= GetPreRow($dep, $indep, $sessionid, $yyyymmdd_,$is_ret_lrdb_);
  my $bigchange_str_ = "";

  if ( $#row_ > 0 ) {
    my $prev_beta=$row_[0];
    my $prev_correlation=$row_[1];
    my $corr_ch_ = abs($correlation-$prev_correlation);
    my $corr_ch_fr_ = 0.3;
    $corr_ch_fr_ = abs(($correlation-$prev_correlation)/$prev_correlation) if $prev_correlation != 0;
    my $beta_ch_fr_ = 0.3;
    $beta_ch_fr_ = abs(($beta-$prev_beta)/$prev_beta) if $prev_beta != 0;


    my $large_corr_change_ = ( $corr_ch_ > 0.3 ) && ( $corr_ch_fr_ > 0.3 );
    my $large_beta_change_ = ( $beta_ch_fr_ > 0.3 + $corr_ch_fr_ );
    #approximation for just considering large changes in stdev_ratio because corr change is already considered
    if ( $large_corr_change_ || $large_beta_change_ ) {
      $bigchange_str_ .= sprintf( "$dep" . " " x (11 - length($dep)) . "$indep" . " " x (15 - length ($indep)) . " %.6f   %.4f         %.4f    %.4f         %.4f          %.4f      %s", $beta, $correlation, $dep_stdev_, $indep_stdev_, $prev_beta, $prev_correlation, $row_[2]);
      $bigchange_str_ .= sprintf( "  %.2f         %.2f\n", 100 * $corr_ch_fr_, 100 * $beta_ch_fr_ );
    }
  }
  return $bigchange_str_;
}

sub signal_handler {
  die "Caught a signal $!\n";
}

sub END
{
  ClearTemporaryFiles(@temporary_files_);
}
