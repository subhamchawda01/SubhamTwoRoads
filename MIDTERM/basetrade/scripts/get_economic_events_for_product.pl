#!/usr/bin/perl
use strict;
use warnings ;
use feature "switch"; # for given, when
use sigtrap qw(handler signal_handler normal-signals error-signals);
use File::Basename ;
use FileHandle;

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $SCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/scripts";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";

my $MODELING_BASE_DIR = $HOME_DIR."/modelling";

require "$GENPERLLIB_DIR/get_dates_for_shortcode.pl";

if ( $#ARGV < 0 ) {
  print "USAGE: <script> <shortcode> <end_date> <numdays> <start_hhmm> <end_hhmm>\n";
  exit(0);
}

my $shortcode_ = shift;
my $enddate_ = shift;
my $numdays_ = shift;
my $stime_ = shift;
my $etime_ = shift;

my @dates_vec_ = GetDatesFromNumDays( $shortcode_, $enddate_, $numdays_ );

my $get_events_exec_ = $BIN_DIR."/get_economic_events_of_the_day_for_shortcode";
foreach my $date_ ( @dates_vec_ ) {
  my $getev_cmd_ = "$get_events_exec_ $shortcode_ $stime_ $etime_ $date_";
  my @evlines_ = `$getev_cmd_ | tail -n +2`; chomp ( @evlines_ );

  my %evname_to_details_ = ( );
  my %time_to_eventvec_ = ( );
  foreach my $line_ ( @evlines_ ) {
    my @lwords_ = split(/\s+/, $line_ );
    next if $#lwords_ < 15;

    my $evname_ = $lwords_[6];
    my $ezone_ = $lwords_[4];
    my $sev_ = $lwords_[5];
    my $time_ = $lwords_[0];
    my $end_mfm_ = $lwords_[12];
    my $ev_mfm_ = $lwords_[11];

    $evname_ =~ s/\(.*$//;
    $evname_ =~ s/\-_[^-]*$//;
    $evname_ =~ s/_[a-z.]*[_]*$/_/g;
    $evname_ = "$ezone_ $evname_";

    if ( defined $evname_to_details_{ $evname_ } ) { next; }

    $evname_to_details_{ $evname_ }{ "SEVERITY" } = $sev_;
    $evname_to_details_{ $evname_ }{ "FLAT_SECS" } = $end_mfm_ - $ev_mfm_;
    $evname_to_details_{ $evname_ }{ "TIME" } = $time_;
    push ( @{$time_to_eventvec_{ $time_ } }, $evname_ );
  }
  foreach my $time_ ( sort keys %time_to_eventvec_ ) {
    foreach my $ev_ ( @{ $time_to_eventvec_{ $time_ } } ) {
      my $dtout_ = `date -d \@$time_ +'%Y%m%d %H%M'`; chomp ( $dtout_ );
      my ($date_, $hhmm_) = split(" ", $dtout_);

      print $dtout_." ".$evname_to_details_{$ev_}{"FLAT_SECS"}." ".$evname_to_details_{$ev_}{"SEVERITY"}." ".$ev_."\n";
    }
  }
}

