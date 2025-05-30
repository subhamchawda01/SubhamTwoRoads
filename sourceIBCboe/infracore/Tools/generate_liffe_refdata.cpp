// =====================================================================================
//
//       Filename:  generate_liffe_refdata.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/10/2013 06:26:27 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#include <iostream>
#include <string>
#include <string.h>
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include "infracore/rapidxml/rapidxml_utils.hpp"

// process children of interest
// for all outrights use the decimals value, tick size values of the
// ContractStandindDataUnitary and get IntrumentTradingCode from OutrightStandingDataUnitary

void parseContract(rapidxml::xml_node<char>* n) {
  std::string symbol_index;
  std::string contract_symbol_index;
  std::string contract_type;
  std::string price_decimals;
  std::string quantity_decimals;
  std::string amount_decimals;
  std::string instrument_tick_size;
  std::string instrument_decimals_ratio;
  std::string instrument_trading_code;

  for (rapidxml::xml_node<char>* cn = n->first_node(); cn; cn = cn->next_sibling()) {
    std::string node_name = cn->name();
    if (node_name == "SymbolIndex") {
      symbol_index = cn->value();
    } else if (node_name == "ContractType") {
      contract_type = cn->value();
      if (contract_type == "O") return;  // ignoring ref data for options
    } else if (node_name == "PriceDecimals") {
      price_decimals = cn->value();
    } else if (node_name == "QuantityDecimals") {
      quantity_decimals = cn->value();
    } else if (node_name == "AmountDecimals") {
      amount_decimals = cn->value();
    } else if (node_name == "InstrumentTickSize") {
      instrument_tick_size = cn->value();
    } else if (node_name == "InstrumentDecimalsRatio") {
      instrument_decimals_ratio = cn->value();
    } else if (node_name == "OutrightStandingDataUnitary") {
      symbol_index = cn->first_node("SymbolIndex")->value();
      contract_symbol_index = cn->first_node("ContractSymbolIndex")->value();
      instrument_trading_code = cn->first_node("InstrumentTradingCode")->value();
      std::cout << symbol_index << " " << contract_symbol_index << " " << instrument_trading_code << " "
                << price_decimals << " " << quantity_decimals << " " << amount_decimals << " " << instrument_tick_size
                << " " << instrument_decimals_ratio << std::endl;
    }
  }
  return;
}

// iterates over the nodes ContractStandingDataUnitary and parses the required fields
// using parsseContract field
void parseFIXML(rapidxml::xml_node<char>* n) {
  for (rapidxml::xml_node<char>* contract_node = n->first_node("ContractStandingDataUnitary"); contract_node;
       contract_node = contract_node->next_sibling()) {
    std::string node_name = contract_node->name();
    if (node_name == "ContractStandingDataUnitary") {
      parseContract(contract_node);
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << " Usage : <exec> <liffe_fixml> \n";
    exit(1);
  }

  rapidxml::file<char> fl = rapidxml::file<char>(argv[1]);
  rapidxml::xml_document<> doc;  // character type defaults to char
  doc.parse<0>(fl.data());       // 0 means default parse flags
  parseFIXML(doc.first_node());

  return 0;
}
