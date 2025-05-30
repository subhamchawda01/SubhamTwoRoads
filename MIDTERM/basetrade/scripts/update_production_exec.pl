#!/usr/bin/perl

# \file scripts/update_production_exec.pl
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
use POSIX;
use List::Util qw[min max]; # max , min

my $USER = $ENV { 'USER' };
my $HOME_DIR = $ENV { 'HOME' };

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # for ExistsWithSize
require "$GENPERLLIB_DIR/sync_running_exec_to_prod_machine.pl"; # for SyncToAllMachines

if ( $#ARGV < 1 )
{
    print "USAGE: server-ip full_filename backup_name changes_in_new_exec \n";
    exit (0);
}

my $prod_machine_ = $ARGV[0];
my $this_filename_ = $ARGV[1];
my $this_backupname_ = $ARGV[2]; 
my $changes_description_ = $ARGV[3] ;
if ( ExistsWithSize ( $this_filename_ ) )
{
    SyncRunningExecToProdMachine ( $prod_machine_, $this_filename_, $this_backupname_, $changes_description_ ) ;
}

