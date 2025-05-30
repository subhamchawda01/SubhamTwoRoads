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
my $USAGE="$0 CONFIG_FILE FIELD_NAME";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $config_file_ = $ARGV [ 0 ];
my $field_name_ = $ARGV [ 1 ];

my $random_id_ = `date +%N`; $random_id_ = $random_id_ + 0;

open ( CONFIG_FILE , "<" , $config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $config_file_" );

my $current_param_ = 0;

while ( my $line_ = <CONFIG_FILE> )
{
    chomp ( $line_ );

    if ( $current_param_ == 1 )
    {
	    my @t_words_ = split ( ' ' , $line_ );
	    if ( $#t_words_ < 0 )
	    {
	        $current_param_ = 0;
	    }
        else
        {
            print $line_."\n";
        }
    }
    else
    {
	    if ( index ( $line_ , $field_name_ ) == 0 ) # Found the field to edit
	    {
	        $current_param_ = 1; # Set it to 1 , so that the original contents are ignored.
	    }
    }
}

close ( CONFIG_FILE );

exit ( 0 );
