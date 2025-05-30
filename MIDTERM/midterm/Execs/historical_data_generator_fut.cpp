#include <map> 
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <stdlib.h>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

struct Metrics {
  int64_t start_time;
  int64_t end_time;
  uint32_t expiry;
  double open_price;
  double close_price;
  double high_price;
  double low_price;
  uint64_t volume;
  uint64_t trades;
};

std::vector<uint32_t> getSortedExpiryVec(std::string _expiry_file_, uint32_t _date_) {
  std::ifstream if_stream_;
  if_stream_.open(_expiry_file_, std::ifstream:: in );
  if (!if_stream_.is_open()) {
    std::cerr << "Error : Unable to open expiry file => " << _expiry_file_ << std::endl;
    exit(-1);
  }
  std::string line_;
  std::vector<uint32_t> expiry_vec_;
  while (std::getline(if_stream_, line_)) {
    uint32_t expiry_ = atoi(line_.c_str());
    if (expiry_ >= _date_) {
      expiry_vec_.push_back(expiry_);
    }
  }
  sort(expiry_vec_.begin(), expiry_vec_.end());
  return expiry_vec_;
}

int main(int argc, char const * argv[]) {
  if (argc < 5) {
    std::cout << "USAGE : <exec> <date> <products_data_filenames_file> <expiry_file> <output_dir>" << std::endl;
    exit(-1);
  }

  uint32_t date = atoi(argv[1]);
  std::string products_data_filenames_file_(argv[2]);
  std::string expiry_filename_(argv[3]);
  std::string output_dir_(argv[4]);

  std::vector<uint32_t> expiry_vec_ = getSortedExpiryVec(expiry_filename_, date);
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
    uint32_t expiry_ = atoi(tokens_[3].c_str());
    std::string hash_string_ = ticker_ + "_FF_0_";
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
    NSE_MDS::NSEDotexOfflineCommonStruct data_struct_;
    std::map < std::string, std::map<uint64_t, Metrics>> hash_string_to_metrics_map;
    while (true) {
      size_t available_length_ = reader_.read( & data_struct_, sizeof(NSE_MDS::NSEDotexOfflineCommonStruct));
      if (available_length_ < sizeof(data_struct_)) {
        break;
      }
      if (data_struct_.msg_type != NSE_MDS::MsgType::kNSETrade) {
        continue;
      }
      uint64_t bucket_time_ = data_struct_.source_time.tv_sec - (data_struct_.source_time.tv_sec % 60);
      double price_ = data_struct_.data.nse_dotex_trade.trade_price;
      double qty_ = data_struct_.data.nse_dotex_trade.trade_quantity;
      //Case when no map from a particular hash string 
      if (hash_string_to_metrics_map.find(hash_string_) == hash_string_to_metrics_map.end()) {
        std::map <uint64_t, Metrics> prices_;
        Metrics t_metrics_;
        t_metrics_.start_time = data_struct_.source_time.tv_sec;
        t_metrics_.end_time = data_struct_.source_time.tv_sec;
        t_metrics_.expiry = expiry_;
        t_metrics_.open_price = price_;
        t_metrics_.close_price = price_;
        t_metrics_.high_price = price_;
        t_metrics_.low_price = price_;
        t_metrics_.volume = qty_;
        t_metrics_.trades = 1;
        prices_.insert(std::make_pair(bucket_time_, t_metrics_));
        hash_string_to_metrics_map.insert(std::make_pair(hash_string_, prices_));
      }
      //Case when hash string exists in the map 
      else {
        //Case when the timestamp does not exist in the smaller map 
        if (hash_string_to_metrics_map[hash_string_].find(bucket_time_) == hash_string_to_metrics_map[hash_string_].end()) {
          std::map<uint64_t, Metrics> prices_;
          Metrics t_metrics_;
          t_metrics_.start_time = data_struct_.source_time.tv_sec;
          t_metrics_.end_time = data_struct_.source_time.tv_sec;
          t_metrics_.expiry = expiry_;
          t_metrics_.open_price = price_;
          t_metrics_.close_price = price_;
          t_metrics_.high_price = price_;
          t_metrics_.low_price = price_;
          t_metrics_.volume = qty_;
          t_metrics_.trades = 1;
          hash_string_to_metrics_map[hash_string_].insert(std::make_pair(bucket_time_, t_metrics_));
        } else {
          //Case when everything in place.
          Metrics t_metrics_ = hash_string_to_metrics_map[hash_string_][bucket_time_];
          if (data_struct_.source_time.tv_sec < t_metrics_.start_time) {
            t_metrics_.start_time = data_struct_.source_time.tv_sec;
            t_metrics_.open_price = price_;
          }
          if (data_struct_.source_time.tv_sec > t_metrics_.end_time) {
            t_metrics_.end_time = data_struct_.source_time.tv_sec;
            t_metrics_.close_price = price_;
          }
          if (price_ > t_metrics_.high_price) {
            t_metrics_.high_price = price_;
          }
          if (price_ < t_metrics_.low_price) {
            t_metrics_.low_price = price_;
          }
          t_metrics_.volume += qty_;
          t_metrics_.trades++;
          hash_string_to_metrics_map[hash_string_][bucket_time_] = t_metrics_;
        }
      }
    }
    reader_.close();
    typedef std::map<std::string, std::map<uint64_t, Metrics>>::iterator it_type;
    for (it_type iterator = hash_string_to_metrics_map.begin(); iterator != hash_string_to_metrics_map.end(); iterator++) {
      typedef std::map<uint64_t, Metrics> ::iterator it_typeX;
      int indexR = iterator->first.find("_");
      std::string outfilename = iterator->first.substr(0, indexR);
      std::ofstream outstream_;
      outstream_.open(output_dir_ + outfilename, std::ofstream::app);
      outstream_ << std::fixed;
      outstream_ << std::setprecision(2);
      for (it_typeX iteratorX = iterator->second.begin(); iteratorX != iterator->second.end(); iteratorX++) {
        uint64_t time = (iteratorX->first) - ((iteratorX->first) % 60);
        outstream_ << time << "\t" <<
          iterator -> first << "\t" <<
          iteratorX -> second.start_time << "\t" <<
          iteratorX -> second.end_time << "\t" <<
          iteratorX -> second.expiry << "\t" <<
          iteratorX -> second.open_price << "\t" <<
          iteratorX -> second.close_price << "\t" <<
          iteratorX -> second.low_price << "\t" <<
          iteratorX -> second.high_price << "\t" <<
          iteratorX -> second.volume << "\t" <<
          iteratorX -> second.trades << "\n";
      }
    }
    hash_string_to_metrics_map.clear();
  }
  return 0;
};
