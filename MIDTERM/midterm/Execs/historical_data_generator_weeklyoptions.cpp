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
    // t_temp_oss << "BuyAlgo:         " <<
    // GetAlgoIndicatorString(buy_algo_indicator) << "\n";
    // t_temp_oss << "BuyClientId:     " <<
    // GetClientIdString(buy_client_id_flag) << "\n";
    t_temp_oss << "SellOrderNum:    " << sell_order_number << "\n";
    t_temp_oss << "SellLeftOver:    " << ask_size_remaining << "\n";
    // t_temp_oss << "SellAlgo:        " <<
    // GetAlgoIndicatorString(sell_algo_indicator) << "\n";
    // t_temp_oss << "SellClientId:    " <<
    // GetClientIdString(sell_client_id_flag) << "\n";

    return t_temp_oss.str();
  }
};

struct NSEDotexOrderDelta {
  double order_price;
  double old_price;
  //    uint32_t aggregate_size ;
  uint32_t volume_original;
  uint32_t old_size;
  //    uint16_t number_of_orders ;
  //    uint8_t price_level ;
  char buysell;
  char activity_type;
  char spread_comb_type;
  char algo_indicator;
  char client_id_flag;
  //    char is_pricefeed_event ;
  //    char price_level_activity ;
  //    char is_orderfeed_event ;

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
  //std::cout <<"Date ->" <<  YYYYMMDDDate_ << std::endl;
  ifstream if_;
  string filename_ = "/home/dvctrader/midterm/Data_Generator/expiries_weekly.txt";
  //      "/home/rishabh/Desktop/Exec_Logic/med_exec/core/expiries.txt";
  if_.open(filename_.c_str());
  string line;
  vector<int> expiries;
  while (getline(if_, line)) {
    // Do the tokenization to get each entry of a trade into a vector
    size_t pos = 0;
    string token;
    vector<string> temp;
    //std::cout << "Line->" << line << std::endl;
    while ((pos = line.find("\t")) != string::npos) {
      token = line.substr(0, pos);
      line.erase(0, pos + 1);
      temp.push_back(token);
    }
    temp.push_back(line.substr(0, line.size()));
    if (atoi(temp[0].c_str()) != YYYYMMDDDate_)
      continue;
    //std::cout << "Line->" << line << std::endl;
    expiries.push_back(atoi(temp[1].c_str()));
    expiries.push_back(atoi(temp[2].c_str()));
    expiries.push_back(atoi(temp[3].c_str()));
    expiries.push_back(atoi(temp[4].c_str()));
    expiries.push_back(atoi(temp[5].c_str()));
    expiries.push_back(atoi(temp[6].c_str()));
    expiries.push_back(atoi(temp[7].c_str()));
    expiries.push_back(atoi(temp[8].c_str()));
    expiries.push_back(atoi(temp[9].c_str()));
    expiries.push_back(atoi(temp[10].c_str()));
    expiries.push_back(atoi(temp[11].c_str()));

    return expiries;
  }
  return expiries;
}

