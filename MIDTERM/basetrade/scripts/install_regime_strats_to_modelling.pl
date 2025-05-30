#!/usr/bin/perl
#
# \file ModelScripts/find_best_model_for_strategy_var_pert.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#
#

use strict;
use warnings;
use feature "switch";
use FileHandle;
use POSIX;
use List::Util qw/max min/; # for max
use File::Basename;
use Term::ANSIColor;

my $HOME = $ENV{'HOME'};
my $REGIME_MODELLING_DIR = "$HOME/kishenp/RegimeModelling";
my $REGIME_MODELS_DIR = "$REGIME_MODELLING_DIR/models";
my $REGIME_STRATS_DIR = "$REGIME_MODELLING_DIR/strats";
my $REGIME_PARAMS_DIR = "$REGIME_MODELLING_DIR/params";
my $MODELLING_DIR = "$HOME/modelling";
my $MODELS_DIR = "$MODELLING_DIR/models";
my $STRATS_DIR = "$MODELLING_DIR/strats";
my $PARAMS_DIR = "$MODELLING_DIR/params";
my $GENPERLLIB_DIR = "$HOME/basetrade_install/GenPerlLib";
my $USAGE="$0 shc start_time-end_time(STRAT) start_time-end_time(MODEL) set_of_strats_ use_database=1";
require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

