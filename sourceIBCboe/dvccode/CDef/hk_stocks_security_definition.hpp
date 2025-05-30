/**
   \file dvccode/CDef/hk_stocks_security_definition.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include <vector>
#include <boost/date_time/gregorian/gregorian.hpp>
#include "dvccode/CDef/common_security_definition_structs.hpp"
#include "dvccode/CDef/file_utils.hpp"

namespace HFSAT {

typedef struct {
  int security_code;
  int lot_size;
  double min_tick;
  char shortcode[19];
} HKStocksParams;

class HKStocksSecurityDefinitions {
 private:
  explicit HKStocksSecurityDefinitions(int t_intdate_);
  static HKStocksSecurityDefinitions* p_uniqueinstance_;

  static int intdate_;
  static std::vector<HKStocksParams*> stock_params_;

  static std::map<std::string, std::string> shortcode_2_exchsymbol_;
  static std::map<std::string, std::string> exchsymbol_2_shortcode_;

 public:
  static inline HKStocksSecurityDefinitions& GetUniqueInstance(int t_intdate_) {
    if (p_uniqueinstance_ == NULL) {
      p_uniqueinstance_ = new HKStocksSecurityDefinitions(t_intdate_);
    }
    return *(p_uniqueinstance_);
  }

  // adds contract specs for HK Stocks
  static void AddHKStocksContractSpecifications(ShortcodeContractSpecificationMap& t_contract_spec_map_);

  // method to read instrument params - currently from a dated file
  static void LoadInstParams(const int date);

  // read file for mapping between shortcode and exchange symbol
  static void LoadShortcodeExchangeSymbolMap();

  // Exchange Symbol function for HKStocks
  static std::string GetExchSymbolHKStocks(const std::string& shortcode);

  static std::string GetShortCodeFromExchSymbol(const std::string& exch_symbol);
};
}
