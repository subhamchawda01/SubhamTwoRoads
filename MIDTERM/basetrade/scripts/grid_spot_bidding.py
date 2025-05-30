from __future__ import division, absolute_import, print_function
import boto
import boto3
import datetime
import dateutil.parser
import logging
import logging.config
import os
import math
import pytz
import sys
from datetime import *
from boto.ec2.autoscale import AutoScaleConnection
import time



def get_conn():
    aws_access_key_id="AKIAIX6E2SVJSSQ4X32Q"
    aws_secret_access_key="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"
    try:
        ec2_conn = boto.connect_ec2(aws_access_key_id, aws_secret_access_key)
        as_conn = boto.connect_autoscale(aws_access_key_id, aws_secret_access_key, use_block_device_types=True)
    except Exception:
        self.write_log("The connection to ec2 cluster failed")
        sys.exit("Problem in connecting.")
    return (ec2_conn, as_conn)



def get_min_price_(ec2_conn,lookback=1800):
    min_price_ = 1000
    current_time_ = datetime.utcfromtimestamp(time.time())
    start_time_ = current_time_ - timedelta(seconds=lookback)
    price_list = ec2_conn.get_spot_price_history(instance_type='r4.4xlarge',start_time=start_time_.isoformat(),end_time=current_time_.isoformat(),availability_zone='us-east-1a')
    for price in price_list:
        if price.price < min_price_:
            min_price_ = price.price
    return min_price_

def create_new_launch_config(new_launch_config_name,old_launch_config_object,bid_price,updating_old=False):
    new_lc_object = old_launch_config_object
    new_lc_object.name = new_launch_config_name
    print ("The new launch config name : ",new_launch_config_name)
    new_lc_object.instance_type = 'r4.4xlarge'
    new_lc_object.block_device_mappings = [old_launch_config_object.block_device_mappings]
    new_lc_object.spot_price = bid_price * 1.5
    new_lc_object.instance_monitoring = True
    try:
        print ("Trying to create a new launch config on aws")
        client = boto3.client('autoscaling',region_name='us-east-1',
                             aws_access_key_id="AKIAIX6E2SVJSSQ4X32Q",
                             aws_secret_access_key="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")
#     client.create_launch_configuration(new_lc_object.name,
#                                       new_lc_object.image_id,
#                                       new_lc_object.instance_type,
#                                       new_lc_object.name,
#                                       new_lc_object.security_groups)
        client.create_launch_configuration(ImageId=new_lc_object.image_id,InstanceType=new_lc_object.instance_type,LaunchConfigurationName=new_lc_object.name,SpotPrice=str(bid_price * 1.5),
                                          KeyName=new_lc_object.key_name,EbsOptimized=new_lc_object.ebs_optimized,
                                          InstanceMonitoring={'Enabled':new_lc_object.instance_monitoring},
                                          SecurityGroups=new_lc_object.security_groups,
                                          IamInstanceProfile=new_lc_object.instance_profile_name,
                                         AssociatePublicIpAddress=True)
        print ("New launch config with created SUCESSFULLY on aws , launch config name : ",new_lc_object.name)
        return (new_lc_object,1)
    except Exception as e:
        print (str(e))
        print ("New launch config FAILED to create on aws , launch config name : ",new_lc_object.name)
        return (-1,-1)


def delete_launch_config_on_aws_cluster(launch_config_name_to_delete):
    autoscale_connection_delete_ = AutoScaleConnection(aws_access_key_id="AKIAIX6E2SVJSSQ4X32Q",aws_secret_access_key="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")
    try:
        print ("Starting delete launch config with name : ",launch_config_name_to_delete)
        autoscale_connection_delete_.delete_launch_configuration(launch_config_name_to_delete)
        print ("Deletion of launch config with name : ",launch_config_name_to_delete," successful")
    except:
        print ("Deletion of launch config with name : ",launch_config_name_to_delete," FAILED")
        print ("Exiting because launch config is not deleted")
        sys.exit(1)


def attach_local_launch_config_to_autoscaling_group(autoscaling_group_object,launch_config_name):
    try:
        print ("Updating the launch config with name : ",launch_config_name, " on aws")
        autoscaling_group_object.launch_config_name = launch_config_name
        autoscaling_group_object.update()
        print ("Launch config update on aws successfull , launc config name : ",launch_config_name)
    except:
        print ("Launch config update failed launch config name : ",launch_config_name)
        sys.exit(0)


def main(autoscaling_group_name):
    autoscaling_to_launch_config_mapping={}
    autoscaling_to_launch_config_mapping["grid_worker_manual_jobs_asg"] = "grid_worker_manual_jobs_lc_version_5"
    autoscaling_to_launch_config_mapping["grid_worker_eod_jobs_asg"] = "grid_worker_eod_jobs_lc_version_5"
    bid_price_scaling_factor_ = 1.5
    #get the connection to ec2 cluster
    (ec2_conn, as_conn) = get_conn()
    #get the name of the launch config corresponding to this autoscaling group
    if autoscaling_group_name in autoscaling_to_launch_config_mapping.keys():
        launch_config_name = autoscaling_to_launch_config_mapping[autoscaling_group_name]
        #get the autoscaling group object 
        autoscaling_group_object = as_conn.get_all_groups([autoscaling_group_name])[0]
        #get the launch config object for this autoscaling_group
        present_launch_config = as_conn.get_all_launch_configurations(names=[autoscaling_group_object.launch_config_name])[0]
        #create a temporary launch config
        bid_price = float(get_min_price_(ec2_conn)) * bid_price_scaling_factor_
        new_LC_name = launch_config_name+"_"+datetime.now().strftime("%Y%m%d%H%M%S")
        (new_LC_object,success_flag) = create_new_launch_config(new_LC_name,present_launch_config,bid_price,False)
        print ("A copy of the existing launch config created")
        print ("\n\n")
        if new_LC_object==-1 or success_flag==-1:
            sys.exit(1)
        #update the temp launch config to the auto scaling group
        attach_local_launch_config_to_autoscaling_group(autoscaling_group_object,new_LC_name)
        print ("The backup copy of the launch config is attached to the autoscaling grpup ")
        print ("\n\n")
        #delete the old launch config
        delete_launch_config_on_aws_cluster(launch_config_name)
        print ("The orignal launch config removed from aws cluster, the removed launch config name : ",launch_config_name)
        print ("\n\n")

        #update the bidding price of the launch config
        (new_LC_object,success_flag) = create_new_launch_config(launch_config_name,new_LC_object,bid_price,True)
        if new_LC_object==-1 or success_flag==-1:
            sys.exit(1)
        print ("The orignal launch config with modified spot price created, the name of the orignal launch config : ",new_LC_object.name)
        print ("\n\n")

        #attach the orignal launch config to the autoscaling group
        attach_local_launch_config_to_autoscaling_group(autoscaling_group_object,launch_config_name)
        print ("The orignal copy of the launch config attached to the autoscaling group")
        print ("\n\n")

        #delete the temporary_launch config
        delete_launch_config_on_aws_cluster(new_LC_name)
        print ("The backup copy of the launch config has been deleted, the name of the deleted launch config : ",new_LC_name)
        print ("\n\n")
    else:
        print ("The autoscaling group not supported ,hence EXITING  the asg name is : ",autoscaling_group_name)
        sys.exit(0)



if __name__ == "__main__":
        autoscaling_group_name = sys.argv[1]
        print (autoscaling_group_name)
        main(autoscaling_group_name)
