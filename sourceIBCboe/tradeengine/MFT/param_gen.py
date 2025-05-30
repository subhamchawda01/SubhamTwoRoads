import argparse
import copy
import os
home_path = "/home/nishitbhandari/Desktop/MFT/"
file_num = 0

def print_to_file(_content, _file_name):
	global file_num
	global file_list
	# if file_num > 20:
	# 	return
	_file = home_path + _file_name + str(file_num) + ".cfg"
	file_list.append(_file_name + str(file_num) + ".cfg")
	with open(_file,"w") as f:
		for list_ in _content:
			strlist_ = ' '.join(list_) + "\n"
			# print(strlist_)
			f.write(strlist_)




def generate_content(_strat_content,_index, _content_till_now,_file_name):
	global file_num
	if isinstance(_strat_content[_index][2], (list,)):
		for el in _strat_content[_index][2]:
			current_row = [_strat_content[_index][0],_strat_content[_index][1],el]
			_content_till_now.append(current_row)
			if _index == len(_strat_content)-1:
				# print(_content_till_now,_file_name)
				print_to_file(_content_till_now,_file_name)
				file_num = file_num + 1
			else:
				generate_content(_strat_content,_index+1,_content_till_now,_file_name)
			_content_till_now.pop()
	else:
		if _strat_content[_index][0] == "PKL_PATH":
			row_ = _strat_content[_index][:]
			row_[2] = row_[2] + str(file_num)
			_content_till_now.append(row_)	
		else:
			_content_till_now.append(_strat_content[_index])
		if _index == len(_strat_content)-1:
			# print(_content_till_now,_file_name)
			print_to_file(_content_till_now,_file_name)
			file_num = file_num +1
		else:
			generate_content(_strat_content,_index+1,_content_till_now,_file_name)
		_content_till_now.pop()	



parser = argparse.ArgumentParser()
parser.add_argument('config_file', help='config file to permute params on')
parser.add_argument('strat_file', help='strat config file to permute params on')
parser.add_argument('permute_file', help='permute file to permute params on')

args = parser.parse_args()


if args.config_file:
    config_file = args.config_file
else:
    sys.exit('Please provide config file')

if args.strat_file:
    strat_file = args.strat_file
else:
    sys.exit('Please provide strat config file')


if args.permute_file:
    permute_file = args.permute_file
else:
    sys.exit('Please provide permute file')

main_content = []
strat_content = []
param_content = []

with open(home_path+config_file, "r") as main_file:
    for line in main_file:
    	main_content.append(str.split(line))


with open(home_path+strat_file, "r") as strat_file:
    for line in strat_file:
    	strat_content.append(str.split(line))

with open(home_path+permute_file, "r") as permute_file:
    for line in permute_file:
    	param_content.append(str.split(line))



for param_list in param_content:
	param = param_list[0]
	for index in range(0,len(strat_content)):
		if param == strat_content[index][0]:
			strat_content[index] = param_list
			strat_content[index][2] = strat_content[index][2].split(',')

	for index in range(0,len(main_content)):
		if param == main_content[index][0]:
			main_content[index] = param_list
			print(main_content[index][2])
			main_content[index][2] = main_content[index][2].split(',')


momentum_file_index = 0
file_list = []
momentum_file_list = []
file_num = 0
generate_content(strat_content,0,[],"Momentum_param")
momentum_file_list = file_list
# print(momentum_file_list)


for index in range(0,len(main_content)):
	if main_content[index][0] == "MOMENTUM":
		main_content[index][2] = momentum_file_list
# print(main_content)
file_num = 0
file_list = []
generate_content(main_content,0,[],"MainConfig_param")
mainconfig_file_list = file_list

# print(mainconfig_file_list,momentum_file_list)



for f in mainconfig_file_list:
	os.system("python3.6 trade_main.py " + f + " 2012_01_01 2016_12_31 > log" + f)
	f = home_path + f
	# os.system("rm "+ f)

# for f in momentum_file_list:
# 	f = home_path + f
# 	os.system("rm "+ f)
