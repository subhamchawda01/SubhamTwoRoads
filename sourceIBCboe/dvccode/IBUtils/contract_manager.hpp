// =====================================================================================
// 
//       Filename:  contract_manager.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  09/18/2024 09:06:36 AM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551 
// 
// =====================================================================================


#pragma once

#include <sstream>
#include <cstdlib>
#include <map>

#include "dvccode/Utils/mds_logger.hpp"

#define BARDATA_SHORTCODE_LENGTH 32

enum IBUpdateType {IB_TICK_UPDATE = 1, IB_TICK_SIZE_ONLY_UPDATE = 2, IB_TRADE = 3, IB_GREEKS_UPDATE = 4, IB_INFO_UPDATE = 5};

class TickerMapping{
 
  private:
   TickerMapping() {}
   TickerMapping(TickerMapping const & disabled_copy_constructor) = delete;

  public:
   std::map<int, std::string> ticker_id_to_symbol;
   static TickerMapping & GetUniqueInstance(){
     static TickerMapping unique_instance;
     return unique_instance;
   }

  std::string GetStringFromIBUpdateType(IBUpdateType ib_update_type){
    switch(ib_update_type){
      case IB_TICK_UPDATE:
        return "IB_TICK_UPDATE";
        break;
      case IB_TICK_SIZE_ONLY_UPDATE:
        return "IB_TICK_SIZE_ONLY_UPDATE";
        break;
      case IB_TRADE:
        return "IB_TRADE";
        break;
      case IB_GREEKS_UPDATE:
        return "IB_GREEKS_UPDATE";
        break;
      case IB_INFO_UPDATE:
        return "IB_INFO_UPDATE";
        break;
      default:
        return "INVALID";
        break;
    }
  }

};

struct OptionGreeks{
  double imp_vol;
  double delta;
  double option_price;
  double p_div;
  double gamma;
  double vega;
  double theta;
  double spot;

  std::string ToString(){
    std::ostringstream t_temp_oss;
    t_temp_oss << "IMPLIED_VOL: " << imp_vol << "\n";
    t_temp_oss << "DELTA: " << delta << "\n";
    t_temp_oss << "OPT_PRICE: " << option_price << "\n";
    t_temp_oss << "P_DIVIDEND: " << p_div << "\n";
    t_temp_oss << "GAMMA: " << gamma << "\n";
    t_temp_oss << "VEGA: " << vega << "\n";
    t_temp_oss << "THETA: " << theta << "\n";
    t_temp_oss << "SPOT: " << spot << "\n";
    return t_temp_oss.str();
  }

};

struct IBL1UpdateTick{
  uint64_t packet_seq;
  struct timeval time;
  IBUpdateType ib_update_type;
  int ticker_id;
  char symbol[48];
  char side;
  double bid_price;
  int bid_size;
  double ask_price;
  int ask_size;
  double trade_price;
  int trade_size;
  double high_price;
  double low_price;
  int volume;
  double last_close_price;
  double open_interest;
  OptionGreeks opg;

  std::string ToString(){
    std::ostringstream t_temp_oss;
    t_temp_oss << "========================== IBL1UpdateTick ==========================\n";
    t_temp_oss << "PACKET_SEQ: " << packet_seq << "\n";
    t_temp_oss << "TIME: " << time.tv_sec << "." << std::setw(6) << std::setfill('0') << time.tv_usec << "\n";
    t_temp_oss << "UPDATE_TYPE: " << TickerMapping::GetUniqueInstance().GetStringFromIBUpdateType(ib_update_type) << "\n";
    t_temp_oss << "TICKER_ID: " << ticker_id << "\n";
    t_temp_oss << "SIDE: " << side << "\n";
    t_temp_oss << "SYMBOL: " << symbol << "\n";
    t_temp_oss << "BID_SIZE: " << bid_size << "\n";
    t_temp_oss << "BID_PRICE: " << bid_price << "\n";
    t_temp_oss << "ASK_PRICE: " << ask_price << "\n";
    t_temp_oss << "ASK_SIZE: " << ask_size << "\n";
    t_temp_oss << "TRADE_PRICE: " << trade_price << "\n";
    t_temp_oss << "TRADE_SIZE: " << trade_size << "\n";
    t_temp_oss << "HIGH_PRICE: " << high_price << "\n";
    t_temp_oss << "LOW_PRICE: " << low_price << "\n";
    t_temp_oss << "VOLUME: " << volume << "\n";
    t_temp_oss << "LAST_CLOSE: " << last_close_price << "\n";
    t_temp_oss << "OPEN_INTEREST: " << open_interest << "\n";
    t_temp_oss << opg.ToString() << "\n";
    return t_temp_oss.str();
  }

  const char* getContract(){
    return (TickerMapping::GetUniqueInstance().ticker_id_to_symbol[ticker_id]).c_str();
  }

  const char* getShortcode(){
    return "INVALID";
  }

};

struct CBOEBarDataCommonStruct {
  int32_t bardata_time;
  char shortcode[BARDATA_SHORTCODE_LENGTH];
  int32_t start_time;
  int32_t close_time;
  int32_t expiry;
  double open_price;
  double close_price;
  double low_price;
  double high_price;
  int32_t volume;
  int32_t trades;
      
  std::string ToString() {
    std::ostringstream t_temp_oss; 
      
    t_temp_oss << bardata_time << " " << shortcode << " " << start_time << " " << close_time << " "
               << expiry << " " << std::fixed << std::setprecision(2) << open_price << " "
               << close_price << " " << low_price << " " << high_price << " " << volume << " " << trades << "\n";
      
    return t_temp_oss.str();
  }   
        
  char const* getShortcode() {
    return shortcode;
  }     
        
  char const* getContract() {
    return "INVALID";
  }
};

class IBDataListener{
 public:
  ~IBDataListener() {}
  virtual void OnIBKRRawDataUpdate(IBL1UpdateTick* market_event) {
    std::cout << "SHOULDN'T POINT HERE FROM IBDataListener: " << std::endl;
    std::exit(-1);
  }
};

