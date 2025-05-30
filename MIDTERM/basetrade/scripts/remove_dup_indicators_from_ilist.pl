#!/usr/bin/perl

# \file scripts/remove_dup_indicators_from_ilist.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551
#

use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use FileHandle;

my $USER = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'}; 

my $REPO="basetrade";

my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

if ( $USER ne "dvctrader" )
{
    $LIVE_BIN_DIR = $BIN_DIR;
}

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; # FindItemFromVec

# start
my $USAGE="$0 ILIST_FILE";

if ( $#ARGV < 0 ) { 
    my $ind_line="INDICATOR 1.88418 TDLvlSumTTypeCombo UBFUT 0.1 # 0.08";
    if ( $ind_line =~ m/INDICATOR ([\d\.\-e]+) ([^#]*) (#?.*)/ ) {
	my ($coeff, $ind, $comment)  = ( $1, $2, $3 );
	print "coeff: $coeff, $ind, $comment\n";
    }
    print $USAGE."\n"; exit ( 0 ); 
}

my $ilist_file_ = $ARGV [ 0 ];


my $random_id_ = `date +%N`; $random_id_ = $random_id_ + 0;
my $edited_ilist_file_ = $ilist_file_.$random_id_;

open ( ILIST_FILE , "<" , $ilist_file_ ) or PrintStacktraceAndDie ( "Could not open config file $ilist_file_" );
my @indicators_ = <ILIST_FILE>; chomp ( @indicators_ );
close ( ILIST_FILE );

my @unique_indicator_list_ = ( );
my @final_indicator_lines_ = ( );

open ( ILIST_FILE , ">" , $edited_ilist_file_ ) or PrintStacktraceAndDie ( "Could not open config file $edited_ilist_file_" );

if ( $USER eq "rahul" )  {
    my %indicators_hash_ = () ;
    foreach my $i_line_ ( @indicators_ )
    {
	if ( $i_line_ =~ m/INDICATOR ([\d\.\-e]+) ([^#]*) ?#?(.*$)/ ) {
	    my @vec_ = ( $1, $3 );
	    my $ind_desc_ = $2;
	    my $comment_ = $3;
	    $ind_desc_ =~ s/\s+$//g ;
	    if ( ! $indicators_hash_{ $ind_desc_ } ) {
		@{$indicators_hash_{ $ind_desc_ }}=@vec_ ;
	    }
	}
	elsif ( $i_line_ !~ m/INDICATOREND/ ) {
	    print ILIST_FILE $i_line_."\n";
	}
    }
    for my $ind_ ( keys %indicators_hash_ ){ 
	printf ILIST_FILE "INDICATOR %.3f %s # %s\n", $indicators_hash_{$ind_}[0], $ind_, $indicators_hash_{$ind_}[1];
    }
    print ILIST_FILE "INDICATOREND\n";
    close ( ILIST_FILE );
    `mv $edited_ilist_file_ $ilist_file_`;
    exit;
}

close ( ILIST_FILE );

foreach my $indicator_line_ ( @indicators_ )
{
    # OnlineComputedCutoffPair HHI_0 NKM_0 0.25 MktSizeWPrice # TSTAT 0.937115 CORR 0.0215105
    # to
    # OnlineComputedCutoffPair HHI_0 NKM_0 0.25 MktSizeWPrice
    my @t_indicator_words_ = split ( ' ' , $indicator_line_ ); chomp ( @t_indicator_words_ );

    my $t_indicator_line_ = $t_indicator_words_ [ 0 ];
    
    my $t_indicator_line_w_coeff_ = $t_indicator_words_ [0];
    
    for ( my $i = 1 ; $i <= $#t_indicator_words_ && $t_indicator_words_ [ $i ] ne "#" ; $i ++ )
    {
	if ( $i > 1 )
	{  
	    $t_indicator_line_ = $t_indicator_line_." ".$t_indicator_words_ [ $i ];
	}

	$t_indicator_line_w_coeff_ = $t_indicator_line_w_coeff_." ".$t_indicator_words_[ $i ]; 
    }

    if ( ! FindItemFromVec ( $t_indicator_line_ , @unique_indicator_list_ ) )
    {
	push ( @final_indicator_lines_ , $t_indicator_line_w_coeff_ );
	push ( @unique_indicator_list_, $t_indicator_line_ );
#	print $t_indicator_line_."\n";
    }
}

open ( ILIST_FILE , ">" , $edited_ilist_file_ ) or PrintStacktraceAndDie ( "Could not open config file $edited_ilist_file_" );
print ILIST_FILE join ( "\n" , @final_indicator_lines_ );
close ( ILIST_FILE );

`mv $edited_ilist_file_ $ilist_file_`;
	
exit ( 0 );
