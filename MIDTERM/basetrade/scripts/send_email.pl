#!/usr/bin/env perl

my $hostname = `hostname -s` ;
chomp ( $hostname ) ;

push ( @ARGV, "HOSTNAME:: ".$hostname )  ;

my $mail_body_ = join ( ' ', @ARGV );
$mail_body_ =~ s/__n__/\n/g;
#print $mail_body_ ;

open(MAIL, "|/usr/sbin/sendmail -t");
print MAIL $mail_body_ ;
close(MAIL);



