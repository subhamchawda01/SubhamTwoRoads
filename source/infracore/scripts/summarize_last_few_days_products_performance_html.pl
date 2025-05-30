#!/usr/bin/perl

# \file scripts/summarize_last_few_days_performance.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 162, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 

my $REPO="infracore";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."/GenPerlLib";

require "$GENPERLLIB_DIR/array_ops.pl"; # GetSum

my @NUM_DAYS_PAST_VEC=();
if ( $#ARGV >= 0 ) 
{
    push ( @NUM_DAYS_PAST_VEC, $ARGV[0] );
}
else
{
    push ( @NUM_DAYS_PAST_VEC, 5 );
    push ( @NUM_DAYS_PAST_VEC, 20 );
    push ( @NUM_DAYS_PAST_VEC, 60 );
    push ( @NUM_DAYS_PAST_VEC, 180 );
    push ( @NUM_DAYS_PAST_VEC, 500 );
}

print "From: nseall@tworoads.co.in\n";
print "To: nseall@tworoads.co.in\n";
print "Subject: Performance summary over last few days\n";
print "X-Mailer: htmlmail 1.0\nMime-Version: 1.0\nContent-Type: text/html; charset=US-ASCII\n\n";

print "<html><body>\n";

for my $NUM_DAYS_PAST ( @NUM_DAYS_PAST_VEC )
{

    print "Performance last $NUM_DAYS_PAST days\n";

    print "<table border = \"1\"><tr>";
    printf "<td>%s</td>", "SHCODE";
    printf "<td>%s</td>", "SUMPNL";
    printf "<td>%s</td>", "AVG";
    printf "<td>%s</td>", "STD";
    printf "<td>%s</td>", "SHRP";
    printf "<td>%s</td>", "AVGVOL";
    printf "<td>%s</td><td>%s</td><td>%s</td>", "MAXDD", "AVG/DD", "GPR";
    printf "</tr>\n";

    my @SHORTCODE_VEC=();
    # push ( @SHORTCODE_VEC, "ALL" ); 
    # push ( @SHORTCODE_VEC, "CME" ); 
    # push ( @SHORTCODE_VEC, "EUREX" ); 
    # push ( @SHORTCODE_VEC, "TMX" ); 
    # push ( @SHORTCODE_VEC, "BMF" ); 
    # push ( @SHORTCODE_VEC, "LIFFE" ); 
    # push ( @SHORTCODE_VEC, "HKEX" ); 

    push ( @SHORTCODE_VEC, "ZF" ); 
    push ( @SHORTCODE_VEC, "ZN" ); 
    push ( @SHORTCODE_VEC, "ZB" ); 
    push ( @SHORTCODE_VEC, "UB" ); 

    push ( @SHORTCODE_VEC, "FGBS" ); 
    push ( @SHORTCODE_VEC, "FGBM" ); 
    push ( @SHORTCODE_VEC, "FGBL" ); 
    push ( @SHORTCODE_VEC, "FESX" ); 
    push ( @SHORTCODE_VEC, "FDAX" ); 
    push ( @SHORTCODE_VEC, "FOAT" ); 
    push ( @SHORTCODE_VEC, "FBTP" );
    push ( @SHORTCODE_VEC, "FBTS" );

    push ( @SHORTCODE_VEC, "CGB" ); 
    push ( @SHORTCODE_VEC, "CGF" );
    push ( @SHORTCODE_VEC, "CGZ" );
    push ( @SHORTCODE_VEC, "BAX" ); 
    push ( @SHORTCODE_VEC, "SXF" ); 

    push ( @SHORTCODE_VEC, "DOL" ); 
    #push ( @SHORTCODE_VEC, "WDO" ); 
    push ( @SHORTCODE_VEC, "IND" ); 
    push ( @SHORTCODE_VEC, "WIN" ); 
    push ( @SHORTCODE_VEC, "DI" ); 
    # push ( @SHORTCODE_VEC, "DI1F13" ); 
    # push ( @SHORTCODE_VEC, "DI1F14" ); 
    # push ( @SHORTCODE_VEC, "DI1F15" ); 
    # push ( @SHORTCODE_VEC, "DI1F16" ); 
    # push ( @SHORTCODE_VEC, "DI1F17" ); 
    # push ( @SHORTCODE_VEC, "DI1N13" ); 
    # push ( @SHORTCODE_VEC, "DI1N14" ); 

    push ( @SHORTCODE_VEC, "LFZ" ); 
    push ( @SHORTCODE_VEC, "JFFCE" ); 
    push ( @SHORTCODE_VEC, "KFFTI" ); 
    push ( @SHORTCODE_VEC, "LFR" ); 
    push ( @SHORTCODE_VEC, "LFI" ); 
    push ( @SHORTCODE_VEC, "LFL" ); 

    push ( @SHORTCODE_VEC, "YFEBM" ); 
    push ( @SHORTCODE_VEC, "XFC" ); 
    push ( @SHORTCODE_VEC, "XFRC" ); 

    push ( @SHORTCODE_VEC, "HHI" ); 
    push ( @SHORTCODE_VEC, "HSI" ); 
    push ( @SHORTCODE_VEC, "MHI" );

    push ( @SHORTCODE_VEC, "NKM" );
    push ( @SHORTCODE_VEC, "NK" );
    push ( @SHORTCODE_VEC, "JGBL" );
    push ( @SHORTCODE_VEC, "TOPIX" );

    for my $SHORTCODE ( @SHORTCODE_VEC )
    {
	my @pnl_lines_ = `~/infracore/scripts/query_pnl_data_mult.pl TODAY-$NUM_DAYS_PAST TODAY $SHORTCODE`;
	chomp ( @pnl_lines_ );

	my @date_vec_ = ();
	my @pnl_vec_ = ();
	my @volume_vec_ = ();

	for ( my $pnl_lines_idx_ = 0; $pnl_lines_idx_ <= $#pnl_lines_; $pnl_lines_idx_ ++ )
	{
	    my @rwords_ = split ( ',', $pnl_lines_ [ $pnl_lines_idx_ ] );
	    if ( $#rwords_ >= 2 )
	    {
		push ( @date_vec_, $rwords_[0] );
		push ( @pnl_vec_, $rwords_[1] );
		push ( @volume_vec_, $rwords_[2] );
	    }
	}
	my $SUMOUT=GetSum ( \@pnl_vec_ );
	my $GPR=GetGainToPainRatio ( \@pnl_vec_ );
	if ( $SUMOUT != 0 ) 
	{
	    my $PNL_AVG_ = GetAverage ( \@pnl_vec_ );
	    my $PNL_STD_ = GetStdev ( \@pnl_vec_ );
	    my $PNL_SHRP_ = 0;
	    if ( $PNL_STD_ > 0 ) { $PNL_SHRP_ = $PNL_AVG_ / $PNL_STD_ ; }
	    my $VOLMEAN = GetAverage ( \@volume_vec_ );
	    my $DRAWD=GetMaxDrawdown ( \@pnl_vec_ );
	    my $AVG_DD_RATIO = 0;
	    if ( $DRAWD > 0 ) { $AVG_DD_RATIO = $PNL_AVG_ / $DRAWD ; }
	    
	    printf "<tr><td>%s</td>", $SHORTCODE;
	    printf "<td>%d</td>", $SUMOUT;
	    printf "<td>%d</td><td>%d</td><td>%.2f</td>", $PNL_AVG_, $PNL_STD_, $PNL_SHRP_;
	    printf "<td>%d</td>", $VOLMEAN;
	    printf "<td>%d</td><td>%.2f</td>", $DRAWD, $AVG_DD_RATIO;
	    printf "<td>%.2f</td>", $GPR;
	    printf "</tr>\n";
	}
    }
    
    print "</table>";
}

print "</body></html>\n";
