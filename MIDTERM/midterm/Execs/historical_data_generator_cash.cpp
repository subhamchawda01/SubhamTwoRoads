//============================================================================
// Name        : binaryconverter.cpp
// Author      : Rishabh Garg
// Version     :
// Description : Implemented for FUT only
//============================================================================
#include <iostream>
#include <ctime>
#include <sstream>
#include <fstream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <map>
#include <memory>
#include <cstdio>
#include <string>
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
using namespace std;
using namespace boost;

struct Output {
  double open_price, close_price, low_price, high_price;
  int trades, volume;
  int start_time;
  int close_time;
  string expiry;

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
map<string, map<int, Output>> map_priceX_;

enum class MsgType {
  kInvalid = 0,
  kNSEOrderDelta,
  kNSETrade,
  kNSETradeDelta,
  kNSEOrderSpreadDelta,
  kNSESpreadTrade
};

struct NSEDotexTrade {
  double trade_price;
  uint64_t buy_order_number;
  uint64_t sell_order_number;
  int32_t bid_size_remaining;
  int32_t ask_size_remaining;
  uint32_t trade_quantity;
  char buy_algo_indicator;
  char sell_algo_indicator;
  char buy_client_id_flag;
  char sell_client_id_flag;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "TradePrice:      " << trade_price << "\n";
    t_temp_oss << "TradeQty:        " << trade_quantity << "\n";
    t_temp_oss << "BuyOrderNum:     " << buy_order_number << "\n";
    t_temp_oss << "BuyLeftOver:     " << bid_size_remaining << "\n";
    t_temp_oss << "SellOrderNum:    " << sell_order_number << "\n";
    t_temp_oss << "SellLeftOver:    " << ask_size_remaining << "\n";

    return t_temp_oss.str();
  }
};

struct NSEDotexOrderDelta {
  double order_price;
  double old_price;
  uint32_t volume_original;
  uint32_t old_size;
  char buysell;
  char activity_type;
  char spread_comb_type;
  char algo_indicator;
  char client_id_flag;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "OrderPrice:      " << order_price << "\n";
    t_temp_oss << "OldPrice:        " << old_price << "\n";
    //      t_temp_oss << "AggSize:         " << ( uint32_t ) aggregate_size <<
    //      "\n" ;
    t_temp_oss << "VolumeOrig:      " << (uint32_t)volume_original << "\n";
    t_temp_oss << "OldVolume:       " << (uint32_t)old_size << "\n";
    //      t_temp_oss << "NumberOfOrders:  " << ( uint32_t ) number_of_orders
    //      << "\n" ;
    t_temp_oss << "BuySell:         " << buysell << "\n";
    // t_temp_oss << "EntryType:       " << GetActivityStr(activity_type) <<
    // "\n";
    // t_temp_oss << "SpreadType:      " << GetCombTypeString(spread_comb_type)
    // << "\n";
    // t_temp_oss << "AlgoIndicator:   " <<
    // GetAlgoIndicatorString(algo_indicator) << "\n";
    // t_temp_oss << "ClientId:        " << GetClientIdString(client_id_flag) <<
    // "\n";
    //      t_temp_oss << "IsPriceFeed:     " << ( is_pricefeed_event ? "Y" :
    //      "N" ) << "\n" ;
    //      t_temp_oss << "PLEntryType:     " << GetActivityStr (
    //      price_level_activity ) << "\n" ;
    //      t_temp_oss << "PriceLevel:      " << ( uint32_t ) price_level <<
    //      "\n" ;

    return t_temp_oss.str();
  }
};

struct NSEDotexOfflineCommonStruct {
  struct timeval source_time;
  double strike_price;
  char symbol[48];
  uint64_t order_number;
  MsgType msg_type;
  char option_type[2];

