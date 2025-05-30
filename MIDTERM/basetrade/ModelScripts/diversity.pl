#!/usr/bin/perl

# \file ModelScripts/diversity.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
#       Suite No 353, Evoma, #14, Bhattarhalli,
#       Old Madras Road, Near Garden City College,
#       KR Puram, Bangalore 560049, India
#       +91 80 4190 3551

use strict;
use warnings;
use Class::Struct;

struct InstDef => {
    shortcode => '$',
    maxinstall => '$',
    exch => '$',
    destserv => '$',
    startid => '$',
    starthhmm => '$',
    endhhmm => '$',
};

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $DVCTRADER_HOME="/home/dvctrader";

my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

my $FIXED_FILE_DIR=$HOME_DIR."/choices";
my $CHOICE_BASE_DIR=$HOME_DIR."/choices";

require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/find_item_from_vec_ref.pl"; # FindItemFromVecRef
require "$GENPERLLIB_DIR/install_strategy_production.pl"; # InstallStrategyProduction
require "$GENPERLLIB_DIR/get_utc_hhmm_str.pl"; # GetUTCHHMMStr

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $yyyymmdd_ = `date +%Y%m%d`; chomp ( $yyyymmdd_ );
if ( $#ARGV < 0 ) 
{ print "USAGE : timeperiod\n"; exit ( 0 ); }
my $timeperiod_ = $ARGV[0];

my @instruments_ = ();

if ( $timeperiod_ eq "EU_MORN_DAY" )
{
    {
	my $t_inst_def_ = InstDef->new();
	$t_inst_def_->shortcode("FGBS_0");
	$t_inst_def_->maxinstall(5);
	$t_inst_def_->exch("EUREX");
	$t_inst_def_->destserv("10.3.3.11");
	$t_inst_def_->startid(2021);
	$t_inst_def_->starthhmm ( GetUTCHHMMStr ( "CET_730", $yyyymmdd_ ) );
	$t_inst_def_->endhhmm ( GetUTCHHMMStr ( "EST_830", $yyyymmdd_ ) );
	push ( @instruments_, $t_inst_def_ );
    }
}
if ( $timeperiod_ eq "US_MORN_DAY" )
{
    # EUREX products 730 to 1430
    {
     	my $t_inst_def_ = InstDef->new();
     	$t_inst_def_->shortcode("FGBS_0");
     	$t_inst_def_->maxinstall(5);
     	$t_inst_def_->exch("EUREX");
     	$t_inst_def_->destserv("10.3.3.11");
     	$t_inst_def_->startid(2011);
     	$t_inst_def_->starthhmm ( GetUTCHHMMStr ( "EST_730", $yyyymmdd_ ) );
     	$t_inst_def_->endhhmm ( GetUTCHHMMStr ( "EST_1430", $yyyymmdd_ ) );
     	push ( @instruments_, $t_inst_def_ );
    }

    # CME produtcs 730 to 1630
    {
	my $t_inst_def_ = InstDef->new();
	$t_inst_def_->shortcode("ZF_0");
	$t_inst_def_->maxinstall(5);
	$t_inst_def_->exch("CME");
	$t_inst_def_->destserv("10.2.3.13");
	$t_inst_def_->startid(12011);
	$t_inst_def_->starthhmm ( GetUTCHHMMStr ( "EST_730", $yyyymmdd_ ) );
	$t_inst_def_->endhhmm ( GetUTCHHMMStr ( "EST_1630", $yyyymmdd_ ) );
	push ( @instruments_, $t_inst_def_ );
    }

    {
	my $t_inst_def_ = InstDef->new();
	$t_inst_def_->shortcode("ZN_0");
	$t_inst_def_->maxinstall(1);
	$t_inst_def_->exch("CME");
	$t_inst_def_->destserv("10.2.3.11");
	$t_inst_def_->startid(13011);
	$t_inst_def_->starthhmm ( GetUTCHHMMStr ( "EST_730", $yyyymmdd_ ) );
	$t_inst_def_->endhhmm ( GetUTCHHMMStr ( "EST_1630", $yyyymmdd_ ) );
	push ( @instruments_, $t_inst_def_ );
    }

}

for ( my $i = 0; $i <= $#instruments_; $i ++ )
{
    my $t_inst_def_ = $instruments_[$i];
    
    my $STARTHH = int ( $t_inst_def_->starthhmm() / 100 ) ;
    my $STARTMM = $t_inst_def_->starthhmm() % 100 ;
    my $ENDHH = int ( $t_inst_def_->endhhmm() / 100 ) ;
    my $ENDMM=$t_inst_def_->endhhmm() % 100 ;
    
    my $EXCH = $t_inst_def_->exch();
    my $DEST_SERV = $t_inst_def_->destserv();
    my $DEST_ID = $t_inst_def_->startid();

    my @install_queue_ = ();

    printf "%s %d %s %s %d %d %d\n", $t_inst_def_->shortcode, $t_inst_def_->maxinstall, $t_inst_def_->exch, $t_inst_def_->destserv, $t_inst_def_->startid, $t_inst_def_->starthhmm, $t_inst_def_->endhhmm ;

    # for each product we write the chosen files in installed 
    # and the crontab lines in crontab.$DEST_SERV
    my $install_list_filename_ = $CHOICE_BASE_DIR."/installed.".$timeperiod_.".".$t_inst_def_->shortcode.".".$yyyymmdd_ ; # installed.US_MORN_DAY.FGBS_0.20131225

    open INSTALL_LIST_FILEHANDLE, "> $install_list_filename_" or PrintStacktraceAndDie ( "Could not open $install_list_filename_ for writing!\n" );

    my $fixed_filename_ = $FIXED_FILE_DIR."/FIXED.".$timeperiod_.".".$t_inst_def_->shortcode;
    if ( ExistsWithSize ( $fixed_filename_ ) )
    { # take upto maxinstall options and write to CHOICE_BASE_DIR
	print STDERR "Processing $fixed_filename_\n";

	open FIXEDFILEHANDLE, "< $fixed_filename_ " or PrintStacktraceAndDie ( "Could not open $fixed_filename_\n" );
	while ( my $inline_ = <FIXEDFILEHANDLE> )
	{
	    chomp ( $inline_ );
	    my @rwords_ = split ( ' ', $inline_ );
	    if ( ( $#rwords_ >= 0 ) &&
		 ( substr ( $rwords_[0], 0, 1) ne '#' ) )
	    {
		my $strat_file_base_ = $rwords_[0];
		my $full_strat_file_path_ = `~/basetrade/scripts/print_strat_from_base.sh $strat_file_base_`; chomp ( $full_strat_file_path_ );
		if ( ExistsWithSize ( $full_strat_file_path_ ) )
		{
		    if ( ! ( FindItemFromVecRef ( $full_strat_file_path_, \@install_queue_ ) ) &&
			 ( $#install_queue_ + 1 < $t_inst_def_->maxinstall ) )
		    { # not already installed and there is space
			push ( @install_queue_, $full_strat_file_path_ );
			printf INSTALL_LIST_FILEHANDLE "%s\n", $strat_file_base_ ;
			if ( $#install_queue_ + 1 == $t_inst_def_->maxinstall )
			{ # no need to read fixed file any more
			    last;
			}
		    }
		}
	    }
	}
	close FIXEDFILEHANDLE;
    }

    my $todays_choice_filename_ = $CHOICE_BASE_DIR."/".$timeperiod_."_d_".$t_inst_def_->shortcode.".".$yyyymmdd_;
    if ( $#install_queue_ + 1 < $t_inst_def_->maxinstall )
    {
	print STDERR "Processing $todays_choice_filename_\n";

	open CHOICEFILEHANDLE, "< $todays_choice_filename_ " or PrintStacktraceAndDie ( "Could not open $todays_choice_filename_\n" );
	while ( my $inline_ = <CHOICEFILEHANDLE> )
	{ # 
	    chomp ( $inline_ );
	    my @rwords_ = split ( ' ', $inline_ );
	    if ( ( $#rwords_ >= 2 ) &&
		 ( substr ( $rwords_[0], 0, 1) ne '#' ) )
	    {
		my $strat_file_base_ = $rwords_[2];
		my $full_strat_file_path_ = `~/basetrade/scripts/print_strat_from_base.sh $strat_file_base_`; chomp ( $full_strat_file_path_ );
		if ( ExistsWithSize ( $full_strat_file_path_ ) )
		{
		    if ( ! ( FindItemFromVecRef ( $full_strat_file_path_, \@install_queue_ ) ) &&
			 ( $#install_queue_ + 1 < $t_inst_def_->maxinstall ) )
		    { # not already installed and there is space
			push ( @install_queue_, $full_strat_file_path_ );
			printf INSTALL_LIST_FILEHANDLE "%s\n", $strat_file_base_ ;
			if ( $#install_queue_ + 1 == $t_inst_def_->maxinstall )
			{ # no need to read fixed file any more
			    last;
			}
		    }
		}
	    }
	}
    }

    for ( my $install_idx = 0 ; $install_idx <= $#install_queue_ ; $install_idx ++ )
    {
	my $full_strat_file_path_ = $install_queue_[$install_idx];
#	my $dest_strat_file_ = join ( " ", $full_strat_file_path_, $DEST_SERV, $DEST_ID, "dvctrader" ); # InstallStrategyProduction ( $full_strat_file_path_, $DEST_SERV, $DEST_ID, "dvctrader" );
	my $dest_strat_file_ = InstallStrategyProduction ( $full_strat_file_path_, $DEST_SERV, $DEST_ID, "dvctrader" );
	print "$STARTMM $STARTHH * * 1-5 $DVCTRADER_HOME/LiveExec/ModelScripts/onload_start_real_trading.sh $EXCH $DEST_ID $dest_strat_file_\n";
        print "$ENDMM $ENDHH * * 1-5 $DVCTRADER_HOME/LiveExec/ModelScripts/stop_real_trading.sh $EXCH $DEST_ID\n";
	$DEST_ID ++;
    }
    close INSTALL_LIST_FILEHANDLE ;
}

#my $dest_strat_file_ = InstallStrategyProduction ( $ARGV[0], $ARGV[1], $ARGV[2], "dvctrader" );
#print "$dest_strat_file_\n";
