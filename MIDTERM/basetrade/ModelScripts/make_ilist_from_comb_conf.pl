#!/usr/bin/perl

# \file ModelScripts/make_ilist_from_comb_config.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#      Suite No 353, Evoma, #14, Bhattarhalli,
#      Old Madras Road, Near Garden City College,
#      KR Puram, Bangalore 560049, India
#      +91 80 4190 3551
#
# This script takes a prod_config
# START
# TIMEPERIODSTRING US_MORN_DAY
# PREDALGO na_e3 
# PREDDURATION 0.05 0.5 2 8 32 96 
# DATAGEN_START_HHMM EST_700 
# DATAGEN_END_HHMM EST_1200 
# DATAGEN_TIMEOUT 3000 4 0
# DATAGEN_BASE_FUT_PAIR MktSizeWPrice MktSizeWPrice
# SELF FGBM_0 
# SOURCE FGBS_0 FGBM_0 
# SOURCECOMBO UBEFC 
# END

use strict;
use warnings;
use feature "switch"; # for given, when
use Fcntl qw (:flock);
use File::Basename; # for basename and dirname
use File::Copy; # for copy
use List::Util qw/max min/; # for max
use FileHandle;

my $HOME_DIR=$ENV{'HOME'}; 
my $USER=$ENV{'USER'}; 
my $SPARE_HOME="/spare/local/".$USER."/";
my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $IWORK_DIR=$HOME_DIR."/indicatorwork" ;
my $ITEMPLATES_DIR=$HOME_DIR."/modelling/indicatorwork";

require "$GENPERLLIB_DIR/has_liffe_source.pl"; # HasLIFFESource
require "$GENPERLLIB_DIR/has_espeed_source.pl"; # HasESPEEDSource
require "$GENPERLLIB_DIR/has_tmx_source.pl"; # HasTMXSource
require "$GENPERLLIB_DIR/has_hkfe_source.pl"; # HasHKFESource
require "$GENPERLLIB_DIR/has_cfe_source.pl"; # HasCFESource
require "$GENPERLLIB_DIR/is_order_weighted_indicator.pl"; # IsOWIndicator
# require "$GENPERLLIB_DIR/skip_combo_book_indicator.pl"; # SkipComboBookIndicator

# start 
my $SCRIPTNAME="$0";

my $USAGE="$0 prodconfig";

sub Trim_And_Split{
    my($arg) = @_;
    if ( $arg ) {
       chomp($arg);
       $arg =~ s/^\s+//;
       $arg =~ s/\s+$//;
       return split ( /\s+/ , $arg ) ;
    }
    else{
        my @empty=();
        return @empty;
    }
}

