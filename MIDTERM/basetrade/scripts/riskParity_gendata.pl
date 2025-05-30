#!/usr/bin/perl

use strict;
use warnings;
use feature "switch";          # for given, when
use File::Basename;            # for basename and dirname
use File::Copy;                # for copy
use List::Util qw/max min/;    # for max
use FileHandle;
use Scalar::Util qw(looks_like_number);

my $USER     = $ENV{'USER'};
my $HOME_DIR = $ENV{'HOME'};
my $REPO     = "basetrade";

my $SCRIPTS_DIR      = $HOME_DIR . "/" . $REPO . "_install/scripts";
my $MODELSCRIPTS_DIR = $HOME_DIR . "/" . $REPO . "_install/ModelScripts";
my $GENPERLLIB_DIR   = $HOME_DIR . "/" . $REPO . "_install/GenPerlLib";

#my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $BASETRADE_BIN_DIR = $HOME_DIR . "/basetrade_install/bin";

require "$GENPERLLIB_DIR/valid_date.pl";                                  # ValidDate
require "$GENPERLLIB_DIR/is_date_holiday.pl";                             # IsDateHoliday
require "$GENPERLLIB_DIR/is_product_holiday.pl";                          # IsProductHoliday
require "$GENPERLLIB_DIR/skip_weird_date.pl";                             # SkipWeirdDate
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl";
require "$GENPERLLIB_DIR/no_data_date.pl";                                # NoDataDate

my $workdir = $HOME_DIR . "/RiskParity";
if ( !( -d $workdir ) ) { `mkdir -p $workdir`; }

my $nproducts=0;
my @prodcodes=();
my $datafname="";
my $end_date="";
my $ndays=0;
my $gendata=1;
my $datagen_out_="";
my $closeEOD="F";
my $aggress="F";
my $hor_msecs=0;

{
    my $CMDLINEINSTRUCTION = "RUN: script n:products prod1shortcode prod2shortcode .. prodnshortcode datafile_name end_yyyymmdd n:days closeEOD(T/F) aggressive(T/F) horizon(msecs)";

    if ($#ARGV < 0) {
        print $CMDLINEINSTRUCTION."\n";
	exit(0);
    }
    $nproducts = $ARGV[0];
    if ($#ARGV < ($nproducts+6)) {
        print $CMDLINEINSTRUCTION."\n";
	exit(0);
    }

    foreach my $prodno (1 .. $nproducts) {
        push(@prodcodes, $ARGV[$prodno]);
    }
    $datafname = $ARGV[$nproducts+1];
    $end_date = $ARGV[$nproducts+2];
    $ndays = $ARGV[$nproducts+3];
    $closeEOD = $ARGV[$nproducts+4];
    $aggress = $ARGV[$nproducts+5];
    $hor_msecs = $ARGV[$nproducts+6];

    if ( -e "$workdir/$datafname") {
        $gendata = 0;
    }        
    $datagen_out_ = "$workdir/$datafname";
}

