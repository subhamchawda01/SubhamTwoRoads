#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import jinja2
import subprocess
import pandas as pd
import datetime as dt
from jinja2 import Environment, FileSystemLoader, Template

HTML_REPORT_TEMPLATE = 'html_report_template.html'
HTML_OUT_FILE = '/home/dvctrader/trash/earnings_report.html'
HTML_REPORT_TITLE = 'Earnings Report'
OEBU_PRODUCTS_FILE = '/spare/local/files/oebu_product_list.txt'
me = 'sanjeev.kumar@tworoads-trading.co.in'
#you = ['sanjeev.kumar@tworoads-trading.co.in']
you = ['sanjeev.kumar@tworoads-trading.co.in','nseall@tworoads.co.in ']

def ExecBash(command):
    command = command.replace('&', '\&')
    return subprocess.check_output(['bash', '-c',
                                   command]).decode('UTF-8')


def DumpEarnings(df_):
    env = Environment(loader=FileSystemLoader('/home/dvctrader/important/'))
    template = env.get_template(HTML_REPORT_TEMPLATE)
    template_vars = {'stats_table': df_.to_html()}
    html = template.render(template_vars)
    file_ = open(HTML_OUT_FILE, 'w')
    file_.write(html)
    file_.flush()
    file_.close()


def SendEarningsReport(df):
    DumpEarnings(df);
    subject = 'Earnings Report';
    command = \
        '(echo To: "%s" ; echo From: "%s"; echo Subject: "%s"; \
				echo "Content-Type: text/html;";cat %s) | /usr/sbin/sendmail -t' \
        % (' '.join(you), me, subject, HTML_OUT_FILE)
    ExecBash(command)


def GetOebuProducts():
    df = pd.read_csv(OEBU_PRODUCTS_FILE,names=['Instrument']);
    return df;

def main():
    date = dt.datetime.today().strftime('%Y%m%d')
    earnings_file = '/spare/local/files/NSE/MidTermEarningsLogs/upcoming_earnings.csv'
    df = pd.read_csv(earnings_file, delim_whitespace=True, names=['Instrument','Date'])
    df = df.sort(columns='Date',ascending=True).reset_index(drop=True);
    oebu_prod_df_ = GetOebuProducts();
    df = df[ df['Instrument'].isin(oebu_prod_df_['Instrument']) ];
    SendEarningsReport(df)

if __name__ == '__main__':
	main()
