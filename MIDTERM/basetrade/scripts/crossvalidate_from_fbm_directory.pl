#!/usr/bin/perl
use strict;
use warnings;
use feature "switch";
use FileHandle;
use POSIX;
use List::Util qw/max min/; # for max
use File::Basename; # basename
use Term::ANSIColor;
use Math::Complex ; # sqrt
require "/home/dvctrader/basetrade_install/GenPerlLib/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
require "/home/dvctrader/basetrade_install/GenPerlLib/get_iso_date_from_str_min1.pl"; # GetIsoDateFromStrMin1
require "/home/dvctrader/basetrade_install/GenPerlLib/skip_weird_date.pl"; # SkipWeirdDate
require "/home/dvctrader/basetrade_install/GenPerlLib/valid_date.pl"; # ValidDate
require "/home/dvctrader/basetrade_install/GenPerlLib/no_data_date.pl"; # NoDataDate
require "/home/dvctrader/basetrade_install/GenPerlLib/calc_next_date.pl"; # CalcNextDate
require "/home/dvctrader/basetrade_install/GenPerlLib/calc_prev_date.pl"; # CalcPrevDate
require "/home/dvctrader/basetrade_install/GenPerlLib/calc_prev_date_mult.pl"; # CalcPrevDateMult
require "/home/dvctrader/basetrade_install/GenPerlLib/exists_with_size.pl"; # ExistsWithSize
require "/home/dvctrader/basetrade_install/GenPerlLib/get_unique_sim_id_from_cat_file.pl";
my $unique_dir_ = `date +%N`; chomp ( $unique_dir_ ); $unique_dir_ = int($unique_dir_) + 0;
my $work_dir_ = "/spare/local/temp_results/".$unique_dir_;
if ( -d $work_dir_ ) { `rm -rf $work_dir_`; }
printf("Directory : %s\n",$work_dir_);
`mkdir -p $work_dir_ `; #make_work_dir
my $SAVE_TRADELOG_FILE=1;
my $sort_algo_ = "kCNAPnlAverage";
if ( $#ARGV < 3 ) { print "More arguments needed\n"; exit ( 0 ); }
print "$ARGV[0]\n";
my $basedir=$ARGV [ 0 ];
$sort_algo_ = $ARGV[1];
my $last_trading_date_ = GetIsoDateFromStrMin1 ( $ARGV[2] ) ;
chomp($basedir);
my @file_list_ = ();
my @model_file_list_ = ();
@file_list_ = `ls $basedir/stdoutfile*`;
#@strat_file_list_ = `ls $basedir/opt_model*`;
my $final_combined_strat_=$work_dir_."/final_strat_";
open OUTSTRAT, "> $final_combined_strat_";
my $base_shortcode_ = $ARGV[3];
my $local_results_base_dir = $work_dir_."/local_results_base_dir";
my %models_list_map_ = ( );
my $t_models_list_ = $work_dir_."/"."tmp_models_list_";
open OUTMODELLIST, "> $t_models_list_";
my @stats_vec_ = ();
my @volume_vec_ = ();
my @ttc_vec_ = ();
my $start_date_ = "";
my $end_date_ = "";
my @date_vec_ = ();
my @temp_date_vec_ = ();
my $tradingdate_ = $last_trading_date_;
my $t_num_days_ = 100;
my %date_to_selected_map_ = ();
    for ( my $t_day_index_ = 0; $t_day_index_ < $t_num_days_; $t_day_index_ ++ )
    {
        if ( SkipWeirdDate ( $tradingdate_ ) ||
             ( NoDataDateForShortcode ( $tradingdate_ , $base_shortcode_ ) ) ||
             ( IsDateHoliday ( $tradingdate_ ) || ( ( $base_shortcode_ ) && ( IsProductHoliday ( $tradingdate_, $base_shortcode_ ) ) ) ) )
        {
            $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
            $t_day_index_ --;
            next;
        }
        push ( @temp_date_vec_, $tradingdate_ );
	$date_to_selected_map_{$tradingdate_} = 0;
        $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1 );
    }
