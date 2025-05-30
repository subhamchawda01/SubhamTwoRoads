from __future__ import division, absolute_import, print_function
from pyrabbit.api import Client
import boto
import datetime
import dateutil.parser
import logging
import logging.config
import os
import math
import pytz
import sys
import time
#import yaml
from datetime import datetime
from grid.client.resources import JobResource
from grid.client.api import GridClient

class AutoScalingGroup(object):
    def __init__(self,name):
        self.auto_scaling_group_name_=name
        self.instance_type = []
        self.max_instance = 40
        self.min_instance = 0
        self.desired_instance = 5
        self.availiabiliy_zone = ['us-east-1a']
    def set_instance_type(self,instance_type):
        self.instance_type.append(instance_type)
    def set_max_instance(self,new_max_instance):
        self.max_instance=new_max_instance
    def set_min_instance(self,new_min_instance):
        self.min_instance=new_min_instance
    def set_desired_instance(self,new_desired_insatance):
        self.desired_instance = new_desired_insatance
    def get_name(self):
        return self.auto_scaling_group_name_


class Autoscale(object):
    def __init__(self,name):
        if "eod" in name:
            self.log_file_name = "/home/dvctrader/animesh/grid_logs/autoscaling_eod.log"
        elif "manual" in name:
            self.log_file_name = "/home/dvctrader/animesh/grid_logs/autoscaling.log"
        else:
            self.log_file_name = "/home/dvctrader/animesh/grid_logs/autoscaling_DEFAULT.log"

        self.ec2_conn,self.as_conn=self.get_conn()
        self.grid_client_ = None
        self.asg_name=name
        self.asg_object = self.as_conn.get_all_groups([self.asg_name])[0]
        self.auto_scaling_group_object = AutoScalingGroup(self.asg_name)
        self.price_scaling_factor_ = 1.3
        self.write_log("Autoscale check initiated")
        self.max_instance_number = 10
        self.min_instance_number = 1
        self.instance_increment_number = 2
        self.last_load_number = 100000
        self.client=GridClient("http://10.1.4.15:5000", "animesh", "animesh@pass435")
        self.job_resource=JobResource()
        
    def get_conn(self):
        aws_access_key_id="AKIAIX6E2SVJSSQ4X32Q"
        aws_secret_access_key="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"
        try:
            ec2_conn = boto.connect_ec2(aws_access_key_id, aws_secret_access_key)
#             print (ec2_conn)
            as_conn = boto.connect_autoscale(aws_access_key_id, aws_secret_access_key, use_block_device_types=True)
