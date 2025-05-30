import os		
import sys		
import argparse		
import pandas as pd		
	
def strat_match(strat,strat_list):		
	for l in strat_list:		
		if l in strat:		
			return True		
		
	return False		
		
def union(current_folder1, current_folder2):		
	combined_list = []		
	for (dirpath, dirnames, filenames) in os.walk(current_folder1):		
		combined_list.extend(dirnames)		
		break		
	for (dirpath, dirnames, filenames) in os.walk(current_folder2):		
		combined_list.extend(dirnames)		
		break		
		
	combined_list = set(combined_list)		
	return combined_list		
		
def grouping_mismatch(mismatch_frame,group_field):		
	# return mismatch_frame		
	output_string=""		
	grouped = mismatch_frame.groupby(group_field)		
	if group_field == 'Strat_name':		
		for key, strat in grouped:		
			date_output = grouping_mismatch(strat,'Date')		
			if date_output !="error":		
				output_string = output_string +"\n"+ key + date_output		
		
		
	elif group_field == 'Date':		
		for key, date_strat in grouped:		
			product_output = grouping_mismatch(date_strat,'Product')		
			if product_output !="error":		
				output_string = output_string+"\n\t" + key +product_output		
	elif group_field == 'Product':		
		for key, prod_date_strat in grouped:		
			if len(prod_date_strat.index) ==2 and prod_date_strat.iloc[0][29] + prod_date_strat.iloc[1][29] == 3:
				mismatch_index = int(prod_date_strat.iloc[0][30])
				output_string = output_string + "\n\t\t" +key +"\n"		
				index =0		
				prod_date_strat.sort_values(by=['Flag'])		
				index_list = [mismatch_index]		
						
				for i in range(mismatch_index+1,29):
					if round(float(prod_date_strat.iloc[0][i])) != round(float(prod_date_strat.iloc[1][i])):		
						index_list.append(i)		
				output = prod_date_strat.iloc[:,index_list]		
				output_string = output_string + "\t\t\t"+"\t".join(output.dtypes.index) + "\n"		
				output_string = output_string + "\t\t\t"+output.iloc[0].to_frame().T.to_string(index=False,header=False) + "\n"		
				output_string = output_string + "\t\t\t"+output.iloc[1].to_frame().T.to_string(index=False,header=False) + "\n"		
				# output_string = output_string + prod_date_strat.iloc[:,index_list].to_string(index = False)		
				# output_string = output_string + prod_date_strat.iloc[index][mismatch_index] + " Test " \		
				# 				+ list(prod_date_strat)[mismatch_index] + " "+ prod_date_strat.iloc[(index+1)%2][mismatch_index] 		
	else:		
		output_string = "error"		
		
	if output_string =="":		
		output_string = "error"		
	return output_string		
		
		
		
