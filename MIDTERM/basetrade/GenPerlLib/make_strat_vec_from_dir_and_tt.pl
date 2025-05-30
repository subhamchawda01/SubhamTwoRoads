use strict;
use warnings;
use feature "switch"; # for given, when
use File::Basename; # for basename and dirname

my $HOME_DIR=$ENV{'HOME'};

my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $LIVE_BIN_DIR=$HOME_DIR."/LiveExec/bin";

my $GET_UTC_HHMM_STR_EXEC_ = "$BIN_DIR/get_utc_hhmm_str" ;
if ( -e "$LIVE_BIN_DIR/get_utc_hhmm_str" )
{
    $GET_UTC_HHMM_STR_EXEC_ = "$LIVE_BIN_DIR/get_utc_hhmm_str" ;
}

require "$GENPERLLIB_DIR/make_strat_vec_from_dir.pl"; # MakeStratVecFromDir

sub TimeDiff
{
    my $orig_time_ = shift;
    my $diff = shift;
    my $new_min = (int($orig_time_/100))*60 + ($orig_time_%100) + $diff;
    (int($new_min/60))*100 + $new_min%60;
}
sub MakeStratVecFromDirAndTT
{
    my $dirname_ = shift;
    my $timeperiod_ = shift;

    
    my $allowed_gap_in_tt_ = 60;
    
    my @ret_strat_files_ = ();
    my @words_ = split ( '-', $timeperiod_ ) ;
    if ( $#words_ == 1 )
    {
        my $g_st_str_ = $words_[0];
        my $g_et_str_ = $words_[1];

        my $st_ = `$GET_UTC_HHMM_STR_EXEC_ $g_st_str_`;
        my $et_ = `$GET_UTC_HHMM_STR_EXEC_ $g_et_str_`;
        
        my $st_start_limit_ = TimeDiff($st_ ,-1*$allowed_gap_in_tt_); 
        my $st_end_limit_ = TimeDiff($st_ ,$allowed_gap_in_tt_);
        
        my $et_start_limit_ = TimeDiff($et_ ,-1*$allowed_gap_in_tt_); 
        my $et_end_limit_ = TimeDiff($et_ ,$allowed_gap_in_tt_);
        
        #print "$st_ $st_start_limit_ $st_end_limit_ $et_ $et_start_limit_ $et_end_limit_\n";
        my @abs_file_paths_ = ();
        push @abs_file_paths_, MakeStratVecFromDir ( "$dirname_" );
        
        
        for my $strat_file_ (@abs_file_paths_)
        {
            my $exec_cmd_ = "cat $strat_file_ |  awk '{print \$6, \$7}'";
            #print $exec_cmd_."\n";
            my $output_line_ = `$exec_cmd_`;
            my @tt_ = split ( ' ', $output_line_ ) ;
            if($#tt_ == 1)
            {
                my $t_g_st_str_ = $tt_[0];
                my $t_g_et_str_ = $tt_[1];
                
                my $t_st_str_ = `$GET_UTC_HHMM_STR_EXEC_ $t_g_st_str_`;
                my $t_et_str_ = `$GET_UTC_HHMM_STR_EXEC_ $t_g_et_str_`;
                
                if($t_st_str_ >= $st_start_limit_ && $t_st_str_ <= $st_end_limit_ && $t_et_str_ >= $et_start_limit_ && $t_et_str_ <= $et_end_limit_)
                {
                    push @ret_strat_files_,  $strat_file_;
                }
            }
        }
        #print "@ret_strat_files_";
    }
    @ret_strat_files_;
}

#&MakeStratVecFromDirAndTT($ARGV[0], $ARGV[1]);
1
