from __future__ import absolute_import
import sys
import os
import socket
import subprocess
from celery import Celery

sys.path.append(os.getcwd())

import json

def exec_bash(command):
    process = subprocess.Popen(command.split(), stdout=subprocess.PIPE)
    output = process.communicate()[0]
    return output

def getKey():
    instance_id = exec_bash('ec2-metadata -i')
    instance_id = instance_id[13:-1]
    instance_info = exec_bash('aws ec2 describe-instances --instance-ids ' + instance_id.decode())
    print( 50 *"$")
    instance_json = json.loads(instance_info.decode())
    infoarr = instance_json[str('Reservations')][0][str('Instances')][0][str('Tags')]
    for info in infoarr:
        if info[str('Key')] == str('aws:autoscaling:groupName'):
            #print info[unicode('Value')]
            return info[str('Value')].encode()

#Change this to change concurrency, by default no of cores
#CELERYD_CONCURRENCY = 1

#Disable prefecting
CELERYD_PREFETCH_MULTIPLIER = 1

#Enable retrying of lost or failed tasks
CELERY_ACKS_LATE = True

# default RabbitMQ broker
BROKER_URL = 'amqp://mainClient:mainClient@52.91.139.132:5672/vhostClient'

# default RabbitMQ backend
#CELERY_RESULT_BACKEND = 'amqp'
CELERY_RESULT_BACKEND = 'db+mysql://dvcwriter:dvcwiki@10.0.0.31/celeryDB'

#Delete a message in queue after 60 minutes
CELERY_EVENT_QUEUE_TTL = 20*60

#Delete temporary error queues
CELERY_STORE_ERRORS_EVEN_IF_IGNORED = False

#Expiry Time in secs of Celery Result Task Queue
CELERY_TASK_RESULT_EXPIRES=10*60

#kill worker executing task more than given seconds and replace with a new one
CELERYD_TASK_TIME_LIMIT=5*24*60*60 #5 days

#Track submitted tasks yet to be scheduled
CELERY_TRACK_STARTED = True
CELERY_SEND_TASK_SENT_EVENT = True
CELERY_SEND_EVENTS = True

#To avoid MySQL server gone away
CONN_MAX_AGE = 5*24*60*60

#To avoid MySQL server has gone away error
CELERY_RESULT_DB_SHORT_LIVED_SESSIONS = True

CELERY_ROUTES = {
    'chord_unlock'  : {'queue': 'test', 'routing_key': 'test'}
}

CELERYD_CONCURRENCY = 25
CELERY_QUEUES = {}
hostname = socket.gethostname()
qname = getKey()
print(qname)
if hostname == 'ip-10-0-1-46' or hostname == 'ip-10-0-1-79' or qname == "autoscalegroup":
	CELERY_QUEUES = {"autoscalegroup": {"exchange": "autoscalegroup", "routing_key": "autoscalegroup"}}
else:
	CELERY_QUEUES = {"autoscalegroupmanual": {"exchange": "autoscalegroupmanual", "routing_key": "autoscalegroupmanual"}}

