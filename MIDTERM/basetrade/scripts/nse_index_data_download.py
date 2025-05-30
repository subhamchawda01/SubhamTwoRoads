from selenium import webdriver
import time
import os
import errno
import sys
from selenium.common.exceptions import NoSuchElementException
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import TimeoutException

current_day_string = time.strftime("%d%m%y")
# change these two as required
download_dir = "/spare/local/tradeinfo/NSE_Files/IndexInfo/compressed_files/"
download_button_str_nifty = 'N' + current_day_string + '.zip'
download_button_str_banknifty = 'CB' + current_day_string + '.zip'

if not os.path.exists(download_dir):
    try:
        os.makedirs(download_dir)
    except Exception as e:
        print("Error : " + str(e))
        sys.exit(1)

# adding these preferences otherwise the SAVE button pops up which can't be clicked through selenium
fp = webdriver.FirefoxProfile()
fp.set_preference("browser.download.folderList", 2)
fp.set_preference("browser.download.dir", download_dir)
fp.set_preference("browser.download.manager.showWhenStarting", False)
fp.set_preference("browser.helperApps.neverAsk.saveToDisk", "APPLICATION/OCTET-STREAM")

url = "https://www.connect2nse.com/iislNet/"
username_str = "Two_Roads"
password_str = "Two_Roads123"

browser = webdriver.Firefox(firefox_profile=fp)

browser.get(url)

username = browser.find_element_by_name('username')
password = browser.find_element_by_name('password')

username.send_keys('Two_Roads')
password.send_keys('Two_Roads123')

username.submit()

try:
    WebDriverWait(browser, 3).until(EC.alert_is_present(),
                                    'Timed out waiting for PA creation ' +
                                    'confirmation popup to appear.')

    alert = browser.switch_to_alert()
    alert.accept()
    print("alert accepted")
except TimeoutException:
    print("no alert")

try:
    view_folder_link = browser.find_element_by_link_text('View Folder')
    view_folder_link.click()
    nifty_50_link = browser.find_element_by_link_text('Nifty 50')
    nifty_50_link.click()
    download_button = browser.find_element_by_link_text(download_button_str_nifty)
    download_button.click()
except NoSuchElementException as e:
    print("Error in finding Nifty index constituent file")

try:
    view_folder_link = browser.find_element_by_link_text('View Folder')
    view_folder_link.click()
    banknifty_50_link = browser.find_element_by_link_text('Nifty Bank')
    banknifty_50_link.click()
    download_button = browser.find_element_by_link_text(download_button_str_banknifty)
    download_button.click()
except NoSuchElementException as e:
    print("Error in finding BankNifty index constituent file")

browser.close()