my $date_count_=0;
while(1){
my $unique_num_ = `date +%N`; chomp ( $unique_num_ ); $unique_num_ = (int($unique_num_) + 0) % $t_num_days_;
if($date_to_selected_map_{$temp_date_vec_[$unique_num_]} == 0 ){
push (@date_vec_,$temp_date_vec_[$unique_num_]);
$date_to_selected_map_{$temp_date_vec_[$unique_num_]} = 1;
$date_count_++;
if ($date_count_==1) {$start_date_= $temp_date_vec_[$unique_num_]; $end_date_=$temp_date_vec_[$unique_num_]; }
else { if ($temp_date_vec_[$unique_num_] > $end_date_ ) {$end_date_ = $temp_date_vec_[$unique_num_];}
if ($temp_date_vec_[$unique_num_] < $start_date_ ) { $start_date_ = $temp_date_vec_[$unique_num_];}
}
}

if($date_count_==1) {last;}

}
printf ( "Num days = %d\n%s\nStart date = %d\tEnd date = %d\n", ( 1 + $#date_vec_ ), join ( ' ', @date_vec_ ), $start_date_, $end_date_ ) ;

for (my $index_=0, my $progid_=1; $index_<=$#file_list_; $index_ ++ )
{

my $OUTFILE= $file_list_[$index_];
chomp($OUTFILE);
#printf ("%s\n",$OUTFILE);


open OUT_FILE, "< $OUTFILE";
my $t_line_ = 1;
my $DIR = "NoDirectory";
while ( my $line_ = <OUT_FILE> )
    {
	if($t_line_==4)
	{
	chomp($line_);
	my @words_ = split ' ', $line_;
	if($#words_ >=2 ){
		$DIR = $words_[2];
	}
	last;
	}
	$t_line_++;
    }
if($DIR eq "NoDirectory"){next;}
my $MAINLOGFILE="$DIR/main_log_file.txt";
#printf ("%s\n",$MAINLOGFILE);
my @log_file_lines_=();
@log_file_lines_ = `cat $MAINLOGFILE`;
my $orig_strat_ = "Invalid";
my $opt_model_ = "None";
=pod
while ( my $r_line_=0; $r_line )
{
if ($r_line_==1)
{
chomp($line_);
my @words_ = split ' ', $line_;
if($#words_ >=1 ){
$orig_strat_ = $words_[1];
}
$r_line_++;

}
}
=cut
if ($#log_file_lines_ > 0 ){
my $log_line_ = $log_file_lines_[0];
chomp($log_line_);
my @words_ = split ' ', $log_line_;
if($#words_ >=1 ){
$orig_strat_ = $words_[1];
}
$log_line_ = $log_file_lines_[$#log_file_lines_];
chomp($log_line_);
@words_ = split ' ', $log_line_;
if($#words_ >=1 ){
$opt_model_ = $words_[2];
}
}
if ($orig_strat_ eq "Invalid" || $opt_model_ eq "None") {next;}
my $base_pbat_dat_ = " ";
my $base_model_file_ = " ";
my $base_param_file_ = " ";
my $base_start_time_ = " ";
my $base_end_time_ = " ";
my $base_prog_id_=" ";
    my @strat_line_ = `cat $orig_strat_`;
    if ($#strat_line_<0) {next;}
    my @strat_words_ = split ' ', $strat_line_[0];

    #$base_shortcode_ = $strat_words_[1];
    $base_pbat_dat_ = $strat_words_[2];
    $base_model_file_ = $strat_words_[3];
    $base_param_file_ = $strat_words_[4];
    $base_start_time_ = $strat_words_[5];
    $base_end_time_ = $strat_words_[6];
    $base_prog_id_ = $strat_words_[7];
my $strat_pre_text_ = "STRATEGYLINE ".$base_shortcode_." ".$base_pbat_dat_." ";
my $strat_post_text_ = " ".$base_param_file_." ".$base_start_time_." ".$base_end_time_." ";
my $t_strat_text_ = $strat_pre_text_.$base_model_file_.$strat_post_text_." ".$progid_."\n";
#printf("%s\n%s\n", $base_model_file_,$opt_model_);
my @model_words_ = split('/',$base_model_file_);
print OUTMODELLIST $model_words_[$#model_words_]."\n";
push (@{$models_list_map_{$model_words_[$#model_words_]}},  $progid_-1);
printf("%s %s\n",$model_words_[$#model_words_],$#@{$models_list_map_{$model_words_[$#model_words_]}};
push(@stats_vec_,0);
push(@volume_vec_,0);
push(@ttc_vec_,0);
$progid_++;
print OUTSTRAT $t_strat_text_;
$t_strat_text_ = $strat_pre_text_.$opt_model_.$strat_post_text_." ".$progid_."\n";
@model_words_ = split('/',$opt_model_);
print OUTMODELLIST $model_words_[$#model_words_]."\n";
push (@{$models_list_map_{$model_words_[$#model_words_]}}, $progid_-1);
push(@stats_vec_,0);
push(@volume_vec_,0);
push(@ttc_vec_,0);
$progid_++;
print OUTSTRAT $t_strat_text_;


}
close OUTMODELLIST;
close OUTSTRAT;
my @nonzero_tradingdate_vec_ = ();
my $key= "";
foreach $key (%models_list_map_)
{
printf("%s %s\n", $key, join(' ',@{$models_list_map_{$key}}));
exit;
}

for ( my $t_day_index_ = 0; $t_day_index_ <= $#date_vec_ ; $t_day_index_ ++ )
    {
my $tradingdate_ = $date_vec_[$t_day_index_];
my $sim_strat_cerr_file_ = $final_combined_strat_."_cerr";
my $market_model_index_ = GetMarketModelForShortcode ( $base_shortcode_ );
my $unique_gsm_id_ = `date +%N`; chomp ( $unique_gsm_id_ ); $unique_gsm_id_ = int($unique_gsm_id_) + 0;
my $exec_cmd_ = "/home/dvctrader/basetrade_install/bin/sim_strategy SIM $final_combined_strat_ $unique_gsm_id_ $tradingdate_ $market_model_index_ ADD_DBG_CODE -1 2>$sim_strat_cerr_file_";
my $t_sim_res_filename_ = $work_dir_."/"."sim_res_".$tradingdate_;
if ( -e $t_sim_res_filename_ ) { `rm -f $t_sim_res_filename_`; }
my @sim_strategy_output_lines_=();
my %unique_id_to_pnlstats_map_ =();
`$exec_cmd_ > $t_sim_res_filename_  `;

if ( ExistsWithSize ( $t_sim_res_filename_ ) )
        {
            push (@nonzero_tradingdate_vec_, $tradingdate_);
            @sim_strategy_output_lines_=`cat $t_sim_res_filename_`;
        }
my $this_tradesfilename_ = "/spare/local/logs/tradelogs/trades.".$tradingdate_.".".int($unique_gsm_id_);
if ( ExistsWithSize ( $this_tradesfilename_ ) )
        {
            my $exec_cmd="/home/dvctrader/basetrade_install/ModelScripts/get_pnl_stats_2.pl $this_tradesfilename_";
            my @pnlstats_output_lines_ = `$exec_cmd`;
            for ( my $t_pnlstats_output_lines_index_ = 0 ; $t_pnlstats_output_lines_index_ <= $#pnlstats_output_lines_; $t_pnlstats_output_lines_index_ ++ )
            {
                my @rwords_ = split ( ' ', $pnlstats_output_lines_[$t_pnlstats_output_lines_index_] );
                if( $#rwords_ >= 1 )
                {
                    my $unique_sim_id_ = $rwords_[0];
                    splice ( @rwords_, 0, 1 ); # remove the first word since it is unique_sim_id_
                    $unique_id_to_pnlstats_map_{$unique_sim_id_} = join ( ' ', @rwords_ );
                }
            }
        }
        if ( $SAVE_TRADELOG_FILE == 0 )
        {
        `rm -f $this_tradesfilename_`;
        my $this_tradeslogfilename_ = "/spare/local/logs/tradelogs/log.".$tradingdate_.".".int($unique_gsm_id_);
        `rm -f $this_tradeslogfilename_`;
        }
        my $temp_results_list_file_ = $work_dir_."/temp_results_list_file_".$tradingdate_ ;
        open TRLF, "> $temp_results_list_file_" ;

        for ( my $t_sim_strategy_output_lines_index_ = 0, my $psindex_ = 0; $t_sim_strategy_output_lines_index_ <= $#sim_strategy_output_lines_; $t_sim_strategy_output_lines_index_ ++ )
        {
            if ( $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] =~ /SIMRESULT/ )
            { # SIMRESULT pnl volume sup% bestlevel% agg%
                my @rwords_ = split ( ' ', $sim_strategy_output_lines_[$t_sim_strategy_output_lines_index_] );
                if ( $#rwords_ >= 2 )
                {
                    splice ( @rwords_, 0, 1 ); # remove the first word since it is "SIMRESULT", typically results files just have pnl, volume, etc
                   my $remaining_simresult_line_ = join ( ' ', @rwords_ );
                    if ( ( $rwords_[1] > 0 ) || # volume > 0
                         ( ( $base_shortcode_ =~ /BAX/ ) && ( $rwords_[1] >= 0 ) ) ) # volume >= 0 ... changed to allow 0 since some bax queries did not trade all day
                   {
                        my $unique_sim_id_ = GetUniqueSimIdFromCatFile ( $final_combined_strat_, $psindex_ );
                        if ( ! exists $unique_id_to_pnlstats_map_{$unique_sim_id_} )
                        {
                            $unique_id_to_pnlstats_map_{$unique_sim_id_} = "0 0 0 0 0 0 0 0 0 0 0 0 0";
#                               PrintStacktraceAndDie ( "unique_id_to_pnlstats_map_ missing $unique_sim_id_ for listfile: $temp_results_list_file_ catfile: $temp_strategy_cat_file_ rline: $remaining_simresult_line_\n" );
                       }
                        #printf $main_log_file_handle_ "PRINTING TO TRLF %s %s %s\n",$remaining_simresult_line_, $unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_ ;
                       printf TRLF "%s %s %s\n",$remaining_simresult_line_,$unique_id_to_pnlstats_map_{$unique_sim_id_}, $unique_sim_id_;
                    }
                }
                else
                {
                  printf ( "ERROR: SIMRESULT line has less than 3 words\n" );
		  exit;
                }
                $psindex_ ++;
            }
        }
        close TRLF;
        if ( ExistsWithSize ( $temp_results_list_file_ ) )
        {
            my $exec_cmd="/home/dvctrader/basetrade_install/ModelScripts/add_results_to_local_database_2.pl $final_combined_strat_ $temp_results_list_file_ $tradingdate_ $local_results_base_dir/$base_shortcode_"; # TODO init $local_results_base_dir
            #print $main_log_file_handle_ "$exec_cmd\n";
            my $this_local_results_database_file_ = `$exec_cmd`;
            #push ( @non_unique_results_filevec_, $this_local_results_database_file_ );
            if ($end_date_ eq ""){  $end_date_ = $tradingdate_; }
            $start_date_ = $tradingdate_;
        }

        $tradingdate_ = CalcPrevWorkingDateMult ( $tradingdate_, 1);
    }
my $statistics_result_file_ = $work_dir_."/"."stats_res_file_";
my $exec_cmd="/home/dvctrader/LiveExec/bin/summarize_strategy_results $base_shortcode_ $t_models_list_ $local_results_base_dir $start_date_ $end_date_ INVALIDFILE $sort_algo_ > $statistics_result_file_";
printf("%s\n", $exec_cmd);
`$exec_cmd`;
        if ( ExistsWithSize ($statistics_result_file_) )
        {
            open(FILE, $statistics_result_file_) or die ("Unable to open statistics result file $statistics_result_file_");
            my @t_stats_output_lines = <FILE>;
            close(FILE);
            for (my $j = 0; $j <= $#t_stats_output_lines ; $j ++ )
            {
                my @t_stat_rwords_ = split ( ' ', $t_stats_output_lines[$j]);
                #my @index_list_ = @$models_list_map_{$t_stat_rwords_[0]};
		for (my $index=0; $index<=$#{$models_list_map_{$t_stat_rwords_[1]}}; $index++){
                $stats_vec_[$index]=$t_stat_rwords_[-1];
                $volume_vec_[$index]=$t_stat_rwords_[4];
                $ttc_vec_[$index]=$t_stat_rwords_[9];
            }
	}
	}

for ( my $i = 0 ; $i <= $#stats_vec_ ; $i ++ )
        {
           printf ( "%d\t%.2f\t%d\t%d\n", $i, $stats_vec_[$i], $volume_vec_[$i], $ttc_vec_[$i] ) ;
        }

