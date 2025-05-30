#!/usr/bin/perl

# \file AwsScripts/sanity_check_instance.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $SSH_CMD = "ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no ";

# start
my $USAGE="$0 INSTANCE_IP";

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }

my $instance_ip_ = $ARGV [ 0 ];

my $exec_cmd_ = "";
my @exec_cmd_output_ = ( );

# check accessibility
$exec_cmd_ = "ping -i 1 -c 1 -W 3 ".$instance_ip_." | grep -c icmp_seq";
print $exec_cmd_."\n";

@exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );
if ( $#exec_cmd_output_ >= 0 )
{
    if ( $exec_cmd_output_ [ 0 ] != 1 )
    { # ping unsuccessful
	print "FAILURE ping\n";
    }
}
else
{
    print "FAILURE ping\n";
}

# check for ephemeral partition setup
$exec_cmd_ = $SSH_CMD.$instance_ip_." 'df | grep ephemeral | awk '{ print \$NF; }'' 2>/dev/null";
print $exec_cmd_."\n";

@exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );
if ( $#exec_cmd_output_ >= 0 )
{
    
}