if ($gendata) {
    my @datagen_day_vec_ = ();
    my $shortcode_ = $prodcodes[0]; 
    my $datagen_date_ = $end_date;
    my $max_days_ = $ndays;

    for ( my $t_day_index_ = 0 ; $t_day_index_ < $max_days_ ; $t_day_index_++ ) {
        #print $main_log_file_handle_ "Considering datagen day $datagen_date_ \n";

        if ( ( !ValidDate($datagen_date_) ) )
        {
            last;
        }
        if (    SkipWeirdDate($datagen_date_)
            || NoDataDateForShortcode( $datagen_date_, $shortcode_ )
            || ( IsDateHoliday($datagen_date_)
                || (    ($shortcode_)
                    && ( IsProductHoliday( $datagen_date_, $shortcode_ ) ) ) ) )
        {
            $datagen_date_ = CalcPrevWorkingDateMult( $datagen_date_, 1 );
            next;
        }
    
        push( @datagen_day_vec_, $datagen_date_ );
        
        $datagen_date_ = CalcPrevWorkingDateMult( $datagen_date_, 1 );
    }

    print "Generating Ilist File\n";

    my @px_types = ("MktSizeWPrice", "BidPrice", "AskPrice");
    my $codesjoined = join('_', @prodcodes);
    my $ilist_filename = $workdir . "/ilist_$codesjoined";

    open ILIST_OUT, ">$ilist_filename";
    
    print ILIST_OUT "MODELINIT DEPBASE $prodcodes[0] $px_types[0] $px_types[0]\n";
    print ILIST_OUT "MODELMATH LINEAR CHANGE\nINDICATORSTART\n";
    foreach my $pxtype (@px_types) {
        foreach my $scode (@prodcodes) {
            print ILIST_OUT "INDICATOR 1.0 SimplePriceType $scode $pxtype\n";
        }
    }
    print ILIST_OUT "INDICATOREND\n";
    close(ILIST_OUT);
    print "Ilist File Generated: $ilist_filename\n";

    print "Generating DataFile\n";
    
    my $unique_gsm_id_ = `date +%N`;
    chomp($unique_gsm_id_);
    $unique_gsm_id_ = int($unique_gsm_id_) + 0;
    my $day_out_ = "";
    open DATAGEN_FILE_HANDLE, ">$datagen_out_";
    my $days_dir = $workdir . "/days_out_$codesjoined";
    if (! (-d $days_dir)) {
        `mkdir -p $days_dir`;
    }
    
    foreach my $tradingdate_ (@datagen_day_vec_) {
            
        $day_out_ = $days_dir . "/datagen_$tradingdate_";
        my $exec_cmd_ = "$BASETRADE_BIN_DIR/datagen $ilist_filename $tradingdate_ CET_900 EST_1500 $unique_gsm_id_ $day_out_ 2000 0 0 0";
        print "$exec_cmd_\n";
        `$exec_cmd_`;

        if ( -e $day_out_ ) {  
            open DAY_FILE_HANDLE, "<$day_out_" or PrintStacktraceAndDie("$0 Could not open $day_out_\n");
            while (my $line = <DAY_FILE_HANDLE>) {
                chomp($line);
                print DATAGEN_FILE_HANDLE "$tradingdate_ $line\n";
            }
            close(DAY_FILE_HANDLE);

        #$exec_cmd_ = "cat $day_out_ >> $datagen_out_";
        #`$exec_cmd_`;
        #`rm -f $this_day_reg_data_filename_`;
        }
        else {
            print "No datagen output for $tradingdate_\n";
        }
    }
    close(DATAGEN_FILE_HANDLE);
}

my @stdevs = ();
my @n2d = ();

{
    my $sd_temp = "";
    my $n2d_temp = "";
    foreach my $prodno (0 .. $#prodcodes) {
        $sd_temp = `cat /spare/local/tradeinfo/PCAInfo/shortcode_stdev_DEFAULT.txt | grep $prodcodes[$prodno] | cut -d' ' -f3`;
        chomp($sd_temp);
        push(@stdevs, $sd_temp);

        $n2d_temp = `~/LiveExec/bin/get_numbers_to_dollars $prodcodes[$prodno]  $end_date`;
        chomp($n2d_temp);
        push(@n2d, $n2d_temp);
    }
}

{
    my $Rconfig = "";
    foreach my $trail (1 .. 30) {
        $Rconfig = $datagen_out_."_config".$trail.".R";
        if (!(-e $Rconfig)) {
            last;
        }
    }
    print "Rconfig File: $Rconfig\n";
    open RCONFIG, ">$Rconfig";

    print RCONFIG "datF <- \"$datagen_out_\";\n";
    print RCONFIG "codes <- c(\"$prodcodes[0]\"";
    foreach my $pno (1 .. $#prodcodes) {
        print RCONFIG ", \"$prodcodes[$pno]\"";
    }
    print RCONFIG ");\n";

    print RCONFIG "num2dollars <- c($n2d[0]";
    foreach my $n2dno (1 .. $#n2d) {
        print RCONFIG ", $n2d[$n2dno]";
    }
    print RCONFIG ");\n";

    print RCONFIG "stdevs_pxdiff <- c($stdevs[0]";
    foreach my $sdno (1 .. $#stdevs) {
        print RCONFIG ", $stdevs[$sdno]";
    }
    print RCONFIG ");\n";

    print RCONFIG "close_EOD <- $closeEOD;\n";
    print RCONFIG "aggress <- $aggress;\n";
    print RCONFIG "horizon_msecs <- $hor_msecs;\n";
    print RCONFIG "ndays <- $ndays;\n";

    close(RCONFIG);
}


exit(0);

