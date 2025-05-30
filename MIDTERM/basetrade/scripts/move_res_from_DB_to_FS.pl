#!/usr/bin/perl
use strict;
use warnings;
use Fcntl qw (:flock);
use sigtrap qw(handler signal_handler normal-signals error-signals);

my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";

my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie
require "$GENPERLLIB_DIR/exists_with_size.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/lock_utils.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/calc_prev_working_date_mult.pl"; # ExistsWithSize
require "$GENPERLLIB_DIR/results_db_access_manager.pl"; # GetDataStartTimeForStrat
require "$GENPERLLIB_DIR/break_date_yyyy_mm_dd.pl"; # BreakDateYYYYMMDD
require "$GENPERLLIB_DIR/create_enclosing_directory.pl"; # CreateEnclosingDirectory
require "$GENPERLLIB_DIR/string_utils.pl"; # trim
require "$GENPERLLIB_DIR/parallel_sim_utils.pl";

my $SCRIPT_NAME = $0;
my $USAGE = "$SCRIPT_NAME output_dir [normal(N)/staged(S)/all(A)=N] [parrallel=1] [shc/ALL/FILE:<ShcFileName>=ALL] [days=400] [start-date]";
if ( $#ARGV < 0 )
{
  print $USAGE."\n";
  exit(0);
}

#reading args
my $dir = $ARGV[0];
my $strat_type = "N";
if ( $#ARGV >= 1 ) { $strat_type = $ARGV[1];}
my $is_parrallel = 1;
if ( $#ARGV >= 2 ) { $is_parrallel = $ARGV[2];}
my $shc = "ALL";
if ( $#ARGV >= 3 ) { $shc = $ARGV[3];}
my $days = 400;
if ( $#ARGV >= 4 ) { $days = $ARGV[4];}
my $dt = `date +%Y%m%d`; chomp($dt); 
if ( $#ARGV >= 5 ) { $dt  = $ARGV[5];}

# this lock is just to avoid running more than one process with same args
# more robust locking id done by flock on individual result files while writing to files
my $lock_str_ ;
my $main_lock_str_ ;

#filling shc_vec
my $shc_vec_ref_;
@$shc_vec_ref_ = ($shc);
if ( $shc eq "ALL" )
{
  $shc_vec_ref_ = GetAllShcVecRef();
}
elsif ( substr($shc, 0, 5) eq "FILE:" )
{
  my $shc_file_ = substr( $shc, 5 );
  open SHCFILE , "< $shc_file_" or PrintStacktraceAndDie("Can't open $shc_file_ for reading\n");
  @$shc_vec_ref_ = <SHCFILE>; chomp(@$shc_vec_ref_); 
  close SHCFILE;
  @$shc_vec_ref_ = map { trim($_) } @$shc_vec_ref_;
}

if ( $#$shc_vec_ref_ >= 1 )
{
  $main_lock_str_ = $dir."_".$shc."_MoveResDBToFS";
  if ( ! TakeLock( $main_lock_str_, "EX_NB" ) )
  {
    print "Already Dumping for $shc into $dir. Exiting\n";
    exit(0);
  }
}

print "Shc List\n".join( " ", @$shc_vec_ref_ )."\n";

#dumping for each shc  
if ( ! $is_parrallel )
{
  foreach my $this_shc ( @$shc_vec_ref_ )
  {
    $lock_str_ = $dir."_".$this_shc."_MoveResDBToFS";
    if ( TakeLock( $lock_str_, "EX_NB" ) )
    {
      my $t_num_res_ = DumpForOneShc($this_shc, $dir, $strat_type, $days, $dt); 
      print "Dumped $t_num_res_ Results for $this_shc\n";
      RemoveLock($lock_str_);
    }
    else
    {
      print "Already Dumping for $this_shc into $dir. Moving to Next shc.\n";
    }
  }
}
else
{
  my $MAX_CORES_TO_USE_IN_PARALLEL = GetMaxCoresToUseInParallel ( ) ;
  my %pid_to_shc_map_ = ();
  for ( my $shc_idx = 0; $shc_idx <= $#$shc_vec_ref_ ;)
  {
    my $THIS_MAX_CORES_TO_USE_IN_PARALLEL = TemperCoreUsageOnLoad ( $MAX_CORES_TO_USE_IN_PARALLEL );
    print "num of cores available = ".$THIS_MAX_CORES_TO_USE_IN_PARALLEL."\n";
    my @child_pids_ = ();

    for ( my $num_parallel_ = 1 ; $num_parallel_ <= $THIS_MAX_CORES_TO_USE_IN_PARALLEL && $shc_idx <= $#$shc_vec_ref_ ; $num_parallel_++ )
    {
      my $this_shc = $$shc_vec_ref_[$shc_idx];
      my $exec_cmd_ = "$SCRIPT_NAME $dir $strat_type 0 $this_shc $days";

      my $pid_ = fork();
      PrintStacktraceAndDie("unable to fork $!") if ( !defined $pid_ );
      push(@child_pids_, $pid_) ;

      if ( !$pid_ )
      {
        # child process has pid 0
        exec($exec_cmd_);
      }

      # back to parent process
      $pid_to_shc_map_{$pid_} = $this_shc;
      printf "Dumping %s [PID:%d]...\n", $this_shc, $pid_;
      $shc_idx++;
    }
    print "waiting for processes to complete...\n";
    for my $pid_ ( @child_pids_ )
    {
      waitpid ($pid_ , 0);
      printf "Done %s [PID:%d]\n", $pid_to_shc_map_{$pid_}, $pid_;
    }
    print "all processes finished\n";
  }
}

exit(0);

sub DumpForOneShc
{
  my ( $_shc, $_output_dir, $_strat_type, $_days, $_end_dt ) = @_ ;
  my $num_res_dumped_ = 0;
  while ( $_days >= 0 )
  {
    my @res_vec = ();
    my $db_last_update_ = GetLastUpdate( $_shc, $_end_dt, $_strat_type );
    my ( $yyyy, $mm, $dd ) = BreakDateYYYYMMDD( $_end_dt );
    my $file = "$_output_dir/$_shc/$yyyy/$mm/$dd/results_database.txt";
    if ( -f $file )
    {
      my $file_last_update_ = `ls --time-style='+%Y-%m-%d %H:%M:%S' -l $file | cut -d' ' -f6,7`;
      #print "$db_last_update_ $file_last_update_ \n";
      if ( $db_last_update_ lt $file_last_update_ )
      {
      	print "No recent update in DB for $_shc $_end_dt $_strat_type \n";
        $_end_dt = CalcPrevWorkingDateMult($_end_dt, 1);
        $_days--;
        next;
      }
    }
    $num_res_dumped_ += FetchResults ( $_shc, $_end_dt, \@res_vec, $_strat_type  );
    if ( $#res_vec >= 0 )
    {
      my ( $yyyy, $mm, $dd ) = BreakDateYYYYMMDD( $_end_dt );
      my $file = "$_output_dir/$_shc/$yyyy/$mm/$dd/results_database.txt";
      CreateEnclosingDirectory($file);
      open FILE , "> $file" or PrintStacktraceAndDie("Can't open $file to write");
      flock ( FILE, LOCK_EX );
      foreach my $line ( @res_vec )
      {
        print FILE join(" ", @$line)."\n";
      }
      close FILE; 
    }
    $_end_dt = CalcPrevWorkingDateMult($_end_dt,1);
    $_days--;
  }
  return $num_res_dumped_;
}

sub signal_handler
{
    die "Caught a signal $!\n";
}

sub END
{
  if ( $lock_str_ ) { RemoveLock($lock_str_); } 
  if ( $main_lock_str_ ) { RemoveLock($main_lock_str_); } 
}
