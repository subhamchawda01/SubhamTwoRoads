# \file GenPerlLib/get_prune_strats_config.pl
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

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $MODELING_BASE_DIR=$HOME_DIR."/modelling";
my $MODELING_PRUNE_STRATS_CONFIG_DIR=$MODELING_BASE_DIR."/prune_strats_config/"; # this directory is used to store the configs for daily choosing for files to run
my $def_prune_models_config_filename_=$MODELING_PRUNE_STRATS_CONFIG_DIR."config.default.txt";

if ( $USER eq "sghosh" || $USER eq "ravi")
{
    $def_prune_models_config_filename_ = $MODELING_PRUNE_STRATS_CONFIG_DIR."config.sghosh.txt";
}

if ($USER eq "rahul"){
    $def_prune_models_config_filename_ = $MODELING_PRUNE_STRATS_CONFIG_DIR."config.rahul.txt";
}

if ( $USER eq "ankit") {
    $def_prune_models_config_filename_ =  $MODELING_PRUNE_STRATS_CONFIG_DIR."config.ankit.txt"
}

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_file_vec_excluding_comments.pl"; # GetFileVecExcludingComments

sub GetPruneStratsConfig
{
    my ( $shortcode_, $timeperiod_ ) = @_;

    my $num_days_past_ = 10;
    my $min_ppt_ = 10000;
    my $min_volume_ = 5000;
    my $max_ttc_ = 300;
    my $max_num_to_keep_ = -1;
    my $min_num_mid_mid_ = 0;
    my $min_num_mid_mkt_ = 0;
    my $min_num_mkt_mkt_ = 0;
    my @exclude_tp_dirs_ = ();

    my @t_lines_ = GetFileVecExcludingComments ( $def_prune_models_config_filename_ );

    my $found_strats_ = 0;

    for ( my $t_idx_ = 0; $t_idx_ <= $#t_lines_ ; $t_idx_ ++ )
    {
	my $thisline_ = $t_lines_[ $t_idx_ ];
	
	my @t_words_ = split ( ' ', $thisline_ );
	
	if ( $#t_words_ >= 5 )
	{

#SHORTCODE TIME_PERIOD NUM_DAYS_PAST MIN_PPT MIN_VOLUME MAX_TTC [ MAX_NUM_TO_KEEP ] [ MIN_NUM_MID_MID ] [ MIN_NUM_MID_MKT ] [ MIN_NUM_MKT_MKT ] [ EXCLUDE_TP_DIRS ] 
	    if ( ( $t_words_[0] eq $shortcode_ ) &&
		 ( $t_words_[1] eq $timeperiod_ ) )
	    {
		$num_days_past_ = $t_words_[2];
		$min_ppt_ = $t_words_[3];
		$min_volume_ = $t_words_[4];
		$max_ttc_ = $t_words_[5];
		if ( $#t_words_ >= 6 ) { $max_num_to_keep_ = $t_words_[6]; }
		if ( $#t_words_ >= 7 ) { $min_num_mid_mid_ = $t_words_[7]; }
		if ( $#t_words_ >= 8 ) { $min_num_mid_mkt_ = $t_words_[8]; }
		if ( $#t_words_ >= 9 ) { $min_num_mkt_mkt_ = $t_words_[9]; }
		if ( $#t_words_ >= 10 ) { @exclude_tp_dirs_ = splice ( @t_words_, 0, 10 ) ; }

		$found_strats_ = 1;

		last; # breaking ... since we found an uncommented line with the shortcode and timeperiod
	    }
	}
    }

    if ( ! $found_strats_ )
    { # Don't pass along invalid values for pruning.
	PrintStacktraceAndDie ( "No prune_config information found for $shortcode_ $timeperiod_" );
    }

    if ( $USER eq "sghosh" || $USER eq "ravi" || $USER eq "anshul" ) #|| $USER eq "rahul")
    {
	print "GetPruneStratsConfig $num_days_past_ $min_ppt_ $min_volume_ $max_ttc_ $max_num_to_keep_ $min_num_mid_mid_ $min_num_mid_mkt_ $min_num_mkt_mkt_\n";
    }
    $num_days_past_, $min_ppt_, $min_volume_, $max_ttc_, $max_num_to_keep_, $min_num_mid_mid_, $min_num_mid_mkt_, $min_num_mkt_mkt_, @exclude_tp_dirs_ ;
}
if ( $0 eq __FILE__ ) {
    my @r= GetPruneStratsConfig( $ARGV[0], $ARGV[1] ) ;
    print join( ' ', @r );
}
1
