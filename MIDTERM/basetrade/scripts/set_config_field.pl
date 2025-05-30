#!/usr/bin/perl

# \file scripts/set_config_field.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use FileHandle;

sub LoadConfigFile;
sub SanityCheckConfigParams;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
  $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

# start
my $USAGE="$0 CONFIG_FILE FIELD_NAME FIELD_VALUE1 FIELD_VALUE2 FIELD_VALUE3 ... ";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $config_file_ = $ARGV [ 0 ];
my $field_name_ = $ARGV [ 1 ];

my $random_id_ = `date +%N`; $random_id_ = $random_id_ + 0;
my $edited_config_file_ = $config_file_.$random_id_;

my @field_values_ = ( );

for ( my $i = 2 ; $i <= $#ARGV ; $i ++ )
{
  push ( @field_values_ , $ARGV [ $i ] );
}

open ( EDITED_CONFIG_FILE , ">" , $edited_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $edited_config_file_" );
open ( CONFIG_FILE , "<" , $config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $config_file_" );

my $current_param_ = "";
my $found_ = 0;
while ( my $line_ = <CONFIG_FILE> )
{
  chomp ( $line_ );
  my @t_words_ = split ( ' ' , $line_ );

  if ( $current_param_ )
  {
    if ( $#t_words_ < 0 )
    {
      $current_param_ = "";
    }
  }
  else
  {
    print EDITED_CONFIG_FILE $line_."\n";

    if ( $#t_words_ >= 0 && ($t_words_[0] eq $field_name_) ) # Found the field to edit
    {
      $current_param_ = 1; # Set it to 1 , so that the original contents are ignored.  
      $found_ = 1;

# Write out provided field values.
          foreach my $field_value_ ( @field_values_ )
          {
            print EDITED_CONFIG_FILE $field_value_."\n";
          }
      print EDITED_CONFIG_FILE "\n";
    }
  }
}

if ( ! $found_ )
{ 
  print EDITED_CONFIG_FILE "\n$field_name_\n";
  foreach my $field_value_ ( @field_values_ )       
  {
    print EDITED_CONFIG_FILE $field_value_."\n";        
  }
}

close ( CONFIG_FILE );
close ( EDITED_CONFIG_FILE );

`mv $edited_config_file_ $config_file_`;

exit ( 0 );
