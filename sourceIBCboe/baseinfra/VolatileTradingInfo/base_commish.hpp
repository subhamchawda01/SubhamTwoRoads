/**
    \file CDef/base_commish.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_CDEF_BASE_COMMISH_H
#define BASE_CDEF_BASE_COMMISH_H

#include <iostream>
#include <string>
#include <map>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/error_utils.hpp"
#include "dvccode/CDef/nse_security_definition.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

#define RETAIL_COMMISH_FILE "/spare/local/tradeinfo/retail_commish.txt"
#define DI1_RETAIL_COMMISH_TIERS_FILE "/spare/local/tradeinfo/di1_retail_commish_tiers.txt"
#define LARGE_COMMISH 1000000

namespace HFSAT {

typedef std::map<std::string, double> CommishMap;
typedef std::vector<std::pair<int, double> > CommishTiers;

class BaseCommish {
 private:
  static CommishMap commish_map_;
  static CommishMap retail_commish_map_;
  static CommishTiers di1_retail_commish_tiers_;
  static void InitCommishMap(CommishMap& _commish_map_);
  static void InitRetailCommishMap(CommishMap& _commish_map_);
  static void LoadDI1RetailCommishTiers(CommishTiers& _commish_tiers_);

 public:
  static double GetDI1Commish(const std::string& _shortcode_, int _YYYYMMDD_);
  static double GetCommishPerContract(const std::string& _shortcode_, int _YYYYMMDD_);
  static void SetSgxCNCommishPerContract();

  static double GetDI1RetailCommish(const std::string& _shortcode_, int _YYYYMMDD_);
  static double GetRetailCommishPerContract(const std::string& _shortcode_, int _YYYYMMDD_);

  static double GetCommishPerContract(const std::string& _shortcode_, const double _trade_price_, const int _qty_,
                                      const int _tier_ = 4);

  static double GetBMFEquityCommishPerContract(const std::string& _shortcode_, const double _trade_price_,
                                               const int _qty_, const int _tier_ = 1);

  static double GetNSECommishPerContract(const std::string& _shortcode_, const double _trade_price_, const int _qty_);
  static double GetBSECommishPerContract(const std::string& _shortcode_, const double _trade_price_, const int _qty_);
  static double SwitchSlab(const std::string& _shortcode_, double commission_per_contract, const int volume);
  static double GetCommission(const std::string& _shortcode_, int _YYYYMMDD, const double _trade_price_ = 1,
                              const int _qty_ = 1, const int volume = 0);
};
}

#endif  // BASE_CDEF_BASE_COMMISH_H
