#!/usr/bin/perl

use strict;
use warnings;
use File::Basename;    # for basename and dirname
use Fcntl qw (:flock);

push (@INC, "/home/ec2-user/perl5/lib/perl5/x86_64-linux-thread-multi");

my $USER     = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};

my $REPO = "basetrade";

my $GENPERLLIB_DIR   = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";
my $SCRIPTS_DIR      = $HOME_DIR . "/" . $REPO . "_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR . "/" . $REPO . "_install/ModelScripts";

my $BIN_DIR                   = $HOME_DIR . "/" . $REPO . "_install/bin";
my $BASETRADEINFODIR          = "/spare/local/tradeinfo/";
my $SPARE_HOME                     = "/spare/local/" . $USER . "/";
my $SPARE_LRDB_DIR                 = $SPARE_HOME . "lrdbdata";
my $SPARE_LOCAL_EXCHANGE_FILES_DIR = $BASETRADEINFODIR;
my $PORTFOLIO_INPUTS = "/spare/local/tradeinfo/PCAInfo/portfolio_inputs";

require "$GENPERLLIB_DIR/exists_with_size.pl";
require "$GENPERLLIB_DIR/create_enclosing_directory.pl";
require "$GENPERLLIB_DIR/calc_prev_date.pl";
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl";    # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/load_portfolio_names.pl";
require "$GENPERLLIB_DIR/get_kth_word.pl";
require "$GENPERLLIB_DIR/clear_temporary_files.pl";          # for ClearTemporaryFiles
require "$GENPERLLIB_DIR/sync_to_all_machines.pl";           # for SyncToAllMachines
require "$GENPERLLIB_DIR/lock_utils.pl";                     # for TakeLock, RemoveLock
require "$GENPERLLIB_DIR/get_port_constituents.pl";          # for IsValidPort
require "$GENPERLLIB_DIR/get_exch_from_shortcode.pl";        # for IsValidShc
require "$GENPERLLIB_DIR/ec2_utils.pl";                      # IsEc2Worker
require "$GENPERLLIB_DIR/print_stacktrace.pl";               # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/date_utils.pl";                     # GetTZStrFromDVCTZCode,  GetTimestampFromTZ_HHMM_DATE
require "$GENPERLLIB_DIR/lrdb_db_access_manager.pl";

if ( $#ARGV < 1 ) {
  print "USAGE : $0 dep indep [replace_existing=0] [TYPE(CHANGE|RETURNS)=CHANGE]\n";
  print "Example : $0 FGBM_0 FOAT_0 \n";
  exit(0);
}

my $dep_   = shift;    # if all is specified we use all the *indep pairs from the lrdb_pairs_ file
my $indep_ = shift;    # if all is specified we use all the dep* paris from the lrdb_pairs_ file

my $mode_ = 0;
if (@ARGV) { $mode_ = shift; }

my $lrdb_type_str_ = "CHANGE";
if (@ARGV) { $lrdb_type_str_ = shift; }

if (!($lrdb_type_str_ eq "RETURNS" || $lrdb_type_str_ eq "CHANGE")) {
  print "USAGE : $0 dep indep [replace_existing=0] [TYPE(CHANGE|RETURNS)=CHANGE]\n";
  print "Type should be CHANGE or RETURNS \n";
  exit(0);
}


if ( !IsValidShc($dep_) ) {
  print "$dep_ is not valid SHC\n";
  exit(0);
}

if ( !IsValidShc($indep_) && !IsValidPort($indep_) ) {
  print "$indep_ is not valid SHC|PORT\n";
  exit(0);
}

my $dep_exchange_ = GetProductExchange($dep_);

my $indep_exchange_ = "";
if ( IsValidShc($indep_) ) { $indep_exchange_ = GetProductExchange($indep_); }


my ( $dep_start_trd_time_, $dep_end_trd_time_, $dep_timezone_, @dep_break_times_ ) =
  GetProductTradingTimes( $dep_, $dep_exchange_ );
my ( $indep_start_trd_time_, $indep_end_trd_time_, $indep_timezone_, @indep_break_times_ ) =
  GetProductTradingTimes( $indep_, $indep_exchange_ );

my $is_ret_lrdb_ = $lrdb_type_str_ eq "RETURNS" ? 1 : 0;

my @sessions_ = ();
GetExchangeSessions($dep_exchange_, \@sessions_);
my %sessionids_to_start_end_times_ = map { $$_[0] => $$_[1]."-".$$_[2] } @sessions_;

