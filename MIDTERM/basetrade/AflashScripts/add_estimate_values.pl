#!/usr/bin/perl

# \file AflashScripts/add_estimate_values.pl
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

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $SPARE_HOME = "/spare/local/".$USER."/";

my $REPO = "basetrade";

my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $AFLASHSCRIPTS_DIR = $HOME_DIR."/".$REPO."/AflashScripts";
my $AFLASH_TRADEINFO_DIR = "/spare/local/tradeinfo/Alphaflash";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $TEMP_DIR = "/spare/local/temp";
my $ESTIMATE_DIR = $AFLASH_TRADEINFO_DIR."/Estimates";
my $event_map_fxs_ = $AFLASH_TRADEINFO_DIR."/event_fxstreet_map";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec
require "$GENPERLLIB_DIR/valid_date.pl"; # ValidDate
require "$GENPERLLIB_DIR/skip_weird_date.pl"; # SkipWeirdDate
require "$GENPERLLIB_DIR/is_date_holiday.pl"; # IsDateHoliday
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # CalcPrevWorkingDateMult
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr
require "$GENPERLLIB_DIR/get_unique_list.pl"; #GetUniqueList

my %event_name_to_evid_ = ( );
my %event_name_to_datumid_ = ( );
my %event_name_to_scale_ = ( );
my %evid_to_event_names_ = ( );
my $event_id_ = 0;

my $date_ = `date +%Y%m%d`; chomp ( $date_ );
LoadEventMapFXS ( );

GetEventId ( );

my $estimate_string_ = "";
GetEstimates ( );

WriteAndEstimateFile ( );