if ( $#ARGV < 0 ) { print $USAGE."\n"; exit ( 0 ); }
my $prodconfig_ = $ARGV[0];
my $SELF = ` grep -w SELF $prodconfig_ | awk '{print \$2}' `;
chomp($SELF);
my @SOURCE = Trim_And_Split ( ` grep -w SOURCE $prodconfig_ | awk '{\$1=""; print \$0}' ` );
my @SOURCECOMBO = Trim_And_Split ( ` grep -w SOURCECOMBO $prodconfig_ | awk '{\$1=""; print \$0}' ` );
my @NOSELFSOURCE = ();
foreach my $src (@SOURCE ) {
    next if ($src eq $SELF);
    push ( @NOSELFSOURCE, $src);
}
my @PORT = Trim_And_Split ( ` grep -w PORT $prodconfig_ | awk '{\$1=""; print \$0}' ` );

#NOTE that we expect files to end in newline for cat to work as expected.
#If we cant ensure this, we must add some corrective script to 
#ensure each line contains exactly one INDICATOR
my @self_ind_ = `cat $ITEMPLATES_DIR/indicator_list_*_SELF | grep -v "^#" `;
my @noselfsource_ind_= `cat $ITEMPLATES_DIR/indicator_list_*_NOSELFSOURCE | grep -v "^#" `;
my @sourcecombo_ind_ = `cat $ITEMPLATES_DIR/indicator_list_*_SOURCECOMBO | grep -v "^#" `;
my @source_ind_ = `cat $ITEMPLATES_DIR/indicator_list_*_SOURCE | grep -v "^#" `;

my @full_indicator_body_vec_ = ();
#self
foreach my $line(@self_ind_ )
{
    chomp ($line);
    my @parts = Trim_And_Split($line);
    my $is_ow_ind = IsOWIndicator($parts[2]);
    my $is_liffe = HasLIFFESource ( $SELF ) ; 
    my $is_espeed = HasESPEEDSource ( $SELF ) ; 
    my $is_tmx   = HasTMXSource   ( $SELF ) ; 
    my $is_hkfe  = HasHKFESource   ( $SELF ) ; 
    my $is_cfe  = HasCFESource   ( $SELF ) ; 

    next if ( ( $is_tmx == 1 || $is_liffe == 1 || $is_hkfe == 1 || $is_espeed == 1 || $is_cfe == 1 ) && $is_ow_ind == 1);
    $line =~ s/SELF/$SELF/g;
    push ( @full_indicator_body_vec_, $line );
}
#source
foreach my $line(@source_ind_ )
{
    chomp ($line);
    my @parts = Trim_And_Split($line);
    my $is_ow_ind = IsOWIndicator($parts[2]);
    $line =~ s/ SELF / $SELF /g;
    foreach my $prod( @SOURCE ) {
	    my $is_liffe = HasLIFFESource ( $prod ) ; 
	    my $is_espeed = HasESPEEDSource ( $prod ) ; 
	    my $is_tmx   = HasTMXSource   ( $prod ) ; 
	    my $is_hkfe  = HasHKFESource   ( $prod ) ; 
            my $is_cfe  = HasCFESource   ( $SELF ) ; 

	    next if ( ( $is_tmx == 1 || $is_liffe == 1 || $is_hkfe == 1 || $is_espeed == 1 || $is_cfe == 1 ) && $is_ow_ind == 1);
	    my $copy = $line;
            $copy =~ s/SOURCE/$prod/g;
            push ( @full_indicator_body_vec_, $copy );
    }
}
#noselfsource
foreach my $line(@noselfsource_ind_ )
{
    chomp ($line);
    my @parts = Trim_And_Split($line);
    my $is_ow_ind = IsOWIndicator($parts[2]);
    $line =~ s/ SELF / $SELF /g;
    foreach my $prod( @NOSELFSOURCE ) {
	    my $is_liffe = HasLIFFESource ( $prod ) ; 
	    my $is_espeed = HasESPEEDSource ( $prod ) ; 
	    my $is_tmx   = HasTMXSource   ( $prod ) ; 
	    my $is_hkfe  = HasHKFESource   ( $prod ) ; 
            my $is_cfe  = HasCFESource   ( $SELF ) ; 

	    next if ( ( $is_tmx == 1 || $is_liffe == 1 || $is_hkfe == 1 || $is_espeed == 1 || $is_cfe == 1 ) && $is_ow_ind == 1);
	    my $copy = $line;
            $copy =~ s/NOSELFSOURCE/$prod/g;
            push ( @full_indicator_body_vec_, $copy );
    }
}
#sourcecombo+port
foreach my $line(@sourcecombo_ind_ )
{
    chomp ($line);
    my @parts = Trim_And_Split($line);
    my $is_ow_ind = IsOWIndicator($parts[2]);
    $line =~ s/ SELF / $SELF /g;
    foreach my $prod( @SOURCECOMBO ) {
	    my $is_liffe = HasLIFFESource ( $prod ) ; 
	    my $is_espeed = HasESPEEDSource ( $prod ) ; 
	    my $is_tmx   = HasTMXSource   ( $prod ) ; 
	    my $is_hkfe  = HasHKFESource   ( $prod ) ; 
            my $is_cfe  = HasCFESource   ( $SELF ) ; 

	    next if ( ( $is_tmx == 1 || $is_liffe == 1 || $is_hkfe == 1 || $is_espeed == 1 || $is_cfe == 1 ) && $is_ow_ind == 1);
	    my $copy = $line;
            $copy =~ s/SOURCECOMBO/$prod/g;
            push ( @full_indicator_body_vec_, $copy );
    }
    
    if ( $line =~ m/Port / ) {
    	foreach my $prod( @PORT ) {
	    my $is_liffe = HasLIFFESource ( $prod ) ; 
	    my $is_espeed = HasESPEEDSource ( $prod ) ; 
	    my $is_tmx   = HasTMXSource   ( $prod ) ; 
	    my $is_hkfe  = HasHKFESource   ( $prod ) ; 
            my $is_cfe  = HasCFESource   ( $SELF ) ; 

	    next if ( ( $is_tmx == 1 || $is_liffe == 1 || $is_hkfe == 1 || $is_espeed == 1 || $is_cfe == 1 ) && $is_ow_ind == 1);
	    my $copy = $line;
            $copy =~ s/SOURCECOMBO/$prod/g;
            push ( @full_indicator_body_vec_, $copy );
    	}
    }
}
#print to console
print ( join ("\n", @full_indicator_body_vec_ ) ) ; 
