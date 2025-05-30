#!/usr/bin/perl
#
## \file ModelScripts/multiple_pick_strats_and_install.pl
##
## \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
##  Address:
##    Suite No 162, Evoma, #14, Bhattarhalli,
##    Old Madras Road, Near Garden City College,
##    KR Puram, Bangalore 560049, India
##    +91 80 4190 3551
#

use strict;
use warnings;
#use Perl::Critic;
#use warnings::unused;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use FileHandle;
use POSIX;
use sigtrap qw(handler signal_handler normal-signals error-signals);

sub LoadConfigFile;
sub SanityCheck;
sub PrepConfigs;
sub RunPickStrats;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $AFLASHSCRIPTS_DIR = $HOME_DIR."/".$REPO."/AflashScripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $BASETRADE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/install_strategy_production.pl"; # CopyFileCreateDir
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec
my $USAGE="$0 CONFIGFILE TIMEPERIOD SEETRADES/SEETRADES_FULL/LISTPROCS/user_msg_in quotes> [DATE]";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $mail_body_ = "";

my $global_config_file_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $user_msg_ = $ARGV [ 2 ];
my $curr_date_ = `date +%Y%m%d`; chomp ( $curr_date_ );
my $date_ = $curr_date_; 
if ( $#ARGV > 2 ) { $date_ = $ARGV [ 3 ]; }
my $yy_ = substr $date_, 0, 4;
my $mm_ = substr $date_, 4, 2;
my $dd_ = substr $date_, 6, 2;

my @shc_vec_ = ( );
my %shc_to_strat_ = ( );
my %shc_to_uts_ = ( );
my %shc_to_maxloss_ = ( );
my %shc_to_opentrade_loss_ = ( );
my %shc_to_queryid_ = ( );
my %shc_to_exch_ = ( );
my %shc_to_config_ = ( );
my %shc_to_machine_ = ( );
my $query_id_pref_;
my ($exec_start_hhmm_, $exec_end_hhmm_) = ( );
my $mail_address_ = ( );
my $risk_tags_ = ( );

LoadConfigFile ( $global_config_file_ );

SanityCheck ( );

SendUserMsgs ( $user_msg_ );

sub LoadConfigFile
{
  print "Loading Config File ...\n";

  my ( $t_config_file_ ) = @_;

  open ( CONFIG_FILE , "<" , $t_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $t_config_file_" );
  my @config_file_lines_ = <CONFIG_FILE>; chomp ( @config_file_lines_ );
  close ( CONFIG_FILE );

  my $current_param_ = "";
  foreach my $config_file_lines_ ( @config_file_lines_ ) {
    next if ( index ( $config_file_lines_ , "#" ) == 0 ); # not ignoring lines with # not at the beginning

    my @t_words_ = split ( ' ' , $config_file_lines_ );

    if ( $#t_words_ < 0 ) {
      $current_param_ = "";
      next;
    }

    if ( ! $current_param_ ) {
      $current_param_ = $t_words_ [ 0 ];
      next;
    }
    else {
      given ( $current_param_ ) {
        when ( "SHORTCODE-STRAT" )
        {
          if ( $#t_words_ >= 1 ) {
            if ( ! FindItemFromVec ( $t_words_[0], @shc_vec_ ) ) {
              push ( @shc_vec_, $t_words_[0] );
            }
            push ( @{$shc_to_strat_{$t_words_[0]}}, $t_words_[1] );
          }
        }
        when ( "SHORTCODE-UTS" )
        {
          if ( $#t_words_ >= 1 ) {
            push ( @{$shc_to_uts_{$t_words_[0]}}, $t_words_[1] );
          }
        }
        when ( "SHORTCODE-MAX_LOSS_PER_UNIT_SIZE-OPENTRADE_LOSS_PER_UNIT_SIZE" ) {
          if ( $#t_words_ >= 1 ) {
            push ( @{$shc_to_maxloss_{$t_words_[0]}}, $t_words_[1] );
          }
          if ( $#t_words_ >= 2 ) {
            push ( @{$shc_to_maxloss_{$t_words_[0]}}, $t_words_[1] );
          }
        }
        when ( "PROD_QUERY_ID_PREFIX" ) {
          $query_id_pref_ = $t_words_[0];
        }
        when ( "EXEC_START_HHMM" ) {
          $exec_start_hhmm_ = $t_words_[0];
        }
        when ( "EXEC_END_HHMM" ) {
          $exec_end_hhmm_ = $t_words_[0];
        }
        when ( "EMAIL_ADDRESS" ) {
          $mail_address_ = $t_words_[0];
        }
        when ( "RISK_TAGS" ) {
          $risk_tags_ = $t_words_[0];
        }
      }
    }
  }
  return;
}

sub SanityCheck
{
  print "Sanity Checking ...\n";

  if ( $#shc_vec_ < 0 ) {
    PrintErrorAndDie ( "Strats for no SHORTCODE provided" );
  }

  my $machine_shc_file_ = "/home/pengine/prod/live_configs/machine_shc.txt";
  open MSFILE, "< $machine_shc_file_" or PrintErrorAndDie( "Could not open $machine_shc_file_ for reading" );
  my @ms_lines_ = <MSFILE>; chomp ( @ms_lines_ );
  close MSFILE;
  %shc_to_machine_ = map { (split(/\s+/, $_))[1] => (split(/\s+/, $_))[0] } @ms_lines_;

  my $queryids_map_file_ = $AFLASHSCRIPTS_DIR."/queryids_map.txt";
  open FHANDLE, "< $queryids_map_file_" or PrintStacktraceAndDie ( "Could not open $queryids_map_file_ for reading" );
  my @qlines_ = <FHANDLE>; chomp ( @qlines_ );
  close FHANDLE;
  my %shc_to_id_ = map { (split(/\s+/, $_))[0] => (split(/\s+/, $_))[1] } @qlines_;

  foreach my $shc_ ( @shc_vec_ ) {
    if ( ! defined $shc_to_strat_{ $shc_ } ) {
      PrintErrorAndDie ( "Strat for $shc_ not provided" );
    }
    if ( ! defined $shc_to_id_{ $shc_ } ) {
      PrintErrorAndDie ( "Shortcode index for $shc_ not mentioned" );
    }

    my $nstrats_ = scalar @{$shc_to_strat_{ $shc_ }};
    if ( $nstrats_ < 1 ) {
      PrintErrorAndDie ( "No strats for $shc_" );
    }

    my $shc_id_ = $query_id_pref_.sprintf("%03d",$shc_to_id_{$shc_});

    if ( $date_ < 20170303 ) {
      push ( @{$shc_to_queryid_{$shc_}}, $shc_id_ );
    } else {
      push ( @{$shc_to_queryid_{$shc_}}, $shc_id_.$_ ) foreach 0 .. ($nstrats_-1);
    }
    my @ADDITIONAL_EXEC_PATHS ;
    my $get_contract_specs_exec = SearchExec ( "get_contract_specs", @ADDITIONAL_EXEC_PATHS ) ;

    my $t_exchange_ = `$get_contract_specs_exec $shc_ $curr_date_ EXCHANGE | awk '{ print \$2}'`;

#my $t_exchange_ = `$BASETRADE_BIN_DIR/get_contract_specs $shc_ $curr_date_ EXCHANGE | awk '{ print \$2}' `;
    if ( ! defined $t_exchange_ || $t_exchange_ eq "" ) {
      PrintErrorAndDie ( "Exchange could not be fetched from $shc_" );
    }
    $shc_to_exch_{ $shc_ } = $t_exchange_;

    if ( ! defined $shc_to_machine_{ $shc_ } ) {
      PrintErrorAndDie ( "Prod Server could not be fetched for $shc_" );
    }
  }
}

sub SendUserMsgs
{
  my $user_msg_ = shift;

  if ( $date_ < 20161216 &&
      ( $user_msg_ eq "SEETRADES" || $user_msg_ eq "SEETRADES_FULL" ) ) {
    my $t_start_query_id_ = $query_id_pref_."01";
    my $t_stop_query_id_ = $query_id_pref_."20";

    foreach my $query_id_ ( $t_start_query_id_..$t_stop_query_id_ ) {
      my $cmd_ = "";
      my $tradefile_ = "/NAS1/logs/QueryTrades/$yy_/$mm_/$dd_/trades.$date_.$query_id_";
      next if ( ! -f $tradefile_ );

      if ( $user_msg_ eq "SEETRADES" ) {
        $cmd_ = "tail -n1 $tradefile_";
      } elsif ( $user_msg_ eq "SEETRADES_FULL" ) {
        $cmd_ = "cat $tradefile_";
      }
      my @cmdout_ = `$cmd_ 2>&1`; chomp(@cmdout_);
      print $_."\n" foreach @cmdout_;
    }
    return;
  }

  foreach my $shc_ ( @shc_vec_ ) {
    my $main_query_id_ = "None";
    foreach my $query_id_ ( @{$shc_to_queryid_{ $shc_ }} ) {
      my $serv_id_ = $shc_to_machine_{ $shc_ };

     if ($main_query_id_ eq "None"){
       $main_query_id_ = $query_id_;
     }

      my $cmd_;

      if ( $user_msg_ eq "SEETRADES" ) {
        print "Trades for $query_id_:";
        if ( $date_ == $curr_date_ ) {
          $cmd_ = "ssh dvctrader\@$serv_id_ \"cat /spare/local/logs/tradelogs/trades.$date_.$main_query_id_|grep $query_id_|tail -n1\"";
        }
        else {
          $cmd_ = "cat /NAS1/logs/QueryTrades/$yy_/$mm_/$dd_/trades.$date_.$main_query_id_|grep $query_id_|tail -n1";
        }
      }
      elsif ( $user_msg_ eq "SEETRADES_FULL" ) {
        print "Trades for $query_id_:";
        if ( $date_ == $curr_date_ ) {
          $cmd_ = "ssh dvctrader\@$serv_id_ \"cat /spare/local/logs/tradelogs/trades.$date_.$query_id_\"";
        }
        else {
          $cmd_ = "cat /NAS1/logs/QueryTrades/$yy_/$mm_/$dd_/trades.$date_.$query_id_";
        }
      }
      elsif ( $user_msg_ eq "LISTPROCS" ) {
        $cmd_ = "ssh dvctrader\@$serv_id_ \"ps -efH | grep tradeinit | grep -w $query_id_\"";
      }
      else {
        $cmd_ = "ssh dvctrader\@$serv_id_ \"/home/pengine/prod/live_execs/user_msg $user_msg_ --traderid $query_id_\"";
        print "\n".$cmd_."\n";
      }
      my @cmdout_ = `$cmd_ 2>&1`; chomp(@cmdout_);
      print $_."\n" foreach @cmdout_;
    }
  } 
}

sub PrintErrorAndDie
{
  my $error_string_ = "@_";
  print STDERR "\nERROR: ".$error_string_."\n";
  die;
}

