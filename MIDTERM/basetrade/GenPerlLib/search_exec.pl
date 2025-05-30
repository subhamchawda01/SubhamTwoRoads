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
# my @ADDITIONAL_EXEC_PATHS=();
# my $SIM_STRATEGY_EXEC = SearchExec ( "sim_strategy", @ADDITIONAL_EXEC_PATHS ) ;
# 
# Shown an example in ModelScripts/run_sim_and_summarize.pl

use strict;
use warnings;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";
my $BASETRADE_BIN_DIR=$HOME_DIR."/cvquant_install/basetrade/bin";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
# my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize

sub SearchExec {
    my $exec_name_ = shift;
    my @textvec_ = @_ ;
    my @default_exec_paths_ = () ;
    my @combined_list_exec_paths_ = () ;

    if (defined $ENV{'DEPS_INSTALL'}){
        my $JENKINS_BIN_DIR = $ENV{'DEPS_INSTALL'}."/basetrade/bin";
        push ( @combined_list_exec_paths_, $JENKINS_BIN_DIR);
    }
    push ( @default_exec_paths_, $LIVE_BIN_DIR );
    push ( @default_exec_paths_, $BASETRADE_BIN_DIR );
    push @combined_list_exec_paths_, @textvec_ ; # paths given by userR
    push @combined_list_exec_paths_, @default_exec_paths_ ; # paths I had in defaults
    # Search default_paths after the given list
    
    my $found_exec_ = "";
    foreach my $dirpath_ (@combined_list_exec_paths_)
      {
	# For this path, see what the exec path would be if it were to exist
	my $searching_path_ = $dirpath_."/".$exec_name_ ;
	
	if ( ExistsWithSize ( $searching_path_ ) )
	{ # if I find it then effectively return it
	    # In Perl, for that we need to break out of this loop
	    # after setting the return variable
	    $found_exec_ = $searching_path_;
	    last;
	}
      }

    if ( $found_exec_ eq "" ) {
      print "Did not find $exec_name_ anywhere in search paths\n";
    }
    $found_exec_;
}

1;
