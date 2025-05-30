// =====================================================================================
//
//       Filename:  nse_data_viewer.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  04/30/2015 02:10:41 PM
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

#include <cstdlib>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <errno.h>
#include <string.h>
#include <map>
#include <set>

struct NSEDotexOfflineDataStruct {
  char record_indicator_type[2];
  char segment[4];
  char date[8];
  char order_number[8];
  char time_in_jiffies[14];
  char buy_or_sell;
  char activity_type;
  char symbol[10];
  char instrument[6];
  char expiry[9];
  char strike_price_integer[6];
  char strike_price_decimal_point[2];
  char option_type[2];
  char volume_disclosed[8];
  char volume_original[8];
  char limit_price_integer[6];
  char limit_price_decimal_point[2];
  char trigger_price_integer[6];
  char trigger_price_decimal_point[2];
  char mkt_flag;
  char stoploss_indicator;
  char ioc_flag;
  char spread_comb_type;
  char algo_trading_flag;
  char client_identity_flag;

  std::string GetEntryTypeString() {
    switch (activity_type) {
      case '1':
        return "OrderAdd";
      case '3':
        return "OrderDelete";
      case '4':
        return "OrderModify";
      default:
        return "Invalid";
    }

    return "Invalid";
  }

  std::string GetUnderlyingSymbol() {
    std::string symbol_without_padding = "";
    int i = 0;

    for (i = 0; i < 10; i++) {
      if (symbol[i] != 'b') break;
    }

    while (i < 10) {
      symbol_without_padding += (symbol[i]);
      i++;
    }

    return symbol_without_padding;
  }

  std::string GetInstrumentType(std::string _instrument_) {
    if ("FUTIDX" == std::string(_instrument_))
      return "IndexFut";
    else if ("OPTIDX" == std::string(_instrument_))
      return "IndexOpt";
    else if ("FUTSTK" == std::string(_instrument_))
      return "StockFut";
    else if ("OPTSTK" == std::string(_instrument_))
      return "StockOpt";

    return "Invalid";
  }

  std::string GetOptionType(std::string _option_type_) {
    if ("CA" == std::string(_option_type_))
      return "CallAmerican";
    else if ("PA" == std::string(_option_type_))
      return "PutAmerican";
    else if ("CE" == std::string(_option_type_))
      return "CallEuropean";
    else if ("PE" == std::string(_option_type_))
      return "PutEuropean";

    return "Invalid";
  }

  std::string GetSpreadType() {
    switch (spread_comb_type) {
      case 'S':
        return "SpreadOrder";
      case '2':
        return "2 Leg Order";
      case '3':
        return "3 Leg Order";

      default:
        return "NoLegOrder";
    }

    return "NoLegOrder";
  }

  std::string GetAlgoType() {
    switch (algo_trading_flag) {
      case '0':
        return "Algo Order";
      case '1':
        return "Non-Algo Order";
      case '2':
        return "Algo SOR Order";
      case '3':
        return "Non-Algo SOR Order";

      default:
        return "Invalid";
    }

    return "Invalid";
  }

  std::string GetClientIdentity() {
    switch (client_identity_flag) {
      case '1':
        return "Client - CP";
      case '2':
        return "Proprietary";
      case '3':
        return "Non CP - Non Prop";

      default:
        return "Invalid";
    }

    return "Invalid";
  }

  std::string GetNullPadValue(char *input, int length) {
    char output_char[length + 1];
    memset(output_char, 0, length + 1);
    memcpy(output_char, input, length);

    return output_char;
  }

