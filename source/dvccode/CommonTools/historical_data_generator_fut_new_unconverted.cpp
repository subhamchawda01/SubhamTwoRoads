#include <map>
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <stdlib.h>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/mds_messages.hpp"

struct Metrics {
  int64_t start_time;
  int64_t end_time;
  double open_price;
  double close_price;
  double high_price;
  double low_price;
  uint64_t volume;
  uint64_t trades;
  double total_trade_qty_price;
};

int main(int argc, char const* argv[]) {
  if (argc < 5) {
    std::cout << "USAGE : <exec> <date> <products_data_filenames_file> <expiry_file> <output_dir>" << std::endl;
    exit(-1);
  }
  // uint32_t date = atoi(argv[1]);
  
  std::string products_data_filenames_file_(argv[2]);
  std::string expiry_file (argv[3]);
  std::ifstream file(expiry_file);
    if (!file) {
        std::cerr << "Error: Could not open file " << expiry_file << std::endl;
        return 1;
    }
  std::vector<std::string> expiry_values;
  std::string line;
  while (std::getline(file, line)) {
    expiry_values.push_back(line);
  }
  
    
  file.close();
  std::sort(expiry_values.begin(), expiry_values.end());
  std::string output_dir_(argv[4]);
  std::string line_;
  std::ifstream if_stream_;
  if_stream_.open(products_data_filenames_file_, std::ifstream::in);
  if (!if_stream_.is_open()) {
    std::cerr << "Error : Unable to open product file " << products_data_filenames_file_ << std::endl;
    exit(-1);
  }
  while (std::getline(if_stream_, line_)) {
    size_t indexR = line_.rfind("/");
    std::string file_name_ = line_.substr(indexR, line_.size());
    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(file_name_, '_', tokens_);
    std::string ticker_ = tokens_[1];
    std::string hash_string_ = ticker_;
    int expiry_number = 0;
    HFSAT::BulkFileReader reader_;
    std::string expiry = tokens_[3];
    auto it = std::find(expiry_values.begin(), expiry_values.end(), expiry);
    if (it != expiry_values.end()) {
     expiry_number = std::distance(expiry_values.begin(), it);
    }
    std::stringstream ss;
    ss << "_FF_0_" <<expiry_number;
    reader_.open(line_);
    if (!reader_.is_open()) {
      std::cerr << "Error : failed to open " << line_ << " , exiting... " << std::endl;
      continue;
    }

    HFSAT::MDS_MSG::GenericMDSMessage data_struct_generic_;
    NSE_MDS::NSETBTDataCommonStruct data_struct_;
    std::map<std::string, std::map<uint64_t, Metrics>> hash_string_to_metrics_map;
    while (true) {
      size_t available_length_ = reader_.read(&data_struct_generic_, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
      if (available_length_ < sizeof(data_struct_generic_)) {
        break;
      }
      memcpy((void*)(&data_struct_), (void*)(&(data_struct_generic_.generic_data_.nse_data_)),sizeof( NSE_MDS::NSETBTDataCommonStruct));

      if (data_struct_.msg_type == NSE_MDS::MsgType::kNSETrade) {
      uint64_t bucket_time_ = data_struct_.source_time.tv_sec - (data_struct_.source_time.tv_sec % 60);
      int date_time = bucket_time_ % (24 * 3600);
      int hour = date_time / 3600;
      date_time %= 3600;
      int minutes = date_time / 60;
      if (hour < 3 || hour >= 10 || (hour == 3 && minutes < 45)) continue;
      double price_ = double(data_struct_.data.nse_trade.trade_price)/100;
      double qty_ = data_struct_.data.nse_trade.trade_qty;
      // Case when no map from a particular hash string
      if (hash_string_to_metrics_map.find(hash_string_) == hash_string_to_metrics_map.end()) {
        std::map<uint64_t, Metrics> prices_;
        Metrics t_metrics_;
        t_metrics_.start_time = data_struct_.source_time.tv_sec;
        t_metrics_.end_time = data_struct_.source_time.tv_sec;
        t_metrics_.open_price = price_;
        t_metrics_.close_price = price_;
        t_metrics_.high_price = price_;
        t_metrics_.low_price = price_;
        t_metrics_.volume = qty_;
        t_metrics_.total_trade_qty_price += price_ * qty_;
        t_metrics_.trades = 1;
        prices_.insert(std::make_pair(bucket_time_, t_metrics_));
        hash_string_to_metrics_map.insert(std::make_pair(hash_string_, prices_));
      }
      // Case when hash string exists in the map
      else {
        // Case when the timestamp does not exist in the smaller map
        if (hash_string_to_metrics_map[hash_string_].find(bucket_time_) ==
            hash_string_to_metrics_map[hash_string_].end()) {
          std::map<uint64_t, Metrics> prices_;
          Metrics t_metrics_;
          t_metrics_.start_time = data_struct_.source_time.tv_sec;
          t_metrics_.end_time = data_struct_.source_time.tv_sec;
          t_metrics_.open_price = price_;
          t_metrics_.close_price = price_;
          t_metrics_.high_price = price_;
          t_metrics_.low_price = price_;
          t_metrics_.volume = qty_;
          t_metrics_.total_trade_qty_price += price_ * qty_;
          t_metrics_.trades = 1;
          hash_string_to_metrics_map[hash_string_].insert(std::make_pair(bucket_time_, t_metrics_));
        } else {
          // Case when everything in place.
          Metrics& t_metrics_ = hash_string_to_metrics_map[hash_string_][bucket_time_];
          if (data_struct_.source_time.tv_sec < t_metrics_.start_time) {
            t_metrics_.start_time = data_struct_.source_time.tv_sec;
            t_metrics_.open_price = price_;
          }
          if (data_struct_.source_time.tv_sec > t_metrics_.end_time) {
            t_metrics_.end_time = data_struct_.source_time.tv_sec;
            t_metrics_.close_price = price_;
          }
          t_metrics_.high_price = std::max(t_metrics_.high_price, price_);
          t_metrics_.low_price = std::min(t_metrics_.low_price, price_);
          t_metrics_.total_trade_qty_price += price_ * qty_;
          t_metrics_.volume += qty_;
          t_metrics_.trades++;
          //hash_string_to_metrics_map[hash_string_][bucket_time_] = t_metrics_;
        }
      }
      }
    }
    reader_.close();
    typedef std::map<std::string, std::map<uint64_t, Metrics>>::iterator it_type;
    for (it_type iterator = hash_string_to_metrics_map.begin(); iterator != hash_string_to_metrics_map.end();
         iterator++) {
      typedef std::map<uint64_t, Metrics>::iterator it_typeX;
      int indexR = iterator->first.find("_");
      std::string outfilename = iterator->first.substr(0, indexR);
      std::ofstream outstream_;
      outstream_.open(output_dir_ + outfilename, std::ofstream::app);
      outstream_ << std::fixed;
      outstream_ << std::setprecision(2);
      for (it_typeX iteratorX = iterator->second.begin(); iteratorX != iterator->second.end(); iteratorX++) {
        uint64_t time = (iteratorX->first) - ((iteratorX->first) % 60);
        outstream_ << time << "\t" << iterator->first<<ss.str() << "\t" << iteratorX->second.start_time << "\t"
                   << iteratorX->second.end_time << "\t" << expiry << "\t" <<iteratorX->second.open_price << "\t"
                   << iteratorX->second.close_price << "\t" << iteratorX->second.low_price << "\t"
                   << iteratorX->second.high_price << "\t" << iteratorX->second.volume << "\t"
                   << iteratorX->second.trades << "\n";
      }
    }
    hash_string_to_metrics_map.clear();
  }
  return 0;
};
