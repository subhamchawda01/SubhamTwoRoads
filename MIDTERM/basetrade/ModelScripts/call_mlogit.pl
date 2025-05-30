#!/usr/bin/perl

# \file ModelScripts/call_mlogit.pl
#
#    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#     Address:
#	 Suite No 353, Evoma, #14, Bhattarhalli,
#	 Old Madras Road, Near Garden City College,
#	 KR Puram, Bangalore 560049, India
#	 +91 80 4190 3551
#
# 

use strict;
use warnings;

my $USER=$ENV{'USER'};
my $HOME_DIR=$ENV{'HOME'}; 
my $REPO="basetrade";
my $GENPERLLIB_DIR=$HOME_DIR."/".$REPO."_install/GenPerlLib";
my $MODELSCRIPTS_DIR=$HOME_DIR."/".$REPO."_install/ModelScripts";

require "$GENPERLLIB_DIR/print_stacktrace.pl"; # PrintStacktrace , PrintStacktraceAndDie

# start 
my $USAGE="$0  filtered_reg_data_filename_ zero_threshold_ max_model_size_ regression_output_filename_";

if ( $#ARGV < 3 ) { print $USAGE."\n"; exit ( 0 ); }
my $filtered_reg_data_filename_ = $ARGV[0];
my $zero_threshold_ = $ARGV[1];
my $max_model_size_ = $ARGV[2];
my $regression_output_filename_ = $ARGV[3];

my $std_dev_all_ =`cat $filtered_reg_data_filename_ | awk '{sum+=\$1;sum2+=\$1*\$1;}END{print sqrt(sum2/NR - (sum/NR)*(sum/NR))}'`;
my $sampled_reg_data_filename_ = $filtered_reg_data_filename_."_sampled";
my $exec_cmd_ = "cat $filtered_reg_data_filename_ | awk '{if(NR%10==0){print\$0}}' > $sampled_reg_data_filename_ ";
`$exec_cmd_`;
my $std_dev_sample_ =`cat $sampled_reg_data_filename_ | awk '{sum+=\$1;sum2+=\$1*\$1;}END{print sqrt(sum2/NR - (sum/NR)*(sum/NR))}'`;

#$zero_threshold_ = $zero_threshold_*$std_dev_;
my $len=`cat $filtered_reg_data_filename_ | wc -l`;
my $lineNum=int($len*(1-$zero_threshold_))."p";
#$zero_threshold_=`cat $filtered_reg_data_filename_ |awk '{tgt=\$1>0?\$1:-\$1;print tgt}' | sort -k1 -nr | sed -n '$lineNum' | awk '{print \$1}'`;
$zero_threshold_ = $zero_threshold_ * $std_dev_all_;
print "$std_dev_all_ $std_dev_sample_ $zero_threshold_\n";
my $sampled_mlogit_reg_data_filename_ = $sampled_reg_data_filename_."_mlogit";
$exec_cmd_ = "cat $sampled_reg_data_filename_ | awk '{if(\$1>=$zero_threshold_){label=2;} else if(\$1<=-$zero_threshold_){label=0;} else if(\$1>-$zero_threshold_ && \$1 < $zero_threshold_){label=1;} print label}' > $sampled_mlogit_reg_data_filename_";
print $exec_cmd_."\n";
`$exec_cmd_`;
$exec_cmd_ = "cat $sampled_mlogit_reg_data_filename_ |  awk '{if(\$1==0){sum0+=1;}if(\$1==1){sum1+=1;}if(\$1==2){sum2+=1;}}END{print (sum0/NR),(sum1/NR),(sum2/NR)}'";
print $exec_cmd_."\n";
my $output_line_ = `$exec_cmd_`;
print $output_line_."\n";
$exec_cmd_ = "$MODELSCRIPTS_DIR/build_mlogit_model.R $sampled_reg_data_filename_ $sampled_mlogit_reg_data_filename_ $max_model_size_ $regression_output_filename_";
print $exec_cmd_."\n";
my @output_lines_ = `$exec_cmd_`;
print "@output_lines_";
`rm -f $sampled_reg_data_filename_`;
`rm -f $sampled_mlogit_reg_data_filename_`;