def mismatch_results(results_folder1_, results_folder2_,existing_strat_):		
	# print(existing_strat_)		
	strat_list=[]		
	mismatch_list = []	
	check_list = []		
	database_list = ['Strat_name','Product', 'Date', 'PnL', 'Vol', 'SO', 'BO', 'MktPercent', 'AO', 'AvgAbsPos',		
					 'TTCMedian', 'AvgTTC','CTMedianPnl', 'CTAvgPnl', 'CTStdPnl', 'CTSharpe', 'PosPercentTrades',		
	   					'MinPnl', 'MaxPnl', 'DrawDown', 'MaxTTC', 'NumMessages', 'Exposure', 'TTV', 		
	   					'AbsFinalPos', 'UTS', 'PosTrades', 'TotalClosedTrades','TotalTrades','Flag','First_mismatch_index']
	current_folder1_ = results_folder1_		
	current_folder2_ = results_folder2_		
	strategy_ =  open(existing_strat_,'r')		
	for line in strategy_:		
		strat_list.extend([line.strip()])		
	# print(strat_list)		
	if not os.path.exists(results_folder1_):		
		print(results_folder1_ + " does not exist")		
		
	elif not os.path.exists(results_folder2_):		
		print(results_folder2_ + " does not exist")		
		
	else:		
		product_list_ = union(current_folder1_, current_folder2_)		
		# print(product_list_)		
		
		for product_ in product_list_:		
			product_path1_ = os.path.join(current_folder1_, product_)		
			product_path2_ = os.path.join(current_folder2_, product_)		
		
			if not os.path.exists(product_path1_):		
				# print(product_ + " does not exist in " + results_folder1_)		
				continue		
		
			elif not os.path.exists(product_path2_):		
				# print(product_ + " does not exist in " + results_folder2_)		
				continue		
		
			else:		
				current_folder1_ = product_path1_		
				current_folder2_ = product_path2_		
		
				year_list_ = union(current_folder1_, current_folder2_)		
				# print(year_list_)		
		
		
				for year_ in year_list_:		
					year_path1_ = os.path.join(current_folder1_, year_)		
					year_path2_ = os.path.join(current_folder2_, year_)		
		
					if not os.path.exists(year_path1_):		
						# print(year_ + " does not exist in " + product_ + " in "+results_folder1_)		
						continue		
		
					elif not os.path.exists(year_path2_):		
						# print(year_ + " does not exist in " + + product_ + " in " +results_folder2_)		
						continue		
		
					else:		
						current_folder1_ = year_path1_		
						current_folder2_ = year_path2_		
		
						month_list_ = union(current_folder1_, current_folder2_)		
		
						for month_ in month_list_:		
							month_path1_ = os.path.join(current_folder1_, month_)		
							month_path2_ = os.path.join(current_folder2_, month_)		
		
							if not os.path.exists(month_path1_):		
								# print(month_+"/"+year_ + " does not exist in " + product_ + " in "+results_folder1_)		
								continue		
		
							elif not os.path.exists(month_path2_):		
								# print(month_ + "/"+year_+" does not exist in " + + product_ + " in " +results_folder2_)		
								continue		
		
							else:		
								current_folder1_ = month_path1_		
								current_folder2_ = month_path2_		
		
								day_list_ = union(current_folder1_, current_folder2_)		
		
								for day_ in day_list_:		
									day_path1_ = os.path.join(current_folder1_, day_)		
									day_path2_ = os.path.join(current_folder2_, day_)		
		
									if not os.path.exists(day_path1_):		
										# print(day_ +"/" + month_+"/"+year_ + " does not exist in " + product_ + " in "+results_folder1_)		
										continue		
		
									elif not os.path.exists(day_path2_):		
										# print(day_ +"/" + month_ + "/"+year_+" does not exist in " + + product_ + " in " +results_folder2_)		
										continue		
		
									else:		
										current_folder1_ = day_path1_		
										current_folder2_ = day_path2_		
										database1 = os.path.join(current_folder1_, "results_database.txt")		
										database2 = os.path.join(current_folder2_, "results_database.txt")		
										# print (day_+"/"+month_+"/"+year_+" "+product_)		
										# print (database1,database2)		
										# print("\n")		
										content1 = []		
										content2 = []		
										for line in open(database1,"r"):		
											content1.extend([line.split(" ")])		
										for line in  open(database2,"r"):		
											content2.extend([line.split(" ")])		
										# print(content1,content2)		
										content1.sort(key=lambda x: x[0])		
										content2.sort(key=lambda x: x[0])		
		
										row2 =0		
										row1=0		
										while True:		
											if row1 ==len(content1):		
												break		
											check_list.append(content1[row1][0].replace(product_,'')[:-1] + " " +content1[row1][1])		
											if strat_match(content1[row1][0],strat_list):		
												while not strat_match(content2[row2][0],strat_list):		
													row2 = row2 +1		
													if row2 >= len(content2):		
														break		
												if row2 >= len(content2):		
													break		
												for index in range(0,len(content1[row1])):		
													strat_absent = False		
													if content1[row1][index] != content2[row2][index]:		
														if index <2 or round(float(content1[row1][index]),2) != round(float(content2[row2][index]),2):		
															if index==0:		
																if content1[row1][0] < content2[row2][0]:		
																	strat_absent = True		
																	print(content1[row1][0].replace(product_,'')[:-1] + " not present for " + product_ + " on "+ content1[row1][1])		
																	break		
																else:		
																	row1 = row1 -1		
															content1[row1].append(1)		
															content1[row1].append(index+1)		
															content2[row2].append(2)		
															content2[row2].append(index+1)		
															# index+1 as strat and product are split in data frame		
															mismatch_list.extend([([content1[row1][0].replace(product_,'')[:-1]] + [product_] + content1[row1][1:])])		
															mismatch_list.extend([([content2[row2][0].replace(product_,'')[:-1]] + [product_] + content2[row2][1:])])		
		
															content1[row1].pop()		
															content1[row1].pop()		
															content2[row2].pop()		
															content2[row2].pop()		
															# print("MISMATCH in " + content1[row1][0].replace(product_,'')[:-1] + " in " + day_ +"/" + month_ + "/"+year_ + " for " + product_ )		
															break		
												if strat_absent == False:	
													# print(content1[row1][0].replace(product_,'')[:-1],content1[row1][1],product_)	
													# print(content2[row2][0].replace(product_,'')[:-1],content2[row2][1],product_)	
													row2 = row2 +1		
												if row2 >= len(content2):		
													break		
											row1 = row1+1		
										current_folder1_,x = os.path.split(current_folder1_)		
										current_folder2_,x = os.path.split(current_folder2_)		
		
								current_folder1_,x = os.path.split(current_folder1_)		
								current_folder2_,x = os.path.split(current_folder2_)		
		
						current_folder1_,x = os.path.split(current_folder1_)		
						current_folder2_,x = os.path.split(current_folder2_)		
					
				current_folder1_,x = os.path.split(current_folder1_)		
				current_folder2_,x = os.path.split(current_folder2_)		
				# print(current_folder1_, current_folder2_)		
		
	if mismatch_list:		
		# print(mismatch_list)
		mismatch_frame = pd.DataFrame.from_records(mismatch_list, columns=database_list)		
		
		output_string = grouping_mismatch(mismatch_frame,'Strat_name')		
		print(output_string)		
	list_ = sorted(set(check_list))
	for l in list_:
		print(l)
parser = argparse.ArgumentParser()		
parser.add_argument('results_folder1_', help='Result folder 1')		
parser.add_argument('results_folder2_', help='Result folder 2')		
parser.add_argument('existing_strat_', help='Strategies to check')		
		
args = parser.parse_args()		
if args.results_folder1_:		
 results_folder1_ = args.results_folder1_		
else:		
 sys.exit('Please provide input strat folder')		
		
if args.results_folder2_:		
 results_folder2_ = args.results_folder2_		
else:		
 sys.exit('Please provide pattern to choose folder from')		
		
if args.existing_strat_:		
 existing_strat_ = args.existing_strat_		
else:		
 sys.exit('Please provide strat list to check from')		

mismatch_results(results_folder1_, results_folder2_,existing_strat_)
