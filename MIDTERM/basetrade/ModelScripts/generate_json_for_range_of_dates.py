import json
from datetime import timedelta, date
import sys

if len(sys.argv) < 3:
    print "Usage:<script><start date><end date>"
    exit(1)


def daterange(start_date, end_date):
    for n in range(int((end_date - start_date).days)):
        yield start_date + timedelta(n)


start_date = sys.argv[1]
end_date = sys.argv[2]

start_date = date(int(start_date[:4]), int(start_date[4:6]), int(start_date[6:]))
end_date = date(int(end_date[:4]), int(end_date[4:6]), int(end_date[6:]))

d = {}
for date in daterange(start_date, end_date):
    if date.isoweekday() in xrange(1, 6):
        d[date.strftime("%Y%m%d")] = 0.0

print json.dumps(d)
