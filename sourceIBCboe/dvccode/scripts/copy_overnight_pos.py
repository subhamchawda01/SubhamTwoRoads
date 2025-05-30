#!/usr/bin/python
import sys
import datetime
import subprocess

if (len(sys.argv) < 2):
	print sys.argv[0]+" <dir to position file> [YYYYMMDD]"
	exit(0)

dir = sys.argv[1]
today = datetime.datetime.now()
try:
	today = datetime.datetime.strptime(sys.argv[2], '%Y%m%d')	
except:
	pass

symbol_to_expiry_month_ = {"F" : "01", "G" : "02", "H" : "03", "J" : "04", "K" : "05", "M" : "06", "N" : "07", "Q" : "08", "U" : "09", "V" : "10", "X" : "11", "Z" : "12"}
month_to_expiry_symbol_ = {"01" : "F", "02" : "G", "03" : "H", "04" : "J", "05" : "K", "06" : "M", "07" : "N", "08" : "Q", "09" : "U", "10" : "V", "11" : "X", "12" : "Z"}

def spread_to_legs (spread):
	s_legs = spread.split('-')
	leg_1 = ""
	leg_2 = ""	
	if ( "LFL" in spread ):
		leg_1 = "L   FM" + month_to_expiry_symbol_[s_legs[0][5:7]] + "00" + s_legs[0][3:5] + "!"
		leg_2 = "L   FM" + month_to_expiry_symbol_[s_legs[1][5:7]] + "00" + s_legs[1][3:5] + "!"
	elif ("LFI" in spread ):
		leg_1 = "I   FM" + month_to_expiry_symbol_[s_legs[0][5:7]] + "00" + s_legs[0][3:5] + "!"
		leg_2 = "I   FM" + month_to_expiry_symbol_[s_legs[1][5:7]] + "00" + s_legs[1][3:5] + "!"
	else:
		print "Unexpected Security"
	return (leg_1, leg_2)

def is_spread (symbol):
	if ('-' in symbol):
		return True
	return False


today_str = today.strftime("%Y%m%d")
tomorrow =  today + datetime.timedelta(days=1)
tomm_str = tomorrow.strftime("%Y%m%d")

pos_lines = open(dir+"/position."+today_str).read().split('\n')

num_lines =  int(pos_lines[0].split(':')[-1])

symbol_to_position_map = {}

for line in pos_lines[2:2+num_lines]:
	tokens = line.split(':')
	if (is_spread(tokens[1])):
		spread_pos_ = int(tokens[-2])
                
		try:
			symbol_to_position_map[spread_to_legs(tokens[1])[0]]+=spread_pos_
		except KeyError:
			symbol_to_position_map[spread_to_legs(tokens[1])[0]]=spread_pos_

                try:
			symbol_to_position_map[spread_to_legs(tokens[1])[1]]-=spread_pos_
		except KeyError:
			symbol_to_position_map[spread_to_legs(tokens[1])[1]]=-spread_pos_

		symbol_to_position_map[tokens[1]]=0
	else:
		try:
			symbol_to_position_map[tokens[1]]+= int(tokens[-2])
		except KeyError:
			symbol_to_position_map[tokens[1]]= int(tokens[-2])

f=open(dir+"/position."+tomm_str,'w')

out_ = ""
count_ = 0

for sec in symbol_to_position_map.keys():
	pos = symbol_to_position_map[sec]
	if ( (pos != 0) and not(len(sec)>2 and sec[0:2]=="C_") ):
		count_+=1
		out_ += " Security_Id:" + sec + ":" + "   Position:" + str(pos) + ":\n"

f.write("No. of securities: "+str(count_)+"\n")
f.write("Security Position Map\n")
f.write(out_)



f.close()









