#!/usr/bin/env perl
use strict;
use warnings;
use Scalar::Util qw(looks_like_number);

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

sub ProcessPickStratForServer ;

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/find_item_from_vec.pl"; #FindItemFromVec

my $time_=`date +%H%M` ;
my $week_day_ =`date +%w`;

chomp ( $time_) ;
chomp ($week_day_);

#my $usd_3deg_event_=`/home/dvctrader/cvquant_install/basetrade/ModelScripts/isevent.py TODAY USD 3`;
#my $eur_3deg_event_=`/home/dvctrader/cvquant_install/basetrade/ModelScripts/isevent.py TODAY EUR 3`;


my $config_file = "/home/dvctrader/modelling/pick_strats_config/config_info.txt" ;
ProcessPickStratForServer($config_file);

sub ProcessPickStratForServer 
{
    my $instruction_file_ = shift;
    my $instruction_ = "nothing to do.\n";
    open INSTRUCTIONFILEHANDLE, " < $instruction_file_" or PrintStackTrace ( "Could not open file $instruction_file_ for reading.\n");

    while ( my $this_line_ = <INSTRUCTIONFILEHANDLE> ) 
    {
        chomp ( $this_line_ ) ;

        my @instruction_line_words_ = split ( ' ', $this_line_ ) ;
        if ( scalar ( @instruction_line_words_ ) < 4 ) { next ; }
        if ( substr ( $this_line_, 0 , 1) eq '#' ) { next; } 

        $instruction_   = $instruction_line_words_[ 0 ]." ".$instruction_line_words_[ 1 ]." ".$instruction_line_words_[ 2 ]." ".$instruction_line_words_[ 3 ]." 2>>/tmp/pick_strat_err 1>>/tmp/pick_strat_log" ;
#print " Instruction: ".$instruction_."\n";
        my $exec_start_result_ = `~/basetrade/scripts/get_config_field.pl $instruction_line_words_[ 3 ] EXEC_START_HHMM`;
        chomp($exec_start_result_);
        my $date = `date +%Y%m%d`;
        chomp $date;
        my @day_vec_ = (1,2,3,4,5);   
        my $start_time_utc_ = `/home/dvctrader/basetrade_install/bin/get_utc_hhmm_str $exec_start_result_ $date`;

        my $run_start_time_ = GetTimeBeforeTwoHours($start_time_utc_);
        my $run_day = $week_day_;
        if($run_start_time_ > 2200)
        {
            for my $i (0 .. $#day_vec_)
            {

                if($day_vec_[$i]==0)
                {
                    $day_vec_[$i]=6;
                }
                else
                {
                    $day_vec_[$i]=$day_vec_[$i]-1;
                }
            }
        }
        #Check if it is the correct time and day for script to run. The script runs every five minutes so the current time may not match the $run_start_time.  
        if (($run_start_time_ - $time_)>=0 && ($run_start_time_ - $time_)<5 && FindItemFromVec($run_day,@day_vec_)) 
        {
            print "Generating command\n";
            unless ( fork )
            {
                system ( $instruction_ ) ;
                exit ;
            }
        }
    }
    close INSTRUCTIONFILEHANDLE ;

}

sub GetTimeBeforeTwoHours
{
    my $time = shift;
    if($time - "0200" < 0)
    {
        return $time + 2200;
    }
    else
    {
        return $time - 200;
    }
}
