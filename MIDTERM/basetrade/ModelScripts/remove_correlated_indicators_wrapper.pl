#!/usr/bin/perl

# \file ModelScripts/remove_correlated_indicators_wrapper.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use strict;
#use warnings;
use feature "switch"; # for given, when
use FileHandle;
use Cwd;
use POSIX;

sub OpenNewIListFile ;
sub CloseCurrentIListFile ;

my $HOME_DIR = $ENV { 'HOME' };
my $USER = $ENV { 'USER' };
my $REPO = "basetrade";

my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";
my $BIN_DIR = $HOME_DIR."/".$REPO."_install/bin";
my $GENPERLLIB_DIR = $HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR = $HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie


if ( $USER eq "sghosh" || $USER eq "ravi" )
{
    $LIVE_BIN_DIR = $BIN_DIR;
}

my $USAGE="$0 shortcode=na_shc modelfile cross_correlation_threshold max_num startdate enddate starthhmm endhhmm";

if ( $#ARGV < 7 ) { print $USAGE."\n"; exit ( 0 ); }
my $shortcode_ = $ARGV[0];
my $indicator_list_filename_ = $ARGV[1];
my $cross_correlation_threshold_ = $ARGV[2];
my $max_indicators_ = $ARGV[3];
my $datagen_start_yyyymmdd_ = $ARGV[4] ;
my $datagen_end_yyyymmdd_ = $ARGV[5] ;
my $datagen_start_hhmm_ = $ARGV[6];
my $datagen_end_hhmm_ = $ARGV[7];

my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;

my $exec_ = $MODELSCRIPTS_DIR."/remove_correlated_indicators.pl";
my $this_work_dir_ = "$HOME_DIR/indicatorwork/ilists/";
`mkdir -p $this_work_dir_` ;
chdir ( $this_work_dir_ ) ;
my $main_ilist_filehandle_ = FileHandle->new; # made global to be accessed in the subroutines easily

my $indicator_list_filename_index_ = 0;
my $num_indicators_added_in_current_file_ = -1; # initial condition signifying no ilist file open
my @indicator_list_filename_vec_ = (); # a vector of files in case an individual indicator file becomes too big

my @pre_indicator_strings_=();
my @indicator_strings_=();
my @refined_indicator_strings_=();
my @post_indicator_strings_=();


open (FH, $indicator_list_filename_) or die "Couldn't open location file: $indicator_list_filename_";
my @all_lines= <FH>;
close FH;
my $indicator_reading_ = -1; # pre(-1),at(0),post(1)
foreach my $line_( @all_lines )
{
    chomp ( $line_ );
    $line_ =~ s/^\s+//;
    my @words_ = split(/\s+/, $line_);

    given ( $indicator_reading_ )
    {
	when ( -1 ) 
	{ # pre
	    if ( $words_[0] ne "INDICATOR" )
	    {
		push ( @pre_indicator_strings_, $line_ );
	    }
	    else
	    { # moving to INDICATOR lines
		$indicator_reading_ = 0;
		push ( @indicator_strings_, $line_ );
	    }
	}
	when ( 0 )
	{ # at
	    if ( $words_[0] ne "INDICATOR" )
	    { # moving to post
		$indicator_reading_ = 1;
		push ( @post_indicator_strings_, $line_ );
	    }
	    else
	    { # INDICATOR lines
		push ( @indicator_strings_, $line_ );
	    }
	}
	when ( 1 )
	{ # post
	    push ( @post_indicator_strings_, $line_ );
	}
    }
}


my $MAX_INDICATORS_IN_FILE = 300 ;
my $sub_max_indicators_ = int ( 2 * $max_indicators_ / ceil ( scalar ( @indicator_strings_ ) / $MAX_INDICATORS_IN_FILE ) ) ;

my $cmd_ = "";
if ( scalar ( @indicator_strings_ ) <= $MAX_INDICATORS_IN_FILE )
{
    $cmd_ = $exec_." "
	.$shortcode_." "
	.$indicator_list_filename_." "
	.$cross_correlation_threshold_." "
	.$max_indicators_." "
	.$datagen_start_yyyymmdd_." "
	.$datagen_end_yyyymmdd_." "
	.$datagen_start_hhmm_." "
	.$datagen_end_hhmm_;
    
    print `$cmd_`;
    exit ( 1 ) ;
}


for ( my $ib_idx_ = 0 ; $ib_idx_ <= $#indicator_strings_ ; $ib_idx_ ++ )
{
    if ( $num_indicators_added_in_current_file_ < 0 )
    { 
	OpenNewIListFile ( ); 
    }    

    print $main_ilist_filehandle_ $indicator_strings_[ $ib_idx_ ],"\n" ;
    $num_indicators_added_in_current_file_ ++;
    
    if ( $num_indicators_added_in_current_file_ == $MAX_INDICATORS_IN_FILE )
    { 
	CloseCurrentIListFile ( ); 
    }
}

if ( $num_indicators_added_in_current_file_ >= 0 )
{ 
    CloseCurrentIListFile ( );
}

my @result_ = ( );

push ( @refined_indicator_strings_ , @pre_indicator_strings_ );
for ( my $j = 0 ; $j <= $#indicator_list_filename_vec_ ; $j ++ )
{
    $cmd_ = $exec_." "
	.$shortcode_." "
	.$indicator_list_filename_vec_[$j]." "
	.$cross_correlation_threshold_." "
	.$sub_max_indicators_." "
	.$datagen_start_yyyymmdd_." "
	.$datagen_end_yyyymmdd_." "
	.$datagen_start_hhmm_." "
	.$datagen_end_hhmm_;

    print STDERR $cmd_."\n";
    @result_ = `$cmd_`;
    chomp ( @result_ );
    
    foreach my $line_(@result_)
    {
	chomp ( $line_ );
	$line_ =~ s/^\s+//;
	my @words_ = split(/\s+/, $line_);
	
	if ( $words_[0] eq "INDICATOR" )
	{
	    push ( @refined_indicator_strings_, $line_ );
	}
    }
}
push ( @refined_indicator_strings_ , @post_indicator_strings_ );

# final iteration 
my $new_indicator_list_filename_ = $this_work_dir_."/ilist_".$shortcode_.$unique_gsm_id_."_gil_final".".txt"; 
$main_ilist_filehandle_->open ( "> $new_indicator_list_filename_ " ) or PrintStacktraceAndDie ( "Could not open $new_indicator_list_filename_\n" );
$main_ilist_filehandle_->autoflush(1);
for my $line_ ( @refined_indicator_strings_ ) 
{
    print $main_ilist_filehandle_ $line_ ,"\n" ;
}
close $main_ilist_filehandle_ ;

if ( scalar ( @refined_indicator_strings_ ) > 4 ) 
{
    $cmd_ = $exec_." "
	.$shortcode_." "
	.$new_indicator_list_filename_." "
	.$cross_correlation_threshold_." "
	.$max_indicators_." "
	.$datagen_start_yyyymmdd_." "
	.$datagen_end_yyyymmdd_." "
	.$datagen_start_hhmm_." "
	.$datagen_end_hhmm_;
    
    print `$cmd_`;
}
# end


#**********SUBS**************************************#
sub OpenNewIListFile
{
    # add header and open next file 
    my $new_indicator_list_filename_ = $this_work_dir_."/ilist_".$shortcode_.$unique_gsm_id_."_gil_".$indicator_list_filename_index_.".txt"; 
    $indicator_list_filename_index_ ++; # for next time

    # push file in vec
    push ( @indicator_list_filename_vec_, $new_indicator_list_filename_ ) ;

    $main_ilist_filehandle_->open ( "> $new_indicator_list_filename_ " ) or PrintStacktraceAndDie ( "Could not open $new_indicator_list_filename_\n" );
    $main_ilist_filehandle_->autoflush(1);

    #print $main_ilist_filehandle_ @pre_indicator_strings_."\n";

    for ( my $i = 0 ; $i <= $#pre_indicator_strings_ ; $i ++ )
    {
	print $main_ilist_filehandle_ $pre_indicator_strings_[$i]."\n";
    }

    $num_indicators_added_in_current_file_ = 0;
}

sub CloseCurrentIListFile
{
    if ( $num_indicators_added_in_current_file_ >= 0 )
    { # open
        # add trailer 
	print $main_ilist_filehandle_ "INDICATOREND\n";
	# close current file handle
	close $main_ilist_filehandle_ ;
	$num_indicators_added_in_current_file_ = -1; # indicating closed
    }
}

#************END****************************************#
