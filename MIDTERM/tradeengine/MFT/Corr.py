import numpy as np
import pandas as pd
import math
import argparse
import matplotlib.pyplot as plt




parser = argparse.ArgumentParser()
# parser.add_argument('all_prod_file', help='List of products to trade on')
parser.add_argument('log1', help='log1')
parser.add_argument('log2', help='log2')
parser.add_argument('--factor', help='factor')
parser.add_argument('--file', help='file')

args = parser.parse_args()

# if args.all_prod_file:
#     all_prod_file = args.all_prod_file
# else:
#     sys.exit('Please provide exhaustive products file')

if args.log1:
    log1 = args.log1
else:
    sys.exit('Please provide log1')


if args.log2:
    log2 = args.log2
else:
    sys.exit('Please provide log2')

if args.factor:
	factor_ = float(args.factor)
else:
	factor_  = 1

if args.file:
	file_ = args.file
else:
	file_  = ""

log1_f = open(log1)
log2_f = open(log2)
log1_content = []
log2_content = []
date1 = []
date2 = []

for line in log1_f:
	if "Strat0" in line and "Flat" not in line and "HIT" not in line and "Squaring" not in line:
		log1_content.append(float(line.split(' ')[1]))
		#date1.append(int(line.split(',')[2][2:][:-3].replace("-","")))

for line in log2_f:
	if "Strat0" in line and "Flat" not in line and "HIT" not in line and "Squaring" not in line:
		log2_content.append([line.split(' ')[0],factor_*float(line.split(' ')[1]),line.split(' ')[2]])
		# date2.append(line.split(' ')[2])

# print(log1_content,log2_content)
# print([row[1] for row in log2_content])
corr_ = pd.DataFrame(columns=["date","f1","f2"])
corr_["f1"] = log1_content
corr_["f2"] = [row[1] for row in log2_content]
# corr_["date"] = date2
#corr_["date"] = corr_["date"].astype(int)
corr_["combined"] = corr_["f1"] + corr_["f2"]
corr_["f1_cumpnl"] = corr_["f1"].cumsum()
corr_["f2_cumpnl"] = corr_["f2"].cumsum()
corr_["cumpnl"] = corr_["combined"].cumsum()


new_f1_ = corr_[corr_["f1"] != 0]["f1"]
new_f2_ = corr_[corr_["f2"] != 0]["f2"]
new_combined_ = corr_[corr_["combined"] != 0]["combined"]

# new_f1_ = corr_["f1"]
# new_f2_ = corr_["f2"]
# new_combined_ = corr_["combined"]

print("Correlation",corr_["f1"].corr(corr_["f2"]))
log1_draw_ = (corr_["f1_cumpnl"] - corr_["f1_cumpnl"].cummax()).min()
log2_draw_ = (corr_["f2_cumpnl"] - corr_["f2_cumpnl"].cummax()).min()
combined_draw_ = (corr_["cumpnl"] - corr_["cumpnl"].cummax()).min()


#log1_sharpe_ = math.sqrt(252)*corr_["f1"].mean()/corr_["f1"].std()
#log2_sharpe_ = math.sqrt(252)*corr_["f2"].mean()/corr_["f2"].std()

combined_sharpe_ = math.sqrt(252)*corr_["combined"].mean()/corr_["combined"].std()
# print(log1_sharpe_,log2_sharpe_,combined_sharpe_)

log1_sharpe_ = math.sqrt(252)*new_f1_.mean()/new_f1_.std()
log2_sharpe_ = math.sqrt(252)*new_f2_.mean()/new_f2_.std()

nonzero1_sharpe_ = math.sqrt(252)*corr_["f1"].mean()/corr_["f1"].std()
nonzero2_sharpe_ = math.sqrt(252)*corr_["f2"].mean()/corr_["f2"].std()
nonzero_combined_sharpe_ = math.sqrt(252)*corr_["combined"].mean()/corr_["combined"].std()

combined_sharpe_ = math.sqrt(252)*new_combined_.mean()/new_combined_.std()

normal_f1_ = new_f1_[new_f1_ < new_f1_.std()*5]
normal_f2_ = new_f2_[new_f2_ < new_f2_.std()*5]
normal_combined_ = new_combined_[new_combined_ < new_combined_.std()*5]

normal_log1_sharpe_ = math.sqrt(252)*normal_f1_.mean()/normal_f1_.std()
normal_log2_sharpe_ = math.sqrt(252)*normal_f2_.mean()/normal_f2_.std()
normal_combined_sharpe_ = math.sqrt(252)*normal_combined_.mean()/normal_combined_.std()

