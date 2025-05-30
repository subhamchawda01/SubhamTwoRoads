#!/usr/bin/perl
use strict;
use warnings;
use File::Basename;
use List::Util qw/max min/; # for max
use List::Util qw/max min/; # for min

sub FetchSyncedParamsAndModels;
sub ModifyCommonParamFile ;

my $USER = $ENV { 'USER' };
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $BIN_DIR=$HOME_DIR."/".$REPO."_install/bin";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

$BIN_DIR=$HOME_DIR."/"."LiveExec/bin";

require "$GENPERLLIB_DIR/get_iso_date_from_str.pl"; # GetIsoDateFromStr
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/get_trading_location_for_shortcode.pl"; # GetTradingLocationForShortcode

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

require "$GENPERLLIB_DIR/get_query_type_for_id_date.pl"; # GetQueryTypeForIdDate
require "$GENPERLLIB_DIR/get_unix_time_from_utc.pl"; #GetUnixtimeFromUTC
require "$GENPERLLIB_DIR/strat_utils.pl"; #CheckIfRegimeParam
my $print_strat_exec_cmd = 0;
my $usage_ = "run_accurate_sim_real.pl DATE UNIQUE_ID [REMOVE_STRAT_FILES=1] [OVERRIDE_SACI_STR=D] [STRAT_FILE_DIR] [PARAM_FILE_DIR] [MODEL_FILE_DIR]";

if ($#ARGV < 1) {
  print $usage_."\n";
  exit (0);
}

my $yyyymmdd_ = GetIsoDateFromStr ( $ARGV[0] ) ;
my ( $yyyy_, $mm_, $dd_ ) = BreakDateYYYYMMDD ( $yyyymmdd_ );

my $unique_id_ = $ARGV[1]; chomp ($unique_id_);

