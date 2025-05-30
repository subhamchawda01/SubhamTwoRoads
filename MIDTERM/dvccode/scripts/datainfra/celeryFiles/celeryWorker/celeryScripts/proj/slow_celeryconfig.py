import sys
import os
import socket

sys.path.append(os.getcwd())

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

CELERYD_CONCURRENCY = 3
CELERY_QUEUES = {"slow": {"exchange": "slow", "routing_key": "slow"}}
