# \file GenPerlLib/get_basepx_strat_first_model.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

#use feature "switch"; # for given, when

my $HOME_DIR=$ENV{'HOME'}; 

sub GetBasepxStratFirstModel
{
    my $strategy_file_ = shift;
    my $basepx_pxtype_first_model_ = "UNDEF"; # so that strats with no model don't match

    if ( -e $strategy_file_ )
    {
	my $MFILE_ = "";
	open ( SFILE_HANDLE, "< $strategy_file_" ) or die " Could not open file $strategy_file_ for reading \n" ;
	while ( my $sline_ = <SFILE_HANDLE> )
	{ 
	    my @words_ = split ( ' ', $sline_ );
	    if ( $#words_ >= 3 )
	    { # STRATEGYLINE ZT_0 PriceBasedTrading /home/dvctrader/modelling/models/ZT_0/EST_830-EST_1130/w_model_ilist_ZT_0_US_Mkt_Mkt_20_na_e3_20110628_20110804_EST_830_EST_1130_1000_13_2_fsg1_FSHLR_0.01_0_0_0.7 /home/dvctrader/modelling/params/ZT_0/param_ZT_0_US_20_1207 EST_830 EST_1545 11011

		$MFILE_=$words_[3];
		last; # first model
	    }
	}
	close ( SFILE_HANDLE );

	if ( $MFILE_ )
	{
	    if ( -e $MFILE_ )
	    {
		open ( MFILE_HANDLE, "< $MFILE_" ) or die " Could not open file $MFILE_ for reading \n" ;
		while ( my $mline_ = <MFILE_HANDLE> )
		{
		    my @words_ = split ( ' ', $mline_ );
		    if ( ( $#words_ >= 3 ) &&
			 ( $words_[0] eq "MODELINIT" ) )
		    { # MODELINIT DEPBASE ZT_0 MktSizeWPrice MktSizeWPrice
			$basepx_pxtype_first_model_=$words_[3];
			last; # first line
		    }
		}
		close ( MFILE_HANDLE );
	    }
	}
    }
    
    $basepx_pxtype_first_model_;
}
1
