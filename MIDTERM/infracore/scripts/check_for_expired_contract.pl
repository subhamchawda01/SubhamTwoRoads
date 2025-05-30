#!/usr/bin/perl
use warnings ;
use strict ;

if ( $#ARGV < 1 ) {
    printf "USAGE: position_file date\n";
    exit;
}

my $HOME_DIR=$ENV{'HOME'};
my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";


my $position_file = $ARGV[0];
my $today_date = $ARGV[1];

my $date = `$BIN_DIR/calc_next_day $today_date`; chomp($date);

open ( POS_FILE, " < $position_file" ) or PrintStacktraceAndDie (  " Could not open position file" );

while ( my $line = <POS_FILE> ) {
    chomp ( $line ) ;
    my @line_words = split ( ',', $line );
    if ( $#line_words <= 2 ) {
        my $secname = $line_words[0];
        my $pos = $line_words[1];
        my $price = $line_words[2];
        my $exec_cmd  = $BIN_DIR."/get_shortcode_for_symbol '".$secname."' ".$date;
        my $shc = `$exec_cmd`;
        if ( $shc ) {
            next;
        }else {    
          my $to     = "nseall@tworoads.co.in";
          my $from   =$to;
          my $title  ='ERROR: Positions in expiring contract';
          my $subject='ERROR: Positions in expiring contract';
          open(MAIL, "|/usr/sbin/sendmail -t");
    
## Mail Header

          print MAIL "To: $to\n";
          print MAIL "From: $from\n";
          print MAIL "Subject: $subject\n\n";

          print MAIL " ERROR: $secname is Expired on date $date\n";
          close ( MAIL);
        }
    }
}
