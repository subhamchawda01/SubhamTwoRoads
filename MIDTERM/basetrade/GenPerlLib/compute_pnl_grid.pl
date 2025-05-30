#!/usr/bin/perl
use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'};
my $REPO="basetrade";
my $SCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/scripts";

my $GRID_CMD_ = "grid -u http://10.1.4.15:5000 -s ";
if (defined $ENV{'EOD'}){
    $GRID_CMD_ = "grid -u http://10.1.4.28:5000 -s "
}

my $json_splitter_ = "$SCRIPTS_DIR/split_json_grid_job.py";
my $json_result_combiner_ = "$SCRIPTS_DIR/combine_grid_results.py";

# Subroutine that takes a json file and the results directory, calls grid compute_pnl job
# and copies the results from artifacts to results directory
sub RunGrid
{
    # just run the grid command and return the artifact directory
  my $strat_json_file_ = shift;
  my $work_dir_ = shift;

  my $json_splitter_output_ = `$json_splitter_ $strat_json_file_`;
  my @strat_json_files_ = split(/\n/, $json_splitter_output_);
  my $result_directories_ = "";
  my $unique_id = `date +%N`; chomp($unique_id);
  my $main_result_dir_ = $work_dir_."/grid_results_".$unique_id;
  my $temp_result_dir_ = $work_dir_."/grid_artifacts";
  `mkdir -p $temp_result_dir_`;

  foreach my $single_strat_json_file_ (@strat_json_files_) {

      my $exec_cmd_ = $GRID_CMD_.$single_strat_json_file_." -o ".$temp_result_dir_;
      my $grid_output_ = `$exec_cmd_`;

      print $grid_output_;
      my @grid_output_lines_ = split(/\n/, $grid_output_);

      foreach my $out_line_ (@grid_output_lines_)
      {
          if ($out_line_ =~ /Artifacts downloaded into/)
          {
              my @tokens_ = split(/\s/, $out_line_);
              my $result_directory_ = $tokens_[- 1];
              my $artifacts_result_directory_ = $result_directory_;
              print "Aritifacts result directory: ".$artifacts_result_directory_."\n";
              $result_directories_ = $result_directories_." ".$artifacts_result_directory_;
          }
      }
  }

  my $combine_cmd = $json_result_combiner_." ".$result_directories_." ".$main_result_dir_;
  `$combine_cmd`;
  print "Final Results/Artifacts Directory: ".$main_result_dir_."\n";
  `rm -rf $temp_result_dir_`;

  return $main_result_dir_."/artifacts";
}

sub CopyFromArtifactsToLocalDir
{
    my $artifacts_dir = shift;
    my $local_results_base_dir_ = shift;
    $artifacts_dir = $artifacts_dir."/pnls/";	
    opendir(DH, $artifacts_dir);
    my @results_files_ = readdir(DH);
    closedir(DH);

    foreach my $dated_file_ (@results_files_)
    {
        next if (! -f $artifacts_dir.$dated_file_);
        my $year_ = substr $dated_file_, 0, 4;
        my $month_ = substr $dated_file_, 4, 2;
        my $date_ = substr $dated_file_, 6, 2;
        my $exec_cmd_ = "mkdir -p ".$local_results_base_dir_."/".$year_;
        `$exec_cmd_`;
        $exec_cmd_ = "mkdir -p ".$local_results_base_dir_."/".$year_."/".$month_;
        `$exec_cmd_`;
        $exec_cmd_ = "mkdir -p ".$local_results_base_dir_."/".$year_."/".$month_."/".$date_;
        `$exec_cmd_`;
        $exec_cmd_ = "cp ".$artifacts_dir.$dated_file_." ".$local_results_base_dir_."/".$year_."/".$month_."/".$date_."/results_database.txt";
        `$exec_cmd_`;
    }
}


