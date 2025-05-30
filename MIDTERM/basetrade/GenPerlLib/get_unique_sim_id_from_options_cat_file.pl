# \file GenPerlLib/get_unique_sim_id_from_options_cat_file.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

use feature "switch"; # for given, when

my $HOME_DIR=$ENV{'HOME'}; 

sub GetUniqueSimIdFromOptionsCatFile
{

    my ( $temp_strategy_cat_file_, $psindex_ ) = @_;
    my $unique_sim_id_ = 0;

    open ( DAT, $temp_strategy_cat_file_ ) || die(" get_unique_sim_id_from_stir_cat_file.pl could not open temp_strategy_cat_file_ $temp_strategy_cat_file_!\n");

    my $this_psindex_ = 0; # index of lines with data
    while ( my $datline_ = <DAT> )
    {
	my @dwords_ = split ( ' ', $datline_ );
	if ( $#dwords_ > 1 )
	{ # only looking at lines with at least 3 words
	    if ( $this_psindex_ == $psindex_ )
	    {
		$unique_sim_id_ = $dwords_[4];
	    }
	    $this_psindex_ ++;
	}
    }
    close ( DAT );
    $unique_sim_id_;
}

1;
