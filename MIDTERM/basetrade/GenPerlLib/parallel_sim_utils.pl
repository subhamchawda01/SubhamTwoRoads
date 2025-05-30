# \file GenPerlLib/parallel_sim_utils.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite 217, Level 2, Prestige Omega,
# 	 No 104, EPIP Zone, Whitefield,
# 	 Bangalore - 560066, India
# 	 +91 80 4060 0717
#

use List::Util qw/max min/; # for max
use Math::Complex ; # sqrt
my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

my $HOME_DIR = $ENV { 'HOME' };
my $LIVE_BIN_DIR = $HOME_DIR."/LiveExec/bin";

sub GetMaxCoresToUseInParallel
{
  my $num_cores_ = `nproc`; chomp ( $num_cores_ );
  return ( max ( 1, int ( $num_cores_/3 ) ) ) ;  #maximum use 1/3rd of total computation power
}

# TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
sub TemperCoreUsageOnLoad
{
  my ( $max_max_cores_to_use_in_parallel_ ) = @_;

  my $num_cores_ = `nproc`; chomp ( $num_cores_ );
  my $last_5_minute_core_normalised_load_average_ = `uptime | grep \"load average\" | awk '{ print \$11; }' | awk -F, '{ print \$1; }'`; chomp($last_5_minute_core_normalised_load_average_);
  $last_5_minute_core_normalised_load_average_ = $last_5_minute_core_normalised_load_average_ / $num_cores_;

  my $max_normalized_load_allowed_ = 1.3;
  my $new_max_cores_to_use_in_parallel_ = max ( 1 , int ( ( $max_normalized_load_allowed_ - $last_5_minute_core_normalised_load_average_ ) * $num_cores_ ) );
  $new_max_cores_to_use_in_parallel_ = min ( $max_max_cores_to_use_in_parallel_ , $new_max_cores_to_use_in_parallel_ );

  return $new_max_cores_to_use_in_parallel_;
}

sub GetGlobalUniqueId
{
  my $global_unique_id_ = 1;

  my $gui_directory_ = $HOME_DIR."/gui";
  `mkdir -p $gui_directory_`;

  my $gui_filename_ = $gui_directory_."/gui.txt";

  if ( ! -e $gui_filename_ ) { `> $gui_filename_`; }

  open ( GUI_FILE , "+<" , $gui_filename_ ) or PrintStacktraceAndDie ( "Could not open $gui_filename_ in read+write mode\n" );
  flock ( GUI_FILE , LOCK_EX );

  seek ( GUI_FILE , 0 , 0 ); # go to beginning
      my @lines_ = <GUI_FILE>; chomp ( @lines_ );

  if ( $#lines_ >= 0 )
  {
    my $line_ = $lines_ [ 0 ];
    $global_unique_id_ = ( $line_ + 1 ) % 8888888;
  }

  seek ( GUI_FILE , 0 , 0 ); # go to beginning again
      print GUI_FILE $global_unique_id_."\n";

  close ( GUI_FILE );

  return $global_unique_id_;
}

sub AllOutputFilesPopulated
{
  my ( @output_files_to_poll_this_run_ ) = @_;

  my $retval_ = 1;

  for ( my $output_file_index_ = 0 ; $output_file_index_ <= $#output_files_to_poll_this_run_ ; $output_file_index_ ++ )
  {
    my $exec_cmd_ = "grep SIMRESULT ".$output_files_to_poll_this_run_ [ $output_file_index_ ]." 2>/dev/null | wc -l 2>/dev/null";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

    if ( $#exec_cmd_output_ >= 0 )
    {
      my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ 0 ] ); chomp ( @exec_cmd_output_words_ );
      if ( $#exec_cmd_output_words_ >= 0 )
      {
        my $simresult_count_ = $exec_cmd_output_words_ [ 0 ];
        if ( $simresult_count_ <= 0 )
        {
          $retval_ = 0;
          last;
        }
      }
      else
      {
        $retval_ = 0;
        last;
      }
    }
    else
    {
      $retval_ = 0;
      last;
    }
  }

  return $retval_;
}

sub AllOutputFilesExist
{
  my ( @output_files_to_poll_this_run_ ) = @_;
  my $retval_ = 1;
  for ( my $output_file_index_ = 0 ; $output_file_index_ <= $#output_files_to_poll_this_run_ ; $output_file_index_ ++ )
  {
    if ( !( -e $output_files_to_poll_this_run_ [ $output_file_index_ ] ) )
    {
      $retval_ = 0;
      last;
    }
  }
  return $retval_;
}

sub AllPIDSTerminated
{
  my ( @pids_to_poll_this_run_ ) = @_;

  my $retval_ = 1;

  for ( my $pid_index_ = 0 ; $pid_index_ <= $#pids_to_poll_this_run_ ; $pid_index_ ++ )
  {
    my $exec_cmd_ = "ps cax | grep \"".$pids_to_poll_this_run_ [ $pid_index_ ]." \" | awk '{ if ( \$1 == \"".$pids_to_poll_this_run_ [ $pid_index_ ]."\" ) { print \$0; } }' | wc -l";
    my @exec_cmd_output_ = `$exec_cmd_`; chomp ( @exec_cmd_output_ );

    if ( $#exec_cmd_output_ >= 0 )
    {
      my @exec_cmd_output_words_ = split ( ' ' , $exec_cmd_output_ [ 0 ] );
      if ( $#exec_cmd_output_words_ >= 0 )
      {
        my $running_count_ = $exec_cmd_output_words_ [ 0 ];

        if ( $running_count_ > 0 )
        {
          $retval_ = 0;
          last;
        }
      }
    }
  }

  return $retval_;
}

sub RunParallelProcesses
{
  my $independent_parallel_commands_ = shift;
  my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel();
  if ( @_ > 0 ) { $MAX_CORES_TO_USE_IN_PARALLEL = shift; } 
  my $log_fh_;
  if ( @_ > 0 ) { $log_fh_ = shift; }
  else { open $log_fh_,">-"; }

  print {$log_fh_} "running ".($#$independent_parallel_commands_ + 1)." parallel processes\n";
  # Run parallel datagens, all datagens are stored in independent_parallel_commands
  for ( my $command_index_ = 0 ; $command_index_ <= $#$independent_parallel_commands_ ; )
  {
    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
    print {$log_fh_} "num of cores available = ".$THIS_MAX_CORES_TO_USE_IN_PARALLEL."\n";
    my @child_pids_ = ();
    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $command_index_ <= $#$independent_parallel_commands_ ; $num_parallel_++ )
    {
      print {$log_fh_} $$independent_parallel_commands_ [ $command_index_ ]."\n";
      my $exec_cmd_ = $$independent_parallel_commands_ [ $command_index_ ];
      my $pid_ = fork();
      die "unable to fork $!" unless defined($pid_);
      push(@child_pids_, $pid_) ;
      if ( !$pid_ )
      {
        # child process has pid 0
        exec($exec_cmd_);
      }
      # back to parent process
      print {$log_fh_} "PID of cmd:$command_index_ is $pid_\n";
      $command_index_++;
      sleep ( 1 );
    }
    print {$log_fh_} "waiting for processes to complete...\n";
    for my $pid_ ( @child_pids_ ) 
    {
      waitpid ($pid_ , 0);
      print {$log_fh_} "PID of completed process: $pid_\n";
    }
    print {$log_fh_} "all process finished\n";
  }
}

1;