  union {
    NSEDotexOrderDelta nse_dotex_order_delta;
    NSEDotexTrade nse_dotex_trade;

  } data;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    switch (msg_type) {

    case MsgType::kNSEOrderDelta: {
      t_temp_oss
          << "\n=================== NSEBookDelta ===================\n\n";
      t_temp_oss << "Time:            " << source_time.tv_sec << "."
                 << std::setw(6) << std::setfill('0') << source_time.tv_usec
                 << "\n";
      t_temp_oss << "OrderNum:        " << order_number << "\n";
      // t_temp_oss << "Symbol:          " << symbol << "\n";
      t_temp_oss << "StrikePrice:     " << strike_price << "\n";
      t_temp_oss << "OptionType:      " << option_type << "\n";

      t_temp_oss << data.nse_dotex_order_delta.ToString()
                 << "===================================================\n";

    } break;

    case MsgType::kNSETrade: {
      t_temp_oss << "\n=================== NSETrade ===================\n\n";
      t_temp_oss << "Time:            " << source_time.tv_sec << "."
                 << std::setw(6) << std::setfill('0') << source_time.tv_usec
                 << "\n";
      t_temp_oss << "TradeNumber:     " << order_number << "\n";
      t_temp_oss << "Symbol:          " << symbol << "\n";
      t_temp_oss << "StrikePrice:     " << strike_price << "\n";
      t_temp_oss << "OptionType:      " << option_type << "\n";

      t_temp_oss << data.nse_dotex_trade.ToString()
                 << "===================================================\n";

    } break;

    default: { t_temp_oss << ""; } break;
    }

    return t_temp_oss.str();
  }
};

string PrintCompletedBucket(int bucket_) {
  if (bucket_ == -1)
    return "";
  ostringstream t_temp_oss;
  string ready_string = "";
  string delim_ = "^";
  typedef map<string, map<int, Output>>::iterator it_type;
  for (it_type iterator = map_priceX_.begin(); iterator != map_priceX_.end();
       iterator++) {
    typedef map<int, Output>::iterator it_typeX;
    for (it_typeX itX = iterator->second.begin(); itX != iterator->second.end();
         itX++) {
      if (itX->first != bucket_)
        continue;
      t_temp_oss << itX->first << "\t" << iterator->first << "\t"
                 << (itX->second).ToString();
    }
  }
  return t_temp_oss.str();
}

vector<int> GetExpiriesFromMap(int YYYYMMDDDate_) {
  ifstream if_;
  string filename_ = "/home/dvctrader/midterm/Data_Generator/expiries.txt";
  if_.open(filename_.c_str());
  string line;
  vector<int> expiries;
  while (getline(if_, line)) {
    // Do the tokenization to get each entry of a trade into a vector
    size_t pos = 0;
    string token;
    vector<string> temp;
    while ((pos = line.find("\t")) != string::npos) {
      token = line.substr(0, pos);
      line.erase(0, pos + 1);
      temp.push_back(token);
    }
    temp.push_back(line.substr(0, line.size()));
    if (atoi(temp[0].c_str()) != YYYYMMDDDate_)
      continue;
    expiries.push_back(atoi(temp[1].c_str()));
    expiries.push_back(atoi(temp[2].c_str()));
    expiries.push_back(atoi(temp[3].c_str()));
    return expiries;
  }
  return expiries;
}

void print_usage_and_exit( char const * prog_name ) {
  cout << "THIS IS THE HISTORICAL BAR GENERATOR FOR HFT TRADE DATA" << std::endl;

}



