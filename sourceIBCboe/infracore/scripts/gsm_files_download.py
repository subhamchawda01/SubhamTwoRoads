import sys
import requests
import csv
baseurl = "https://www.nseindia.com/"
filename=sys.argv[1]
url=sys.argv[2]
headers = {"Referer":"https://www.nseindia.com", 'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.4896.127 Safari/537.36','accept-language': 'en,gu;q=0.9,hi;q=0.8'}
objNseResponse = requests.get(baseurl, headers=headers, timeout=5)
cookiejar = objNseResponse.cookies
response = requests.get(url, headers=headers, timeout=5, cookies=cookiejar)
arrNseList = [[record] for record in response.content.decode("utf-8-sig").split("\n")]
with open(filename, 'w') as csvfile:
   csvwriter=csv.writer(csvfile, delimiter="'",quotechar = "'") 
   csvwriter.writerows(arrNseList)
