#!/usr/bin/python

from datetime import date
import sys
import os
import subprocess
import math
import operator
import heapq
import argparse
import getpass
import time

def changeIndicator(indicator_,price_type):
    ind_wordvec_=indicator_.rstrip().split();
    price_type_list=["OfflineMixMMS","MktSizeWPrice","OrderWPrice","MktSinusoidal","TradeMktSizeWPrice","OnlineMixPrice","MidPrice"]
    if len(ind_wordvec_)>2:
     ind_wordvec_[1]=str(1.0)
     if "#" in ind_wordvec_[0]:
         return "";
    else:
        return "";
    new_ind_wordvec_=[];
    for word_ in ind_wordvec_:
        if word_ in price_type_list:
            word_=price_type;
        if "#" in word_:
            break;
        new_ind_wordvec_.append(word_);
    new_indicator_=" ".join(new_ind_wordvec_);
    return new_indicator_;

def addPriceType(indicator_,price_type):
    new_indicator_ = indicator_+" "+price_type;
    new_indicator_ = new_indicator_.strip();
    return new_indicator_;

def removePriceType(indicator_,price_type):
    new_indicator_=indicator_;
    new_indicator_.replace(price_type,"");
    new_indicator_=new_indicator_.strip();
    return new_indicator_;


def concat_ilists_uniquely(ilist1,ilist2,price_type):
    ilist_concat=[];
    for indicator_ in ilist1:
        new_indicator_ = changeIndicator(indicator_, price_type);
        if new_indicator_ not in ilist_concat and removePriceType(new_indicator_,price_type) not in ilist_concat and addPriceType(new_indicator_,price_type) not in ilist_concat:
            ilist_concat.append(new_indicator_)
    for indicator_ in ilist2:
        new_indicator_ = changeIndicator(indicator_, price_type);
        if new_indicator_ not in ilist_concat:
            ilist_concat.append(new_indicator_)
    return ilist_concat

def generate_indicators_from_pool(shortcode,poolStartTime, poolEndTime,price_type):
        pool_configs_ = os.popen("~/basetrade/scripts/fetch_strats_from_pool.sh "+shortcode+" "+poolStartTime+" "+poolEndTime).read();
        pool_config_vec_=pool_configs_.strip().split("\n");
        #print(pool_configs_);
        indicator_list_=[];
        for config_ in pool_config_vec_:
            model_=os.popen("~/basetrade/scripts/show_model_from_base.sh "+config_+ " | grep INDICATOR | grep -v INDICATORSTART | grep -v INDICATOREND | grep -v REGIMEINDICATOR | grep -v INDICATORINTERMEDIATE").read();


            for indicator_ in model_.rstrip().split("\n"):
                new_indicator_=changeIndicator(indicator_,price_type);
                if new_indicator_ == "":
                    continue;
                if new_indicator_ not in indicator_list_:
                    indicator_list_.append(new_indicator_);
        return indicator_list_;

def generate_indicators_from_config(config, outputilistFile):
        os.system("~/basetrade/ModelScripts/create_ilist_from_config.pl "+config+" "+outputilistFile);
        ilist=os.popen("cat "+outputilistFile+"| grep INDICATOR | grep -v INDICATORSTART | grep -v INDICATOREND ").read();
        ilist=ilist.rstrip().split("\n");
        return ilist;

def generate_indicator_list(config,mode=0):
    file=open(config,"r");
    configOutput=file.read();
    file.close();
    price_type="MktSizeWPrice";
    configOutput_vec_=configOutput.split("\n");
    poolStartTime=""
    poolEndTime=""
    ilist1=[]
    ilist2=[]
    outstring=""
    shortcode=""
    for line in configOutput_vec_:
        if "DATAGEN_BASE_FUT_PAIR" in line:
            price_type=line.strip().split()[1];
        if "DATAGEN_START_HHMM" in line:
            poolStartTime=line.strip().split()[1];
        if "DATAGEN_END_HHMM" in line:
            poolEndTime=line.strip().split()[1];
        if "SELF" in line:
            shortcode=line.strip().split()[1];
        if "TIMEPERIODSTRING" in line:
            outstring=line.strip().split()[1];

    if poolStartTime == "":
        print("ERROR : DATAGEN_START_HHMM has not been mentioned in config file")
        sys.exit(-1)

    if poolEndTime == "":
        print("ERROR : DATAGEN_END_HHMM has not been mentioned in config file")
        sys.exit(-1)
    if shortcode == "":
        print("ERROR : SELF has not been mentioned in config file")
        sys.exit(-1);
    if outstring == "":
        print("ERROR : TIMEPERIODSTRING has not been mentioned in config file")
        sys.exit(-1);

    outputilistFile = "/home/dvctrader/modelling/stratwork/" + shortcode + "/ilist_" + shortcode + "_" + outstring + "_master_ilist"

    if mode==0 or mode==1:
        ilist1=generate_indicators_from_pool(shortcode,poolStartTime, poolEndTime,price_type);
    if mode==0 or mode==2:
        ilist2=generate_indicators_from_config(config, outputilistFile);
    ilist_final=concat_ilists_uniquely(ilist1,ilist2,price_type);
    ilistString = ("\n".join(ilist_final)).rstrip();
    ilistString="MODELINIT DEPBASE "+shortcode+" "+price_type+" "+price_type+"\n"+"MODELMATH LINEAR CHANGE \n"+"INDICATORSTART\n"+ilistString+"\n"+"INDICATOREND";
    file = open(outputilistFile, "w");
    file.write(ilistString);
    file.close();



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-config', dest='config', help="config", type=str, required=True)
    parser.add_argument('-mode',dest='mode',help='mode',type=int,required=False,default=0)
    args = parser.parse_args()

    generate_indicator_list(args.config,args.mode);




