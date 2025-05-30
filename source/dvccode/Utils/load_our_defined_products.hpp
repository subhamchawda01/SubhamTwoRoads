// =====================================================================================
//
//       Filename:  load_our_defined_products.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  02/14/2014 12:29:14 PM
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

#include <vector>

#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include <set>

namespace HFSAT {
namespace Utils {

class LoadOurDefinedProducts {
 private:
  int32_t tradingdate_;
  HFSAT::SecurityDefinitions& sec_definitions_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_;

  // Disable Copy Constructor
  LoadOurDefinedProducts(const LoadOurDefinedProducts&);
  LoadOurDefinedProducts();

  LoadOurDefinedProducts(const std::set<HFSAT::ExchSource_t>& _list_of_exch_sources_)
      : tradingdate_(HFSAT::DateTime::GetCurrentIsoDateLocal()),
        sec_definitions_(HFSAT::SecurityDefinitions::GetUniqueInstance(tradingdate_)),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance())

  {
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

    HFSAT::ShortcodeContractSpecificationMap& contract_specification_map = sec_definitions_.contract_specification_map_;
    HFSAT::ShortcodeContractSpecificationMapCIter_t contract_specification_map_itr = contract_specification_map.begin();

    for (; contract_specification_map_itr != contract_specification_map.end(); contract_specification_map_itr++) {
      if (_list_of_exch_sources_.find((contract_specification_map_itr->second).exch_source_) ==
          _list_of_exch_sources_.end())
        continue;

      // GetExch Symbol
      const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(contract_specification_map_itr->first);
      sec_name_indexer_.AddString(exchange_symbol_, contract_specification_map_itr->first);
    }
  }

 public:
  static inline LoadOurDefinedProducts& GetUniqueInstance(const std::set<HFSAT::ExchSource_t>& _list_of_exch_sources_) {
    static LoadOurDefinedProducts uniq_instance_(_list_of_exch_sources_);
    return uniq_instance_;
  }

  void AddExchange(HFSAT::ExchSource_t exch_source) {
    HFSAT::ShortcodeContractSpecificationMap& contract_specification_map = sec_definitions_.contract_specification_map_;
    HFSAT::ShortcodeContractSpecificationMapCIter_t contract_specification_map_itr = contract_specification_map.begin();

    for (; contract_specification_map_itr != contract_specification_map.end(); contract_specification_map_itr++) {
      if ((contract_specification_map_itr->second).exch_source_ == exch_source) {
        // GetExch Symbol
        const char* exchange_symbol_ =
            HFSAT::ExchangeSymbolManager::GetExchSymbol(contract_specification_map_itr->first);
        sec_name_indexer_.AddString(exchange_symbol_, contract_specification_map_itr->first);
      }
    }
  }

  void AddExchanges(const std::set<HFSAT::ExchSource_t>& _list_of_exch_sources_) {
    HFSAT::ShortcodeContractSpecificationMap& contract_specification_map = sec_definitions_.contract_specification_map_;
    HFSAT::ShortcodeContractSpecificationMapCIter_t contract_specification_map_itr = contract_specification_map.begin();

    for (; contract_specification_map_itr != contract_specification_map.end(); contract_specification_map_itr++) {
      if (_list_of_exch_sources_.find((contract_specification_map_itr->second).exch_source_) ==
          _list_of_exch_sources_.end())
        continue;

      // GetExch Symbol
      const char* exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(contract_specification_map_itr->first);
      sec_name_indexer_.AddString(exchange_symbol_, contract_specification_map_itr->first);
    }
  }
};
}
}
