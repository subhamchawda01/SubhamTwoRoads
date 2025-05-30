#!/usr/bin/perl

# \file ModelScripts/normalize_correlation_by_volume.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use List::Util qw/max min/; # for max
use FileHandle;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $SPARE_HOME="/spare/local/".$USER."/";

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/get_value_from_file.pl"; # GetValueFromFile

sub GetDepVolumeOnDate ;
sub GetDepVolumeNormalizingFactor;

if ( $USER eq "rkumar" ) 
{ 
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "mayank" || $USER eq "rahul" || $USER eq "ankit" )
{
    $LIVE_BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
}

my $USAGE="$0 shortcode correlation_record_file";

if ( $#ARGV < 1 ) { print $USAGE."\n"; exit ( 0 ); }

my $dep_shortcode_ = $ARGV[0];
my $input_correlation_filename_ = $ARGV[1];

my $VOLUME_EXEC_=$LIVE_BIN_DIR."/get_avg_volume_for_shortcode";
my $NUMBER_OF_DAYS_=30 ;


# cache results
my %date_to_volume_map_ ;
my %date_to_avg_volume_map_ ;
my %date_to_volume_normalizing_factor_map_ ;



#####################################
# processing
open INPUT_CORRELATION_FILEHANDLE, "< $input_correlation_filename_ " or PrintStacktraceAndDie ( "$0 Could not open $input_correlation_filename_\n" );

while ( my $thisline_ = <INPUT_CORRELATION_FILEHANDLE> ) 
{
    chomp ( $thisline_ ); # remove newline character at end
    
    my @this_words_ = split ( ' ', $thisline_ );
    if ( $#this_words_ >= 2 ) 
    { # empty line ... ignore
	my $this_date_ = $this_words_[0];
	my $this_correlation_ = $this_words_[2];

	my $volume_normalizing_factor_ = GetDepVolumeNormalizingFactor ( $dep_shortcode_, $this_date_ );
	my $this_volume_normalized_correlation_ = $this_correlation_ * $volume_normalizing_factor_ ;
	$this_words_[2] = $this_volume_normalized_correlation_ ; 
    }
    print join (' ', @this_words_),"\n";
}
close ( INPUT_CORRELATION_FILEHANDLE );

exit ( 0 );






####################################
# subs #
sub GetDepVolumeNormalizingFactor
{
    my $shortcode_ = shift;
    my $this_date_ = shift;

    my $volume_normalizing_factor_ = 1.00;
    if ( ! ( exists $date_to_volume_normalizing_factor_map_ { $this_date_ } ) )
    {
	# file cache
	if ( ! ( -d $SPARE_HOME."NCBVStore" )) {
	    mkdir $SPARE_HOME."NCBVStore" ;
	}
	my $STORED_VOLUME_NORMALIZING_FACTOR_FILENAME = $SPARE_HOME."NCBVStore/ncbvdata.txt";
	my $search_string_ = $shortcode_."^".$this_date_."NCBV";
	$volume_normalizing_factor_ = GetValueFromFile ( $STORED_VOLUME_NORMALIZING_FACTOR_FILENAME, $search_string_ ) ;

	if ( $volume_normalizing_factor_ < 0 ) # not in file cache 
	{
	    my ( $dep_volume_on_this_date_, $avg_dep_volume_for_this_date_ ) = GetDepVolumeOnDate ( $shortcode_, $this_date_ ) ; 
	    if ( $avg_dep_volume_for_this_date_ > 0 ) 
	    {
		$volume_normalizing_factor_ = $dep_volume_on_this_date_ / $avg_dep_volume_for_this_date_ ;

		# store in file cache
		open ( STORE_FILEHANDLE, ">> $STORED_VOLUME_NORMALIZING_FACTOR_FILENAME" );
		printf STORE_FILEHANDLE "%s %s\n", $search_string_, $volume_normalizing_factor_;
		close ( STORE_FILEHANDLE );
	    }
	}

	# cache
#	print STDERR "CACHE: $this_date_ $volume_normalizing_factor_\n";
	$date_to_volume_normalizing_factor_map_ { $this_date_ } = $volume_normalizing_factor_;
    }
    else
    {
	$volume_normalizing_factor_ = $date_to_volume_normalizing_factor_map_ { $this_date_ };
    }
    # return
    $volume_normalizing_factor_ ;
}

sub GetDepVolumeOnDate
{
    my $shortcode_ = shift;
    my $this_date_ = shift;

    my $dep_volume_on_this_date_ = 1;
    my $avg_dep_volume_for_this_date_ = 1;

    if ( ! ( ( exists $date_to_volume_map_ { $this_date_ } ) &&
	     ( exists $date_to_avg_volume_map_ { $this_date_ } ) ) )
    {
	my $exec_cmd_ = "$LIVE_BIN_DIR/get_avg_volume_for_shortcode $shortcode_ $this_date_ $NUMBER_OF_DAYS_";
	if ( ! ( -e "$LIVE_BIN_DIR/get_avg_volume_for_shortcode" ) )
	{
	    if ( -e "$BIN_DIR/get_avg_volume_for_shortcode" )
	    {
		$exec_cmd_ = "$BIN_DIR/get_avg_volume_for_shortcode $shortcode_ $this_date_ $NUMBER_OF_DAYS_";
	    }
	    else
	    {
		$exec_cmd_ = ""; # flag for non callability of command"
	    }
	}
	if ( ! $exec_cmd_ )
	{ # in case we can't call command then just return 1,1
	    $dep_volume_on_this_date_ = 1;
	    $avg_dep_volume_for_this_date_ = 1;
	    $date_to_volume_map_ { $this_date_ } = $dep_volume_on_this_date_;
	    $date_to_avg_volume_map_ { $this_date_ } = $avg_dep_volume_for_this_date_;
	}
	else
	{
	    my $exec_line_ = `$exec_cmd_`;
	    if ( ! $exec_line_ )
	    {
		print STDERR "FAILED: $exec_line_\n";
	    }
	    chomp ( $exec_line_ );
	    
	    my @exec_words_ = split ( ' ', $exec_line_ );
	    if ( $#exec_words_ >= 1 )
	    {
		$dep_volume_on_this_date_ = max ( 1, $exec_words_[0] ); 
		$avg_dep_volume_for_this_date_ = $exec_words_[1];

		if ( ( $avg_dep_volume_for_this_date_ == 1 ) ||
		     ( $avg_dep_volume_for_this_date_ < ( $dep_volume_on_this_date_ / 5 ) ) )
		{
		    $avg_dep_volume_for_this_date_ = $dep_volume_on_this_date_ ;
		}
		
		# cache
#	    print STDERR "CACHE: $this_date_ $dep_volume_on_this_date_ $avg_dep_volume_for_this_date_\n";
		$date_to_volume_map_ { $this_date_ } = $dep_volume_on_this_date_;
		$date_to_avg_volume_map_ { $this_date_ } = $avg_dep_volume_for_this_date_;
	    }
	    else
	    { # error handling
		$dep_volume_on_this_date_ = 1;
		$avg_dep_volume_for_this_date_ = 1;
		$date_to_volume_map_ { $this_date_ } = $dep_volume_on_this_date_;
		$date_to_avg_volume_map_ { $this_date_ } = $avg_dep_volume_for_this_date_;
	    }
	}
    }
    else
    {
	$dep_volume_on_this_date_ = $date_to_volume_map_ { $this_date_ } ;
	$avg_dep_volume_for_this_date_ = $date_to_avg_volume_map_ { $this_date_ } ;
    }
    # return
    $dep_volume_on_this_date_, $avg_dep_volume_for_this_date_;
}

