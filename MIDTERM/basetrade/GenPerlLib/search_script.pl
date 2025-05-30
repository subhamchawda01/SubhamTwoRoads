# \file GenPerlLib/search_exec.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

# This perl module is to be used in scripts like
# my @ADDITIONAL_SCRIPT_PATHS=();
# my $SIM_STRATEGY_EXEC = SearchExec ( "create_strategy_file.pl", @ADDITIONAL_SCRIPT_PATHS ) ;
# 
# Shown an example in ModelScripts/run_sim_and_summarize.pl

use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $SCRIPTS_INSTALL_DIR=$HOME_DIR."/LiveExec/scripts";
my $MODELSCRIPTS_INSTALL_DIR=$HOME_DIR."/LiveExec/ModelScripts";
my $SCRIPTS_BUILD_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_BUILD_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $PYSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/PyScripts";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

sub SearchScript {
    my $script_name_ = shift;
    my @textvec_ = @_ ;
    my @default_script_paths_ = () ;
    push ( @default_script_paths_, $SCRIPTS_INSTALL_DIR );
    push ( @default_script_paths_, $MODELSCRIPTS_INSTALL_DIR );
    push ( @default_script_paths_, $SCRIPTS_BUILD_DIR );
    push ( @default_script_paths_, $MODELSCRIPTS_BUILD_DIR );
    push ( @default_script_paths_, $PYSCRIPTS_DIR ); 
    my @combined_list_script_paths_ = () ;
    push @combined_list_script_paths_, @textvec_ ; # paths given by user
    push @combined_list_script_paths_, @default_script_paths_ ; # paths I had in defaults
    # Search default_paths after the given list
    
    my $found_script_ = "";
    foreach my $dirpath_ (@combined_list_script_paths_)
      {
	# For this path, see what the exec path would be if it were to exist
	my $searching_path_ = $dirpath_."/".$script_name_ ;
	
	if ( ExistsWithSize ( $searching_path_ ) )
	{ # if I find it then effectively return it
	    # In Perl, for that we need to break out of this loop
	    # after setting the return variable
	    $found_script_ = $searching_path_;
	    last;
	}
      }

    if ( $found_script_ eq "" ) {
      print "Did not find $script_name_ anywhere in search paths\n";
    }
    $found_script_;
}

1;
