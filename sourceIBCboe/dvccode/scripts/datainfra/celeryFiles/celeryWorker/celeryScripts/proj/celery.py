
from celery import Celery

# instantiate Celery object
app = Celery(include= [
			 'proj.workerProg'	
			])

# import celery config file
# app.config_from_object('proj.celeryconfig')

if __name__ == '__main__':
   app.start()


