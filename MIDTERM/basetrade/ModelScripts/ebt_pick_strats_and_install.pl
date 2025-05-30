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
use Term::ANSIColor;
use sigtrap qw(handler signal_handler normal-signals error-signals);

use Term::ANSIColor 1.04 qw(uncolor);
my $names = uncolor('01;31');

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

my $USAGE="$0 CONFIGFILE TIMEPERIOD";


if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $mail_body_ = "";

my $global_config_file_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $curr_date_ = `date +%Y%m%d`; chomp ( $curr_date_ );

my @shc_vec_ = ( );
my %shc_to_strat_ = ( );
my %shc_to_uts_ = ( );
my %shc_to_maxloss_ = ( );
my %shc_to_opentrade_loss_ = ( );
my %shc_to_queryid_ = ( );
my %shc_to_stop_queryid_ = ( );
my %shc_to_exch_ = ( );
my %shc_to_config_ = ( );
my $query_id_pref_;
my ($exec_start_hhmm_, $exec_end_hhmm_) = ( );
my $mail_address_ = ( );
my $risk_tags_ = ( );

LoadConfigFile ( $global_config_file_ );

SanityCheck ( );

PrepConfigs ( );

RunPickStrats ( );

#UpdateCrontab ( );

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
            push ( @{$shc_to_opentrade_loss_{$t_words_[0]}}, $t_words_[2] );
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
          if ( $risk_tags_ !~ /EBT/ ) {
            $risk_tags_ .= ":EBT";
          }
        }
      }
    }
  }
#print all the values of hash table
#foreach (sort keys %shc_to_uts_) {
#      print "$_ : $shc_to_uts_{$_}[1]\n";
#        }
  return;
}

sub SanityCheck
{
  print "Sanity Checking ...\n";

  if ( $#shc_vec_ < 0 ) {
    PrintErrorAndDie ( "Strats for no SHORTCODE provided" );
  }

  my $queryids_map_file_ = $AFLASHSCRIPTS_DIR."/queryids_map.txt";
  open FHANDLE, "< $queryids_map_file_" or PrintStacktraceAndDie ( "Could not open $queryids_map_file_ for reading" );
  my @qlines_ = <FHANDLE>; chomp ( @qlines_ );
  close FHANDLE;
  my %shc_to_id_ = map { (split(/\s+/, $_))[0] => (split(/\s+/, $_))[1] } @qlines_;

  foreach my $shc_ ( @shc_vec_ ) {
    if ( ! defined $shc_to_strat_{ $shc_ } ) {
      PrintErrorAndDie ( "Strat for $shc_ not provided" );
    }
    if ( ! defined $shc_to_uts_{ $shc_ } ) {
      PrintErrorAndDie ( "UTS for $shc_ not provided" );
    }
    if ( ! defined $shc_to_maxloss_{ $shc_ } ) {
      PrintErrorAndDie ( "MaxLoss for $shc_ not provided" );
    }
    if ( ! defined $shc_to_opentrade_loss_{ $shc_ } ) {
      PrintErrorAndDie ( "OpenTrade Loss for $shc_ not provided" );
    }
    if ( ! defined $shc_to_id_{ $shc_ } ) {
      PrintErrorAndDie ( "Shortcode index for $shc_ not mentioned" );
    }

    my $nstrats_ = scalar @{$shc_to_strat_{ $shc_ }};
    if ( $nstrats_ < 1 ) {
      PrintErrorAndDie ( "No strats for $shc_" );
    }
    if ( $nstrats_ > 10 ) {
      PrintErrorAndDie ( "More strats for $shc_ than allowed (10)" );
    }
    if ( $#{$shc_to_uts_{ $shc_ }} != $nstrats_-1 ) {
      PrintErrorAndDie ( "No. of UTS entries for $shc_ don't match no. of strats" );
    }
    if ( $#{$shc_to_maxloss_{ $shc_ }} != $nstrats_-1 ) {
      PrintErrorAndDie ( "No. of MaxLoss entries for $shc_ don't match no. of strats" );
    }
    if ( $#{$shc_to_opentrade_loss_{ $shc_ }} != $nstrats_-1 ) {
      PrintErrorAndDie ( "No. of OpenTradeLoss entries for $shc_ don't match no. of strats" );
    }

    my $shc_id_ = $query_id_pref_.sprintf("%03d",$shc_to_id_{$shc_});

    push ( @{$shc_to_queryid_{$shc_}}, $shc_id_.$_ ) foreach 0 .. ($nstrats_-1);
    $shc_to_stop_queryid_{$shc_} = $shc_id_."9";

    my @ADDITIONAL_EXEC_PATHS ;
    my $get_contract_specs_exec = SearchExec ( "get_contract_specs", @ADDITIONAL_EXEC_PATHS ) ;    

    my $t_exchange_ = `$get_contract_specs_exec $shc_ $curr_date_ EXCHANGE | awk '{ print \$2}'`;
    chomp ( $t_exchange_ );
    if ( ! defined $t_exchange_ || $t_exchange_ eq "" ) {
      PrintErrorAndDie ( "Exchange could not be fetched from $shc_" );
    }
    $shc_to_exch_{ $shc_ } = $t_exchange_;
  }
}

