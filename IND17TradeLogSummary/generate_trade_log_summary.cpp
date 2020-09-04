#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <dirent.h>
#include <sys/types.h>
//#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/algorithm/string.hpp>

struct trade_log_data {
	int profit_count=0, loss_count=0, profit_count_wrt_limit=0, total_profit_wrt_limit=0, loss_count_wrt_limit=0, 
            total_loss_wrt_limit=0, total_pnl=0, total_product_count=0;
};

//read all trade files from directory and store it in vector
void ReadDirectory(const std::string& name, std::vector<std::string>& trade_file) {

	DIR* dirp = opendir(name.c_str());
	struct dirent * dp; 
	std::string file_name;
	while ((dp = readdir(dirp)) != NULL) {
		file_name = dp->d_name;
		if(file_name.find("trades.") != std::string::npos)
			trade_file.push_back(name + file_name);
	}
	closedir(dirp);

}

//fetch current date
std::string CurrentDate() {

	auto date = std::time(nullptr);
	std::stringstream today_date;
	today_date << std::put_time(std::localtime(&date), "%Y%m%d");
	return today_date.str();

}

//fetch previous date
std::string GetPrevDate(std::string today_date) {

	int year = std::stoi(today_date.substr(0,4));
	int month = std::stoi(today_date.substr(4,2));
	int day = std::stoi(today_date.substr(6,2));
	std::string prev_date;
	//if there is first day of month
	if(day==1) {
		//months which have 30 days in previous month
		if(month==4|| month==6|| month==9|| month==11) {
			day=31;
			month = month -1;	
		}
		//for MARCH, to define february last day
		else if(month==3) {
			if(year%4==0)	
			day=29;
			else
			day=28;

			month = month -1;
		}
		//for January, to define December last day
		else if(month==1) {
			day=31;
			month = 12;
			year = year - 1 ;
		}
		//for Feb, to define January last day
		else if(month==2) {
			day=31;
			month = month -1;
		}
		//for other months
		else {
			day=30;
			month = month -1;
		}
	}
	//other days of month
	else {
		day = day-1;
	}
	//set precision to 2 character
	std::stringstream temp;
	prev_date = std::to_string(year);
	temp << std::setw(2) << std::setfill('0') << month;
	prev_date += temp.str();
	temp.str("");
	temp<< std::setw(2) << std::setfill('0') << day;
	prev_date += temp.str();
	return prev_date;

}

//logging traded product summary to file
trade_log_data InsertProductSummaryToFile(int &pnl_limit, std::string trade_filename) {

	std::string line;
	std::map<std::string,int> product_pnl;
	std::vector<std::string> data;
	std::ifstream fin;
	std::ofstream fout;
	fout.open("trade_log_summary.txt", std::ios::app);
	fin.open(trade_filename, std::ios::in);
        trade_log_data log_data;
	
	while (std::getline(fin, line)) {
		data.clear();
		//boost::split(data,line,boost::is_any_of(" "));
		std::string word;
		std::stringstream str(line);
		while(str >> word)
			data.push_back(word);
		if((stoi(data[8]) < (-1*pnl_limit)) || (product_pnl.find(data[2]) != product_pnl.end()))
			product_pnl[data[2]] = std::stoi(data[8]);
	}
	for(auto product : product_pnl) {
		++log_data.total_product_count;
		log_data.total_pnl += product.second;
		(product.second > 0) ? ++log_data.profit_count : ++log_data.loss_count;
		if ((pnl_limit + product.second) > 0) {
                      ++log_data.profit_count_wrt_limit;
                      log_data.total_profit_wrt_limit += product.second;
                }
                else {
                      ++log_data.loss_count_wrt_limit;
                      log_data.total_loss_wrt_limit += product.second;
                }
	}
	//inserting data to trade summary file
	fout << trade_filename.substr(52) << "," << log_data.profit_count << "," << log_data.loss_count << "," << log_data.profit_count_wrt_limit << "," << log_data.loss_count_wrt_limit << "," << log_data.total_pnl << "," << log_data.total_product_count << "\n";
	fout.close();
	return log_data;
}

