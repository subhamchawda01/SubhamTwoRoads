#!/usr/bin/perl
use strict;
use warnings;

my $MASTER_INFRA_LOCATION = "/home/sghosh/master/infracore";
my $BACKUP_LOCATION = "/home/sghosh/master/fxstreet/backup";

# Pull most recent master.
my @pull_output_ = `cd $MASTER_INFRA_LOCATION ; git checkout master ; git pull`;

print "Pull output :\n";
for (my $i = 0; $i <= $#pull_output_; $i++) {
    print $pull_output_[$i];
}
print "\n";

# We need to make updates to the following files:
# (i) Create new fxstreet csv file for today (SysInfo/FXStreetEcoReports/fxstreet_02032012_02032012.csv)
# (ii) Append these envents to this year's csv file (SysInfo/FXStreetEcoReports/fxstreet_eco_2012.csv)
# (iii) Merge events to our format to fxstreet eco file (SysInfo/FXStreetEcoReports/fxstreet_eco_2012_processed.txt)
# (iv) Merge events to our format to bloomberg eco file (SysInfo/BloombergEcoReports/merged_eco_2012_processed.txt)

my $FXSTREET_ECOCALENDAR_URL = "http://www.fxstreet.com/fundamental/economic-calendar";
my $MASTER_FX_PARSE_EXEC = "/home/sghosh/master/infracore_install/bin/parse_fxstreet_economic_events";
my $temp_fx_html_ = "/home/sghosh/master/temp/fxstreet.html";

my $yyyymmdd_ = `date +%Y%m%d`; chomp ($yyyymmdd_);
my $yyyy_ = substr ($yyyymmdd_, 0, 4);
my $mm_ = substr ($yyyymmdd_, 4, 2);
my $dd_ = substr ($yyyymmdd_, 6, 2);

my $MASTER_FX_NEW_CSV = "/home/sghosh/master/infracore/SysInfo/FXStreetEcoReports/fxstreet_".$mm_.$dd_.$yyyy_."_".$mm_.$dd_.$yyyy_.".csv"; # (i)
my $MASTER_FX_MERGED_CSV = $MASTER_INFRA_LOCATION."/SysInfo/FXStreetEcoReports/fxstreet_eco_2012.csv"; # (ii)
my $MASTER_FX_MERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/FXStreetEcoReports/fxstreet_eco_2012_processed.txt"; # (iii)
my $MASTER_BB_MERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/merged_eco_2012_processed.txt"; # (iv)

#revert today's changes done by the fetch_fx 
`cp $BACKUP_LOCATION/$MASTER_FX_MERGED_CSV $MASTER_FX_NEW_CSV`; 
`cp $BACKUP_LOCATION/$MASTER_FX_MERGED_TXT $MASTER_FX_MERGED_TXT`; 
`cp $BACKUP_LOCATION/$MASTER_BB_MERGED_TXT $MASTER_BB_MERGED_TXT`; 
`rm $MASTER_FX_NEW_CSV`

# Commit to master branch.
my $auto_commit_message_ = "$yyyymmdd_ : FX-STREET Auto Commit Revert";
my @commit_output_ = `cd $MASTER_INFRA_LOCATION ; git add SysInfo ; git commit -v -m \"$auto_commit_message_\" SysInfo ;`;

print "Commit message :\n";
for (my $i = 0; $i <= $#commit_output_; $i++) {
    print $commit_output_[$i];
}
print "\n";


# now fetch the new events from fxstreet and commit to master

# Fetch the html file
`wget -nv -O $temp_fx_html_ \"$FXSTREET_ECOCALENDAR_URL\"`;
# Extract events and dump out to csv (i)
`$MASTER_FX_PARSE_EXEC $temp_fx_html_ $MASTER_FX_NEW_CSV`;
# Merge new events to existing csv events list (ii)
`cat $MASTER_FX_NEW_CSV >> $MASTER_FX_MERGED_CSV`;

# (iii) & (iv) need to be done remotely (perl modules)

# Filter+merge scripts to be run on local m/c need the following files:
# fxstreet_eco_2012.csv, bbg_us_eco_2012_processed.txt, fxstreet_eco_2012_processed.txt, auction_us_eco_2012_processed.txt, merged_eco_2012_processed.txt
# fxstreet_eco_2012.csv
`rsync -avz $MASTER_FX_MERGED_CSV sghosh\@10.124.97.26:/home/sghosh/fxstreet`;
# bbg_us_eco_2012_processed.txt
my $MASTER_BB_UNMERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/bbg_us_eco_2012_processed.txt";
`rsync -avz $MASTER_BB_UNMERGED_TXT sghosh\@10.124.97.26:/home/sghosh/fxstreet`;
# fxstreet_eco_2012_processed.txt
`rsync -avz $MASTER_FX_MERGED_TXT sghosh\@10.124.97.26:/home/sghosh/fxstreet`;
# auction_us_eco_2012_processed.txt
my $MASTER_AUCTION_UNMERGED_TXT = $MASTER_INFRA_LOCATION."/SysInfo/BloombergEcoReports/auction_us_eco_2012_processed.txt";
`rsync -avz $MASTER_AUCTION_UNMERGED_TXT sghosh\@10.124.97.26:/home/sghosh/fxstreet`;
# merged_eco_2012_processed.txt
`rsync -avz $MASTER_BB_MERGED_TXT sghosh\@10.124.97.26:/home/sghosh/fxstreet`;

# Run filter+merge scripts on local m/c
`ssh sghosh\@10.124.97.26 'sh /home/sghosh/fxstreet/xyz.sh'`;
`ssh sghosh\@10.124.97.26 'sh /home/sghosh/fxstreet/abc.sh'`;

# (i) & (ii) are done.
# Sync back (iii) & (iv).
`rsync -avz sghosh\@10.124.97.26:/home/sghosh/fxstreet/fxstreet_eco_2012_processed.txt $MASTER_FX_MERGED_TXT`;
`rsync -avz sghosh\@10.124.97.26:/home/sghosh/fxstreet/merged_eco_2012_processed.txt $MASTER_BB_MERGED_TXT`;

# Remove temporary copies on local desktop
`ssh sghosh\@10.124.97.26 'rm -f /home/sghosh/fxstreet/*.csv /home/sghosh/fxstreet/*.txt'`;

# Push to master branch.
my $auto_commit_message_ = "$yyyymmdd_ : FX-STREET EOD Auto Commit";
my @commit_output_ = `cd $MASTER_INFRA_LOCATION ; git add SysInfo ; git commit -v -m \"$auto_commit_message_\" SysInfo ; git push origin master`;

print "Commit message :\n";
for (my $i = 0; $i <= $#commit_output_; $i++) {
    print $commit_output_[$i];
}
print "\n";

# Mail the changes so we can catch erroneous commits to master.
`cd $MASTER_INFRA_LOCATION ; git show | /bin/mail -s \"$auto_commit_message_\" sghosh\@circulumvite.com ravi\@tworoads.co.in ankit\@tworoads.co.in rahul\@tworoads.co.in`;

# Remove local temporary files
`rm -rf $temp_fx_html_`;
