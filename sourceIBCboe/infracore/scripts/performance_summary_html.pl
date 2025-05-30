#!/usr/bin/perl
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

print "From: dvcinfraOnNY11\@circulumvite.com\n";
#print "To: kp\@circulumvite.com\n";
print "To: nseall@tworoads.co.in\n";
print "Subject: Performance summary\n";
print "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n";

print "<html><body>\n";

my @pnl_lines_ = `perl ~/infracore_install/scripts/performance_summary.pl 2>/dev/null`;
chomp ( @pnl_lines_ );

printf "<table border = \"1\" class=\"inlineTable\">" ;
for ( my $pnl_lines_idx_ = 0; $pnl_lines_idx_ <= $#pnl_lines_; $pnl_lines_idx_ ++ )
{
    my @rwords_ = split ( / {2,}/, $pnl_lines_ [ $pnl_lines_idx_ ] );

if ( $rwords_[0] eq "WKEY" || $rwords_[0] eq "W" )
{    
    printf "<tr>" ;

    for ( my $rwords_idx_ = 1 ; $rwords_idx_ <= $#rwords_; $rwords_idx_ ++ )
    {
	printf "<td>%s</td>", $rwords_[ $rwords_idx_] ;
    }
    
    printf "</tr>\n";
}
}
printf "</table>";

printf "\n";

printf "<table border = \"1\" class =\"inlineTable\">" ;
for ( my $pnl_lines_idx_ = 0; $pnl_lines_idx_ <= $#pnl_lines_; $pnl_lines_idx_ ++ )
{
    my @rwords_ = split ( / {2,}/, $pnl_lines_ [ $pnl_lines_idx_ ] );

if ( $rwords_[0] eq "MKEY" || $rwords_[0] eq "M" )
{    
    printf "<tr>" ;

    for ( my $rwords_idx_ = 1 ; $rwords_idx_ <= $#rwords_; $rwords_idx_ ++ )
    {
	printf "<td>%s</td>", $rwords_[ $rwords_idx_] ;
    }
    
    printf "</tr>\n";
}
}

printf "</table>";

printf "\n";

printf "<table border = \"1\">" ;

for ( my $pnl_lines_idx_ = 0; $pnl_lines_idx_ <= $#pnl_lines_; $pnl_lines_idx_ ++ )
{
    my @rwords_ = split ( / {2,}/, $pnl_lines_ [ $pnl_lines_idx_ ] );

if ( $rwords_[1] ne "SS" && $rwords_[1] ne "TS" &&
     $rwords_[0] ne "MKEY" && $rwords_[0] ne "WKEY" && $rwords_[0] ne "W" && $rwords_[0] ne "M" )
{    
    printf "<tr>" ;

    for ( my $rwords_idx_ = 0 ; $rwords_idx_ <= $#rwords_; $rwords_idx_ ++ )
    {
	printf "<td>%s</td>", $rwords_[ $rwords_idx_] ;
    }
    
    printf "</tr>\n";
}

}

for ( my $pnl_lines_idx_ = 0; $pnl_lines_idx_ <= $#pnl_lines_; $pnl_lines_idx_ ++ )
{
    my @rwords_ = split ( / {2,}/, $pnl_lines_ [ $pnl_lines_idx_ ] );

if ( $rwords_[1] ne "FS" && $rwords_[1] ne "TS" &&
     $rwords_[0] ne "MKEY" && $rwords_[0] ne "WKEY" && $rwords_[0] ne "W" && $rwords_[0] ne "M" )
{
    printf "<tr>" ;

    for ( my $rwords_idx_ = 0 ; $rwords_idx_ <= $#rwords_; $rwords_idx_ ++ )
    {
        printf "<td>%s</td>", $rwords_[ $rwords_idx_] ;
    }

    printf "</tr>\n";
}

}

for ( my $pnl_lines_idx_ = 0; $pnl_lines_idx_ <= $#pnl_lines_; $pnl_lines_idx_ ++ )
{
    my @rwords_ = split ( / {2,}/, $pnl_lines_ [ $pnl_lines_idx_ ] );

if ( $rwords_[1] ne "FS" && $rwords_[1] ne "SS" &&
     $rwords_[0] ne "MKEY" && $rwords_[0] ne "WKEY" && $rwords_[0] ne "W" && $rwords_[0] ne "M" )
{
    printf "<tr>" ;

    for ( my $rwords_idx_ = 0 ; $rwords_idx_ <= $#rwords_; $rwords_idx_ ++ )
    {
        printf "<td>%s</td>", $rwords_[ $rwords_idx_] ;
    }

    printf "</tr>\n";
}

}

print "</table>";
print "</body></html>\n";
