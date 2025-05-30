import os
import sys
import argparse
import subprocess
import pandas as pd
import datetime
import random
import time

def set_subtract(existing_strat,absent_date_strats):
	result_list=[]
	for es in existing_strat:
		flag =0
		for s in absent_date_strats:
			if es in s:
				flag =1
				# print(es,s)
		if flag == 0:
			result_list.extend([es])	
	return result_list

def strategy_product_results(base_folder, test_folder,config_file):
	random.seed(time.time())
	start_date = '20170101'
	end_date = datetime.datetime.today().strftime('%Y%m%d')
	filename = "/spare/local/logs/tradelogs/product_list_" + str(random.randint(1000000, 9000000))
	product_file = open(filename,'w')
	prod_list = os.listdir(base_folder)
	for prod in prod_list:
		if os.path.isdir(os.path.join(base_folder, prod)):
			# print(prod)
			product_file.write(prod + "\n")
	product_file.close()
	cmd = "python /home/dvctrader/nishit/tradeengine/scripts/summarize_strategy_results.py TOTAL {0} {1} {2} {3} --product_list_file {4}".format(config_file,base_folder,start_date,end_date,filename)
	base_summary = subprocess.check_output(cmd, shell=True).decode().split("\n")
	
	base=[]
	for element in base_summary:
		el_list = element.split(" ")
		if len(el_list)> 15:
			base.extend([el_list[0:2]])
	# base_summary = os.system(cmd).readlines()
	label = ['CONFIG', 'PRODUCT']
	base_frame = pd.DataFrame.from_records(base, columns=label)
	base_frame = base_frame.sort_values('CONFIG')
	grouped_base = base_frame.groupby('CONFIG').apply(lambda x: x.sort_values('PRODUCT'))

	# print(grouped_base)
	cmd = "python /home/dvctrader/nishit/tradeengine/scripts/summarize_strategy_results.py TOTAL {0} {1} {2} {3} --product_list_file {4}".format(config_file, test_folder,start_date,end_date,filename)
	test_summary = subprocess.check_output(cmd, shell=True).decode().split("\n")
	test=[]
	for element in test_summary:
		el_list = element.split(" ")
		if len(el_list)> 15:
			test.extend([el_list[0:2]])
	test_frame = pd.DataFrame.from_records(test, columns=label)
	test_frame = test_frame.sort_values('CONFIG')
	grouped_test = test_frame.groupby('CONFIG').apply(lambda x: x.sort_values('PRODUCT'))
	# print(base_frame)
	config = ""
	all_strat = []
	# print(len(grouped_base))
	for ind in range(0,len(grouped_base)):
		if grouped_base.iloc[ind][0] != config:
			config = grouped_base.iloc[ind][0]
			all_strat.extend([config])
			
	all_strat = set(all_strat)
	absent_strat = []
	config=""		
	base_ind = 0
	
	for test_ind in range(0,len(grouped_test)):
		# print(base_ind,len(grouped_base))
		if base_ind >=len(grouped_base):
			break
		if grouped_base.iloc[base_ind][0] != config:
			config = grouped_base.iloc[base_ind][0]

		while config != grouped_test.iloc[test_ind][0] and base_ind <len(grouped_base):
			absent_strat.extend([config])
			while base_ind <len(grouped_base) and config == grouped_base.iloc[base_ind][0]:
				# print(base_ind, grouped_base.iloc[base_ind][0])
				base_ind = base_ind+1
			if base_ind == len(grouped_base):
				break
			config = grouped_base.iloc[base_ind][0]

		while base_ind <len(grouped_base) and grouped_base.iloc[base_ind][1] < grouped_test.iloc[test_ind][1]:
			# if grouped_base.iloc[base_ind][1] != 'ALL':
			print(config + " not present for " +grouped_base.iloc[base_ind][1])
			base_ind = base_ind+1

		if base_ind >=len(grouped_base):
			break
		if grouped_base.iloc[base_ind][1] > grouped_test.iloc[test_ind][1]:
			print("Base doesnt have "+config + " for " +grouped_base.iloc[base_ind][1])
		else:
			base_ind = base_ind+1
			# existing_strat.extend(config)
	
	absent_strat = set(absent_strat)	
	if len(grouped_test) == 0:
		absent_strat = all_strat
	for config in absent_strat:
		print(config + " not present in test folder")
	absent_strat = set(absent_strat)
	os.remove(filename)
	return all_strat.difference(absent_strat)

