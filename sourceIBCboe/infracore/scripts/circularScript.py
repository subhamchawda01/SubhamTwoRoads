import requests,json
from datetime import datetime, timedelta,date
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
import socket

# the target we want to open    
url="https://www.nseindia.com/api/circulars"
#url="https://www.nseindia.com/api/circulars?fromDate=24-12-2020&toDate=24-12-2021";
PARAMS = {'fromDate':"20-12-2021",'toDate':"27-12-2020"}

headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.162 Safari/537.36'}
#resp=requests.get(url,headers=headers,params = PARAMS)
resp=requests.get(url,headers=headers)
#print(resp)
data=resp.json()
circulars=data['data']
circularSub=["Listing of Equity Shares of","Change in name of","Change in name and symbol","mock","Introduction of Futures & Options","Revision in transaction charges structure for equity market segment","Adjustment of Futures", "Disaster Recovery", "Introduction of Additional Streams", "Revision in Scheme"]

todayDate=datetime.now().strftime('%Y%m%d')
#todayDate="20220322"
yesterdayDate=(datetime.now() - timedelta(1)).strftime('%Y%m%d')
print("Date of today:",todayDate)
#print("Date of yesterday:",yesterdayDate)
#yesterdayDate=""
flag=False
ans=""

#ans="Circular Subject\t\t\t\t\t\t\t\t\tCircular Date\t\t\tDocument Link\n"
for circular in circulars:
    #print(circular)
    if circular['cirDate']==todayDate:
        #print(circular)
        for sub in circularSub:
            if sub.lower() in circular['sub'].lower() and circular['sub'].find("(SME IPO)") == -1 and circular['sub'].find("SME Emerge platform") == -1:
                #print(sub.lower(),circular['sub'].lower())
                flag=True
                ans=ans+circular['sub']+'   '+circular['cirDisplayDate']+'   '+circular['circFilelink']+'\n'
                break;

if flag==False:
    ans="No Updates!"
print(ans);
exit(0)


recipients='ravi.parikh@tworoads-trading.co.in,raghunandan.sharma@tworoads-trading.co.in,subham.chawda@tworoads-trading.co.in'
print (ans)
body = ans
msg = MIMEMultipart()
hostname=socket.gethostname()

msg['Subject'] = 'NSE Circular( '+hostname+' ) updates on '+str(date.today())
msg['From'] = 'taher.makda@tworoads-trading.co.in'
msg['To'] = (', ').join(recipients.split(','))

msg.attach(MIMEText(body,'plain'))

s = smtplib.SMTP('smtp.gmail.com', 587)
s.starttls()
s.login("taher.makda@tworoads-trading.co.in", "aassqlgwyckahtfj")
s.send_message(msg)
s.quit()


