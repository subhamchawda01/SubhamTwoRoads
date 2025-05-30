#!/usr/bin/python
import sys
import datetime
import subprocess

def Index ( s, pat ):
	ind = -1
	try:
		ind = s.index( pat )
	except ValueError:
		pass	
	return ind

if (len(sys.argv) < 2):
	print sys.argv[0]+" <dir to position file>"
	exit(0)

dir = sys.argv[1]
expiry_exec = "/home/pengine/prod/live_execs/get_expiry_from_exchange_symbol"

today = datetime.datetime.now()
today_str = today.strftime("%Y%m%d")
tomorrow =  today + datetime.timedelta(days=1)
tomm_str = tomorrow.strftime("%Y%m%d")


pos_lines = open(dir+"/position."+today_str).read().split('\n')

num_lines =  int(pos_lines[0].split(':')[-1])

symbol_to_position_map = {}

for line in pos_lines[2:2+num_lines]:
	tokens = line.split(':')
	try:
		symbol_to_position_map[tokens[1]]+= int(tokens[-2])
	except KeyError:
		symbol_to_position_map[tokens[1]]= int(tokens[-2])

f=open(dir+"/position."+tomm_str,'w')

for sec in symbol_to_position_map.keys():
	if ( Index( sec, "DOL") == 0 ):
		#print sec, symbol_to_position_map[sec]
		for sec_1 in symbol_to_position_map.keys():
			#print sec_1, symbol_to_position_map[sec_1]
			if ( Index( sec_1, "WDO") == 0 ):
				symbol_to_position_map[sec_1] += 5*symbol_to_position_map[sec]
				symbol_to_position_map[sec] = 0
        if ( Index( sec, "IND") == 0 ):
                for sec_1 in symbol_to_position_map.keys():
                        if ( Index( sec_1, "WIN") == 0 ):
                                symbol_to_position_map[sec_1] += 5*symbol_to_position_map[sec]
                                symbol_to_position_map[sec] = 0          


count_ = 0
out_ = ""

for sec in symbol_to_position_map.keys():
        pos = symbol_to_position_map[sec]
        #print sec, symbol_to_position_map[sec]
        if(pos != 0):
        	p = subprocess.Popen([expiry_exec, sec], stdout=subprocess.PIPE)
        	out, err = p.communicate()
        	if(out[0]=='N'):
                        count_ += 1
        		out_ += " Security_Id:" + sec + ":" + "   Position:" + str(pos) + ":\n"
        	else:
        		pass

f.write("No. of securities: "+str(count_)+"\n")
f.write("Security Position Map\n")
f.write(out_)


f.close()









