#!/usr/bin/perl
# \file scripts/check_running_tradeinits.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use strict;
use warnings;

my @all_machines_ = ( );

push ( @all_machines_, "10.23.200.51" );
push ( @all_machines_, "10.23.200.52" );
push ( @all_machines_, "10.23.200.53" );
push ( @all_machines_, "10.23.200.54" );

push ( @all_machines_, "10.23.196.51" );
push ( @all_machines_, "10.23.196.52" );
push ( @all_machines_, "10.23.196.53" );
push ( @all_machines_, "10.23.196.54" );

push ( @all_machines_, "10.23.182.51" );
push ( @all_machines_, "10.23.182.52" );

push ( @all_machines_, "10.23.23.11" );
push ( @all_machines_, "10.23.23.12" );
push ( @all_machines_, "10.23.23.13" ); # 10.220.40.1" );

push ( @all_machines_, "10.23.52.51" );
push ( @all_machines_, "10.23.52.52" );
push ( @all_machines_, "10.23.52.53" );

for ( my $i = 0 ; $i <= $#all_machines_ ; $i ++ )
{
    my $t_exec_cmd_ = "ssh ".$all_machines_ [ $i ]." \'ps -efH | grep tradeinit | grep -v grep\'";
#    print "$t_exec_cmd_\n";

    my @exec_output_ = `$t_exec_cmd_`;

    for ( my $j = 0 ; $j <= $#exec_output_ ; $j ++ )
    {
	print $exec_output_ [ $j ];
    }
}
