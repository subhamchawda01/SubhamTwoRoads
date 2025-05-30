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
#if ( $#ARGV >= 0 ) 
#{
#    push ( @NUM_DAYS_PAST_VEC, $ARGV[0] );
#}
#else
#{
    push ( @NUM_DAYS_PAST_VEC, 5 );
    push ( @NUM_DAYS_PAST_VEC, 20 );
    push ( @NUM_DAYS_PAST_VEC, 60 );
    push ( @NUM_DAYS_PAST_VEC, 180 );
    push ( @NUM_DAYS_PAST_VEC, 500 );
#}

for my $NUM_DAYS_PAST ( @NUM_DAYS_PAST_VEC )
{

print "Performance over last $NUM_DAYS_PAST days\n";

printf "%s\t", "SHCODE";
printf "%s\t", "SUMPNL";
printf "%s\t", "AVG";
printf "%s\t", "STD";
printf "%s\t", "SHRP";
printf "%s\t", "AVGVOL";
printf "%s\t%s\t%s", "MAXDD", "AVG/DD", "GPR";
printf "\n";

for my $SHORTCODE ( "ALL", "CME", "EUREX", "TMX", "BMF", "LIFFE" , "HKEX" , "OSE" , "ZF", "ZN", "ZB", "UB", "FGBS", "FGBM", "FGBL", "FESX", "FDAX", "FOAT" , "FBTP" , "FBTS" , "CGB" , "CGF", "CGZ" , "BAX", "SXF", "DOL", "WDO", "IND", "WIN" , "DI1F13" , "DI1F14" , "DI1F15" , "DI1F16" , "DI1F17" , "DI1N13" , "DI1N14" , "JFFCE" , "KFFTI" , "LFR" , "LFZ" , "LFI" , "LFL", "YFEBM" , "XFC" , "XFRC" , "HHI" , "HSI" , "MHI" , "NKM" , "NK", "JGBL", "TOPIX" )
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
	
	printf "%s\t", $SHORTCODE;
	printf "%d\t", $SUMOUT;
	printf "%d\t%d\t%.2f\t", $PNL_AVG_, $PNL_STD_, $PNL_SHRP_;
	printf "%d\t", $VOLMEAN;
	printf "%d\t%.2f\t", $DRAWD, $AVG_DD_RATIO;
	printf "%.2f", $GPR;
	printf "\n";
    }
}
}
