#!/usr/bin/python
import pandas as pd
import numpy as np
import os
import subprocess
import shutil
import getpass
from datetime import datetime
import sys
from random import randint
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from email.mime.multipart import MIMEMultipart




def check_cmd_line_args():
    if len(sys.argv)<3:
        print("USAGE: portfolio_input_file end_date lookback_days stdev_output_file")
        sys.exit(1)
    else:
        return True


def read_shortcodes_portfolio_file(portfolio_file):
    shortcode_file_=os.popen(" ".join(["cat",portfolio_file,"|grep -v NSE|grep -v HK|cut -d' ' -f3-"]))
    temp_dict={}
    for line in shortcode_file_.readlines():
        temp_data=line.rstrip().split(' ')
        for elem in temp_data:
            temp_dict[elem]=1
    return temp_dict.keys()


def get_ilist_file(shortcode):    
    ilist_file_path=OUTPUT_DIR+"_"+shortcode+"_ilist"
    with  open(ilist_file_path,"w") as file_handle:
        file_handle.write("MODELINIT DEPBASE "+shortcode+" MktSizeWPrice MktSizeWPrice")
        file_handle.write("\n")
        file_handle.write("MODELMATH LINEAR CHANGE")
        file_handle.write("\n")
        file_handle.write("INDICATORSTART")
        file_handle.write("\n")
        file_handle.write("INDICATOR 1.0 SimpleTrend "+shortcode+" 300 MktSizeWPrice")
        file_handle.write("\n")
        file_handle.write("INDICATOREND")
        file_handle.write("\n")
    return ilist_file_path
    



def create_data_gen_command(shortcode,end_date):
    today_date = datetime.today().strftime('%Y%m%d')
    #create a directory for the shortcode
    output_data_dir_shortcode = os.path.join(OUTPUT_DIR,shortcode)
    if os.path.exists(output_data_dir_shortcode):
        shutil.rmtree(output_data_dir_shortcode)
        os.mkdir(output_data_dir_shortcode)
    else:
        os.mkdir(output_data_dir_shortcode)
    
    
    ilist_file_path = get_ilist_file(shortcode)
    print ("The ilist file for the shortcode: ",shortcode," is ",ilist_file_path) 
    #check whether the directory for the shortcode exists or not, if not raise an error
    
    if not os.path.exists(os.path.join(OUTPUT_DIR,shortcode)):
        raise ValueError('The directory does not exists: ',os.path.join(OUTPUT_DIR,shortcode))
        sys.exit(1)
    output_cmd_list=[]
    
    #get the list of dates between the two times
    get_dates_shortcode_script = os.path.join(BASETRADE_REPO,"scripts","get_dates_for_shortcode.pl")
    cmd = " ".join([get_dates_shortcode_script,shortcode,str(LOOKBACK_DAYS),end_date])
#     print cmd
    out=os.popen(cmd)
    output = os.popen(cmd)
    
    for line in output.readlines():
        dates_list = line.rstrip().split(' ')

    #remove the exchange holidays
    date_list = [dt for dt in dates_list if not is_exchange_holiday(shortcode,dt)]
    datagen_exec = os.path.join(BASETRADE_INSTALL_DIR,"datagen")
    start_time,end_time = get_trading_start_end_time(shortcode)



    datagen_cmd_print = True
    if start_time==-1 or end_time==-1:
        return None
    else:
        datagen_cmd_list=[]
        for dt in dates_list:
            cmd = [datagen_exec,ilist_file_path,str(dt),start_time,end_time,str(randint(1,1000000)),output_data_dir_shortcode+"/out_"+dt,"100 0 0 1"]
            if datagen_cmd_print:
                print (" ".join(cmd))
                datagen_cmd_print = False
            datagen_cmd_list.append(" ".join(cmd))
        return datagen_cmd_list



def get_trading_start_end_time(shortcode):
    exchange = get_product_exchange(shortcode)
#     print BASETRADEINFODIR
    if exchange != -1:
        trade_break_start_end_file = BASETRADEINFODIR+"/NewLRDBBaseDir/"+exchange.lower()+"-trd-hours.txt"
#         print trade_break_start_end_file
        if not (os.path.exists(trade_break_start_end_file)):
            return -1,-1
        cmd=" ".join(["cat",trade_break_start_end_file,"|grep",shortcode])
        print (cmd)
        output_line = os.popen(cmd)
        for line in output_line.readlines():
