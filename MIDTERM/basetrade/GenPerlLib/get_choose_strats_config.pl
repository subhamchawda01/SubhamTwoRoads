# \file GenPerlLib/get_choose_strats_config.pl
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
my $MODELING_CHOOSE_STRATS_CONFIG_DIR=$MODELING_BASE_DIR."/choose_strats_config/"; # this directory is used to store the configs for daily choosing for files to run
my $def_choose_models_config_filename_=$MODELING_CHOOSE_STRATS_CONFIG_DIR."config.default.txt";

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/get_file_vec_excluding_comments.pl"; # GetFileVecExcludingComments

sub GetChooseStratsConfig
{
    my ( $shortcode_, $timeperiod_ ) = @_;
    
    my $stratid_start_ = -1; # error value
    my $stratid_end_ = -1;
    my $num_days_past_ = 10;
    my $min_ppt_ = 10000;
    my $min_volume_ = 5000;
    my $max_ttc_ = 300;
    my $max_num_to_choose_ = 1;
    my $min_num_mid_mid_ = 0;
    my $min_num_mid_mkt_ = 0;
    my $min_num_mkt_mkt_ = 0;
    my @exclude_tp_dirs_ = ();

    my @t_lines_ = GetFileVecExcludingComments ( $def_choose_models_config_filename_ );

    for ( my $t_idx_ = 0; $t_idx_ <= $#t_lines_ ; $t_idx_ ++ )
    {
	my $thisline_ = $t_lines_[ $t_idx_ ];
	
	my @t_words_ = split ( ' ', $thisline_ );
	
	if ( $#t_words_ >= 7 )
	{
#SHORTCODE TIME_PERIOD STRATID_START STRATID_END NUM_DAYS_PAST MIN_PPT MIN_VOLUME MAX_TTC [ MAX_NUM_TO_CHOOSE ] [ MIN_NUM_MID_MID ] [ MIN_NUM_MID_MKT ] [ MIN_NUM_MKT_MKT ] [ EXCLUDE_TP_DIRS ] 
	    if ( ( $t_words_[0] eq $shortcode_ ) &&
		 ( $t_words_[1] eq $timeperiod_ ) )
	    {
		$stratid_start_ = $t_words_[2];
		$stratid_end_ = $t_words_[3];
		$num_days_past_ = $t_words_[4];
		$min_ppt_ = $t_words_[5];
		$min_volume_ = $t_words_[6];
		$max_ttc_ = $t_words_[7];
		if ( $#t_words_ >= 8 ) { $max_num_to_choose_ = $t_words_[8]; }
		if ( $#t_words_ >= 9 ) { $min_num_mid_mid_ = $t_words_[9]; }
		if ( $#t_words_ >= 10 ) { $min_num_mid_mkt_ = $t_words_[10]; }
		if ( $#t_words_ >= 11 ) { $min_num_mkt_mkt_ = $t_words_[11]; }
		if ( $#t_words_ >= 12 ) { @exclude_tp_dirs_ = splice ( @t_words_, 0, 12 ) ; }
		last; # breaking ... since we found an uncommented line with the shortcode and timepreiod
	    }
	}
    }

    $stratid_start_, $stratid_end_, $num_days_past_, $min_ppt_, $min_volume_, $max_ttc_, $max_num_to_choose_, $min_num_mid_mid_, $min_num_mid_mkt_, $min_num_mkt_mkt_, @exclude_tp_dirs_ ;
}
1