sub LoadEventMapFXS
{
  open FXSMAPHANDLE, "< $event_map_fxs_" or PrintStacktraceAndDie ( "Could not open $event_map_fxs_ for reading" );
  my @fxsmap_lines_ = <FXSMAPHANDLE>; chomp ( @fxsmap_lines_ );
  close FXSMAPHANDLE;

  foreach my $tline_ ( @fxsmap_lines_ ) {
    my @twords_ = split(',', $tline_);

    if ( $#twords_ > 0 ) {
      my $evid_ = $twords_[0];
      foreach my $tword_ ( @twords_[1..$#twords_] ) {
        my @tt_words_ = split(':', $tword_);
        if ( $#tt_words_ >= 1 ) {
          my $evname_ = $tt_words_[1];
          $event_name_to_evid_{ $evname_ } = $evid_;
          push ( @{ $evid_to_event_names_{ $evid_ } }, $evname_ );

          $event_name_to_datumid_{ $evname_ } = $tt_words_[0];
          if ( $#tt_words_ >= 2 ) {
            $event_name_to_scale_{ $evname_ } = $tt_words_[2];
          } else {
            $event_name_to_scale_{ $evname_ } = 1;
          }
        }
      }
    }
  }
}

sub GetEventId
{
  print "Enter Name/(Part of Name) of the Event: ";
  my $evname_input_ = <STDIN>; chomp ( $evname_input_ );

  my @matching_evnames_ = grep { $_ =~ /$evname_input_/ } keys %event_name_to_evid_;

  my @matching_categories_ = GetUniqueList ( @event_name_to_evid_{ @matching_evnames_ } );

  print "\nThe Matching Categories are: \n";
  foreach my $catid_ ( @matching_categories_ ) {
    print $catid_.": ".join(",", @{ $evid_to_event_names_{ $catid_ } })."\n";
  }
  print "\nPlease Enter the Category Id: ";
  while (1) {
    my $evid_ = <STDIN>; chomp ($evid_);
    if ( FindItemFromVec( $evid_, @matching_categories_ ) ) {
      $event_id_ = $evid_;
      last;
    } else {
      print "\nEvent Id NOT from the suggestions.. Please retry\n";
    }
  }
}

sub GetEstimates
{
## Fetching last few entries for this event
  my $fetch_estm_cmd_ = "grep \^$event_id_ $ESTIMATE_DIR/estimates_201\[567\]\* | sort -t\: -k1 | tail -n5";
  my @prev_estms_ = `$fetch_estm_cmd_ 2>/dev/null`;
  chomp ( @prev_estms_ );
  my $estimate_msecs_;

  if ( $#prev_estms_ < 0 ) {
    print "\nThere are no previous estimate files for event-id: $event_id_\n";
  } else {
    print "\nThe previous estimate files for event-id: $event_id_:\n".join("\n", @prev_estms_)."\n";
    my @t_last_estm_ = split(" ", $prev_estms_[$#prev_estms_]);
    my $last_estimate_msecs_ = $t_last_estm_[1];
    print "\nThe last estimate msecs(from midnight): $last_estimate_msecs_. Do you want to set the same for the current run (y/n)? "; 
    my $usr_inp_ = <STDIN>; chomp($usr_inp_);
    if ( $usr_inp_ eq "y" || $usr_inp_ eq "Y" ) {
      $estimate_msecs_ = $last_estimate_msecs_;
    } else {
      print "Not considering the last estimate msecs..\n";
    }
  }

  if ( ! defined $estimate_msecs_ ) {
    print "\nEnter The event-time: ";
    my $evtime_ = <STDIN>; chomp($evtime_);
    my $utc_hhmm_ = `$HOME_DIR/basetrade_install/bin/get_utc_hhmm_str $evtime_`; chomp( $utc_hhmm_ );
    $estimate_msecs_ = ( int($utc_hhmm_/100)*60 + ($utc_hhmm_%100))*60000;
  }
  print "Using msecs from midnight: ".$estimate_msecs_."\n\n";

  print "Possible Fields for the event: $event_id_\n";
  print join("\n", map { $event_name_to_datumid_{$_}.":".$_.", scale: ".$event_name_to_scale_{ $_ } } @{ $evid_to_event_names_{ $event_id_ } } )."\n";

  my %fid_to_evname_ = map { $event_name_to_datumid_{$_} => $_ } @{ $evid_to_event_names_{ $event_id_ } };

  my %fid_to_estimate_values_ = ( );

  while(1) {
    print "\nEnter the field id (-1 to finish): ";
    my $t_fid_ = <STDIN>; chomp( $t_fid_ );
    if ( $t_fid_ eq "-1" ) { last; }

    if ( FindItemFromVec( $t_fid_, keys %fid_to_evname_ ) ) {
      my $t_evname_ = $fid_to_evname_{ $t_fid_ };
      print "Enter the estimate for $t_evname_ : ";
      my $estm_value_ = <STDIN>; chomp ( $estm_value_ );
      $fid_to_estimate_values_{ $t_fid_ } = $estm_value_;
    }
    else {
      print "No such field id: ".$t_fid_."\n";
    }
  }

  $estimate_string_ = "$event_id_ $estimate_msecs_ ".join(" ", map { $_.":".$fid_to_estimate_values_{ $_ } } sort keys %fid_to_estimate_values_);
}

sub WriteAndEstimateFile
{
  print "\nThe estimate line to be added for today:\n".$estimate_string_."\n";
  print "\nDo you want to continue (y/n)? ";

  my $usr_inp_ = <STDIN>; chomp($usr_inp_);
  if ( $usr_inp_ eq "y" || $usr_inp_ eq "Y" ) {

    my $estimate_file_ = $ESTIMATE_DIR."/estimates_".$date_;
    `echo $estimate_string_ >> $estimate_file_`;

    print "The complete contents of the file: $estimate_file_\n";
    print `cat $estimate_file_`;
    print "\nSyncing it to all servers:\n";

    my $sync_cmd_ = "$HOME_DIR/basetrade/scripts/sync_file_to_all_machines.pl $estimate_file_";
    print $sync_cmd_."\n";
    `$sync_cmd_`;
  }
  else {
    print "Terminating..\n";
  }
}

