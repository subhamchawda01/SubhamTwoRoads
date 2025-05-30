#!/usr/bin/perl
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

#TDATE       SHC     BR_VOL  DV_VOL   BR_COMM DV_COMM
#20150416    BAX     6718    6773       0.150   0.150
#20150416    BR      761     760        3.149   0.019

print "From: dvcinfraOnNY11\@circulumvite.com\n";
#print "To: kp\@circulumvite.com\n";
print "To: nseall@tworoads.co.in\n";
print "Subject: Reconciliation Summary\n";
print "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n";

print "<html><body>\n";

my @lines_ = `perl ~/infracore_install/scripts/broker_recon.pl 2>/tmp/recon_err`;
chomp ( @lines_ );

printf "<table border = \"1\" class=\"inlineTable\">" ;
for ( my $lines_idx_ = 0; $lines_idx_ <= $#lines_; $lines_idx_ ++ )
{
    my @rwords_ = split ( / {2,}/, $lines_ [ $lines_idx_ ] );
    printf "<tr>" ;
    for ( my $rwords_idx_ = 0 ; $rwords_idx_ <= $#rwords_; $rwords_idx_ ++ )
    {
	if ( $rwords_[ 2 ] != $rwords_[ 3 ] )
	{
	    printf "<td><b>%s</b></td>", $rwords_[ $rwords_idx_] ;
	}
	else
	{
	    printf "<td>%s</td>", $rwords_[ $rwords_idx_] ;
	}
    }
    printf "</tr>\n";
}
printf "</table>";
printf "\n\n\n";

print `cat /tmp/recon_err` ;
printf "\n" ;

printf "</body></html>\n";