#             print (as_conn)
            self.write_log("The connection to ec2 cluster successful")
        except Exception:
            self.write_log("The connection to ec2 cluster failed")
            sys.exit("Problem in connecting.")
        return (ec2_conn, as_conn)

    def get_min_price_(self,lookback=1800):
        min_price_ = 1000 
        current_time_ = datetime.datetime.utcfromtimestamp(time.time())
        start_time_ = current_time_ - datetime.timedelta(seconds=lookback)
        price_list = self.ec2_conn.get_spot_price_history(instance_type='r4.4xlarge',start_time=start_time.isoformat(),end_time=current_time.isoformat(),availability_zone='us-east-1a')
        for price in price_list:
            if price.price < min_price_:
                min_price_ = price.price
        return min_price_
    
    def write_log(self,string_to_write):
        now = datetime.utcnow()
        utc_time = int(now.strftime("%s"))
        with open(self.log_file_name, 'a') as the_file:
            the_file.write(str(utc_time)+" : "+string_to_write+"\n")
    
    def get_wait_time():
        pass
    
    def update_launch_config(self,create_backup_config=False,update_asg_launch_config=False):        
        min_instance_price = self.get_min_price()
        auction_price_ = min_instance_price * self.price_scaling_factor_
        present_lc = self.as_conn.get_all_launch_configurations(names=[self.asg_object.launch_config_name])[0]
        present_lc.price = auction_price_
        self.asg_object.launch_config_name = present_lc.name
        self.asg_object.min_size = 0
        self.asg_object.max_size = 40
        self.asg_object.update()
        min_instance_price = self.get_min_price()
        auction_price_ = min_instance_price * self.price_scaling_factor_
        auto_scaling_group= as_conn.get_all_groups(names=[self.asg_name])[0]
        present_lc = self.as_conn.get_all_launch_configurations(names=[auto_scaling_group.launch_config_name])[0]
        present_lc.spot_price = auction_price_
        if create_backup_config:
            new_lc_name = present_lc.name +"_"+datetime.datetime.now().strftime("%Y-%m-%d_%H-%M-%S")+"_BACKUP"
            new_lc = present_lc
            new_lc.name = new_lc_name
            new_lc.instance_type = 'r4.4xlarge'
            new_lc.block_device_mappings = [present_lc.block_device_mappings]
            new_lc.spot_price = auction_price_
            new_lc.instance_monitoring = True
            self.as_conn.create_launch_configuration(new_lc)
            self.write_log("A backup launch config created the config name ",new_lc_name)
        
        #Update the auto scaling group with the new launch config
        if update_asg_launch_config:
            self.asg_object.launch_config_name(present_lc.name)
            self.asg_object.min_size = 0
            self.asg_object.max_size = 40
            self.asg_object.update()
            self.write_log("The prod launch config has been updated")

        
        
        
        
    def scaling_logic(self):
        #get the current number of instances 
        current_number_running_instance = self.get_number_running_instance()
        self.write_log("Present number of running instance before autoscaling: " + str(current_number_running_instance))
        task_fetched = False
        wait_time_fetched = False
        try:
            queued_task = self.job_resource.queued_tasks()
            task_fetched = True
            self.write_log("The number of queued task now "+ str(queued_task))
        except:
            self.write_log("The fetch queued task failed ")

        try:
            avg_wait_time_ = self.job_resource.avg_time_tasks(30)
            wait_time_fetched = True
            self.write_log("The avg task completion time "+ str(avg_wait_time_))
        except:
            self.write_log("The fetch avg completion time failed ")

        if task_fetched and wait_time_fetched:
            load_number = (float(queued_task['tasks']) * float(avg_wait_time_["avg"]))
            self.last_load_number = load_number
        else: 
            load_number = self.last_load_number

        if queued_task['tasks'] == 0:
            new_number_running_instance = self.min_instance_number
        else:
            if load_number >= 10000 and load_number<150000 and current_number_running_instance < 3:
                new_number_running_instance = 3
            elif load_number>=150000 and load_number<350000 and current_number_running_instance < 5:
                new_number_running_instance = 5
            elif load_number>350000 and load_number<700000 and current_number_running_instance < 7:
                new_number_running_instance = 7
            elif load_number>=700000 and current_number_running_instance < self.max_instance_number:
                new_number_running_instance = self.max_instance_number
            else:
                new_number_running_instance = current_number_running_instance
            
        #check whether to scale up or down the number of instance required
        
        if new_number_running_instance < current_number_running_instance:
            self.set_number_instances(new_number_running_instance)
            self.write_log("The number of instance decreased , now the instance number is " + str(new_number_running_instance))
        elif new_number_running_instance > current_number_running_instance:
            self.set_number_instances(new_number_running_instance)
            self.write_log("The number of instance increased , now the instance number is " + str(new_number_running_instance))
        else:
            self.write_log("The number of instance is unchanged , now the instance number is "+ str(current_number_running_instance))
        self.write_log("Scaling done  \n\n")
    def set_number_instances(self,new_instance_number):
        return self.as_conn.set_desired_capacity(self.asg_name,new_instance_number)
    
    def get_number_running_instance(self):
        return len(self.asg_object.instances)
    

if __name__ == "__main__":
    name = sys.argv[1]
    auto_scale_object=Autoscale(name)
    auto_scale_object.scaling_logic()