print("Sum",new_f1_.sum(),new_f2_.sum(),new_combined_.sum())
print("Sharpe w/o extremes",normal_log1_sharpe_,normal_log2_sharpe_,normal_combined_sharpe_)
# print(math.sqrt(252)*new_f1_.mean(),math.sqrt(252)*new_f2_.mean(),math.sqrt(252)*new_combined_.mean())
# print(new_f1_.std(),new_f2_.std(),new_combined_.std())
print("Sharpe",log1_sharpe_,log2_sharpe_,combined_sharpe_)
print("Non zero sharpe",nonzero1_sharpe_,nonzero2_sharpe_,nonzero_combined_sharpe_)

# print(corr_["f2"].mean(),corr_["f2"].std(),corr_["f2"].sum()/factor_)
print("Drawdown",log1_draw_,log2_draw_,combined_draw_)
#print(-250*corr_["f1"].mean()/log1_draw_,-250*corr_["f2"].mean()/log2_draw_,-250*corr_["combined"].mean()/combined_draw_)
print("Return to Drawdown",-250*new_f1_.mean()/log1_draw_,-250*new_f2_.mean()/log2_draw_,-250*new_combined_.mean()/combined_draw_)
print("Return to Non-zero drawdown",-250*corr_["f1"].mean()/log1_draw_,-250*corr_["f2"].mean()/log2_draw_,-250*corr_["combined"].mean()/combined_draw_)

#logreturns computation
# corr_.loc[0,"f1_cumpnl"] = corr_.loc[0,"f1_cumpnl"] + 10000000
# corr_.loc[0,"f2_cumpnl"] = corr_.loc[0,"f2_cumpnl"] + 3300000
# i=0
# corr_.loc[0,"f1dum"] = math.log10((10000000 + corr_.loc[i,"f1_cumpnl"])/(10000000))
# corr_.loc[0,"f2dum"] = math.log10((3300000 + corr_.loc[i,"f2_cumpnl"])/(3300000))
# # print(20170101,10000000,1,0)
# print(20120101,3300000,1,0)
# # print(int(corr_.loc[i,"date"]),10000000 + corr_.loc[i,"f1_cumpnl"], (10000000 + corr_.loc[i,"f1_cumpnl"])/(10000000), corr_.loc[i,"f1dum"])
# print(int(corr_.loc[i,"date"]),3300000 + corr_.loc[i,"f2_cumpnl"], (3300000 + corr_.loc[i,"f2_cumpnl"])/(3300000), corr_.loc[i,"f2dum"])
# for i in range(1, len(corr_)):
# 	if corr_.loc[i,"f1_cumpnl"]/corr_.loc[i-1,"f1_cumpnl"] > 0:
# 		corr_.loc[i,"f1dum"] = math.log10((10000000 + corr_.loc[i,"f1_cumpnl"])/(10000000 + corr_.loc[i-1,"f1_cumpnl"]))
# 	else:
# 		corr_.loc[i,"f1dum"] = 0
# 	if corr_.loc[i,"f2_cumpnl"]/corr_.loc[i-1,"f2_cumpnl"] > 0:
# 		corr_.loc[i,"f2dum"] = math.log10((3300000 + corr_.loc[i,"f2_cumpnl"])/(3300000 + corr_.loc[i-1,"f2_cumpnl"]))
# 	else:
# 		corr_.loc[i,"f2dum"] = 0


# 	# print(int(corr_.loc[i,"date"]),10000000 + corr_.loc[i,"f1_cumpnl"], (10000000 + corr_.loc[i,"f1_cumpnl"])/(10000000 + corr_.loc[i-1,"f1_cumpnl"]), corr_.loc[i,"f1dum"])
# 	print(int(corr_.loc[i,"date"]),3300000 + corr_.loc[i,"f2_cumpnl"], (3300000 + corr_.loc[i,"f2_cumpnl"])/(3300000 + corr_.loc[i-1,"f2_cumpnl"]), corr_.loc[i,"f2dum"])

plt.plot(corr_["f1_cumpnl"])

# plt.plot(corr_["f2_cumpnl"])

# plt.plot(corr_["f1dum"])
# plt.plot(corr_["f2dum"])


# plt.show()
if file_ != "":
	with open(file_,'w') as handle_:
		for i in range(0,len(log2_content)):
			log2_content[i][1] = log1_content[i] + log2_content[i][1]
			handle_.write('%s' % ' '.join(map(str, log2_content[i])) )

# 	corr_[["date","combined"]].to_csv(file_,sep= ' ',index=False)
