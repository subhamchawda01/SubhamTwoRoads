#!/usr/bin/env python
from proj.workerProg import add_duration
import sys

if len(sys.argv) < 3:
    print("Usage: " + sys.argv[0] + " <duration_time> <duration_tag>")
    sys.exit(1)

duration_time = float(sys.argv[1])
duration_tag = " ".join(sys.argv[2:])

queue_name = 'duration'
print(duration_time, duration_tag)
result = add_duration.apply_async(args=[duration_time, duration_tag], queue=queue_name, routing_key=queue_name, retry=True, retry_policy={'max_retries' : 3})
