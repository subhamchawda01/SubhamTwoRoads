use strict;
use warnings;

sub SendMail
{
  my $to_ = shift;
  my $from_ = shift;
  my $subject_ = shift;
  my $mail_str_ = shift;

  open ( MAIL , "|/usr/sbin/sendmail -t" );
  print MAIL "To: $to_\n";
  print MAIL "From: $from_\n";
  print MAIL "Subject: $subject_\n";
  print MAIL "$mail_str_\n";
  close (MAIL);
}

1