my $to_remove_strat_files_ = 1;
my $saci_str_ = "D";
my $t_strat_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_param_file_dir_ = $HOME_DIR."/"."querytemp";
my $t_model_file_dir_ = $HOME_DIR."/"."querytemp";
if ( $#ARGV > 1 )
{
  $to_remove_strat_files_ = $ARGV [ 2 ];
  if ( $#ARGV > 2 ) { $saci_str_ = $ARGV [ 3 ]; }
  if ( $#ARGV > 3 ) { $t_strat_file_dir_ = $ARGV [ 3 ]; }
  if ( $#ARGV > 4 ) { $t_param_file_dir_ = $ARGV [ 4 ]; }
  if ( $#ARGV > 5 ) { $t_model_file_dir_ = $ARGV [ 5 ]; }

  `mkdir -p $t_strat_file_dir_`;
  `mkdir -p $t_param_file_dir_`;
  `mkdir -p $t_model_file_dir_`;
}


my $query_trades_dir_ = "/NAS1/logs/QueryTrades/".$yyyy_."/".$mm_."/".$dd_;
my $trades_file_name_ = $query_trades_dir_."/"."trades.".$yyyymmdd_.".".$unique_id_;

if ( ! ExistsWithSize ( $trades_file_name_ ) )
{
  exit(0);
}

my $real_query_log_dir_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_;
# Copy possibly gzipped log file to a temp dir.
my $t_query_log_dir_ = $HOME_DIR."/"."querytemp";

`mkdir -p $t_query_log_dir_`;

my $t_=`date +%N |cut -c6-`;chomp($t_);
my $sim_id_ = "$unique_id_"."$t_";
$sim_id_ = $sim_id_ + 0;
while ( $sim_id_ > 2147483647 )
{
  $sim_id_ = int( $sim_id_/10);
}

my $t_query_file_name_ = $t_query_log_dir_."/"."log.".$yyyymmdd_.".".$unique_id_;

{ # Make a local copy of the Query Log from real.

# First try non .gz log.
  my $query_file_name_ = $real_query_log_dir_."/"."log.".$yyyymmdd_.".".$unique_id_;
  print $query_file_name_."\n";
  if (! -e $query_file_name_) {
# Try the .gz version.
    $query_file_name_ = $query_file_name_.".gz";

    if (! -e $query_file_name_) {
      print "File $query_file_name_ doesnot exist\n";
      exit (0);
    } else {
      `cp $query_file_name_ $t_query_log_dir_`;
# Gunzip this .gz log file.
      my $t_gz_query_file_name_ = $t_query_file_name_.".gz";
      `gunzip $t_gz_query_file_name_`;
    }
  } else {
    `cp $query_file_name_ $t_query_log_dir_`;
  }
}

my $sim_strat_file_name_ = "";
my $dep_shortcode_ = "";

my $t_model_file_name_ = "";
my $t_param_file_name_ = "";
my $sim_start_time_sec_ = "";
my %query_id_to_sim_start_time_ = ();
my %query_id_to_real_saci_map_ = ();
my $first_build_index_timestamp_ = 0;

my %prod_to_strat_line_ = () ;
my $remote_login_ = "";
my @to_remove_files_ = () ;

{ # Generate local strategy, model & param file.
# Get the model & param file names from the log.
  open QUERY_LOG_FILE_HANDLE, "< $t_query_file_name_" or PrintStacktraceAndDie ( "Could not open $t_query_file_name_\n" );
  my @query_log_file_lines_ = <QUERY_LOG_FILE_HANDLE>;
  close QUERY_LOG_FILE_HANDLE;
  `rm -rf $t_query_file_name_`;

# Find the time of first call to StartTrading
  my $unix_time_script_ = $SCRIPTS_DIR."/unixtime2gmtstr.pl";

  my $sim_start_time_ = "";
  for ( my $line_ = 0; $line_ <= $#query_log_file_lines_; $line_++ )
  {  
    my @strategy_info_words_ = split (' ', $query_log_file_lines_[$line_]) ;
    if ( index ( $query_log_file_lines_[$line_], "StartTrading Called" ) > 0 )
    {
# First field is the timestamp.
      if(exists($query_id_to_sim_start_time_{$strategy_info_words_[4]}))	{next;}
      $sim_start_time_ = $strategy_info_words_[0]; chomp ($sim_start_time_) ;
      if(! $sim_start_time_sec_)	{$sim_start_time_sec_ = $sim_start_time_;}
# Convert the sec.usec time to UTC.
      $sim_start_time_ = `$unix_time_script_ $sim_start_time_`;

# "Mon Jan 16 08:01:47 2012" => "08:01:47"
      my @start_time_words_ = split (' ', $sim_start_time_) ;
      $sim_start_time_ = $start_time_words_[3]; chomp ($sim_start_time_) ;

# "08:01:47" => "0801"
      @start_time_words_ = split (':', $sim_start_time_) ;
      $sim_start_time_ = $start_time_words_[0].$start_time_words_[1].$start_time_words_[2]; chomp ($sim_start_time_) ;
      $query_id_to_sim_start_time_{$strategy_info_words_[4]}=$sim_start_time_;
#last;
    }
    elsif ( index ( $query_log_file_lines_[$line_], "BuildIndex" ) > 0  )
    {
      if ( not $first_build_index_timestamp_ )
      {
        $first_build_index_timestamp_ = $strategy_info_words_[0]; chomp ($first_build_index_timestamp_);
      }
    }
    elsif ( index ( $query_log_file_lines_[$line_], "STRUCTURED_STRATEGYLINE" ) >= 0  )
    {
#use first BuildIndex timestamp , SACI got ready : after last STRATEGYLINE, to help in case query restarts in live
      $first_build_index_timestamp_ = 0;
      %query_id_to_real_saci_map_ = ();
    }  
    elsif ( $#strategy_info_words_ >=3 && index ( $query_log_file_lines_[$line_], "SACI" ) > 0 && index ( $query_log_file_lines_[$line_], "got ready" ) > 0 )
    {
      my $t_queryid_ = -1;
      if ( index ( $query_log_file_lines_[$line_], "queryid" ) > 0  )
      {
        $t_queryid_ = $strategy_info_words_[-1];
      }
      $query_id_to_real_saci_map_{$t_queryid_} = $strategy_info_words_[3];
    } 
  }

  my $sim_end_time_ = -1;
  my $take_end_from_strat_ = 0;
  if ( $take_end_from_strat_ > 0 )
  {
    for ( my $line_ = $#query_log_file_lines_ ; $line_ >= 0 ; $line_ -- )
    {
      if (
          ( index ( $query_log_file_lines_ [ $line_ ] , "getflat_due_to_external_getflat_" ) >= 0 ) ||
          ( index ( $query_log_file_lines_ [ $line_ ] , "getflat_due_to_max_loss_" ) >=0 ) ||
          ( index ( $query_log_file_lines_ [ $line_ ] , "getflat_due_to_close_" ) >=0 )
         )
      {
        print $query_log_file_lines_ [ $line_ ]."\n";
        $sim_end_time_ = ( split ( ' ' , $query_log_file_lines_ [ $line_ ] ) ) [ 0 ];

# Convert the sec.usec time to UTC.
        $sim_end_time_ = `$unix_time_script_ $sim_end_time_`;

# "Mon Jan 16 08:01:47 2012" => "08:01:47"
        my @end_time_words_ = split ( ' ' , $sim_end_time_ );
        $sim_end_time_ = $end_time_words_ [ 3 ]; chomp ( $sim_end_time_ );

# "08:01:47" => "0801"
        @end_time_words_ = split ( ':' , $sim_end_time_ );
        $sim_end_time_ = $end_time_words_ [ 0 ].$end_time_words_ [ 1 ].$end_time_words_[2]; chomp ( $sim_end_time_ );

        last;
      }
    }
  }


  my %has_processed_strat=();
  my @all_strat_files_ = ();

  my $strategy_line_ = 0;
  my @strategy_lines_= () ;
  my $this_strat_cat_string_ = "";

  for ( $strategy_line_ = $#query_log_file_lines_;$strategy_line_ >= 0; $strategy_line_-- ) 
  {
  	if ( index( $query_log_file_lines_[$strategy_line_], "STRATEGYLINE" ) == 0 )
  	{
  		push ( @strategy_lines_, $query_log_file_lines_[$strategy_line_] );
  	}
  	
  	if ( index ( $query_log_file_lines_[$strategy_line_], "STRUCTURED_TRADING") == 0 )
  	{
  		my @strategy_info_words_ = split (' ', $query_log_file_lines_[$strategy_line_]);
  		if ( $#strategy_info_words_ < 6 ) { exit; }
  		
  		if ( $has_processed_strat{ join(' ', @strategy_info_words_[1..6]) } )  { next; }
  		else  { $has_processed_strat{ join(' ', @strategy_info_words_[1..6]) }  = 1; }
  		
  		$dep_shortcode_ = $strategy_info_words_[1]; chomp ( $dep_shortcode_);
  		my $trading_location_ = GetTradingLocationForShortcode ( $dep_shortcode_ , $yyyymmdd_ );
  		$remote_login_ = "dvctrader\@".$trading_location_;
  		my $this_common_paramfilename_ = $strategy_info_words_[3]; chomp ( $this_common_paramfilename_);
  		my $t_common_paramfile_basename_ = basename ( $this_common_paramfilename_); chomp ( $t_common_paramfile_basename_);
  		my $t_common_paramfile_name_ = $t_param_file_dir_."/".$t_common_paramfile_basename_ ;
  		my $real_common_param_filename_ = $real_query_log_dir_."/".$t_common_paramfile_basename_ ;
  		if ( -e $real_common_param_filename_ ) 
  		{
  			`cp $real_common_param_filename_ $t_common_paramfile_name_`;
  		}
  		else
  		{
  			#print "rsync -avz $remote_login_:$this_common_paramfilename_ $t_param_file_dir_\n";
  			`rsync -avz $remote_login_:$this_common_paramfilename_ $t_param_file_dir_`;
  		}
  		
  		ModifyCommonParamFile($t_common_paramfile_name_) ;
  		push ( @to_remove_files_, $t_common_paramfile_name_ );
  		
  		my $UTC_TIME_EXEC = $BIN_DIR."/get_utc_hhmmss_str";
  		my $t_strat_start_time_ = $strategy_info_words_[4];
        $t_strat_start_time_ = `$UTC_TIME_EXEC $t_strat_start_time_ $yyyymmdd_`;
        $t_strat_start_time_ = max ( $sim_start_time_
            , $t_strat_start_time_ ) ;
        my $utc_time_ = GetUnixtimeFromUTC($yyyymmdd_, $t_strat_start_time_);
        
        if ( (not $take_end_from_strat_) ||  $sim_end_time_ == -1 )
        {
        	$sim_end_time_ = $strategy_info_words_ [ 5 ];
        }
        else
        { # End time is min ( Last_call_to_external_getflat , stratfile_end_time )
        	my $t_strat_end_time_ = $strategy_info_words_ [ 5 ];
        	$t_strat_end_time_ = `$UTC_TIME_EXEC $t_strat_end_time_ $yyyymmdd_`;
        	$sim_end_time_ = min ( $sim_end_time_, $t_strat_end_time_);
        }
      
      $this_strat_cat_string_ = $this_strat_cat_string_."$strategy_info_words_[0] $dep_shortcode_ $strategy_info_words_[2] $t_common_paramfile_name_ $t_strat_start_time_ $sim_end_time_ $strategy_info_words_[6]\n";
      
      for ( my $idx_ = $#strategy_lines_ ; $idx_ >= 0; $idx_--)
      {
      	my $stratline_ = $strategy_lines_[$idx_];
      	my @stratline_words_ = split ( " ", $stratline_ );      	
      	if ( $#stratline_words_ < 3 ) { next ; }
  		else
  		{
  			my $prod_name_ = $stratline_words_[1];
  			my $this_model_name_ =  $stratline_words_[2];
  			my $this_param_name_ = $stratline_words_[3];
  			my $t_param_file_basename_ = basename ( $this_param_name_); chomp ( $t_param_file_basename_ );
  			my $t_model_file_basename_ = basename ( $this_model_name_); chomp ( $t_model_file_basename_);
  			my $t_param_file_name_ = $t_param_file_dir_."/".$t_param_file_basename_;
  			my $t_model_file_name_ = $t_model_file_dir_."/".$t_model_file_basename_ ;
  				
  			my $real_param_filename_ = $real_query_log_dir_."/".$t_param_file_basename_;
  			if ( -e $real_param_filename_ )
  			{
  				`cp $real_param_filename_ $t_param_file_name_`;
  			}
  			else
  			{
  				`rsync -avz $remote_login_:$this_param_name_ $t_param_file_dir_`;
  			}
  			
  			my $real_model_filename_ = $real_query_log_dir_."/".$t_model_file_basename_;
  			if ( -e $real_model_filename_ )
  			{
  				`cp $real_model_filename_ $t_model_file_name_`;
  			}
  			else
  			{
  				`rsync -avz $remote_login_:$this_model_name_ $t_model_file_dir_`;
  			}
  			push ( @to_remove_files_, $t_model_file_name_);
  			push ( @to_remove_files_, $t_param_file_name_);
  			
  			$prod_to_strat_line_{$prod_name_} = $stratline_;
  			$this_strat_cat_string_ = $this_strat_cat_string_."$stratline_words_[0] $prod_name_ $t_model_file_name_ $t_param_file_name_\n";
  		}
      }
  		
  	}
  	
    if ( index ( $query_log_file_lines_[$strategy_line_], "STRUCTURED_STRATEGYLINE" ) >= 0 ) 
    {
    	my @strategy_info_words_ = split ( " ", $query_log_file_lines_[$strategy_line_] );
    	if( $#strategy_info_words_ < 2 ) 
    	{ 
    		$this_strat_cat_string_ =  "";
    		@strategy_lines_ = () ;
    		next ; 
    	}
    	else
    	{
    		my $strat_im_filename_ = $strategy_info_words_[1];
    		my $strat_im_file_basename_ = basename ( $strat_im_filename_ ); chomp ( $strat_im_file_basename_);
    		my $t_strat_im_filename_ = $t_strat_file_dir_."/".$strat_im_file_basename_;
    		open IM_STRAT_FILE_HANDLE , "> $t_strat_im_filename_" or PrintStackTraceAndDir ( "Could not create strat_im file $t_strat_im_filename_\n");
    		print IM_STRAT_FILE_HANDLE $this_strat_cat_string_ ;
    		close IM_STRAT_FILE_HANDLE ;
    		$sim_strat_file_name_ = "$t_strat_file_dir_/$strat_im_file_basename_"."_"."$unique_id_";
    		
    		open SIM_STRAT_FILE_HANDLE, "> $sim_strat_file_name_" or PrintStacktraceAndDie ( "Could not create file $sim_strat_file_name_\n" );
    		print SIM_STRAT_FILE_HANDLE "STRUCTURED_STRATEGYLINE $t_strat_im_filename_ $unique_id_\n";
    		close SIM_STRAT_FILE_HANDLE;
    		push ( @to_remove_files_, $t_strat_im_filename_ );
    		push ( @to_remove_files_, $sim_strat_file_name_ );
    	}
    	
    	last; # assuming just one strat
    }
  }
}


{ # Run SIM Strategy and get SIM pnl & vol.
  my $sim_exec_=$BIN_DIR."/sim_strategy";
  require "$GENPERLLIB_DIR/get_market_model_for_shortcode.pl"; # GetMarketModelForShortcode
  my $mkt_model_ = 1; 
  my $exec_cmd="$sim_exec_ SIM $sim_strat_file_name_ $sim_id_ $yyyymmdd_ $mkt_model_ $saci_str_ $first_build_index_timestamp_ 0 ADD_DBG_CODE -1";
  print "$exec_cmd\n";
  my $sim_output_ = `$exec_cmd`;
  print $sim_output_;
}

if ( ! ( -e $trades_file_name_ ) )
{
  exit ;
}
{ # Find real pnl & vol.
  my $t_trades_file_name_ = $t_query_log_dir_."/"."trades.".$yyyymmdd_.".".$unique_id_;
  `cp $trades_file_name_ $t_query_log_dir_`;

  open TRADES_LOG_FILE_HANDLE, "< $t_trades_file_name_" or PrintStacktraceAndDie ( "Could not open $t_trades_file_name_\n" );
  my @trades_file_lines_ = <TRADES_LOG_FILE_HANDLE>;
  my %real_pnl_per_query_=() ;
  close TRADES_LOG_FILE_HANDLE;
  `rm -rf $t_trades_file_name_`;
  
  my $total_pnl_ = 0 ;
  for (my $trades_ = 0; $trades_ <= $#trades_file_lines_; $trades_++)
  {
    my @trades_words_ = split (' ', $trades_file_lines_[$trades_]);

    if ($#trades_words_ >= 15)
    {
      my $t_pnl_ = $trades_words_[8]; chomp ($t_pnl_);
      $total_pnl_ = $trades_words_[17];
      my $trade_size_ = $trades_words_[4]; chomp ($trade_size_);
      if ( ! exists $real_pnl_per_query_{$trades_words_[2]} ) {
        $real_pnl_per_query_{$trades_words_[2]} = [0, 0];
      }
      $real_pnl_per_query_{$trades_words_[2]}[1] += $trade_size_;
      $real_pnl_per_query_{$trades_words_[2]}[0] =  $t_pnl_;
    }
  }
  foreach my $sh(keys %real_pnl_per_query_ ) {
    print "$sh REALRESULT @{$real_pnl_per_query_{$sh}}\n";
  }
  print "$dep_shortcode_ REALRESULT $total_pnl_\n";
  `cp /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ $t_trades_file_name_`;

  open TRADES_LOG_FILE_HANDLE, "< $t_trades_file_name_" or PrintStacktraceAndDie ( "Could not open $t_trades_file_name_\n" );
  @trades_file_lines_ = <TRADES_LOG_FILE_HANDLE>;
  %real_pnl_per_query_=() ;
  close TRADES_LOG_FILE_HANDLE;
  `rm -rf $t_trades_file_name_`;

  for (my $trades_ = 0; $trades_ <= $#trades_file_lines_; $trades_++)
  {
    my @trades_words_ = split (' ', $trades_file_lines_[$trades_]);

    if ($#trades_words_ >= 15 && ( ($trades_words_[3] eq "B") || ($trades_words_[3] eq "S") ) )
    {
      my $t_pnl_ = $trades_words_[8]; chomp ($t_pnl_);
      $total_pnl_ = $trades_words_[17] ;
      my $trade_size_ = $trades_words_[4]; chomp ($trade_size_);
      if ( ! exists $real_pnl_per_query_{$trades_words_[2]} ) {
        $real_pnl_per_query_{$trades_words_[2]} = [0, 0];
      }
      $real_pnl_per_query_{$trades_words_[2]}[1] += $trade_size_;
      $real_pnl_per_query_{$trades_words_[2]}[0] =  $t_pnl_;
    }
  }
  foreach my $sh(keys %real_pnl_per_query_ ) {
    print "$sh SIMRESULT @{$real_pnl_per_query_{$sh}}\n";
  }
  print "$dep_shortcode_ SIMRESULT $total_pnl_\n";
  
}

{
  my $id_ = `head -n1 $trades_file_name_ | cut -d" " -f3 | cut -d"." -f2`; chomp($id_);
  my $temp_trade_file_ = "/spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_"."_tmp";
  my $sim_temp_trade_file_ = "/spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_"."_simtmp";
  foreach (keys %prod_to_strat_line_ )
  {
  	my $sec_name_ = `$BIN_DIR/get_exchange_symbol $_ $yyyymmdd_`; chomp $sec_name_ ;
	$sec_name_ =~ s/ /~/ig;
  	#print "grep $sec_name_.$id_ /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ > $temp_trade_file_\n";
    `grep "$sec_name_.$id_" /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ > $temp_trade_file_`;
    `grep "$sec_name_.$id_" $trades_file_name_ > $sim_temp_trade_file_`;
    
    if ( -e $temp_trade_file_ )
    {
      my $plot_cmd_ = $SCRIPTS_DIR."/plot_multifile_cols.pl $sim_temp_trade_file_  9 $yyyymmdd_.$_"."_REAL"." WL $temp_trade_file_ 9 $yyyymmdd_.$_"."_SIM"." WL";
#      print "If $plot_cmd_\n";
      `$plot_cmd_`;
    }
  }
  `grep -v PNLSPLIT  /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_ > $sim_temp_trade_file_ `;
  `grep -v PNLSPLIT $trades_file_name_ > $temp_trade_file_ `;
  my $plot_cmd_ = $SCRIPTS_DIR."/plot_multifile_cols.pl  $temp_trade_file_ 18 $yyyymmdd_.$dep_shortcode_"."_REAL WL $sim_temp_trade_file_ 18 $yyyymmdd_.$dep_shortcode_"."_SIM WL ";
#  print "$plot_cmd_\n";
  `$plot_cmd_`;
  `rm -f $temp_trade_file_`;
  `rm -f $sim_temp_trade_file_`;
}

# Remove local strategy, model & param file.
if ( $to_remove_strat_files_ )
{
	foreach my $file_ ( @to_remove_files_ )
	{
		`rm -rf $file_`;
	}
# SIM trade & log file.

  `rm -f /spare/local/logs/tradelogs/log.$yyyymmdd_.$sim_id_`;
  `rm -f /spare/local/logs/tradelogs/trades.$yyyymmdd_.$sim_id_`;
}
exit ( 0 );

sub FetchSyncedParamsAndModels
{
  my ( $t_model_file_name_ , $t_param_file_name_ ) = @_;

  my $synced_file_location_ = "/NAS1/logs/QueryLogs/".$yyyy_."/".$mm_."/".$dd_."/";

  my $synced_model_file_name_ = $synced_file_location_.basename ( $t_model_file_name_ );
  my $synced_param_file_name_ = $synced_file_location_.basename ( $t_param_file_name_ );

  if ( ExistsWithSize ( $synced_param_file_name_ ) )
  {
# print "cp $synced_param_file_name_ $t_param_file_name_\n";
    `cp $synced_param_file_name_ $t_param_file_name_`;
  }

  if ( ExistsWithSize ( $synced_model_file_name_ ) )
  {
# print "cp $synced_model_file_name_ $t_model_file_name_\n";
    `cp $synced_model_file_name_ $t_model_file_name_`;
  }

  return;
}

sub ModifyCommonParamFile 
{
	my $cmn_param_ = shift ;
	open COMMON_PARAM, "< $cmn_param_" or PrintStacktraceAndDie ( "Could not open common param $cmn_param_ for reading\n");
	my @common_param_words_ = <COMMON_PARAM>;
	close COMMON_PARAM ;
	open COMMON_PARAM_WRITE, "> $cmn_param_" or PrintStacktraceAndDie ( "Could not open commonparam $cmn_param_ for writing\n");
	foreach my $line_ ( @common_param_words_ )
	{
		if ( index ( $line_, "STRUCTURE_FILE") >= 0 )
		{
			my @line_words_ = split ( " ", $line_ );
			if ( $#line_words_ > 0 )
			{
				my $structure_file_ = $line_words_[2];
				my $structure_file_basename_ = basename ( $structure_file_); chomp ( $structure_file_basename_);
				my $real_structure_file_name_ = $real_query_log_dir_."/".$structure_file_basename_;
				my $t_structure_file_name_ = $t_param_file_dir_."/".$structure_file_basename_; 
				if ( -e $real_structure_file_name_ )
				{
					`cp $real_structure_file_name_ $t_structure_file_name_`;
				}
				else
				{
					`rsync -avz $remote_login_:$structure_file_ $t_param_file_dir_`;
				}
				print COMMON_PARAM_WRITE "$line_words_[0] $line_words_[1] $t_structure_file_name_\n";
				push ( @to_remove_files_, $t_structure_file_name_ );
			}
		}
		else
		{
			print COMMON_PARAM_WRITE $line_ ;
		}
	}
	close COMMON_PARAM_WRITE ;
}
