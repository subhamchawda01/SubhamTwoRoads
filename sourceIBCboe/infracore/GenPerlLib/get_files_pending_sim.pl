# \file GenPerlLib/get_files_pending_sim.pl
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
use File::Basename;

sub GetFilesPendingSim
{
    my ( $this_day_result_file_, @strategy_filevec_ ) = @_;
    my %uncaliberated_strategies_ ; # mapped to 0 be default

    for ( my $i = 0 ; $i <= $#strategy_filevec_; $i ++ )
    {
	my $strategy_filename_base_ = `basename $strategy_filevec_[$i]`; chomp ( $strategy_filename_base_ );
	$uncaliberated_strategies_{$strategy_filename_base_} = 0; # for all files mark 0
    }

    if ( -e $this_day_result_file_ )
    {
	open RESDBFILE, "< $this_day_result_file_";
	while ( my $resdbline_ = <RESDBFILE> )
	{
	    my @rwords_ = split ' ', $resdbline_;
	    if ( exists $uncaliberated_strategies_{$rwords_[0]} )
	    { # if this strategy_filename_base was there in @strategy_filevec_
		delete $uncaliberated_strategies_{$rwords_[0]};
	    }
	}
	close RESDBFILE;
    }

    my @this_day_strategy_filevec_ = ();
    # note just calling 'keys' on %uncaliberated_strategies_ won't work since that will retun just basenames and not full paths as was input in the argument
    for ( my $i = 0 ; $i <= $#strategy_filevec_; $i ++ )
    {
	my $strategy_filename_base_ = `basename $strategy_filevec_[$i]`; chomp ( $strategy_filename_base_ );
	if ( exists $uncaliberated_strategies_{$strategy_filename_base_} )
	{ # still uncaliberated means need to run sim on this 
	    push ( @this_day_strategy_filevec_, $strategy_filevec_[$i] ) ;
	}
    }


    @this_day_strategy_filevec_ ;
}

1;
