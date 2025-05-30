#!/usr/bin/python

import os
import sys
import re
import time
import argparse
from random import random
import subprocess
from datetime import datetime


script="~/basetrade_install/bin/economic_events_of_the_day";
if len(sys.argv)<3:
    print("Usage : <script> <date|TODAY> <eventzone> <degree 0/1/2/3 =0>")
    print("Example: ")
    sys.exit(1)
date=str(sys.argv[1]);
eventZone=sys.argv[2];
degree=str(0);
if len(sys.argv)==4:
    degree=str(sys.argv[3]);


cmd=[script,date,"|grep "+eventZone, "| awk '{if($6>="+degree+") print $0}' | wc -l"];
#os.system("~/basetrade_install/bin/economic_events_of_the_day +str(date) |grep USD | wc -l >")

feat = subprocess.Popen(' '.join(cmd), shell=True,
                                  stderr=subprocess.PIPE,
                                  stdout=subprocess.PIPE);

out, err =feat.communicate();
if out is not None:
    out = out.decode('utf-8')
if err is not None:
    err = err.decode('utf-8')
#errcode = process.returncode
if len(err) > 0:
    raise ValueError("Error in retrieving events")

print(out);
