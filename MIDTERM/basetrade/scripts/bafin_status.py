# -*- coding: utf-8 -*-

from selenium import webdriver
from selenium.webdriver.support.ui import Select
from selenium.webdriver.common.keys import Keys
from datetime import datetime
import os
import sys
import getpass
import os.path

if len(sys.argv) < 2:
    print("USAGE: python script.py YYYYMMDD  [ Send Slack Notif: Y/N ]")
    exit()

send_slack = 'Y'

if len(sys.argv) > 2:
    send_slack = sys.argv[2]

print("Send Slack Notif ? : " + str(send_slack))

YYYYMMDD = sys.argv[1]
input_date = "DV" + str(datetime.strptime(YYYYMMDD, "%Y%m%d").strftime("%y%m%d"))
slack_exec = "/home/" + getpass.getuser() + "/send_slack_notification"

if not os.path.isfile(slack_exec):
    cmd = "scp dvctrader@10.23.74.55:/home/dvctrader/LiveExec/bin/send_slack_notification " + slack_exec
    os.system(cmd)

user = "gaurav.chakravorty5390"
passwd = "f9fRrJYFK3"
url = "https://portal.mvp.bafin.de/MvpPortalWeb/app/home.html"

browser = webdriver.Firefox()
browser.get(url)

username = browser.find_element_by_name('j_username')
username.send_keys(user)
password = browser.find_element_by_name('j_password')
password.send_keys(passwd)
username.submit()

view_journal_link = browser.find_element_by_link_text('Protokoll einsehen')
view_journal_link.click()

tr = browser.find_elements_by_tag_name('tr')

success = False
submitted = False

print("REGEX: " + input_date)

for row in tr:
    print("ROWTEXT: " + row.text.encode('utf-8'))
    if input_date in row.text:
        print("FOUND REGEX:  " + input_date + " in row text")
        submitted = True
        if "Meldung akzeptiert" in row.text:
            print("FOUND SUBMITTED in row text with " + input_date)
            success = True

last_submitted_date = "NULL"

if success == False:
    for row in tr:
        if "Meldung akzeptiert" in row.text:
            start_index = row.text.find("P9WPHG") + 9
            end_index = start_index + 6
            last_submitted_date = datetime.strptime(row.text[start_index:end_index], "%y%m%d").strftime("%Y%m%d")
            break

cmd = ""

if success == True:
    cmd = slack_exec + " dvc-audits DATA " + "\"SUCCESS: Bafin Report submitted for " + YYYYMMDD + "\""
else:
    if submitted == True:
        cmd = slack_exec + " dvc-audits DATA " + "\"FAILURE: Bafin Report submitted but not accepted yet for " + \
            YYYYMMDD + ", Last successful submission date :" + last_submitted_date + "\""
    else:
        cmd = slack_exec + " dvc-audits DATA " + "\"FAILURE: Bafin Report not submitted for " + \
            YYYYMMDD + ", Last successful submission date :" + last_submitted_date + "\""

print(cmd)

if send_slack == 'Y':
    os.system(cmd)

browser.close()