if ( $#ARGV < 3 ){ print "USAGE : ".$USAGE."\n"; exit(0);}
my $shortcode_ = $ARGV[ 0 ];
my $strat_start_end_time_ = $ARGV[ 1 ];
my $model_start_end_time_ = $ARGV[ 2 ];
my $set_ = $ARGV[ 3 ];
my $use_database_ = 1;
my %regime_strat_to_constituent_strats_ = ();
if ( $#ARGV >= 4 )
{
  $use_database_ = $ARGV[ 4 ];
}

if ( $use_database_ != 0 )
{
  my $database_file_ = "$REGIME_MODELLING_DIR/database/$shortcode_/$strat_start_end_time_/mapping.txt";
  if ( -e $database_file_ )
  {
    open (DB, "<", $database_file_) or PrintStacktraceAndDie ( "Could not open $database_file_ for reading" );
    my @db_lines_ = <DB>; chomp(@db_lines_); close(DB);
    for my $db_line_ ( @db_lines_ )
    {
      my @db_words_ = split (' ', $db_line_ );
      my $constituents_= "";
      for ( my $i=1; $i<=$#db_words_; $i++ )
      {
        $constituents_ = $constituents_." $db_words_[ $i ]";
      }
      $regime_strat_to_constituent_strats_{ $db_words_[ 0 ] } = $constituents_;
    }
  }
}

my $src_strat_dir_ = $REGIME_STRATS_DIR."/$shortcode_/$strat_start_end_time_";
my $dest_strat_dir_ = $STRATS_DIR."/$shortcode_/$strat_start_end_time_";
my $dest_model_dir_ = $MODELS_DIR."/$shortcode_/$model_start_end_time_";
my $dest_param_dir_ = $PARAMS_DIR."/$shortcode_";
my $dest_db_dir_ = "$MODELLING_DIR/RegimeDatabase/$shortcode_/$strat_start_end_time_";
my $dest_db_name_ = $dest_db_dir_."/mapping.txt";

open (SET, "<", $set_ ) or PrintStacktraceAndDie ( "Could not open $set_ for reading" );
my @set_lines_ = <SET>; chomp ( @set_lines_ );
close (SET);

my $regime_strat_number_ = 1;
my $dest_strat_name_prefix_ = $dest_strat_dir_."/regime_strat_$shortcode_"."_$strat_start_end_time_";
my $dest_model_name_prefix_ = $dest_model_dir_."/regime_model_$shortcode_"."_$strat_start_end_time_";

if ( $#set_lines_ >= 0 )
{
  if ( !( -e $dest_strat_dir_ ) )
  {
    `mkdir -p $dest_strat_dir_`;
  }
  if ( !( -e $dest_model_dir_ ) )
  {
    `mkdir -p $dest_model_dir_`;
  }
  if ( !( -e $dest_param_dir_ ) )
  {
    `mkdir -p $dest_param_dir_`;
  }
  if ( !( -e $dest_db_dir_ ) )
  {
    `mkdir -p $dest_db_dir_`;
  }
}

open (DBOUT, ">>", $dest_db_name_ ) or PrintStacktraceAndDie ( "Could not open $dest_db_name_ for writing" );
for my $strat_ (@set_lines_)
{
  my $src_strat_name_ = $strat_;
  my $dest_strat_name_ = $dest_strat_name_prefix_."_$regime_strat_number_";
  while ( -e $dest_strat_name_ )
  {
    $regime_strat_number_++;
    $dest_strat_name_ = $dest_strat_name_prefix_."_$regime_strat_number_";
  }
  if ( !( -e $src_strat_name_ ) )
  {
    $src_strat_name_ = $src_strat_dir_."/$strat_";
    if ( !( -e $src_strat_name_ ) )
    {
      print "Neither $strat_ nor $src_strat_name_ found\n";
      next;
    }
  }
  my $src_strat_base_ = basename( $src_strat_name_ );
  my $dest_strat_base_ = basename( $dest_strat_name_ );
  open ( INSTRAT , "<" , $src_strat_name_ ) or PrintStacktraceAndDie ( "Could not open $src_strat_name_ for reading" );
  my @list_of_strat_lines_ = <INSTRAT>; chomp(@list_of_strat_lines_ );
  close ( INSTRAT );
  if ( $#list_of_strat_lines_ < 0 ) 
  {
    next;
  }
  my $strat_line_ = $list_of_strat_lines_[ 0 ];
  my @strat_words_ = split (' ', $strat_line_ );
  my $src_model_name_ = $strat_words_[ 3 ];
  my $src_param_name_ = $strat_words_[ 4 ];
  my $dest_model_name_ = $dest_model_name_prefix_."_$regime_strat_number_";
  if ( -e $dest_model_name_ )
  {
    print "$dest_model_name_ should not exist\n";
    next;
  }
  else
  {
    `cp $src_model_name_ $dest_model_name_`;
  }
  $strat_words_[ 3 ] = $dest_model_name_;
  my @regime_params_list_ = `cat $src_param_name_ | grep PARAMFILELIST | awk '{print \$2}'`; chomp(@regime_params_list_);
  my $regime_indicator_ = `cat $src_param_name_ | grep INDICATOR`; chomp($regime_indicator_);
  my @regime_indicator_words_ = split (' ', $regime_indicator_ );
  my $dest_param_name_ = $dest_param_dir_."/regm";
  my $multiple_regime_ = 0;
  for ( my $i=2; $i<= $#regime_indicator_words_; $i++ )
  {
    $dest_param_name_= $dest_param_name_."_$regime_indicator_words_[$i]";
  }
  for ( my $i=0; $i<=$#regime_params_list_; $i++ )
  {
    $dest_param_name_= $dest_param_name_."_".basename($regime_params_list_[$i]);
  }
  my $first_param_ = $regime_params_list_[0];
  for ( my $i=1; $i<=$#regime_params_list_; $i++ )
  {
    if ( $first_param_ ne $regime_params_list_[ $i ] )
    {
      $multiple_regime_ = 1;
    }
  }
  if ( $multiple_regime_ == 1 )
  {
    if ( !( -e $dest_param_name_ ) )
    {
      `cp $src_param_name_ $dest_param_name_`;
    }
    $strat_words_[ 4 ] = $dest_param_name_;
  }
  else
  {
    $strat_words_[ 4 ] = $first_param_; 
  }
  $strat_line_ = join(' ', @strat_words_ );
  open ( OUTSTRAT , ">" , $dest_strat_name_ ) or PrintStacktraceAndDie ( "Could not open $dest_strat_name_ for reading" );
  print OUTSTRAT "$strat_line_\n";
  close ( OUTSTRAT );
  if ( exists $regime_strat_to_constituent_strats_{$src_strat_base_} )
  {
    print DBOUT "$dest_strat_base_ $regime_strat_to_constituent_strats_{$src_strat_base_}\n";
  }
  else
  {
    print "mapping does not exist for $src_strat_base_\n";
  }
}
close (DBOUT);