void InsertProductToMap(int pnl_limit) {

	std::string prev_date = CurrentDate();
	int date_count=0;
	std::vector<std::string> all_files;
        std::vector<trade_log_data> overall_summary;
	ReadDirectory("/run/media/root/Elements/SERVERDATA/IND17/tradelogs/",all_files);
	std::ofstream fout;
	fout.open("trade_log_summary.txt", std::ios::out);
	fout << "Trade_Filename,Profit_Count,Loss_Count,Profit_Count_wrt_Limit,Loss_Count_wrt_Limit,Total_PNL,Total_Product_Count\n";
	fout.close();

	//fetching last 90 days trade files
	while(date_count < 90) {
		prev_date = GetPrevDate(prev_date);
		std::vector<std::string> date_files;
		std::copy_if(all_files.begin(), all_files.end(),std::back_inserter(date_files),[prev_date](const std::string& s)
		{ return s.find("trades." + prev_date) != std::string::npos; });
		if(date_files.empty())
			continue;
		else {
			++date_count;
			std::vector<trade_log_data> date_log_data;
			//std::cout<< date_count << " " << prev_date << "\n";
			//logging trade log summary for each file
			for(auto file : date_files)
				date_log_data.push_back(InsertProductSummaryToFile(pnl_limit,file));
			
			//logging trade log summary for that particular date
			trade_log_data final_log_data;
			for(auto log_data : date_log_data) {
				final_log_data.profit_count += log_data.profit_count;
				final_log_data.loss_count += log_data.loss_count;
				final_log_data.profit_count_wrt_limit += log_data.profit_count_wrt_limit;
                                final_log_data.total_profit_wrt_limit += log_data.total_profit_wrt_limit;
				final_log_data.loss_count_wrt_limit += log_data.loss_count_wrt_limit;
                                final_log_data.total_loss_wrt_limit += log_data.total_loss_wrt_limit;
				final_log_data.total_pnl += log_data.total_pnl;
				final_log_data.total_product_count += log_data.total_product_count;
			}
                        overall_summary.push_back(final_log_data);
			fout.open("trade_log_summary.txt", std::ios::app);
			fout << "summary."<< prev_date << "," << final_log_data.profit_count << "," << final_log_data.loss_count <<"," 
			<< final_log_data.profit_count_wrt_limit << "," << final_log_data.loss_count_wrt_limit << "," 
			<< final_log_data.total_pnl << "," << final_log_data.total_product_count << "\n";
			fout.close();
		}
	}
	//logging overall summary for last 90 days data
	trade_log_data final_log_summary;
	for(auto log_data : overall_summary) {
		final_log_summary.profit_count += log_data.profit_count;
		final_log_summary.loss_count += log_data.loss_count;
		final_log_summary.profit_count_wrt_limit += log_data.profit_count_wrt_limit;
                final_log_summary.total_profit_wrt_limit += log_data.total_profit_wrt_limit;
		final_log_summary.loss_count_wrt_limit += log_data.loss_count_wrt_limit;
                final_log_summary.total_loss_wrt_limit += log_data.total_loss_wrt_limit;
		final_log_summary.total_pnl += log_data.total_pnl;
		final_log_summary.total_product_count += log_data.total_product_count;
	}
	fout.open("trade_log_summary.txt", std::ios::app);
	fout << "Overall_Summary,Profit_Count,Loss_Count,Profit_Count_wrt_Limit,Avg_Profit_wrt_Limit,Loss_Count_wrt_Limit,Avg_Loss_wrt_Limit,Total_PNL,Total_Product_Count\n";
	fout << "overall_summary" <<  "," << final_log_summary.profit_count << "," << final_log_summary.loss_count <<"," 
          << final_log_summary.profit_count_wrt_limit << "," 
          << final_log_summary.total_profit_wrt_limit / final_log_summary.profit_count_wrt_limit << ","
          << final_log_summary.loss_count_wrt_limit << ","
          << final_log_summary.total_loss_wrt_limit / final_log_summary.loss_count_wrt_limit << "," 
          << final_log_summary.total_pnl << "," << final_log_summary.total_product_count << "\n";
	fout.close();
}

int main(int argc, char **argv) {
	if(argc != 2) {
		std::cout << "USAGE : <exec> <PNL limit>";
		exit(-1);
	}
	InsertProductToMap(std::stoi(argv[1]));
return 0;
}
