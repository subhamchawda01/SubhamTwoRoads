FROM ubuntu

MAINTAINER Diwakar 

#install libraries
RUN apt-get update
RUN apt-get install -y python

#setup the users
RUN useradd -m testuser
USER testuser



RUN echo $HOME
ADD ./scripts/run_tests.py $HOME/basetrade/basetrade/Tests/scripts/run_tests.py
CMD [ $HOME/basetrade/Tests/scripts/run_tests.py ]
#RUN source /media/shared/ephemeral14/py2_7/bin/activate