#             _,start_time,end_time,time_ezone,break_start_time,break_end_time=line.rstrip().split(' ')
            temp_data=line.rstrip().split()
            start_time = temp_data[1]
            end_time = temp_data[2]
            time_zone = temp_data[3]
            if start_time=="" or end_time=="" or time_zone=="":
                return -1,-1
    else:
        #raise("The traded exchange cannot be found for the shortcode: ",shortcode)
        return -1,-1
        
        
    #check if the start time is after the end time then the start time is for the prev day
    
    #convert the time string to minutes from midnight
    try:
        start_time_minutes = convert_to_minutes_to_midnight(start_time)
        end_time_minutes = convert_to_minutes_to_midnight(end_time)
        
        #check if : in time or not
        if ":" in start_time:
            start_time = "".join(start_time.split(":"))
            
        if ":" in end_time:
            end_time = "".join(end_time.split(":"))
        
        if start_time_minutes >= end_time_minutes:
            return "PREV_"+time_zone+"_"+start_time,time_zone+"_"+end_time
        else:
            return time_zone+"_"+start_time,time_zone+"_"+end_time
    except:
        return -1,-1

def convert_to_minutes_to_midnight(time_string):
    print ("time string: ",time_string)
    hour,minutes=time_string.split(":")
    return 60*int(hour)+int(minutes)
            
    
def is_exchange_holiday(shortcode,date):
    holiday_manager_script = LIVE_EXEC_DIR+"/"+"holiday_manager"
    cmd = " ".join([holiday_manager_script,"PRODUCT",shortcode,date,"T"])
    output_line = os.popen(cmd)
    for line in output_line.readlines():
        temp_data=line.rstrip()
        return True if temp_data==1 else False

def get_product_exchange(shortcode):
    get_contract_spec_exec = "/home/pengine/prod/live_execs/get_contract_specs"
    output_cmd = " ".join([get_contract_spec_exec,shortcode,"20170907","ALL","|grep EXCHANGE:|cut -d\" \" -f2"])
#     print output_cmd
    output_file=os.popen(output_cmd)
    flag=False
    for line in output_file.readlines():
        temp_data=line.rstrip().split(' ')
        flag=True
    
    if flag==True:
        return temp_data[0]
    elif flag==False:
        return -1


def get_stdev(shortcode):
    output_data_dir_shortcode = os.path.join(OUTPUT_DIR,shortcode)
    output_file_list = []
    print (output_data_dir_shortcode)
    print (os.listdir(output_data_dir_shortcode))
    for fl in os.listdir(output_data_dir_shortcode):
        output_file_name = os.path.join(output_data_dir_shortcode,fl)
        print (output_file_name)
        statinfo = os.stat(output_file_name)
        size = statinfo.st_size
        # check size of the file if the file is empty then skip that file
        if size < 1000:
            continue
        else:
        #check the size of the file, if the size of the file is small then ignore
            output_file_list.append(pd.read_csv(output_file_name,sep=" ").as_matrix())
    return np.std(np.row_stack(output_file_list)[:,-1])



def check_whether_stdev_computation_fail(shortcode):
    #check for datagen fail
    output_data_dir_shortcode = os.path.join(OUTPUT_DIR,shortcode)
    data_fail_count = 0.0
    total_data_file_count=0.0
    for fl in os.listdir(output_data_dir_shortcode):
        total_data_file_count+=1
        statinfo = os.stat(os.path.join(output_data_dir_shortcode,fl))
        if statinfo.st_size < 1000:
            data_fail_count+=1
            continue
    #return True
    print ("Data fail count: ",data_fail_count)
    print ("Total data file: ",total_data_file_count)
    try: 
        print ("Ration of the two: ",(data_fail_count/total_data_file_count))
        print ("\n\n")
        return True if (1- (data_fail_count/total_data_file_count))<0.50 else False
    except:
        return True


def main():
    portfolio_list = read_shortcodes_portfolio_file(PORTFOLIO_FILE)
    successful_shortcode_dict={}
    failed_shortcode_dict={}
    for shortcode in portfolio_list:
        #if shortcode not in ["FGBX_0","FGBL_0","FGBM_0"]:
         #   continue
        #print (shortcode)
        #create the ilist file
        try:
            get_ilist_file(shortcode)
            #create the datagen command
            datagen_cmd_list = create_data_gen_command(shortcode,END_DATE)
            if datagen_cmd_list==None:
                failed_shortcode_dict[shortcode]=1
                print("Stdev not computed for the shortcode: ",shortcode)
                continue
            #run the datagen command
            log_file = os.path.join(OUTPUT_DIR,shortcode,"log_file")