def strategy_date_results(base_folder, test_folder,config_file):
	absent_strat = []
	start_date = '20170101'
	end_date = datetime.datetime.today().strftime('%Y%m%d')
	cmd = "python /home/dvctrader/nishit/tradeengine/scripts/summarize_strategy_results.py ALL {0} {1} {2} {3}".format(config_file, base_folder,start_date,end_date)
	base_summary = subprocess.check_output(cmd, shell=True).decode().split("\n")

	base_config = {}
	config=""
	date_list = []
	for base_ind in range(0,len(base_summary)):
		base_row = base_summary[base_ind].split(" ")
		# print(base_row,config,date_list)
		if len(base_row)==2:
			if date_list:
				base_config.update({config:date_list})
			config = base_row[1]
			date_list = []
		elif len(base_row) >1 and base_row[0][0:2] == '20':
			date_list.extend([base_row[0]])
	if date_list != []:
		base_config.update({config:date_list})
	# base_keys = sorted(base_config.keys())

	cmd = "python /home/dvctrader/nishit/tradeengine/scripts/summarize_strategy_results.py ALL {0} {1} {2} {3}".format(config_file,test_folder,start_date,end_date)
	test_summary = subprocess.check_output(cmd, shell=True).decode().split("\n")

	test_config = {}
	config=""
	date_list = []
	for test_ind in range(0,len(test_summary)):
		test_row = test_summary[test_ind].split(" ")
		if len(test_row)==2:
			if date_list:
				test_config.update({config:date_list})
			config = test_row[1]
			date_list = []
		elif len(test_row) >1 and test_row[0][0:2] == '20':
			date_list.extend([test_row[0]])
	if date_list != []:
		test_config.update({config:date_list})
	# test_keys = sorted(test_config.keys())
	for key in base_config.keys():
		if key in test_config:
			# print(base_config,test_config)
			for date in base_config[key]:
				if date not in test_config[key]:
					print(key + " not present for " + date)
		else:
			print(key + " not present for any date")
			absent_strat.extend([config])

	
	return absent_strat

def mismatch_results(base_folder, test_folder,existing_strat):
	cmd = "python /home/dvctrader/stable_exec/scripts/mismatch_results.py {0} {1} {2}".format(base_folder,test_folder, existing_strat)
	# mismatch = subprocess.check_output(cmd, shell=True).decode()
	# print (mismatch)
	os.system(cmd)


parser = argparse.ArgumentParser()
parser.add_argument('base_folder', help='Base folder')
parser.add_argument('test_folder', help='Test folder')
parser.add_argument('config_file', help='Config')

args = parser.parse_args()
if args.base_folder:
    base_folder = args.base_folder
else:
    sys.exit('Please provide base folder')

if args.test_folder:
    test_folder = args.test_folder
else:
    sys.exit('Please provide test folder')

if args.config_file:
    config_file = args.config_file
else:
    sys.exit('Please provide config')

existing_strat = strategy_product_results(base_folder, test_folder,config_file)
# print(existing_strat)
absent_date_strats = strategy_date_results(base_folder, test_folder,config_file)
existing_strat = set_subtract(existing_strat,absent_date_strats)
# print(absent_date_strats,existing_strat)
# print(existing_strat)


if existing_strat:
	strat_names = "/spare/local/logs/tradelogs/existing_strat_" + str(random.randint(1000000, 9000000))
	strategy_ =  open(strat_names,'w')

	for el in existing_strat:
	    # print(strat_)
	    strategy_.write(el+ "\n")

	strategy_.close()
	mismatch_results(base_folder, test_folder,strat_names)
	os.remove(strat_names)
