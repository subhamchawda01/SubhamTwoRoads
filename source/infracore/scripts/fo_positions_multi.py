#!/usr/bin/python
# -*- coding: utf-8 -*
import pandas as pd
import subprocess
import sys
month_dict = {"JAN":"01","FEB":"02","MAR":"03","APR":"04", "MAY":"05", "JUN":"06", "JUL":"07","AUG":"08","SEP":"09","OCT":"10","NOV":"11","DEC":"12"}
INPUT_FILE = '/tmp/input_pos_price.txt'
OUTPUT_FILE= '/tmp/output_pos_price.txt'
getshort_exec = '/home/pengine/prod/live_execs/get_shortCode_from_strike_price'
FO_Position = 'fo_positions.txt'
FUT0 = "fo_positions_FUT0.txt"
FUT0_Inverted = "fo_positions_FUT0_Inverted.txt"


def BashExec(command):
    return subprocess.check_output(['bash', '-c',command]).decode('utf-8')

def GetFoPositionsdf(filename):
	fo_pos_df = pd.read_csv(filename);
	fo_pos_df = fo_pos_df[ ['Instrument','Symbol','Expiry Date','Strike Price','OptionType','B/S'] ];
	fo_pos_df['Positions'] = fo_pos_df['B/S'].apply(lambda x: -1 if x == 'S' else 1);
	fo_pos_df.fillna('',inplace=True);
	fo_pos_df['Symbol'] = fo_pos_df.apply(lambda x: x['Instrument']+" "+x['Symbol']+" "+
		x['Expiry Date']+" "+str(x['Strike Price'])+" "+x['OptionType'],axis=1);
	fo_pos_df = fo_pos_df[['Symbol','Positions']];
	fo_pos_df.fillna('',inplace=True);
	fo_pos_df = fo_pos_df.groupby('Symbol').agg({'Positions':'sum'});
	return fo_pos_df; 

def GenerateAvgPositions(symbol_position,date):
	result = {}
	input_f = open(INPUT_FILE,"w");
	for key in symbol_position:
		name = key;
		if name.split(' ')[0] == "FUTSTK" or  name.split(' ')[0] == "FUTIDX":
			index_gen = "NSE_"+name.split(' ')[1]+"_FUT0";
			if index_gen in result.keys():
				result[index_gen] = int(symbol_position[key])+int(result[index_gen]);
			else:
				result[index_gen] = int(symbol_position[key]);
		elif( name.split(' ')[0] == "OPTIDX" or name.split(' ')[0] == "OPTSTK" ):
			date_str1 = name.split(' ')[2];
			date_str2 = date_str1[5:]+month_dict[date_str1[2:5]]+date_str1[0:2]
			input_f.write("%s %s %s %s %s\n" %( name.split(' ')[1], date_str2, name.split(' ')[3], name.split(' ')[4], symbol_position[key]));
	input_f.close()
	command = getshort_exec + ' ' + INPUT_FILE + ' ' + OUTPUT_FILE  + ' '+date
	generatedCode = BashExec(command);
	f = open(OUTPUT_FILE);
	lines = f.readlines();
	for x in lines:
		x = x.rstrip("\n");
		result[x.split(" ")[0]] = int(x.split(" ")[1]);
	f.close()
	fileAvg = open(FUT0,"w");
	fileAvgInv = open(FUT0_Inverted,"w");
	fileAvg.write("Symbol\tPositions\n");
	fileAvgInv.write("Symbol\tPositions\n")
	for key in sorted (result.keys()):
		fileAvg.write("%s\t%d\n" % (key, result[key]))
		fileAvgInv.write("%s\t%d\n" % (key, -result[key]))
	fileAvg.close()
	fileAvgInv.close()

if __name__ == '__main__':
	if len(sys.argv) < 3:
		print ("Arguments: file.py date TradesFiles");
		exit(0)
	symbol_position = {}
	date = sys.argv[1]
	for filename in sys.argv[2:]:
		fo_pos_df = GetFoPositionsdf(filename);
		with pd.option_context('display.max_rows', None, 'display.max_columns', None):
			print(fo_pos_df);
		for index,row in fo_pos_df.iterrows():
			if index in symbol_position.keys():
				symbol_position[index] += int(row['Positions']); 
			else:
				symbol_position[index] = int(row['Positions']);
	filePosition = open(FO_Position,"w");
	filePosition.write("Symbol\tPositions\n");	
	for key in sorted (symbol_position.keys()):
		filePosition.write("{0}\t{1}\n".format(key, str(symbol_position[key])))
	GenerateAvgPositions(symbol_position,date);
