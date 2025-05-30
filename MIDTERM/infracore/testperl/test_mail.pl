#!/usr/bin/perl
$title='Perl Mail demo';
$to='gchak@circulumvite.com';
$from= 'gchak@circulumvite.com';
$subject='TEST Perl Script for Mail';

open(MAIL, "|/usr/sbin/sendmail -t");

## Mail Header
print MAIL "To: $to\n";
print MAIL "From: $from\n";
print MAIL "Subject: $subject\n\n";
## Mail Body
print MAIL "This is a test \n";
print MAIL "This is a test 2 \n";
print MAIL "This is a test 3 \n";
print MAIL "This is a test 4 \n";

close(MAIL);

