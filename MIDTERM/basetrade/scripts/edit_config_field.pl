#!/usr/bin/perl

# \file scripts/set_config_field.pl
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
use FileHandle;

sub LoadConfigFile;
sub SanityCheckConfigParams;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO = "basetrade";

my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";


require "$GENPERLLIB_DIR/find_item_from_vec.pl"; #FindItemFromVec

my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

# start
my $USAGE="$0 CONFIG_FILE [HARD_REPLACE/REPLACE/APPEND] FIELD_NAME FIELD_VALUE1 FIELD_VALUE2 FIELD_VALUE3 ... ";

if ( $#ARGV < 2 ) { print $USAGE."\n"; exit ( 0 ); }

my $config_file_ = $ARGV [ 0 ];
my $edit_type_ = $ARGV [ 1 ];
my $field_name_ = $ARGV [ 2 ];

my $random_id_ = `date +%N`; $random_id_ = $random_id_ + 0;
my $edited_config_file_ = $config_file_.$random_id_;
my $state_ = 0 ;

my @field_values_ = ( );

for ( my $i = 3 ; $i <= $#ARGV ; $i ++ )
{
    if ( ! FindItemFromVec ( $ARGV [ $i ] , @field_values_ ) )
    {
	push ( @field_values_ , $ARGV [ $i ] );
    }
}

open ( CONFIG_FILE , "<" , $config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $config_file_" );
my @lines_ = <CONFIG_FILE> ;
close ( CONFIG_FILE );

open ( EDITED_CONFIG_FILE , ">" , $edited_config_file_ ) or PrintStacktraceAndDie ( "Could not open config file $edited_config_file_" );

for ( my $i = 0 ; $i < scalar ( @lines_ ) ; $i++ )
{
    if ( $state_ == 0 ) # state 0
    {
	print EDITED_CONFIG_FILE $lines_[ $i ] ;
    }
    elsif ( $lines_[ $i ] =~/^\s*$/ ) # first empty line after our instruction is read # return to state 0
    {
	print EDITED_CONFIG_FILE $lines_[ $i ] ;
	$state_ = 0 ;
    }
    elsif ( $edit_type_ eq "HARD_REPLACE" )
    {
	# do nothing
    }
    elsif ( $edit_type_ eq "REPLACE" )
    {
	print EDITED_CONFIG_FILE "#".$lines_[ $i ] ;  # comment the older fields
    }
    elsif ( $edit_type_ eq "APPEND" )
    {
	chomp ( $lines_[ $i ] ) ;
	if ( ( ! FindItemFromVec ( $lines_[ $i ] , @field_values_ ) ) )
	{
	    print EDITED_CONFIG_FILE $lines_[ $i ]."\n" ;
	}
    }
    elsif ( $edit_type_ eq "REMOVE" )
    {
	chomp ( $lines_[ $i ] ) ;
	if ( ( ! FindItemFromVec ( $lines_[ $i ] , @field_values_ ) ) )
	{
	    print EDITED_CONFIG_FILE $lines_[ $i ]."\n" ;
	}
    }
    
    if ( index ( $lines_[ $i ]  , $field_name_ ) == 0 ) # found the field to edit # enter into state 1
    {
	$state_ = 1 ;
	# Write out provided field values.
	if ( $edit_type_ ne "REMOVE" )
	{
	    foreach my $field_value_ ( @field_values_ )
	    {	    
		print EDITED_CONFIG_FILE $field_value_."\n";
	    }
	}
    }
}

close ( EDITED_CONFIG_FILE );
`mv $edited_config_file_ $config_file_`;
exit ( 0 );
