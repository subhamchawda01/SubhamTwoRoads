# \file GenPerlLib/sync_to_all_machines.pl
#
# \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
#  Address:
# 	 Suite No 353, Evoma, #14, Bhattarhalli,
# 	 Old Madras Road, Near Garden City College,
# 	 KR Puram, Bangalore 560049, India
# 	 +91 80 4190 3551
#

use strict;
use warnings;
use File::Basename; # for basename and dirname

my $USER=$ENV{'USER'}; 
my $HOME_DIR=$ENV{'HOME'}; 
# my $REPO="infracore";

# my $BINDIR=$HOME_DIR."/".$REPO."_install/bin";
my $HOSTNAMEIP=`hostname -i`; chomp($HOSTNAMEIP);

sub SyncRunningExecToProdMachine {

    my $prod_machine_ = shift ;
    my $input_filename_ = shift;
    my $backup_name_ = shift ;
    my $changes_description_ = shift ;

    my $dirname_ = dirname($input_filename_);
    my $execname_ = basename ( $input_filename_ ) ; 
    my $execbackupname_ = $input_filename_.".~".$backup_name_."~" ;

    my $tempcopy_ = "/tmp/".$execname_ ;

    my $debug = 0;

    my $remote_machine_ = $prod_machine_ ;
    
    if ( $remote_machine_ ne $HOSTNAMEIP )
    {
        if ( $debug == 1 )
        {
            print "ssh $remote_machine_ mkdir -p $dirname_"."\n";
            print "ssh $remote_machine_ mv $input_filename_ $execbackupname_"."\n";
            print "scp $input_filename_ $USER\@$remote_machine_:$input_filename_"."\n";
        }

        `ssh $remote_machine_ mkdir -p $dirname_`;
        `ssh $remote_machine_ mv $input_filename_ $execbackupname_`;
        `scp $input_filename_ $USER\@$remote_machine_:$tempcopy_`;
        `ssh $remote_machine_ mv $tempcopy_ $input_filename_`;
        `ssh $remote_machine_ rm -rf $tempcopy_`;
        my $mdsum = `ssh $remote_machine_ md5sum $input_filename_` ;

        print " Location : $remote_machine_ MD5SUM : $mdsum \n" ;

        my $mail_body_ = "Exec Update For $remote_machine_ ,  UpdatedExec :  $input_filename_ PrevStableExec : $execbackupname_ \n"; 

        open ( MAIL , "|/usr/sbin/sendmail -t" );
        print MAIL "To: ravi.parikh\@tworoads.co.in, nseall@tworoads.co.in \n";
        print MAIL "Subject: Production Update - $changes_description_\n\n";
        print MAIL $mail_body_;
        close(MAIL);

    }
}

1;
