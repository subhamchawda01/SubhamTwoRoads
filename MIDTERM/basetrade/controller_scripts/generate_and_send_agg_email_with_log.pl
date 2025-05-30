#!/usr/bin/perl

# \file controller_scripts/generate_and_send_agg_email_with_log.pl
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

my $filename_ = "/mnt/sdf/JOBS/emails/cleanup_agg_email.txt";
my $email_address_ = "nseall@tworoads.co.in";

if ( -e $filename_ &&
     -s $filename_ > 0 )
{
    open ( FILEHANDLE , "<" , $filename_ ) or die "Could not open $filename_";
    my @file_lines_ = <FILEHANDLE>; chomp ( @file_lines_ );
    close ( FILEHANDLE );

    open(MAIL, "|/usr/sbin/sendmail -t");
	
    my $hostname_=`hostname`;
    print MAIL "To: $email_address_\n";
    print MAIL "From: dvctrader\@ip-10-0-0-11\n";
    print MAIL "Subject: AWS JOBS Batch email\n\n";

    for ( my $line_index_ = 0 ; $line_index_ <= $#file_lines_ ; $line_index_ ++ )
    {
	if ( $file_lines_ [ $line_index_ ] eq "START" )
	{
	    print MAIL " ------------------------------------------------------------ \n";

	    $line_index_ ++;
	    my $email_subject_ = $file_lines_ [ $line_index_ ];
	    print MAIL $email_subject_."\n";

	    $line_index_ ++;
	    my $log_filename_ = $file_lines_ [ $line_index_ ];
	    my @log_file_contents_ = `cat $log_filename_`; chomp ( @log_file_contents_ );

	    `rm -f $log_filename_`;

	    print MAIL join ( "\n" , @log_file_contents_ )."\n";

	    print MAIL " ------------------------------------------------------------ \n\n";
	}
    }

    close(MAIL);
}

`> $filename_`;

exit ( 0 );
