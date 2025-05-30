use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };
my $REPO = "basetrade";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."/ModelScripts";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."/scripts";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $TESTS_DIR = $HOME_DIR."/".$REPO."/Tests" ;

my $SIM_STRATEGY = $LIVE_BIN_DIR."/sim_strategy";

# sim_strategy on ~/modelling/strats/FOAT_0/CET_900-EST_800/w_pbat_eu_ilist_foat_fbmfs_Ord_Ord_30_na_t1_20130620_20140805_CET_900_EST_800_1500_20_5_0_fsr.5_3_SIGLR_20_N_L1.tt_CET_900_EST_800.pfi_1 20140811
# SIMRESULT 2821 374 20 79 0 0
sub TestPNL {
	my @sim_strategy_output = `$SIM_STRATEGY SIM ~/modelling/strats/FOAT_0/CET_900-EST_800/w_pbat_eu_ilist_foat_fbmfs_Ord_Ord_30_na_t1_20130620_20140805_CET_900_EST_800_1500_20_5_0_fsr.5_3_SIGLR_20_N_L1.tt_CET_900_EST_800.pfi_1 12345678 20140811`;
	chomp ( @sim_strategy_output );
	if ( $#sim_strategy_output != 0 ) {
		return ( 0 , "unexpected sim_strategy_result" );
	}
	else {
		if ( $sim_strategy_output[0] eq "SIMRESULT 2079 302 18 81 0 0" ) {
			return ( 1 , "" );
		}
		else {
			return ( 0 , "expected SIMRESULT 2079 302 18 81 0 0, received ".$sim_strategy_output[0] );
		}
	}
}

# sim_strategy should not break on bad input files
sub TestNoSIGSEGV1 {
 	my @sim_strategy_output = `$SIM_STRATEGY SIM ~/modelling/stratwork/FOAT_0/iFOAT_test_EU_PBAT_reg1 12345678 20140811 1> /dev/null 2> /dev/null`;
	if ( $? < 0 ) {
		return ( 0 , "sim_strategy failed on bad ilist" );
	}
	else {
		return ( 1, "" );
	}
}

sub TestBadFileAccess {	
	my $out = `$TESTS_DIR/file_access_test.sh "$SIM_STRATEGY SIM /home/dvctrader/modelling/strats/FOAT_0/CET_900-EST_800/w_pbat_eu_ilist_foat_fbmfs_OMix_OMix_200_na_t1_20130612_20140728_CET_900_EST_800_1500_20_5_0_fsr.5_3_FSLR_0.012_0_0_0.8_12_H.tt_CET_900_EST_800.pfi_0 12345678 20140811" ` ;
	print "\n$out";
}