#             print (log_file)
            for cmd in datagen_cmd_list:
                process = subprocess.Popen(cmd.split(" "), stdout=subprocess.PIPE)
                process.communicate()
            #check whether the stdev generation failed 
            if check_whether_stdev_computation_fail(shortcode) == True:
                failed_shortcode_dict[shortcode]=1
                print("Stdev not computed because datagen file not generated for the shortcode: ",shortcode)
                continue
            else:
                #get the stdev value
                print (get_stdev(shortcode))
                successful_shortcode_dict[shortcode]=get_stdev(shortcode)
            #write the successful pair in the file
        except:
            print("Stdev not computed for the shortcode: ",shortcode)
            failed_shortcode_dict[shortcode]=1
    with open(STDEV_OUTPUT_FILE,"w") as output_file_handle:
        for shortcode,stdev in successful_shortcode_dict.items():
            output_file_handle.write(" ".join(["SHORTCODE_STDEV",str(shortcode),str(stdev)]))
            output_file_handle.write("\n")


    #send mail for failed pair
    if len(failed_shortcode_dict.keys())>0:
        failed_shortcode_dict = dump_failed_stdev_values(failed_shortcode_dict)
        mail_string=""
        for shortcode,stdev in failed_shortcode_dict.items():
            mail_string+=str(shortcode)
            mail_string+="<br>"
        send_email("animesh.singh@tworoads.co.in",mail_string)
    #for failed pair writing the old values
        print("For failed pair updating the old stdev values")


def dump_failed_stdev_values(failed_shortcode_dict):
    latest_stdev_file = fetch_latest_file()
    print ("The latest stdev file name: ",latest_stdev_file)
    old_stdev_dict={}
    with open(latest_stdev_file) as old_stdev_file:
        for line in old_stdev_file.readlines():
            temp_data=line.rstrip().split(' ')
            old_stdev_dict[str(temp_data[1])] = temp_data[2]
    #write the old stdev values for failed shortcodes
    for shortcode in failed_shortcode_dict.keys():
        with open(STDEV_OUTPUT_FILE,"a") as output_file_handle:
            try:
                output_file_handle.write(" ".join(["SHORTCODE_STDEV",str(shortcode),str(old_stdev_dict[shortcode])]))
                output_file_handle.write("\n") 
            except:
                failed_shortcode_dict[shortcode] = 1
    return failed_shortcode_dict
    

def fetch_latest_file():
    file_list=[]
    for fl in os.listdir(PCAINFO):
        if "shortcode_stdev_" in fl and "longevity" not in fl and "DEFAULT" not in fl:
            date = fl.split("_")[2].split(".")[0]
            file_list.append((int(date),fl))
    file_list.sort(key=lambda x: x[0],reverse=True)
    return os.path.join(PCAINFO,file_list[0][1])

def send_email(mail_address, mail_body):
    msg = MIMEMultipart()
    msg["To"] = mail_address
    msg["From"] = mail_address
    msg["Subject"] = "PCA Stdev Info Failed Pairs"
    msg.attach(MIMEText(mail_body, 'html'))


    mail_process = subprocess.Popen(["/usr/sbin/sendmail", "-t", "-oi"],
                                    stdin=subprocess.PIPE,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
    out, err = mail_process.communicate(str.encode(msg.as_string()))
    if out is not None:
        out = out.decode('utf-8')
    if err is not None:
        err = err.decode('utf-8')
    errcode = mail_process.returncode

if __name__ == "__main__":
    if check_cmd_line_args() == True :
        PORTFOLIO_FILE=sys.argv[1]
        OUTPUT_DIR = "/media/shared/ephemeral16/pca_stdev_dir/output_"+str(randint(0,10000))
        #OUTPUT_DIR = "/media/shared/ephemeral16/pca_stdev_dir/output_8840"
        BASETRADE_INSTALL_DIR = os.path.join("/home",getpass.getuser(),"basetrade_install","bin")
        BASETRADEINFODIR="/spare/local/tradeinfo/"
        BASETRADE_REPO = os.path.join("/home",getpass.getuser(),"basetrade/")
        LIVE_EXEC_DIR=os.path.join("/home",getpass.getuser(),"LiveExec","bin")
        END_DATE = sys.argv[2]
        LOOKBACK_DAYS = sys.argv[3]
        STDEV_OUTPUT_FILE = sys.argv[4]
        PCAINFO="/spare/local/tradeinfo/PCAInfo"
        print("OUTPUT DIR: ",OUTPUT_DIR)

        #if output directory doesnot exists then remove and create a new one

        if os.path.exists(OUTPUT_DIR):
            shutil.rmtree(OUTPUT_DIR)
            os.mkdir(OUTPUT_DIR)
        else:
            os.mkdir(OUTPUT_DIR)
        main()
