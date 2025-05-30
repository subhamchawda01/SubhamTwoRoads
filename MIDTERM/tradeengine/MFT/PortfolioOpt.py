import numpy as np
import pandas as pd
import math
import argparse
import matplotlib.pyplot as plt

from matplotlib import cm
from matplotlib.ticker import LinearLocator, FormatStrFormatter
from mpl_toolkits.mplot3d import Axes3D


parser = argparse.ArgumentParser()
# parser.add_argument('all_prod_file', help='List of products to trade on')
parser.add_argument('log1', help='log1')
parser.add_argument('log2', help='log2')
parser.add_argument('log3', help='log3')
parser.add_argument('--factor', help='factor')

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

if args.log3:
    log3 = args.log3
else:
    sys.exit('Please provide log3')

if args.factor:
	factor_ = float(args.factor)
else:
	factor_  = 1

log1_f = open(log1)
log2_f = open(log2)
log3_f = open(log3)
log1_content = []
log2_content = []
log3_content = []

for line in log1_f:
	if "'Strat0" in line:
		log1_content.append(float(line.split(',')[1]))

for line in log2_f:
	if "'Strat0" in line:
		log2_content.append(float(line.split(',')[1]))

for line in log3_f:
	if "'Strat0" in line:
		log3_content.append(float(line.split(',')[1]))

# print(log1_content,log2_content)

corr_ = pd.DataFrame(columns=["f1","f2","f3"])
tune_= pd.DataFrame(columns=["alpha","beta","sharpe","rtod"])

corr_["f1"] = log1_content
corr_["f2"] = log2_content
corr_["f3"] = log3_content

new_f1_ = corr_[corr_["f1"] != 0]["f1"]
new_f2_ = corr_[corr_["f2"] != 0]["f2"]
new_f3_ = corr_[corr_["f3"] != 0]["f3"]

correlation_12_ = corr_["f1"].corr(corr_["f2"])
correlation_13_ = corr_["f1"].corr(corr_["f3"])
correlation_23_ = corr_["f3"].corr(corr_["f2"])
# print(correlation_)
mean_1_ = math.sqrt(252)*new_f1_.mean()
mean_2_ = math.sqrt(252)*new_f2_.mean()
mean_3_ = math.sqrt(252)*new_f3_.mean()


var_1_ = new_f1_.std()
std_1_ = var_1_
var_1_ = var_1_*var_1_

var_2_ = new_f2_.std()
std_2_ = var_2_
var_2_ = var_2_*var_2_

var_3_ = new_f3_.std()
std_3_ = var_3_
var_3_ = var_3_*var_3_




for i in xrange(0,100):
	alpha_ = float(i/100.0)

	for j in xrange(0,100-i):
		beta_ = float(j/100.0)		

		gamma_ = 1-alpha_-beta_
		corr_["combined"] = alpha_*corr_["f1"] + beta_*corr_["f2"] + gamma_*corr_["f3"]
		corr_["cumpnl"] = corr_["combined"].cumsum()
		combined_draw_ = (corr_["cumpnl"] - corr_["cumpnl"].cummax()).min()
		rtod_ = -250*corr_["combined"].mean()/combined_draw_
		comb_mean_ = alpha_*mean_1_ + beta_*mean_2_ + gamma_*mean_3_
		comb_var_ = alpha_*alpha_*var_1_ + beta_*beta_*var_2_ + gamma_*gamma_*var_3_
		comb_var_ = comb_var_ + 2*alpha_*beta_*std_1_*std_2_*correlation_12_ + 2*alpha_*gamma_*std_1_*std_3_*correlation_13_ + 2*gamma_*beta_*std_3_*std_2_*correlation_23_
		comb_sharpe_ = comb_mean_/math.sqrt(comb_var_)
		tune_.loc[len(tune_)] = [alpha_,beta_,comb_sharpe_,rtod_]
		# print(alpha_,beta_,comb_sharpe_,rtod_)


# print(tune_)
maxrtod_ = tune_["rtod"].max()
x = tune_[tune_["sharpe"]>=factor_*tune_["sharpe"].max()]
y = x[x["rtod"] >= factor_*maxrtod_]
# print(tune_[tune_["sharpe"]>=0.8*tune_["sharpe"].max() and tune_["rtod"]>=0.8*tune_["rtod"].max()])
print(tune_[tune_["sharpe"]==tune_["sharpe"].max()])
print(tune_[tune_["rtod"]==tune_["rtod"].max()])
print(y)

# threedee = plt.figure().gca(projection='3d')
# threedee.scatter(tune_["alpha"], tune_["beta"], tune_["sharpe"])
# threedee.set_xlabel('Alpha')
# threedee.set_ylabel('Beta')
# threedee.set_zlabel('Sharpe')
# plt.show()

# tune_.plot(kind='scatter',x="alpha",y="sharpe")

# ax1 = tune_.plot.scatter(x="alpha",y="sharpe")
# plt.plot(tune_)
# plt.show()


