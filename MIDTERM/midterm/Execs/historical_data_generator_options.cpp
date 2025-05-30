#include <map> 
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <stdlib.h>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

using namespace std;
using namespace boost;

struct Metrics {
  double open_price, close_price, low_price, high_price;
  int trades, volume;
  int start_time;
  int close_time;
	int	expiry;

  string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << start_time << "\t";
    t_temp_oss << close_time << "\t";
    t_temp_oss << expiry << "\t";
    t_temp_oss << open_price << "\t";
    t_temp_oss << close_price << "\t";
    t_temp_oss << low_price << "\t";
    t_temp_oss << high_price << "\t";
    t_temp_oss << volume << "\t";
    t_temp_oss << trades;

    return t_temp_oss.str();
  }
};

std::vector<int> getSortedExpiryVec(std::string _expiry_file_, int _date_) {
  std::ifstream if_stream_;
  if_stream_.open(_expiry_file_, std::ifstream:: in );
  if (!if_stream_.is_open()) {
    std::cerr << "Error : Unable to open expiry file => " << _expiry_file_ << std::endl;
    exit(-1);
  }
  std::string line_;
  std::vector<int> expiry_vec_;
  while (std::getline(if_stream_, line_)) {
    int expiry_ = atoi(line_.c_str());
    if (expiry_ >= _date_) {
      expiry_vec_.push_back(expiry_);
    }
  }
  sort(expiry_vec_.begin(), expiry_vec_.end());
  return expiry_vec_;
}


int main(int argc, char **argv) {
  if (argc < 5) {
    std::cout << "USAGE : <exec> <date> <products_data_filenames_file> <expiry_file> <output_dir>" << std::endl;
    exit(-1);
  }
  int date = atoi(argv[1]);
  std::string products_data_filenames_file_(argv[2]);
  std::string expiry_filename_(argv[3]);
  std::string output_dir_(argv[4]);
  
  std::vector<int> expiry_vec_ = getSortedExpiryVec(expiry_filename_, date);
  map<string, map<int, Metrics>> map_priceX_;
  std::string line_;
  std::ifstream if_stream_;
  if_stream_.open(products_data_filenames_file_, std::ifstream:: in );
  if (!if_stream_.is_open()) {
    std::cerr << "Error : Unable to open " << products_data_filenames_file_ << std::endl;
    exit(-1);
  }
  while (std::getline(if_stream_, line_)) {
    size_t indexR = line_.rfind("/");
    std::string file_name_ = line_.substr(indexR, line_.size());
    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(file_name_,'_',tokens_);
    std::string ticker_ = tokens_[1];
		std::string type_ = tokens_[2];
		std::string strike_ = tokens_[3];
    int expiry_ = atoi(tokens_[4].c_str());
    std::string hash_string_ = ticker_ + "_" + type_ + "_" + strike_ + "_"; 
    if (expiry_ == expiry_vec_[0]) {
      hash_string_ += "0";
    } else if (expiry_ == expiry_vec_[1]) {
      hash_string_ += "1";
    } else if (expiry_ == expiry_vec_[2]) {
      hash_string_ += "2";
    } else {
      std::cerr << "Error : Invalid expiry for the file " << line_ << std::endl;
      std::cerr << "Ignoring .. " << std::endl;
      continue;
    }

    HFSAT::BulkFileReader reader_;
    reader_.open(line_);
    if (!reader_.is_open()) {
      std::cerr << "Error : failed to open " << line_ << " , exiting... " << std::endl;
      continue;
    }
    while (true) {
      NSE_MDS::NSEDotexOfflineCommonStruct data_struct_;
      if (reader_.read(&data_struct_, sizeof(data_struct_)) < 10)
        break;
      if (data_struct_.msg_type != NSE_MDS::MsgType::kNSETrade)
        continue;
      int bucket_time_ = data_struct_.source_time.tv_sec - (data_struct_.source_time.tv_sec % 60);
      double price_ = data_struct_.data.nse_dotex_trade.trade_price;
      double qty_ = data_struct_.data.nse_dotex_trade.trade_quantity;
      if (map_priceX_.find(hash_string_) == map_priceX_.end()) {
        map<int, Metrics> prices;
        Metrics metrics;
        metrics.open_price = price_;
        metrics.close_price = price_;
        metrics.low_price = price_;
        metrics.high_price = price_;
        metrics.trades = 1;
        metrics.volume = qty_;
        metrics.start_time = data_struct_.source_time.tv_sec;
        metrics.close_time = data_struct_.source_time.tv_sec;
        metrics.expiry = expiry_;
        prices.insert(make_pair(bucket_time_, Metrics()));
        prices[bucket_time_] = metrics;
        map_priceX_.insert(make_pair(hash_string_, prices));
      }
      // Case when hash string exists in the map
      else {
        // Case when the timestamp does not exist in the smaller map
        if (map_priceX_[hash_string_].find(bucket_time_) ==
            map_priceX_[hash_string_].end()) {
          Metrics metrics;
          metrics.open_price = price_;
          metrics.close_price = price_;
          metrics.low_price = price_;
          metrics.high_price = price_;
          metrics.volume = qty_;
          metrics.trades = 1;
          metrics.start_time = data_struct_.source_time.tv_sec;
          metrics.close_time = data_struct_.source_time.tv_sec;
          metrics.expiry = expiry_;
          map_priceX_[hash_string_].insert(make_pair(bucket_time_, Metrics()));
          map_priceX_[hash_string_][bucket_time_] = metrics;
        }
        // Case when everything in place. COMPLEX
        else {
          Metrics metrics = map_priceX_[hash_string_][bucket_time_];
          if (data_struct_.source_time.tv_sec < metrics.start_time) {
            metrics.start_time = data_struct_.source_time.tv_sec;
            metrics.open_price = price_;
          }
          if (data_struct_.source_time.tv_sec > metrics.close_time) {
            metrics.close_time = data_struct_.source_time.tv_sec;
            metrics.close_price = price_;
          }
          if (price_ > metrics.high_price)
            metrics.high_price = price_;
          if (price_ < metrics.low_price)
            metrics.low_price = price_;
          metrics.volume += qty_;
          metrics.trades++;
          map_priceX_[hash_string_][bucket_time_] = metrics;
        }
      }
    }
    reader_.close();
  }

  typedef map<string, map<int, Metrics>>::iterator it_type;
  for (it_type iterator = map_priceX_.begin(); iterator != map_priceX_.end();
       iterator++) {
    typedef map<int, Metrics>::iterator it_typeX;
    int indexR = iterator->first.find("_");
		std::string outfilename = iterator->first.substr(0, indexR);
    std::ofstream outstream_;
    outstream_.open(output_dir_ + outfilename, std::ofstream::app);
    outstream_ << std::fixed;
    outstream_ << std::setprecision(2);
    for (it_typeX itX = iterator->second.begin(); itX != iterator->second.end();
         itX++) {
      int time_ = (itX->first) - ((itX->first) % 60);
      outstream_ << time_ << "\t" << 
				      iterator->first << "\t" << 
              itX->second.start_time << "\t" << 
              itX->second.close_time << "\t" << 
      				itX->second.expiry << "\t" << 
							itX->second.open_price << "\t" << 
      				itX->second.close_price << "\t" << 
      				itX->second.low_price << "\t" << 
      				itX->second.high_price << "\t" <<
							itX->second.volume << "\t" << 
      				itX->second.trades << "\n";
    }
 
  }
}
