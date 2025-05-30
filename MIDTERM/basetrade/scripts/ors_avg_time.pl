#!/usr/bin/perl

# \file scripts/ors_avg_time.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/array_ops.pl" ; # GetAverage GetMedianAndSort

my $FILENAME = $ARGV[0];

open(INFILE, $FILENAME) or die ("Unable to open");
my @data=<FILE>;
my %seqtime;
my $count = 0;
my $total_conf = 0;
my $total_seqd = 0;

my @vals_ = ();
while (<INFILE>)
{
    $line_ = $_;
    my @words_ = split ( ' ', $line_ );
    if ( $#words_ >= 12 )
    {
	if ( $words_[12] eq "Seqd" )
	{
	    $total_seqd++;
	    $seqtime{$words_[15]}=$words_[9];
	}
	if ( $words_[12] eq "Conf" )
	{
	    $total_conf++;
	    if ( exists $seqtime{$words_[15]} )
	    {
		my $diff_ = $words_[9] - $seqtime{$words_[15]} ;
		push ( @vals_, $diff_ );
		$total += $diff_;
		$count++;
	    }
	    else
	    {
#		print $words_[15]."\n";
	    }
	}
    }
}

print "Total Seq/Conf: ".$count."\n";
print "Total Conf: ".$total_conf."\n";
print "Total Seqd: ".$total_seqd."\n";
printf "Avg.Time(sec): %f\n",GetAverage ( \@vals_ );
printf "Median.Time(sec): %f\n",GetMedianAndSort ( \@vals_ );
#SYM : FGBL201112 Price :138.18 INTPX: 13818 TIMESET : 1315843206.975527 ORRTYPE : Exec SAOS : 4766 GBLPOS: 0
