#!/usr/bin/perl

use strict;
use warnings;

my $HOME_DIR = $ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $GET_UTC_HHMM_STR_EXEC_ = "$BIN_DIR/get_utc_hhmm_str" ;
if ( -e "$LIVE_BIN_DIR/get_utc_hhmm_str" )
{
      $GET_UTC_HHMM_STR_EXEC_ = "$LIVE_BIN_DIR/get_utc_hhmm_str" ;
}

require "$GENPERLLIB_DIR/time_utils.pl"; #GetUTCMinutes

sub GetParamListAndL1Norm
{
    my $shc_ = shift;
    my $start_time_ = shift;
    my $end_time_ = shift;
    my $model_filename_ = shift;
    my $strategy_type_ = shift;
    my $param_vec_ref_ = shift;
    my $l1_norm_vec_ref_ = shift;
    @$param_vec_ref_ = ();
    @$l1_norm_vec_ref_ = ();
    my $st_min_ = GetUTCMinutes($start_time_);
    my $et_min_ = GetUTCMinutes($end_time_);
    my $model_type_ = "LINEAR";
    if( -e $model_filename_)
    {
        $model_type_ = `awk 'NR==2{print \$2}' $model_filename_`; chomp($model_type_);
    }
    else
    {
        return;
    }
    my $paramlist_filename_ = "/spare/local/tradeinfo/datageninfo/param_list/param_list_".$shc_;
    if(-e $paramlist_filename_)
    {
        open PARAMLIST, "< $paramlist_filename_" or PrintStackTraceAndDie ("Could not open paramlist_file_ $paramlist_filename_ for reading ");
        while(my $line_ = <PARAMLIST>)
        {
            chomp($line_);
            my @words_ = split(' ',$line_);
            if($#words_ >= 7)
            {
                if(($words_[0] eq $shc_)  && ($words_[3] eq $model_type_) && ($words_[4] eq $strategy_type_))
                {
                    my $this_st_min_  = GetUTCMinutes($words_[1]);
                    my $this_et_min_ = GetUTCMinutes($words_[2]);

                    
                    my $this_param_file_ = $words_[5]; chomp($this_param_file_);
                    my $this_l1_norm_ = $words_[7]; chomp($this_l1_norm_);
                    if((-e $this_param_file_) && ($this_l1_norm_>0) && abs($this_st_min_-$st_min_)<=60 && abs($this_et_min_-$et_min_)<=60)
                    {
                        push(@$param_vec_ref_, $this_param_file_);
                        push(@$l1_norm_vec_ref_, $this_l1_norm_);
                    }
                }
            }
        }
        close PARAMLIST;
    }
}

1
