#!/usr/bin/env python

'''
This script autoscales the workers according to number of messages in queue
'''

import sys
import subprocess
import json
import time
import os.path
import os
from update_config import update_config
from instance_count import instance_count
from cancel_req import cancel_req

#Configs
INITIAL_MSGS=60
INITIAL_MSGS_MANUAL=500
INCREMENT=300 #No of workers = messages/INCREMENT + 1
DELAY=120 	#Check queue after delay of 5 seconds

#Stores name of autoscaling groups
autoscale_names = []

#store last time (time since epoch) when an autoscale group was up/down-scaled
scale_log = dict()

#number of instances up/down-scaled to
num_log = dict()
max_size = dict()

FILE_PATH = os.getcwd() + '/celeryFiles/autoscale/scale.json'
LOG_PATH = os.getcwd() + '/celeryFiles/autoscale/log.txt'

log_file = open(LOG_PATH, 'a')
def exec_func(prog):
    print prog
    log_file.write(prog)
    process = subprocess.Popen(prog, shell=True, stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output, err) = process.communicate()
    #log_file.write(str(output))
    if(err != None):
        log_file.write(str(err))
    ret = process.wait()
    #log errors
    return output

#return no of instances it should scale to
def get_instances(messages, name):
   if name == 'autoscalegroupmanual' or name == 'autoscalegrouplong':
        if int(messages) <= INITIAL_MSGS_MANUAL:
            return 0
        else:
            return int((int(messages) - INITIAL_MSGS_MANUAL - 1)/INCREMENT) + 1
   if int(messages) <= INITIAL_MSGS:
        return 0
   else:
        return int((int(messages) - INITIAL_MSGS - 1)/INCREMENT) + 1


def initialize(scale_log):
	#In case no file, create one with empty dictionary
	if(not os.path.isfile(FILE_PATH)):
		scale_file = open(FILE_PATH, 'w')
		json.dump(scale_log, scale_file)
		scale_file.close()
	else:
		scale_file = open(FILE_PATH, 'r')
		scale_log = json.load(scale_file)
		scale_file.close()
        return scale_log

def scale_instances(name, num_instances):
        #print name, num_instances
        scale_log[name] = time.time()
        cancel_req(name, 40) #40 is the no of max activities to consider
	result = exec_func("aws autoscaling set-desired-capacity --auto-scaling-group-name " + name + " --desired-capacity " + str(num_instances))
	#TODO: log the result

autoscale_info = exec_func("aws autoscaling describe-auto-scaling-groups --auto-scaling-group-names")
autoscale_json = json.loads(autoscale_info)
for group in autoscale_json[unicode('AutoScalingGroups')]:
   name = group[unicode('AutoScalingGroupName')].encode()
   #num_log[name] = group[unicode('DesiredCapacity')]
   autoscale_names.append(name)
   max_size[name] = group[unicode('MaxSize')]
   num_log[name] = instance_count(name)

#Save the log of when last up-scaling, down-scaling has been done
scale_log = initialize(scale_log)

#Check Rabbitmq queues
result = exec_func("sudo rabbitmqctl list_queues -p vhostClient name messages_ready")
queues = result.splitlines()[1:]

for queue in queues:
	(name, messages) = queue.split('\t')
	if name in autoscale_names and name != 'autoscaletest':
                active_messages = exec_func("cd celeryFiles/celeryWorker/celeryScripts; /usr/local/bin/celery -A proj --config=proj.celeryconfig inspect active | grep \"'args'\" | grep \"'" + name + "'\" | wc -l")
                if active_messages == '':
                    active_messages = '0'
                else:
                    active_messages = active_messages[:-1]
                messages = str(int(messages) + int(active_messages))
                print messages, active_messages

		num_instances = get_instances(messages, name)
		log_file.write(name + " " + str(num_instances) + " " + messages + "\n")
                print name + " " + str(num_log[name] ) + " " + str(num_instances) + " " + messages + "\n"
                #num_instances = min(num_instances, int(max_size[name]))

                print "Actual, Required: ", num_log[name], num_instances
		if(num_log[name] != num_instances):
			#Upscale
			if(num_log[name] < num_instances):
                                #num_instances = min(num_instances, int(max_size[name]))
                                #in case i want to upscale more than max, just update in log
                                if num_instances > int(max_size[name]) and num_log[name] == int(max_size[name]):
                                    scale_log[name] = time.time()
                                else:
                                    num_instances = min(num_instances, int(max_size[name]))
                                    print "Upscaling to " + str(num_instances) + " from " +str(num_log[name])
                                    update_config(name)
                                    scale_instances(name, num_instances)
			#Downscale
			elif name in scale_log:
				time_since_scaling = time.time()- scale_log[name]
                                print "Downscaling time: " + str(time_since_scaling) + " " + str( 50*60 )
				if(time_since_scaling >= 50*60): #time since scaling is more than half an hour scale again
					#scale_log[name] = time.time()
					#print name, num_instances
                                        print "Downscaling to " + str(num_instances) + " from " + str( num_log[name])
                                        log_file.write(name + " " + str(num_instances) + " " + messages + "\n")
					scale_instances(name, num_instances)
                        else:
                            print "Not in log"
                            scale_log[name] = time.time()

#Update Scaling logs
scale_file = open(FILE_PATH, 'w')
json.dump(scale_log, scale_file)
scale_file.close()