  std::string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << "\n============== NSEDotexOrderBookMessage ==============\n";
    t_temp_oss << "Market_Type:    Regular Market Order"
               << "\n";
    t_temp_oss << "Segment:        EquityDerivatives"
               << "\n";
    t_temp_oss << "Date:           " << GetNullPadValue(date, 8) << "\n";
    t_temp_oss << "OrderNumber:    " << GetNullPadValue(order_number, 8) << "\n";
    t_temp_oss << "TransactTime:   " << GetNullPadValue(time_in_jiffies, 14) << "\n";
    t_temp_oss << "Buy/Sell:       " << buy_or_sell << "\n";
    t_temp_oss << "Entry_Type:     " << GetEntryTypeString() << "\n";
    t_temp_oss << "Symbol:         " << GetUnderlyingSymbol() << "\n";
    t_temp_oss << "Instrument:     " << GetInstrumentType(GetNullPadValue(instrument, 6)) << "\n";
    t_temp_oss << "ExpiryDate:     " << GetNullPadValue(expiry, 9) << "\n";
    t_temp_oss << "StrikePrice:    "
               << ((atoi(GetNullPadValue(strike_price_integer, 6).c_str()) * 100) +
                   atoi(GetNullPadValue(strike_price_decimal_point, 2).c_str())) /
                      100.00
               << "\n";
    t_temp_oss << "Option_Type:    " << GetOptionType(GetNullPadValue(option_type, 2)) << "\n";
    t_temp_oss << "VolumeDisclosed:" << atoi(GetNullPadValue(volume_disclosed, 8).c_str()) << "\n";
    t_temp_oss << "VolumeOriginal: " << atoi(GetNullPadValue(volume_original, 8).c_str()) << "\n";
    t_temp_oss << "OrderPrice:     "
               << ((atoi(GetNullPadValue(limit_price_integer, 6).c_str()) * 100) +
                   atoi(GetNullPadValue(limit_price_decimal_point, 2).c_str())) /
                      100.00
               << "\n";
    t_temp_oss << "TriggerPrice:   "
               << ((atoi(GetNullPadValue(trigger_price_integer, 6).c_str()) * 100) +
                   atoi(GetNullPadValue(trigger_price_decimal_point, 2).c_str())) /
                      100.00
               << "\n";
    t_temp_oss << "MKT Flag:       " << ('Y' == mkt_flag ? "MARKET_ORDER" : "LIMIT_ORDER") << "\n";
    t_temp_oss << "IsStopLossOrder:" << stoploss_indicator << "\n";
    t_temp_oss << "IsIOCOrder:     " << ioc_flag << "\n";
    t_temp_oss << "SpreadType:     " << GetSpreadType() << "\n";
    t_temp_oss << "AlgoIndicator:  " << GetAlgoType() << "\n";
    t_temp_oss << "ClientIdentity: " << GetClientIdentity() << "\n";

    return t_temp_oss.str();
  }
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << " Usage : < exec > < FO_NSE_DOTEX_FILE > <GET_ALL_DATA/GET_SYMBOL_LIST/GET_DATA_FOR_GIVEN_SYMBOL>"
              << std::endl;
    exit(-1);
  }

  std::string display_type = argv[2];
  std::string filter_with = "";

  if (std::string("GET_DATA_FOR_GIVEN_SYMBOL") == display_type) {
    filter_with = argv[3];
  }

  std::ifstream nse_dotex_fo;
  nse_dotex_fo.open(argv[1], std::ifstream::in);

  if (!nse_dotex_fo.is_open()) {
    std::cerr << "Couldn't Open File For Reading.. " << argv[1] << " Error : " << strerror(errno) << std::endl;
    exit(-1);
  }

  char input_data[128];

  std::set<std::string> instruments;

  while (nse_dotex_fo.good()) {
    memset(input_data, 0, 128);
    nse_dotex_fo.getline(input_data, 128);

    if (nse_dotex_fo.gcount() <= 0) break;

    if (std::string("GET_SYMBOL_LIST") == display_type) {
      if (instruments.find((*((NSEDotexOfflineDataStruct *)((char *)(input_data)))).GetUnderlyingSymbol()) ==
          instruments.end()) {
        instruments.insert((*((NSEDotexOfflineDataStruct *)((char *)(input_data)))).GetUnderlyingSymbol());
        std::cout << " NEW SYMBOL FOUND : "
                  << (*((NSEDotexOfflineDataStruct *)((char *)(input_data)))).GetUnderlyingSymbol() << std::endl;
      }

    } else if (std::string("GET_DATA_FOR_GIVEN_SYMBOL") == display_type) {
      if (std::string((*((NSEDotexOfflineDataStruct *)((char *)(input_data)))).GetUnderlyingSymbol()) == filter_with) {
        std::cout << (*((NSEDotexOfflineDataStruct *)((char *)(input_data)))).ToString() << "\n";
      }

    } else {
      std::cout << (*((NSEDotexOfflineDataStruct *)((char *)(input_data)))).ToString() << "\n";
    }
  }

  //  if ( std::string ( "GET_SYMBOL_LIST" ) == display_type ) {
  //
  //    std::set < std::string > :: iterator it = instruments.begin () ;
  //
  //    while ( it != instruments.end () ) {
  //
  //      std::cout << "SYMBOL : " << *it << std::endl ;
  //      it ++ ;
  //
  //    }
  //
  //  }

  nse_dotex_fo.close();

  return EXIT_SUCCESS;
}  // ----------  end of function main  ----------
