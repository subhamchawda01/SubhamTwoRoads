
from celery import Celery

# instantiate Celery object
app = Celery(include= [
			 'proj.workerProg'	
			])

# import celery config file
# Client needs to import some properties
app.config_from_object('proj.celeryconfig')

if __name__ == '__main__':
   app.start()


