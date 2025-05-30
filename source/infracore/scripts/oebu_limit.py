import json
import os
import time
import subprocess

slack_channel = 'nse_oebu_alerts'
slack_data_mode = 'DATA'
slack_exec = '/home/pengine/prod/live_execs/send_slack_notification'
os.environ['LD_LIBRARY_PATH'] = '/opt/glibc-2.14/lib'
os.environ['http_proxy'] = '127.0.0.1:8181'
os.environ['https_proxy'] = '127.0.0.1:8181'

notified_symbols_ = set();

def SlackNotify(data):
        command = slack_exec + ' ' + slack_channel + ' ' + slack_data_mode + ' \"' + data  + "\""
        subprocess.check_output(['bash', '-c',command]);

def main():
        while True:
                with open('/var/www/html/market_data_ind17.json') as json_file:
                        data = json.load(json_file);
                        logfilename = '/home/dvcinfra/important/IND17_OEBU_STAT/' + 'log_' + time.strftime("%Y%m%d") + '.txt';
                        logfile = open(logfilename,"a");
                        for line in data['data']:
                                traded_price = float(line[3]);
                                if float(line[3]) < 0 :
                                        traded_price = float(line[6]);
                                traded_val = traded_price * float(line[17]);            
                                if float(line[19]) >= 10 and traded_val >= 1 and float(line[13]) <= -700:
                                        if line[0] not in notified_symbols_:
                                                print('Sending slack alert for '+line[0]);
                                                SlackNotify(line[0] + '=> Traded Value : ' + str(traded_val) + " , " + '%v/V : ' + line[19] + ' Pnl : ' + str(line[13]));
                                                strdata = line[0] + '=> Traded Value : ' + str(round(traded_val,6)) + " , " + '%v/V : ' + line[19] + ' Pnl : ' + str(line[13]); 
                                                logfile.write(strdata + '\n');
                                                notified_symbols_.add(line[0]);
                        logfile.close();
                time.sleep(60); 

if __name__ == '__main__':
        main()
