#!/usr/bin/perl

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

if ( $#ARGV >= 0 )
{

my $main_log_file_ = "/home/dvctrader/testp34.txt";
my $main_log_file_handle_ = FileHandle->new;
$main_log_file_handle_->open ( "> $main_log_file_ " ) or PrintStacktraceAndDie ( "Could not open $main_log_file_ for writing\n" );
$main_log_file_handle_->autoflush(1);
$main_log_file_handle_->printf ( "In arg %s\n", $ARGV[0] );

}
exit ();
