#!/usr/bin/python
import os
import sys
sys.path.append(os.path.expanduser('~/basetrade/'))
from random import randint
import datetime
import getpass
import traceback

if __name__ == "__main__":
    today = datetime.datetime.today().strftime('%Y%m%d')
    lookback_days = "60"
    user = str(getpass.getuser())
    output_file_name = "/media/shared/ephemeral16/animesh_trash/stdev_out_"+str(randint(0, 10000000))
    portfolio_input_file = "/spare/local/tradeinfo/PCAInfo/portfolio_inputs"
    pca_input_file = "/spare/local/tradeinfo/PCAInfo/pca_portfolio_stdev_eigen_20170306.txt"
    stdev_output_file = "/spare/local/tradeinfo/PCAInfo/shortcode_stdev_"+str(today)+".txt"
    try:
        os.system(" ".join(["/home/"+user+"/basetrade/scripts/pca_stdev_adjustment.py",portfolio_input_file,today,lookback_days,output_file_name]))
        print("The stdev out file has been generated")
        os .system(" ".join(["/home/"+user+"/basetrade/scripts/pca_db_update.py","-update_portfolio_input 1","-update_portfolio_pca 1","-update_stdev 1","-shortcode_stdev_file",output_file_name,"-portfolio_input_file",portfolio_input_file,"-portfolio_pca_weight_file",pca_input_file,"-stdev_out_file",stdev_output_file]))
        print ("The values in the DB have been updated")
        #os.system(" ".join(["cp",output_file_name,stdev_output_file]))
    except Exception as error:
        print(error)
        print(traceback.print_exc())
        print ("Stdev update failed")
    