# Subroutine to take work_directory, strats_directory, start_date, end_date and results_directory
# Creates the JSON string and call grid
# After completion of job, copy the results from artifacts to results directory
sub ComputePnlGridStartEndDate
{
  my $temp_work_dir_ = shift;
  my $strats_dir_ = shift;
  my $start_date_ = shift;
  my $end_date_ = shift;
  my $local_results_base_dir_ = shift;

  my $strat_json_string_ = "{\n\"job\": \"compute_pnl\",\n\"strats_files_dir\": \"".$strats_dir_."\",\n\"start_date\": \"".$start_date_."\",\n\"end_date\": \"".$end_date_."\"\n}";

  my $strat_json_file_ = $temp_work_dir_."/strat_json";
  open ( JSON, ">",  $temp_work_dir_."/strat_json");
  print JSON $strat_json_string_;
  close (JSON);


  my $artifacts_dir = RunGrid($strat_json_file_, $temp_work_dir_);
  CopyFromArtifactsToLocalDir($artifacts_dir, $local_results_base_dir_);

}

# Subroutine to take work_directory, strats_directory, results_directory, and list of dates
# Creates the JSON string and call grid
# After completion of job, copy the results from artifacts to results directory
sub ComputePnlGridDatesVec
{
  my $temp_work_dir_ = shift;
  my $strats_dir_ = shift;
  my $local_results_base_dir_ = shift;
  my @dates_vec_ = @_;
  my $dates_list_ = join('","', @dates_vec_);
  $dates_list_ = "\"".$dates_list_."\"";
  opendir(DH, $strats_dir_);
  my @strat_files_ = readdir(DH);
    closedir(DH);

    my $strat_json_start_string_ = "{\n \"job\": \"compute_pnl\",\n\"strategies\" : [ \n";
    my $strat_json_string_ = "";
    my $strat_json_end_string_ = " ]\n} ";

    foreach my $strat_file_ (@strat_files_)
    {
        next if (! -f $strats_dir_."/".$strat_file_);

        my $strat_line_cmd_ = "cat ".$strats_dir_."/".$strat_file_." 2>/dev/null | grep -v \"^#\" | head -1\n";
        my $stratline = `$strat_line_cmd_`;
        chomp($stratline);
        next if $stratline eq "";
        my $strat_name = $strat_file_;

        my $this_json_line = "{\n \"name\" : \"$strat_name\",\n \"strat_line\":\"$stratline\",\n \"dates\": [ $dates_list_ ] \n}";
        if ($strat_json_string_ eq "") {
            $strat_json_string_ = $this_json_line;
        } else {
            $strat_json_string_ = $strat_json_string_.",\n".$this_json_line;
        }
    }
    $strat_json_string_ = $strat_json_start_string_.$strat_json_string_.$strat_json_end_string_;

  my $strat_json_file_ = $temp_work_dir_."/strat_json";
  open ( JSON, ">",  $temp_work_dir_."/strat_json");
  print JSON $strat_json_string_;
  close (JSON);

  my $artifacts_dir = RunGrid($strat_json_file_, $temp_work_dir_);
  CopyFromArtifactsToLocalDir($artifacts_dir, $local_results_base_dir_);
}


sub ComputePnlGridStratDateVec
{
    my $temp_work_dir_ = shift;
    my $stratline_to_date_map_ = shift;
    my $stratline_to_strat_map_ = shift;

    # no need to actually put \n there
    my $strat_json_start_string_ = "{\n \"job\": \"compute_pnl\",\n\"strategies\" : [ \n";
  my $strat_json_string_ = "";
    my $strat_json_end_string_ = " ]\n} ";

    foreach my $stratline ( keys %{$stratline_to_date_map_})
  {
        my $strat_name = $$stratline_to_strat_map_{$stratline};
    my $date_list = join('","', @{$$stratline_to_date_map_{$stratline}});
    $date_list = "\"".$date_list."\"";

        my $this_json_line = "{\n \"name\" : \"$strat_name\",\n \"strat_line\":\"$stratline\",\n \"dates\": [ $date_list ] \n}";
    if ($strat_json_string_ eq "") {
        $strat_json_string_ = $this_json_line;
    } else {
        $strat_json_string_ = $strat_json_string_.",\n".$this_json_line;
    }
    }

    $strat_json_string_ = $strat_json_start_string_.$strat_json_string_.$strat_json_end_string_;
    my $json_id = `date +%N`; chomp($json_id);
    my $strat_json_file = $temp_work_dir_."/strat_json_".$json_id;
    open (JSON, ">", $strat_json_file);
    print JSON $strat_json_string_;
    close(JSON);

    my $artifacts_dir = RunGrid($strat_json_file, $temp_work_dir_);

    return $artifacts_dir;
}
1;
