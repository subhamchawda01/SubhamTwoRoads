#!/usr/bin/perl

# \file ModelScripts/multiple_pick_strats_and_install.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#    Suite No 162, Evoma, #14, Bhattarhalli,
#    Old Madras Road, Near Garden City College,
#    KR Puram, Bangalore 560049, India
#    +91 80 4190 3551

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

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";

my $LOCAL_PRODUCTION_STRAT_LIST_DIR = $HOME_DIR."/production_strat_list";
my $REMOTE_PRODUCTION_STRAT_LIST_DIR = "/home/dvctrader/production_strat_list";

require "$GENPERLLIB_DIR/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "$GENPERLLIB_DIR/calc_next_date.pl"; # CalcNextDate
require "$GENPERLLIB_DIR/install_strategy_production.pl"; # CopyFileCreateDir
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/read_shc_machine_mapping.pl"; # GetMachineForProduct
require "$GENPERLLIB_DIR/search_exec.pl"; # SearchExec

my $get_contract_specs_exec = SearchExec ( "get_contract_specs" );

my $USAGE="$0 CONFIGFILE TIMEPERIOD";


if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $mail_body_ = "";

my $config_file_ = $ARGV [ 0 ];
my $timeperiod_ = $ARGV [ 1 ];
my $curr_date_ = `date +%Y%m%d`; chomp ( $curr_date_ );

my @shc_vec_ = ( );
my %shc_to_exch_ = ( );
my %shc_to_serv_ = ( );
my $query_id_pref_;

LoadConfigFile ( );

FixProductionCrontab ( );

FixLocalCrontab ( );

sub LoadConfigFile
{
  print "Loading Config File ...\n";

  open ( CONFIG_FILE , "<" , $config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $config_file_" );
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
            push ( @shc_vec_, $t_words_[0] );
          }
        }
        when ( "PROD_QUERY_ID_PREFIX" ) {
          $query_id_pref_ = $t_words_[0];
        }
      }
    }
  }

  if ( $#shc_vec_ < 0 ) {
    PrintErrorAndDie ( "Strats for no SHORTCODE provided" );
  }
  @shc_vec_ = keys %{ { map {$_ => undef} @shc_vec_ } };

#my $curr_query_id_ = $start_query_id_;
  foreach my $shc_ ( @shc_vec_ ) {
#$shc_to_queryid_{ $shc_ } = $curr_query_id_;
#$curr_query_id_++;

    my $t_exchange_ = `$get_contract_specs_exec $shc_ $curr_date_ EXCHANGE | awk '{ print \$2}' `;
    chomp ( $t_exchange_ );
    if ( ! defined $t_exchange_ || $t_exchange_ eq "" ) {
      PrintErrorAndDie ( "Exchange could not be fetched from $shc_" );
    }
    $shc_to_exch_{ $shc_ } = $t_exchange_;

    my $install_location_ = GetMachineForProduct ( $shc_ );
    if ( ! defined $install_location_ ) {
      PrintErrorAndDie ( "INSTALL_LOCATION could NOT be fetched for shortcode $shc_" );
    }
    $shc_to_serv_{ $shc_ } = $install_location_;
  }
}