#print "$dep_start_trd_time_ , $dep_end_trd_time_ , $dep_timezone_ , @dep_break_times_ \n";
#print "$indep_start_trd_time_, $indep_end_trd_time_ , $indep_timezone_ , @indep_break_times_\n";
#print "@session_timings_\n";


$indep_start_trd_time_ = ConvertTZ_HHMMToNewTZ_HHMM( $indep_start_trd_time_, $dep_timezone_ );
$indep_end_trd_time_   = ConvertTZ_HHMMToNewTZ_HHMM( $indep_end_trd_time_,   $dep_timezone_ );
if ( ( split( "_", $dep_start_trd_time_ ) )[1] > ( split( "_", $dep_end_trd_time_ ) )[1] ) {
  $dep_start_trd_time_ = "PREV_" . $dep_start_trd_time_;
}
if ( ( split( "_", $indep_start_trd_time_ ) )[1] > ( split( "_", $indep_end_trd_time_ ) )[1] ) {
  $indep_start_trd_time_ = "PREV_" . $indep_start_trd_time_;
}

my %sessionid_to_dep_indep_overlap_time_ = ();
foreach my $sessionid_ (keys %sessionids_to_start_end_times_) {
  my ( $session_start_hhmm_, $session_end_hhmm_ ) = split( "-", $sessionids_to_start_end_times_{ $sessionid_ } );
  $session_start_hhmm_ = ConvertTZ_HHMMToNewTZ_HHMM( $session_start_hhmm_, $dep_timezone_ );
  $session_end_hhmm_   = ConvertTZ_HHMMToNewTZ_HHMM( $session_end_hhmm_,   $dep_timezone_ );
  if ( ( split( "_", $session_start_hhmm_ ) )[1] > ( split( "_", $session_end_hhmm_ ) )[1] ) {
    $session_start_hhmm_ = "PREV_" . $session_start_hhmm_;
  }
#  print "$dep_start_trd_time_ $dep_end_trd_time_   $indep_start_trd_time_ $indep_end_trd_time_ $session_start_hhmm_ $session_end_hhmm_\n";
  $sessionid_to_dep_indep_overlap_time_{ $sessionid_ } = GetSessionOverlapTimeNew(
      $dep_start_trd_time_, $dep_end_trd_time_,   $indep_start_trd_time_,
      $indep_end_trd_time_, $session_start_hhmm_, $session_end_hhmm_
      );
}

foreach my $sessionid_ (keys %sessionid_to_dep_indep_overlap_time_) {
  next if ( $sessionid_to_dep_indep_overlap_time_{$sessionid_} eq "NA-NA" );
  my ($start_time_, $end_time_) = split( "-", $sessionid_to_dep_indep_overlap_time_{$sessionid_} );
  next if ( ! defined $start_time_ || ! defined $end_time_ );

  my ($existing_start_time_, $existing_end_time_) = FetchLRDBPair($dep_, $indep_, $sessionid_, $is_ret_lrdb_);

# Insert if the pair doesn't exist or recompute is set to 1
  if ( defined $existing_start_time_ && defined $existing_end_time_ ) {
    print "WARNING! $dep_ $indep_ pair already exists\n";
    if ( $mode_ == 0 ) {
      print "Skipping as mode==0\n";
      next;
    }
    else {
      print "Updating the existing pair as mode!=0\n";
    }
  }

  InsertLRDBPair( $dep_, $indep_, $sessionid_, $start_time_, $end_time_, 0, $is_ret_lrdb_);
}


sub GetProductExchange {
  my $shc_          = shift;
  my $today_        = `date +%Y%m%d`; chomp ($today_);
  my $exec_cmd      = "$BIN_DIR/get_exch_from_shortcode $shc_ $today_";
  my $shc_exchange_ = `$exec_cmd`; chomp($shc_exchange_);
  return $shc_exchange_;
}

