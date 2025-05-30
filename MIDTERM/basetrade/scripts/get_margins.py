#!/usr/bin/env python

import sys
import os
import http.client
import urllib.request
import urllib.parse
import urllib.error
import urllib.request
import urllib.error
import urllib.parse
from bs4 import BeautifulSoup

if(len(sys.argv) < 2):
    print("USAGE: ", sys.argv[0], "date")
    exit(1)

date = int(sys.argv[1])
if(date < 20100101 or date > 20141231):
    print("INVALID Date: ", date)

str_date = str(date)[6:] + "/" + str(date)[4:6] + "/" + str(date)[0:4]

data = dict(sltCenario="1", lstDataPregao=str_date)
#data = dict(txtUltimaDataValidaPregao="07/05/2014", txtDataEncontrada="NAO", txtDataAnterior="05/05/2014", sltCenario="1", lstDataPregao="05/05/2014", txtDataOutros="07/05/2014")
data = urllib.parse.urlencode(data)
req = urllib.request.Request(
    'http://www2.bmf.com.br/pages/portal/bmfbovespa/boletim1/MargensCenarios1.asp?pagetype=pop', data)
response = urllib.request.urlopen(req)
the_page = response.read()
soup = BeautifulSoup(the_page)

first = 1
t = soup.find('table', {"class": "tabConteudo"})
if (t is None):
    exit(0)
for r in t.findAll('tr'):
    if(first):
        first = 0
        continue
    cfirst = 0
    for c in r.findAll('td'):
        val = int(c.find(text=True).replace(",", ""))
        if(cfirst):
            val = val / 1000.0
        print(val, "\t", end=' ')
        cfirst = 1
    print()