int main(int argc, char **argv) {

  ifstream if_;
  string line_;
  if_.open("/home/dvctrader/midterm/Data_Generator/FILENAMES");
  int k = 0;
  //cout<<"Strating";
  while (getline(if_, line_)) {
    //cout << "Starting..." << endl;
    //cout << "loop id " << k << endl;
    k++;
    int bucket_ = -1;
    //string path = "/home/rishabh/Desktop/MISCX/";

    string fullPath = line_;
    std::cout<< line_ << "\n";
    string path_ = fullPath;
    size_t pos = 0;
    vector<string> tempZ;
    string token;
    while ((pos = path_.find("_")) != string::npos) {
      token = path_.substr(0, pos);
      path_.erase(0, pos + 1);

      tempZ.push_back(token);
    }

    // cout << tempZ.size() << endl;
    if (path_.substr(path_.size() - 3, path_.size()) == ".gz")
      tempZ.push_back(path_.substr(0, path_.size() - 3));
    else
      tempZ.push_back(path_.substr(0, path_.size()));
    //for( auto i : tempZ ) cout << i << endl;
    // cout << tempZ[ 5 ] << endl;
    string ticker_ = tempZ[1];
    string expiry_ = tempZ[4];
    string type_ = tempZ[2];
    double strike_ = atof(tempZ[3].c_str());
    //cout << "What's happening?" << endl;
    //cout << tempZ[6].c_str() << endl;
    vector<int> expiries_ = GetExpiriesFromMap(atoi(tempZ[5].c_str()));
    //cout << ticker_ << "\t hello \n";//strike_ << expiry_<< "\n";
    // cout << strike_;
    int exp_;
    // cout << tempZ[5] << endl;
    // cout << "hash expuiry " << tempZ[4] << endl;
    //    cout << expiries_.size() << endl;
    //    cout << tempZ[ 3 ] << endl;
    //        if( fullPath !=
    //        "/media/shared/ephemeral9/s3_cache/NAS1/data/NSELoggedData/NSE/2015/12/31/NSE_NIFTY_FUT_20151231_20151231.gz"
    //        ) continue;
   // cout << "Expiry0 is -> " << expiries_[ 0 ] << endl;
    //cout << "String is -> " << expiry_ << endl;
    // cout << tempZ[ 4 ] << endl;
    if (expiries_[0] == atoi(expiry_.c_str()))
      exp_ = 0;
    else if (expiries_[1] == atoi(expiry_.c_str()))
      exp_ = 1;
    else if (expiries_[2] == atoi(expiry_.c_str()))
      exp_ = 2;
    else if (expiries_[3] == atoi(expiry_.c_str()))
      exp_ = 3;
    else if (expiries_[4] == atoi(expiry_.c_str()))
      exp_ = 4;
    else if (expiries_[5] == atoi(expiry_.c_str()))
      exp_ = 5;
    else if (expiries_[6] == atoi(expiry_.c_str()))
      exp_ = 6;
    else if (expiries_[7] == atoi(expiry_.c_str()))
      exp_ = 7;
    else if (expiries_[8] == atoi(expiry_.c_str()))
      exp_ = 8;
    else if (expiries_[9] == atoi(expiry_.c_str()))
      exp_ = 9;
    else if (expiries_[10] == atoi(expiry_.c_str()))
      exp_ = 10;

    else
      exp_ = -1;
    //cout << "Value is -> " << exp_ << std::endl;
    if (exp_ == -1) {
    //   cout << expiries_[0] << "\t" << expiries_[1] << "\t" << expiries_[2] << "\t" << expiries_[3] << "\t" << expiries_[4] << "\t" << expiries_[5] << "\t" <<expiries_[6] << "\t" << expiries_[7];
      //cout<<endl;

     // cout << fullPath << endl;
      continue;
    }

    //    cout << "expiry is " << exp_ << endl;

    // cout << fullPath << endl;
    string line;
    // ifstream infile;
    // infile.open(fullPath.c_str());
    //    if( fullPath !=
    //    "/media/shared/ephemeral9/s3_cache/NAS1/data/NSELoggedData/NSE/2015/12/31/NSE_NIFTY_FUT_20151231_20151231.gz"
    //    ) continue;

    //cout << "Full path is: " << fullPath << endl;
    HFSAT::BulkFileReader reader_;
    reader_.open(fullPath);
    while (reader_.is_open()) {

      NSEDotexOfflineCommonStruct xxx;
      //cout << "print" << endl;
      //cout<<"Read size: "<<reader_.Read(&xxx, sizeof(xxx)) << endl;
      if (reader_.read(&xxx, sizeof(xxx)) < 10){
        //cout<<"Read size: "<<reader_.Read(&xxx, sizeof(xxx)) << endl;
        break;
      }
      //cout << "printX" << endl;
     //cout << xxx.ToString(); 
     if (xxx.msg_type != MsgType::kNSETrade)
        //cout<<"xxx.msg_type " << xxx.msg_type <<"\tMsgType::kNSETrade " << MsgType::kNSETrade <<endl;
        continue;
      //cout << xxx.ToString();
      int bucket_time_ = xxx.source_time.tv_sec - (xxx.source_time.tv_sec % 60);
      // cout << "bucket calc" << endl;
      if (bucket_time_ != bucket_) {
        // cout << PrintCompletedBucket(bucket_) << endl;
        bucket_ = bucket_time_;
      }
      // cout << "how come ?" << endl;
      // cout << ticker_ << endl;
      // cout << exp_ << endl;
      // if( ticker_ != "NIFTY" ) break;
      // if( exp_ != 0 ) break;
      // cout << "broken" << endl;
      stringstream yy;
      yy << strike_;
      string hash_string = ticker_ + "_" + type_ + "_" + yy.str() + "_";
      stringstream xx;
      xx << exp_;

      hash_string = hash_string + xx.str();
      //cout << hash_string << endl;

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
        metrics.expiry = expiry_;

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
          metrics.expiry = expiry_;
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
  
 
    //cout << "reaching for print" << endl;
  }
  //std::cout<< map_priceX_.size()<<std::endl;
  typedef map<string, map<int, Output>>::iterator it_type;
  for (it_type iterator = map_priceX_.begin(); iterator != map_priceX_.end();
       iterator++) {
    typedef map<int, Output>::iterator it_typeX;
    ofstream myfile;
    // Output path
    // Lets see
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
      //std::cout<<"Time: "<<time_<<std::endl;
      myfile << iterator->first;
      myfile << "\t";

      // First Trade Time
      myfile << itX->second.start_time;
      myfile << "\t";
      // Last Trade Time
      myfile << itX->second.close_time;
      myfile << "\t";
      myfile << itX->second.expiry;
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
}
