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

sub GetAllMachinesVec {
    my @all_machines_ = ();
#ny4
    push ( @all_machines_, "10.23.199.51" );
    push ( @all_machines_, "10.23.199.52" );
    push ( @all_machines_, "10.23.199.53" );
    push ( @all_machines_, "10.23.199.54" );
    push ( @all_machines_, "10.23.199.55" );
#crt
    push ( @all_machines_, "10.23.142.51" );
#cfe
    push ( @all_machines_, "10.23.74.61" );
#dc3
    push ( @all_machines_, "10.23.200.51" );
    push ( @all_machines_, "10.23.200.52" );
    push ( @all_machines_, "10.23.200.53" );
    push ( @all_machines_, "10.23.200.54" );
#fr2
    push ( @all_machines_, "10.23.196.51" );
    push ( @all_machines_, "10.23.196.52" );
    push ( @all_machines_, "10.23.196.53" );
    push ( @all_machines_, "10.23.196.54" );
#tmx
    push ( @all_machines_, "10.23.182.51" );
    push ( @all_machines_, "10.23.182.52" );
#bmf
    push ( @all_machines_, "10.23.23.11" );
#bmfa
    push ( @all_machines_, "10.23.23.12" ); # 10.220.40.2" );
    push ( @all_machines_, "10.23.23.13" ); # 10.220.40.1" );
    push ( @all_machines_, "10.23.23.14" );
#bsl
    push ( @all_machines_, "10.23.52.51" );
    push ( @all_machines_, "10.23.52.52" );
    push ( @all_machines_, "10.23.52.53" );
#HK
    push ( @all_machines_, "10.152.224.145" );
    push ( @all_machines_, "10.152.224.146" );

    push ( @all_machines_, "10.134.210.184" );
    push ( @all_machines_, "10.134.210.182" );
    push ( @all_machines_, "172.18.244.107" );

    return @all_machines_;
}

sub SyncRunningExecToAllMachines {
    my @all_machines_ = GetAllMachinesVec();

    my $input_filename_ = shift;
    my $backup_name_ = shift ;
    my $dirname_ = dirname($input_filename_);
    my $execname_ = basename ( $input_filename_ ) ;
    my $execbackupname_ = $input_filename_.".~".$backup_name_."~" ;

    my $tempcopy_ = "/tmp/".$execname_ ;

    my $debug = 0;

    foreach my $remote_machine_ ( @all_machines_ )
    {
	if ( $remote_machine_ ne $HOSTNAMEIP )
	{

	    if ( $debug == 1 ) { print "ssh $remote_machine_ mkdir -p $dirname_"."\n"; }
	    `ssh $remote_machine_ mkdir -p $dirname_`;
	    if ( $debug == 1 ) { print "ssh $remote_machine_ mv $input_filename_ $execbackupname_"."\n"; }
	    `ssh $remote_machine_ mv $input_filename_ $execbackupname_`;
	    if ( $debug == 1 ) { print "rsync $input_filename_ $USER\@$remote_machine_:$input_filename_"."\n"; }
	    `rsync $input_filename_ $USER\@$remote_machine_:$tempcopy_`;
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
}

1;
