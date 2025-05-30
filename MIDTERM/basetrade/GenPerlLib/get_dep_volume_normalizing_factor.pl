# \file GenPerlLib/get_dep_volume_normalizing_factor.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#
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

# cache results
my %date_to_volume_map_ ;
my %date_to_avg_volume_map_ ;
my %date_to_volume_normalizing_factor_map_ ;

my $NUMBER_OF_DAYS_=30 ;

sub GetNonCachedDepVolumeNormalizingFactor
{
    my $shortcode_ = shift;
    my $this_date_ = shift;

    my $volume_normalizing_factor_ = 1.00;
    if ( ! ( exists $date_to_volume_normalizing_factor_map_ { $this_date_ } ) )
    {
	my ( $dep_volume_on_this_date_, $avg_dep_volume_for_this_date_ ) = GetNonCachedDepVolumeOnDate ( $shortcode_, $this_date_ ) ; 
	if ( $avg_dep_volume_for_this_date_ > 0 ) 
	{
	    $volume_normalizing_factor_ = $dep_volume_on_this_date_ / $avg_dep_volume_for_this_date_ ;
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

sub GetNonCachedDepVolumeOnDate
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

1