sub PrepConfigs
{
  my $config_template_ = "$HOME_DIR/basetrade/AflashScripts/EBT_config.txt";

  if ( ! -e $config_template_ ) { 
    PrintErrorAndDie ( "$config_template_ does not exist" );
  }

  my $config_dir_ = "/home/dvctrader/modelling/pick_strats_config/EBT_temp";

  if ( ! -d $config_dir_ ) {
    `mkdir -p $config_dir_`;
  }

  my $config_base_ = basename($global_config_file_);
  foreach my $shc_ ( @shc_vec_ ) {
    my $config_ = $config_dir_."/".$shc_."_".$config_base_;

    my $cmd_ = "cp $config_template_ $config_";
    `$cmd_`;

    my $nstrats_ = scalar @{$shc_to_strat_{ $shc_ }};
    
    my $global_maxloss_ = 0;
    $global_maxloss_ += ($shc_to_maxloss_{$shc_}[$_] * $shc_to_uts_{$shc_}[$_] * 2.0) foreach 0 .. ($nstrats_-1);

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ SHORTCODE $shc_";
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ EXCHANGE ".$shc_to_exch_{$shc_};
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ NUM_STRATS_TO_INSTALL ".join(" ", map {1} 1..$nstrats_);
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ NUM_STRAT_FILES_TO_INSTALL 4";
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ USE_OPTIMAL_MAX_LOSS_PER_UNIT_SIZE ".join(" ", map {0} 1..$nstrats_);
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ SORT_ALGO ".join(" ", map {"PNL_SHARPE_AVG"} 1..$nstrats_);
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ TOTAL_SIZE_TO_RUN ".join(" ", @{$shc_to_uts_{$shc_}});
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ STRATS_TO_KEEP ".join(" ", @{$shc_to_strat_{$shc_}});
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ MAX_LOSS_PER_UNIT_SIZE ".join(" ", @{$shc_to_maxloss_{$shc_}});
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ OPENTRADE_LOSS_PER_UNIT_SIZE ".join(" ", @{$shc_to_opentrade_loss_{$shc_}});
    print $cmd_."\n";
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ GLOBAL_MAX_LOSS ".$global_maxloss_;
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ PROD_QUERY_START_ID ".min(@{$shc_to_queryid_{$shc_}});
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ PROD_QUERY_STOP_ID ".$shc_to_stop_queryid_{$shc_};
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ EXEC_START_HHMM ".$exec_start_hhmm_;
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ EXEC_END_HHMM ".$exec_end_hhmm_;
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ EMAIL_ADDRESS ".$mail_address_;
    `$cmd_`;

    $cmd_ = "$SCRIPTS_DIR/set_config_field.pl $config_ RISK_TAGS ".$risk_tags_;
    `$cmd_`;

    $shc_to_config_{ $shc_ } = $config_;
  }

  print "Configs:\n";
  print $_." ".$shc_to_config_{ $_ }."\n" foreach @shc_vec_;
}

sub RunPickStrats
{
  my $PICKSTRAT_TEMP_DIR = "/spare/local/pickstrats_logs/temp_dir";
  if ( ! -e $PICKSTRAT_TEMP_DIR ) { `mkdir $PICKSTRAT_TEMP_DIR`; }

  print "\n";
  foreach my $shc_ ( @shc_vec_ ) {
    my $pickstrat_log_ = $PICKSTRAT_TEMP_DIR."/ps_log_".basename($shc_to_config_{$shc_});
    my $exec_cmd_ = "$MODELSCRIPTS_DIR/pick_strats_and_install.pl $shc_ EBT ".$shc_to_config_{ $shc_ };
    print $exec_cmd_."\n";
    `$exec_cmd_ &> $pickstrat_log_`;
    my @pickstrat_lines_ = `cat $pickstrat_log_`; chomp ( @pickstrat_lines_ );
    if ( grep { $_ =~ /Adding to Crontab:/ } @pickstrat_lines_ ) {
      print colored("Pickstrat Run for $shc_ Successful!", 'blue'), "\n";
    } else {
      print colored("Pickstrat Run for $shc_ FAILED!!", 'bold red'), "\n";
    }
    print "\tLogfile: $pickstrat_log_\n\n"
  }
}

sub UpdateCrontab
{
  my $config_base_ = basename($global_config_file_);
  my $LOCAL_PRODUCTION_STRAT_LIST_DIR = $HOME_DIR."/production_strat_list";
  my $crontab_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$config_base_."_crontab";

  `crontab -l > $crontab_file_`;
  my $remove_installation_script_ = $SCRIPTS_DIR."/ebt_remove_crontab_config.pl";
  my $remove_installation_cmd_ = "$remove_installation_script_ $global_config_file_ $timeperiod_";
  my $cron_time_utc_ = `$BASETRADE_BIN_DIR/get_utc_hhmm_str $exec_end_hhmm_ $curr_date_`; chomp($cron_time_utc_);
  my $hh_ = substr($cron_time_utc_,2,2);
  my $mm_ = substr($cron_time_utc_,0,2);

  `echo -e "#EBT REMOVAL\n$hh_ $mm_ * * 0-6 $remove_installation_cmd_\n" >> $crontab_file_`;
  `crontab $crontab_file_`;
}

sub PrintErrorAndDie
{
  my $error_string_ = "@_";

  print STDERR "\nERROR: ".$error_string_."\n";

  die;
}

