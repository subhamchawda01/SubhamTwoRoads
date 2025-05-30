#!/usr/bin/perl


sub ReportSummary ( )
{
    my $subject = shift ;
    my $address = shift ;
    my $body = shift ;
    
    if ( $address eq "NA" )
    {
	print $body ;	
    }
    else
    {
	open ( my $MAIL, "|/bin/mail -s \"$subject\" \"$address\" " ) ;
	print $MAIL $body ;
	close $MAIL ;
    }
}

1