int main(int argc, char **argv) {

  ifstream if_;
  string line_;
  if_.open("/home/dvctrader/midterm/Data_Generator/FILENAMES");

  while (getline(if_, line_)) {
    cout << "Reading -> " << line_ << endl;
    int bucket_ = -1;
    string fullPath = line_;

    string path_ = fullPath;
    size_t pos = 0;
    vector<string> tempZ;
    string token;
    while ((pos = path_.find("_")) != string::npos) {
      token = path_.substr(0, pos);
      path_.erase(0, pos + 1);
      tempZ.push_back(token);
    }
    if (path_.substr(path_.size() - 3, path_.size()) == ".gz")
      tempZ.push_back(path_.substr(0, path_.size() - 3));
    else
      tempZ.push_back(path_.substr(0, path_.size()));

    string ticker_ = tempZ[1];

    string line;
    HFSAT::BulkFileReader reader_;
    reader_.open(fullPath);
    while (reader_.is_open()) {

      NSEDotexOfflineCommonStruct xxx;
      if (reader_.read(&xxx, sizeof(xxx)) < 10)
        break;

      if (xxx.msg_type != MsgType::kNSETrade)
        continue;

      int bucket_time_ = xxx.source_time.tv_sec - (xxx.source_time.tv_sec % 60);
      if (bucket_time_ != bucket_) {
        // cout << PrintCompletedBucket(bucket_) << endl;
        bucket_ = bucket_time_;
      }

      string hash_string = ticker_;
      //stringstream xx;
      //xx << exp_;

      hash_string = hash_string;
      // cout << hash_string << endl;
      double price_ = xxx.data.nse_dotex_trade.trade_price;
      double qty_ = xxx.data.nse_dotex_trade.trade_quantity;
      // cout << price_ << endl;
      //////////////////// ADD TO MAP //////////////
      // Case when no map from a particular hash string
      // cout << "adding to map" << endl;
      if (map_priceX_.find(hash_string) == map_priceX_.end()) {
        map<int, Output> prices;
        Output metrics;
        metrics.open_price = price_;
        metrics.close_price = price_;
        metrics.low_price = price_;
        metrics.high_price = price_;
        metrics.trades = 1;
        metrics.volume = qty_;
        metrics.start_time = xxx.source_time.tv_sec;
        metrics.close_time = xxx.source_time.tv_sec;
        metrics.expiry = -1;

        prices.insert(make_pair(bucket_time_, Output()));
        prices[bucket_time_] = metrics;
        map_priceX_.insert(make_pair(hash_string, prices));
      }
      // Case when hash string exists in the map
      else {
        // Case when the timestamp does not exist in the smaller map
        if (map_priceX_[hash_string].find(bucket_time_) ==
            map_priceX_[hash_string].end()) {
          Output metrics;
          metrics.open_price = price_;
          metrics.close_price = price_;
          metrics.low_price = price_;
          metrics.high_price = price_;
          metrics.volume = qty_;
          metrics.trades = 1;
          metrics.start_time = xxx.source_time.tv_sec;
          metrics.close_time = xxx.source_time.tv_sec;
          metrics.expiry = -1;
          map_priceX_[hash_string].insert(make_pair(bucket_time_, Output()));
          map_priceX_[hash_string][bucket_time_] = metrics;
        }
        // Case when everything in place. COMPLEX
        else {
          Output metrics = map_priceX_[hash_string][bucket_time_];
          if (xxx.source_time.tv_sec < metrics.start_time) {
            metrics.start_time = xxx.source_time.tv_sec;
            metrics.open_price = price_;
          }
          if (xxx.source_time.tv_sec > metrics.close_time) {
            metrics.close_time = xxx.source_time.tv_sec;
            metrics.close_price = price_;
          }
          if (price_ > metrics.high_price)
            metrics.high_price = price_;
          if (price_ < metrics.low_price)
            metrics.low_price = price_;
          metrics.volume += qty_;
          metrics.trades++;
          // No need to set expiry here
          map_priceX_[hash_string][bucket_time_] = metrics;
        }
      }
    }
    reader_.close();
  

  cout << "reaching for print" << endl;
  typedef map<string, map<int, Output>>::iterator it_type;
  for (it_type iterator = map_priceX_.begin(); iterator != map_priceX_.end();
       iterator++) {
    typedef map<int, Output>::iterator it_typeX;
    ofstream myfile;
    // Output path
    string outputpath = "/home/dvctrader/midterm/Data_Generator/Generated_Data/";
    int indexR = iterator->first.find("_");
    string outname = iterator->first.substr(0, indexR);
    myfile.open((outputpath + outname).c_str(), ios_base::app);
    for (it_typeX itX = iterator->second.begin(); itX != iterator->second.end();
         itX++) {
      // Hash String
      //      myfile << iterator->first;
      //      myfile << "\t";

      int time_ = (itX->first) - ((itX->first) % 60);
      // Timestamp
      myfile << time_;
      myfile << "\t";

      myfile << iterator->first;
      myfile << "\t";

      // First Trade Time
      myfile << itX->second.start_time;
      myfile << "\t";
      // Last Trade Time
      myfile << itX->second.close_time;
      myfile << "\t";
      myfile << "-1";
      myfile << "\t";
      myfile << itX->second.open_price;
      myfile << "\t";
      myfile << itX->second.close_price;
      myfile << "\t";
      myfile << itX->second.low_price;
      myfile << "\t";
      myfile << itX->second.high_price;
      myfile << "\t";
      myfile << itX->second.volume;
      myfile << "\t";
      myfile << itX->second.trades;
      myfile << "\n";
    }
  }
  map_priceX_.clear();
  }

}
