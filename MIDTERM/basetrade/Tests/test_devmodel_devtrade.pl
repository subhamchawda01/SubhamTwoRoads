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

sub TestDevmodelDevtradeDiff 
{
	`cd $HOME_DIR"/"$REPO`;
	my @diff_output = `git diff -w origin/devtrade --name-only Indicators`;
	chomp ( @diff_output );
	if ( $#diff_output >= 0 ) 
	{
		return ( 0 , join( "\n" , "diff exists in devmodel and devtrade" , @diff_output ) );		
	}
	else
	{
		return ( 1, "" );
	}
}
