import subprocess
import json

def cancel_req(groupname, maxitems):
   res = subprocess.Popen("aws autoscaling describe-scaling-activities --auto-scaling-group-name " + groupname + " --max-items " + str(maxitems), shell = True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
   [output, err] = res.communicate()
   jobj = json.loads(output)
   arr = jobj[u'Activities']

   for activity in arr:
      if activity[u'StatusCode'] == "WaitingForSpotInstanceId":
         req = activity[u'StatusMessage'].split()[4]
         req = req[:-1].encode()
         res = subprocess.Popen("aws ec2 cancel-spot-instance-requests --spot-instance-request-ids " + req, shell = True, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
         [output, err] = res.communicate()