sub ConvertTZ_HHMMToNewTZ_HHMM {
  my $tz_hhmm_   = shift;
  my $new_tz_    = shift;
  my $tz_string_ = GetTZStrFromDVCTZCode($new_tz_);
  $ENV{TZ} = $tz_string_;

  #  print "$tz_hhmm_, $new_tz_, $tz_string_\n";
  #  if ($tz_hhmm_ eq "NA") { return "NA";}
  my $time_stamp_ = GetTimestampFromTZ_HHMM_DATE( $tz_hhmm_, 20160102 );
  my $new_tz_hhmm_ = `date \"\+\%Z_\%H\%M" -d\@$time_stamp_`;
  chomp($new_tz_hhmm_);
  if ( $new_tz_ eq "AST" ) {
    my @tkns_ = split( "_", $new_tz_hhmm_ );
    $new_tz_hhmm_ = "AST_" . $tkns_[$#tkns_];
  } 
  if ( $new_tz_ eq "BRT" ) {
    my @tkns_ = split( "_", $new_tz_hhmm_ );
    $new_tz_hhmm_ = "BRT_" . $tkns_[$#tkns_];
  }
  if ( $new_tz_ eq "BST" ) {
    my @tkns_ = split( "_", $new_tz_hhmm_ );
    $new_tz_hhmm_ = "BST_" . $tkns_[$#tkns_];
  }
  return ($new_tz_hhmm_);
}

sub GetProductTradingTimes {
  my $dep_shortcode = shift;
  my $dep_exchange  = shift;
  if ( IsValidShc($dep_shortcode) ) {
    my $dep_start_time_  = "NA";
    my $dep_end_time_    = "NA";
    my $dep_timezone_    = "NA";
    my @dep_break_times_ = ();
    my $exchange_trading_time_file_ = $BASETRADEINFODIR . "/NewLRDBBaseDir/" . ( lc $dep_exchange ) . "-trd-hours.txt";

    if ( -e $exchange_trading_time_file_ ) {
      open EXCH_TRADING_TIMES_FILE, "< $exchange_trading_time_file_ "
        or PrintStacktraceAndDie("could not open $exchange_trading_time_file_ for reading\n");
      my @lines_ = <EXCH_TRADING_TIMES_FILE>;
      chomp(@lines_);
      close EXCH_TRADING_TIMES_FILE;

      foreach my $line_ (@lines_) {
        my @words_ = split( /\s+/, $line_ );
        chomp(@words_);
        if ( $words_[0] eq $dep_shortcode ) {
          $dep_start_time_ = $words_[1];
          $dep_start_time_ =~ s/[:]//g;
          $dep_end_time_ = $words_[2];
          $dep_end_time_ =~ s/[:]//g;
          $dep_timezone_    = $words_[3];
          @dep_break_times_ = @words_[ 4 .. $#words_ ];
          last;
        }
      }
      if ( $dep_start_time_ eq "NA" ) {
        PrintStacktraceAndDie("could not find $dep_shortcode in $exchange_trading_time_file_\n");
      }
      $dep_start_time_ = $dep_timezone_ . "_" . $dep_start_time_;
      $dep_end_time_   = $dep_timezone_ . "_" . $dep_end_time_;

      for my $i_ ( 0 .. $#dep_break_times_ ) {
        $dep_break_times_[$i_] =~ s/[:]//g;
        $dep_break_times_[$i_] = $dep_timezone_ . "_" . $dep_break_times_[$i_];
      }
    }
    else { PrintStacktraceAndDie("could not find $exchange_trading_time_file_ \n"); }
    return ( $dep_start_time_, $dep_end_time_, $dep_timezone_, @dep_break_times_ );
  }
  elsif ( IsValidPort($dep_shortcode) ) {
    return GetPortfolioTradingTimes($dep_shortcode);
  }
}

sub GetPortfolioTradingTimes {
  my $portfolio_               = shift;
  my @portfolio_shcs_trd_time_ = ();
  my $exec_cmd                 = "grep -w $portfolio_ $PORTFOLIO_INPUTS | head -1";
  my $portfolio_line_          = `$exec_cmd`;
  chomp($portfolio_line_);
  my @portfolio_shcs_ = split( " ", $portfolio_line_ );
  chomp(@portfolio_shcs_);

  #  print "@portfolio_shcs_\n";
  @portfolio_shcs_ = @portfolio_shcs_[ 2 .. $#portfolio_shcs_ ];

  #  print "@portfolio_shcs_\n";
  foreach my $shc_ (@portfolio_shcs_) {
    my $shc_exchange_ = GetProductExchange($shc_);
    my @shc_trd_time_ = GetProductTradingTimes( $shc_, $shc_exchange_ );

    #    print "@shc_trd_time_\n";
    #    @shc_trd_time_ = @shc_trd_time_[0..2]
    my $shc_start_trd_hhmm_ = ConvertTZ_HHMMToNewTZ_HHMM( $shc_trd_time_[0], "UTC" );
    my $shc_end_trd_hhmm_   = ConvertTZ_HHMMToNewTZ_HHMM( $shc_trd_time_[1], "UTC" );

    #    print "$shc_start_trd_hhmm_,$shc_end_trd_hhmm_\n";
    push( @portfolio_shcs_trd_time_, [ $shc_start_trd_hhmm_, $shc_end_trd_hhmm_ ] );
  }
  my $port_start_hhmm_ = $portfolio_shcs_trd_time_[0][0];
  my $port_end_hhmm_   = $portfolio_shcs_trd_time_[0][1];
  foreach my $shc_start_end_hhmm_ (@portfolio_shcs_trd_time_) {

#    ($port_start_hhmm_,$port_end_hhmm_) = GetOverlapTime ($port_start_hhmm_,$port_end_hhmm_,@$shc_start_end_hhmm_[0],@$shc_start_end_hhmm_[1]);
#    print "$port_start_hhmm_,$port_end_hhmm_,@$shc_start_end_hhmm_[0],@$shc_start_end_hhmm_[1]\n";
    ( $port_start_hhmm_, $port_end_hhmm_ ) = split(
      "-",
      GetSessionOverlapTimeNew(
        $port_start_hhmm_,        $port_end_hhmm_,          @$shc_start_end_hhmm_[0],
        @$shc_start_end_hhmm_[1], @$shc_start_end_hhmm_[0], @$shc_start_end_hhmm_[1]
      )
    );
  }

  #  print "$port_start_hhmm_,$port_end_hhmm_\n";
  return ( $port_start_hhmm_, $port_end_hhmm_, "UTC" );
}

sub GetSessionOverlapTimeNew {
  my $shc1_start_hhmm_    = shift;
  my $st_1_               = $shc1_start_hhmm_;
  my $tz_                 = "UTC";
  my $shc1_end_hhmm_      = shift;
  my $et_1_               = $shc1_end_hhmm_;
  my $shc2_start_hhmm_    = shift;
  my $st_2_               = $shc2_start_hhmm_;
  my $shc2_end_hhmm_      = shift;
  my $et_2_               = $shc2_end_hhmm_;
  my $session_start_hhmm_ = shift;
  my $st_3_               = $session_start_hhmm_;
  my $session_end_hhmm_   = shift;
  my $et_3_               = $session_end_hhmm_;
  if ( $st_1_ eq "NA"
    or $st_2_ eq "NA"
    or $et_1_ eq "NA"
    or $et_2_ eq "NA"
    or $st_3_ eq "NA"
    or $et_3_ eq "NA" )
  {
    return ( "NA", "NA" );
  }
  if ( index( $st_1_, "PREV" ) != -1 ) {
    ( $_, $tz_, $st_1_ ) = split( '_', $st_1_ );
    $et_1_ = ( split( '_', $et_1_ ) )[-1];
  }
  else {
    ( $tz_, $st_1_ ) = split( '_', $st_1_ );
    $et_1_ = ( split( '_', $et_1_ ) )[1];
  }
  if ( index( $st_2_, "PREV" ) != -1 ) {
    ( $_, $tz_, $st_2_ ) = split( '_', $st_2_ );
    $et_2_ = ( split( "_", $et_2_ ) )[-1];
  }
  else {
    $st_2_ = ( split( '_', $st_2_ ) )[1];
    $et_2_ = ( split( '_', $et_2_ ) )[1];
  }
  if ( index( $st_3_, "PREV" ) != -1 ) {
    ( $_, $tz_, $st_3_ ) = split( '_', $st_3_ );
    $et_3_ = ( split( "_", $et_3_ ) )[-1];
  }
  else {
    ( $tz_, $st_3_ ) = split( '_', $st_3_ );
    $et_3_ = ( split( '_', $et_3_ ) )[1];
  }
  my %start_hhmm_to_idx_map_ = ();
  my %end_hhmm_to_idx_map_   = ();
  my %idx_to_active_map_     = ();
  my $overlap_st_            = "NA";
  my $overlap_et_            = "NA";
  my @overlap_st_vec_        = ();
  my @overlap_et_vec_        = ();
  my @st_vec_                = ( $st_1_, $st_2_, $st_3_ );
  my @et_vec_                = ( $et_1_, $et_2_, $et_3_ );

  foreach my $st_idx_ ( 0 .. $#st_vec_ ) {
    push( @{ $start_hhmm_to_idx_map_{ $st_vec_[$st_idx_] } },        $st_idx_ + 1 );
    push( @{ $start_hhmm_to_idx_map_{ $st_vec_[$st_idx_] + 2400 } }, $st_idx_ + 1 );
  }
  foreach my $et_idx_ ( 0 .. $#et_vec_ ) {
    push( @{ $end_hhmm_to_idx_map_{ $et_vec_[$et_idx_] } },        $et_idx_ + 1 );
    push( @{ $end_hhmm_to_idx_map_{ $et_vec_[$et_idx_] + 2400 } }, $et_idx_ + 1 );
  }
  my @start_end_hhmm_ = (
    $st_1_,        $et_1_,        $st_2_,        $et_2_,        $st_3_,        $et_3_,
    $st_1_ + 2400, $et_1_ + 2400, $st_2_ + 2400, $et_2_ + 2400, $st_3_ + 2400, $et_3_ + 2400
  );
  @start_end_hhmm_ = sort { $a <=> $b } @start_end_hhmm_;
  foreach my $key_ ( 1 .. 3 ) {
    $idx_to_active_map_{$key_} = 0;
  }
  my $num_active_intervals_ = 0;
  foreach my $hhmm_ (@start_end_hhmm_) {
    my @st_idx_ = ();
    my @et_idx_ = ();
    if ( $num_active_intervals_ < 3 ) {
      $num_active_intervals_ = 0;
      if ( exists $start_hhmm_to_idx_map_{$hhmm_} ) {
        @st_idx_ = @{ $start_hhmm_to_idx_map_{$hhmm_} };
        foreach my $idx_ (@st_idx_) {
          $idx_to_active_map_{$idx_} = 1;
        }
      }
      if ( exists $end_hhmm_to_idx_map_{$hhmm_} ) {
        @et_idx_ = @{ $end_hhmm_to_idx_map_{$hhmm_} };
        foreach my $idx_ (@et_idx_) {
          $idx_to_active_map_{$idx_} = 0;
        }
      }
      $num_active_intervals_ = 0;
      foreach my $key_ ( 1 .. 3 ) {
        $num_active_intervals_ = $idx_to_active_map_{$key_} + $num_active_intervals_;
      }
      if ( $num_active_intervals_ == 3 ) {
        $overlap_st_ = $hhmm_;
        push (@overlap_st_vec_, $overlap_st_ );
      }
    }
    else {
      if ( exists $end_hhmm_to_idx_map_{$hhmm_} ) { 
        $overlap_et_ = $hhmm_;
        push (@overlap_et_vec_ , $overlap_et_); 
        @et_idx_ = @{ $end_hhmm_to_idx_map_{$hhmm_} };
        foreach my $idx_ (@et_idx_) {
          $idx_to_active_map_{$idx_} = 0;
        }
        $num_active_intervals_ = 0;
        foreach my $key_ ( 1 .. 3 ) {
          $num_active_intervals_ = $idx_to_active_map_{$key_} + $num_active_intervals_;
        }
      }
    }
#    print "$hhmm_ $num_active_intervals_\n";
  }
#  print "@overlap_st_vec_ $#overlap_st_vec_\n";
#  print "@overlap_et_vec_ $#overlap_et_vec_\n";
  my $max_overlap_idx_ = 0;
  if ($#overlap_st_vec_ == $#overlap_et_vec_) {
    my $max_overlap_min_ = 0;
    foreach my $idx_ (0 .. $#overlap_et_vec_) {
      my $overlap_min_ = int($overlap_et_vec_[$idx_] / 100) * 60 + ($overlap_et_vec_[$idx_] % 100)  - (int($overlap_st_vec_[$idx_] / 100) * 60 + ($overlap_st_vec_[$idx_] % 100) );
      if ($overlap_min_ > $max_overlap_min_) {
        $max_overlap_min_ = $overlap_min_ ;
        $max_overlap_idx_ = $idx_ ;
      }
    }
  }
  if ($#overlap_st_vec_ != -1) {$overlap_st_ = $overlap_st_vec_[$max_overlap_idx_];}
  else {$overlap_st_ = "NA";}
  if ($#overlap_et_vec_ != -1) {$overlap_et_ = $overlap_et_vec_[$max_overlap_idx_];}
  else {$overlap_et_ = "NA";}
#  print "$overlap_st_ $overlap_et_\n";
  if ( $overlap_st_ eq "NA" or $overlap_et_ eq "NA" ) { return ("NA-NA"); }
  if ( $overlap_st_ > $overlap_et_ ) { return ("NA-NA"); }
  if ( $overlap_st_ >= 2400 ) { $overlap_st_ = $overlap_st_ % 2400; }
  if ( $overlap_et_ > 2400 )  { $overlap_et_ = $overlap_et_ % 2400; }
  return ( $tz_ . "_" . $overlap_st_ . "-" . $tz_ . "_" . $overlap_et_ );
}


