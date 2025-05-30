from getPrice import get_results
from getPrice import exec_func
import sys

seconds = 0
inc = 0.1
ami_id = 'ami-1742d000'

if len(sys.argv) > 1:
    seconds = int(sys.argv[1])
else:
    seconds = 100

#instance_types = ["t1.micro", "m1.small", "m1.medium", "m1.large", "m1.xlarge", "m3.medium", "m3.large", "m3.xlarge", "m3.2xlarge", "m4.large", "m4.xlarge", "m4.2xlarge", "m4.4xlarge", "m4.10xlarge", "t2.micro", "t2.small", "t2.medium", "t2.large", "m2.xlarge", "m2.2xlarge", "m2.4xlarge", "cr1.8xlarge", "i2.xlarge", "i2.2xlarge", "i2.4xlarge", "i2.8xlarge", "hi1.4xlarge", "hs1.8xlarge", "c1.medium", "c1.xlarge",  "c3.large", "c3.xlarge", "c3.2xlarge", "c3.4xlarge", "c3.8xlarge", "c4.large", "c4.xlarge", "c4.2xlarge", "c4.4xlarge", "c4.8xlarge", "cc1.4xlarge", "cc2.8xlarge", "g2.2xlarge", "cg1.4xlarge", "r3.large", "r3.xlarge", "r3.2xlarge", "r3.4xlarge", "r3.8xlarge", "d2.xlarge", "d2.2xlarge", "d2.4xlarge", "d2.8xlarge"]

def log(logstr):
    print logstr

def get_min_price():
    instance_types = ["c3.8xlarge", "cc2.8xlarge", "d2.8xlarge"]
    min_val = 10
    min_instance = ""
    for instance in instance_types:
        res = get_results(seconds, instance)
        if len(res) > 0:
            print instance, res[0], sum(res)/float(len(res))
            print res
            if res[0] < min_val:
                min_val = res[0]
                min_instance = instance
    return (min_val, min_instance)

def get_min_price_lower():
    instance_types = ["m3.2xlarge", "c3.2xlarge"]
    min_val = 10
    min_instance = ""
    for instance in instance_types:
        res = get_results(seconds, instance)
        if len(res) > 0:
            print instance, res[0], sum(res)/float(len(res))
            print res 
            if res[0] < min_val:
                min_val = res[0]
                min_instance = instance
    return (min_val, min_instance)

def update_config(group_name):
    #if group_name == 'autoscalegroupmanual' or group_name == 'autoscalegrouplong':
        #(val, instance) = get_min_price_lower()
    #else:
    (val, instance) = get_min_price()
    log("Min Price and Instance Type " + str(val) + " " + instance)
    if val <= 0.7:
        prev_name = exec_func('aws autoscaling describe-auto-scaling-groups --auto-scaling-group-name ' + group_name + ' | grep "\"LaunchConfigurationName\""')
        prev_name = (prev_name.splitlines()[-1])[40:][:-3]
        #output = exec_func('aws autoscaling describe-launch-configurations | grep \"autoscaleconfig_' + group_name + '\"')
        #Bug: What if both of them are present, find out current config and dont use that name
        if prev_name != 'autoscaleconfig_'+group_name:
            config_name = 'autoscaleconfig_'+group_name
        else:
            config_name = 'autoscaleconfig2_'+group_name

        log('Config name: ' + config_name)
        
        launch_cmd = 'aws autoscaling create-launch-configuration --launch-configuration-name ' + config_name + ' --image-id ' + ami_id + ' --instance-type ' + instance + ' --spot-price "' + str(val + inc) + '" --security-groups sg-b49f44db --key-name ec2-rsa --instance-monitoring Enabled=false --block-device-mappings "[{\\\"DeviceName\\\":\\\"/dev/sdb\\\",\\\"VirtualName\\\":\\\"ephemeral0\\\"},{\\\"DeviceName\\\":\\\"/dev/sdc\\\",\\\"VirtualName\\\":\\\"ephemeral1\\\"}, {\\\"DeviceName\\\":\\\"/dev/sdd\\\",\\\"VirtualName\\\":\\\"ephemeral2\\\"}, {\\\"DeviceName\\\":\\\"/dev/xvda\\\",\\\"Ebs\\\":{\\\"DeleteOnTermination\\\":true,\\\"VolumeSize\\\":10,\\\"VolumeType\\\":\\\"gp2\\\"}}]"'
        
        #Delete Previous Configs with same name
        exec_func('aws autoscaling delete-launch-configuration --launch-configuration-name ' + config_name)
        
        #Create new config
        exec_func(launch_cmd)
        
        log('Previous Config Name ' + prev_name)

        exec_func('aws autoscaling update-auto-scaling-group --auto-scaling-group-name ' + group_name + ' --launch-configuration-name ' + config_name)

        exec_func('aws autoscaling delete-launch-configuration --launch-configuration-name ' + prev_name)    

#update_config('autoscale-test')
