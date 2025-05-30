#!/usr/bin/perl
use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

# Require
require "Tests/test_sim_strategy.pl";
require "Tests/test_devmodel_devtrade.pl";

# Results
my @successes = ( );
my @error_messages = ( );

#my $mail_address_ = "ashwin.kumar\@tworoads.co.in";
my $mail_address_ = "vedant\@tworoads.co.in";

open ( MAIL , "|/usr/sbin/sendmail -t" );
print MAIL "To: $mail_address_\n";
print MAIL "From: $mail_address_\n";
print MAIL "Subject: [ BASETRADE TESTS ]\n";

sub RunTests 
{
	print "Running test 1 ...\n";
  print MAIL "Running test 1 ...\n";
	my ( $success, $error_message ) = TestPNL( );
	push ( @successes, $success );
	push ( @error_messages, $error_message );
	print "Running test 2 ...\n";
  print MAIL "Running test 2 ...\n";
	( $success, $error_message ) = TestNoSIGSEGV1( );	
	push ( @successes, $success );
	push ( @error_messages, $error_message );
	print "Running test 3 ...\n";
  print MAIL "Running test 3 ...\n";
	( $success, $error_message ) = TestDevmodelDevtradeDiff( );
	push ( @successes, $success );
	push ( @error_messages, $error_message );
	#TestBadFileAccess ();
}

sub DumpResults 
{
	my $total_tests = ( $#successes + 1 );
	my $successful_tests = 0;
	foreach my $i ( 0 .. ( $total_tests - 1 ) ) 
	{
		if ( $successes[$i] eq 0 ) 
		{
			print "Test failed ".( $i + 1 )." ".$error_messages[$i]."\n";
      print MAIL "Test failed ".( $i + 1 )." ".$error_messages[$i]."\n";
		}
		else 
		{
			$successful_tests++;
		}
	}
	print "----------------------------------------------------------\n";
  print MAIL "----------------------------------------------------------\n";
	my $pass_percentage = ( $successful_tests * 100 / $total_tests );
	print "Tests passed: ".$successful_tests." / ".$total_tests." ( ".$pass_percentage."% )\n";
  print MAIL "Tests passed: ".$successful_tests." / ".$total_tests." ( ".$pass_percentage."% )\n";
}

RunTests( );
DumpResults( );

close ( MAIL );