sub FixProductionCrontab
{
# Only remove installs for same timeperiod.
  my $time_period_comment_tag_ = "# ".$timeperiod_;

  foreach my $shortcode_ ( @shc_vec_ ) {
    my $exchange_ = $shc_to_exch_{ $shortcode_ };
    my $install_location_ = $shc_to_serv_{ $shortcode_ };

    my $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep $shortcode_ | grep start_real_trading | grep $exchange_ | grep \"$time_period_comment_tag_\"'";
    my @existing_queries_ = `$exec_cmd_`; chomp ( @existing_queries_ );

    $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l | grep stop_real_trading | grep $exchange_ | grep \"$time_period_comment_tag_\"'";
    my @existing_stops_ = `$exec_cmd_`; chomp ( @existing_stops_ );

# Match starts with stops , so we don't end up messing with
# cron jobs for different products.
    my %start_query_ids_ = ( );

    foreach my $existing_query_line_ ( @existing_queries_ ) {
      my @existing_query_words_ = split ( /\s+/ , $existing_query_line_ );

      my $t_query_id_ = 0;
      foreach my $i ( 0..$#existing_query_words_ ) {
        if ( $existing_query_words_[$i] eq $exchange_ ) {
          $t_query_id_ = $existing_query_words_[$i + 1];
          last;
        }
      }
      $start_query_ids_ { $t_query_id_ } = 1;
    }

    my @stop_lines_to_remove_ = ( );

    foreach my $stop_query_line_ ( @existing_stops_ ) {
      my @existing_stop_words_ = split ( /\s+/ , $stop_query_line_ );

      my $t_query_id_ = 0;
      foreach my $i ( 0..$#existing_stop_words_ ) {
        if ( $existing_stop_words_[$i] eq $exchange_ ) {
          $t_query_id_ = $existing_stop_words_[$i + 1];
          last;
        }
      }
      if ( exists ( $start_query_ids_{ $t_query_id_ } ) ) {
        push ( @stop_lines_to_remove_ , $stop_query_line_ );
      }
    }

# Need to remove all starts for this shortcode and exchange ( all in  @existing_queries_ )
# Need to remove only the stops which were for this shortcode & exchange ( all in @stop_lines_to_remove_ )

    $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab -l'";
    my @crontab_lines_ = `$exec_cmd_`; chomp ( @crontab_lines_ );

# Skip lines from a pervious install.
    my @lines_to_skip_ = ( );
    push ( @lines_to_skip_ , "################################################################################################" );
    push ( @lines_to_skip_ , "## [ $shortcode_ $exchange_ ] [" );

    my @fixed_crontab_lines_ = ( );

    foreach my $crontab_line_ ( @crontab_lines_ )
    {
#Remove existing installs for the same query ID (required here to filter matching installs with different tags)
      if ( ( index ( $crontab_line_ , "start_real_trading" ) >= 0 ) || ( index ( $crontab_line_ , "stop_real_trading" ) >= 0 ) ) {
        my @crontab_line_words_ = split ( /\s+/ , $crontab_line_ );

        my $t_query_id_ = 0;
        foreach my $i ( 0..$#crontab_line_words_ ) {
          if ( $crontab_line_words_[$i] eq $exchange_ ) {
            $t_query_id_ = $crontab_line_words_[$i + 1];
            last;
          }
        }
#next if ( $t_query_id_ == $shc_to_queryid_{ $shortcode_ } );
      }

#Remove existing installs for the same shortcode and tag
      next if ( FindItemFromVec ( $crontab_line_ , @existing_queries_ ) || FindItemFromVec ( $crontab_line_ , @stop_lines_to_remove_  ) );

      next if grep { index ($crontab_line_, $_) >= 0 } @lines_to_skip_;

      if ( $crontab_line_ ) {
        push ( @fixed_crontab_lines_, $crontab_line_ );
      }
    }

    my $local_production_strat_list_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$shortcode_.".".$timeperiod_.".".basename ( $config_file_ );
    my $remote_production_strat_list_file_ = $REMOTE_PRODUCTION_STRAT_LIST_DIR."/".$shortcode_.".".$timeperiod_.".".basename ( $config_file_ );

    open LOCAL_PRODUCTION_FILE, "> $local_production_strat_list_file_" or SendErrorMailifAutomatedAndDie ( "Could not open $local_production_strat_list_file_" );
    print LOCAL_PRODUCTION_FILE $_."\n\n" foreach @fixed_crontab_lines_;
    close LOCAL_PRODUCTION_FILE;

    $exec_cmd_ = "rsync -avz --delete $local_production_strat_list_file_ dvctrader\@$install_location_:$remote_production_strat_list_file_";
    `$exec_cmd_`;
    $exec_cmd_ = "ssh $install_location_ -l dvctrader 'crontab $remote_production_strat_list_file_'";
    print $exec_cmd_."\n";
    `$exec_cmd_`;
  }
}

sub FixLocalCrontab
{
  my $config_base_ = basename($config_file_);
  my $LOCAL_PRODUCTION_STRAT_LIST_DIR = $HOME_DIR."/production_strat_list";
  my $crontab_file_ = $LOCAL_PRODUCTION_STRAT_LIST_DIR."/".$config_base_."_crontab";
  `crontab -l > $crontab_file_`;

  `sed -i '/EBT REMOVAL\\\|ebt_remove_crontab_config.pl/d' $crontab_file_`;
  `crontab $crontab_file_`;
}


sub PrintErrorAndDie
{
  my $error_string_ = "@_";

  print STDERR "\nERROR: ".$error_string_."\n";

  die;
}

