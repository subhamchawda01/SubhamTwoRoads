#ifndef SYMBOL_ALLOCATOR_H
#define SYMBOL_ALLOCATOR_H

// @ramkris : Code might not be latency sensitive or optimized

#include <string.h>
#include <vector>
#include <map>

#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"

struct SymbolInfo {
  std::string base_symbol_;
  std::string symbol_;
  int traded_volume_;
  SymbolInfo() : base_symbol_(" "), symbol_(" "), traded_volume_(0) {}
  SymbolInfo(const std::string& t_base_symbol_, const std::string& t_symbol_, const int& t_traded_volume_)
      : base_symbol_(t_base_symbol_), symbol_(t_symbol_), traded_volume_(t_traded_volume_) {}

  bool operator()(const SymbolInfo& first, const SymbolInfo& second) {
    return (first.traded_volume_ > second.traded_volume_);
  }
  bool operator<(const SymbolInfo& other) const {
    if (traded_volume_ > other.traded_volume_) {
      return true;
    }
    return false;
  }
};

inline bool equate_symbol(const SymbolInfo& first, const SymbolInfo& second) {
  return (first.symbol_.compare(second.symbol_) == 0);
}

class VolumeSymbolManager {
 public:
  VolumeSymbolManager(int current_date_YYYYMMDD_t, int begin_secs_from_midnight_t, int end_secs_from_midnight_t,
                      std::string volume_based_exchnage_symbol_filename_t)
      :

        current_date_YYYYMMDD_(current_date_YYYYMMDD_t),
        begin_secs_from_midnight_(begin_secs_from_midnight_t),
        end_secs_from_midnight_(end_secs_from_midnight_t),
        volume_based_exchnage_symbol_filename_(volume_based_exchnage_symbol_filename_t) {
    current_trading_month_ = (current_date_YYYYMMDD_ / 100) % 100;
    current_trading_date_ = current_date_YYYYMMDD_ % 100;
    current_trading_year_ = (current_date_YYYYMMDD_ / 10000) % 10;
  }
  ~VolumeSymbolManager(){};
  // Set  base_symbols
  void populate_cme_base_symbols();
  void populate_eurex_base_symbols() {}
  void populate_tmx_base_symbols() {}
  void populate_bmf_base_symbols() {}
  // Get Base_Symbols
  std::vector<std::string>& get_cme_base_symbols() { return cme_base_symbols_; }
  std::vector<std::string>& get_eurex_base_symbols() { return eurex_base_symbols_; }
  std::vector<std::string>& get_tmx_base_symbols() { return tmx_base_symbols_; }
  std::vector<std::string>& get_bmf_base_symbols() { return bmf_base_symbols_; }

  // CME Baseed Month functions

  //\@ will return (e.g ESU1 ) for baseename ES, depending on the date
  std::string NextMonthCME(const std::string& baseename, int current_trading_month, int current_trading_year);
  std::string NextNthMonthCME(const std::string& baseename, int current_trading_month, int current_trading_year, int N);

  // Take N next contracts assuming the normal CME cycle of MAR, JUN, SEPT, DEC
  void GetNextNCMEContracts(const std::string& baseename, int current_trading_month, int current_trading_year,
                            std::vector<std::string>& sym_name, int num_contracts);

  std::string ThisMonthCME(const std::string& baseename, int current_trading_month, int current_trading_year);

  void populateAllSymbolsWithContractsCME();
  void populateAllSymbolsWithContractsBMF() {}
  void populateAllSymbolsWithContractsEUREX() {}
  void populateAllSymbolsWithContractsTMX() {}

  // GC Future ( GOLD)  baseed functions
  void Next23MonthsGCcontract(const std::string& baseename, int current_trading_month, int current_trading_year,
                              std::vector<std::string>& sym_name);
  // GC Future ( GOLD)  baseed functions
  void Next72MonthsGCcontract(const std::string& baseename, int current_trading_month, int current_trading_year,
                              std::vector<std::string>& sym_name);

  // Computes volumes for all contracts of a base_symbol
  void computeVolumesForSymbol(const std::string this_base_symbol, const int input_date_,
                               const int begin_secs_from_midnight_, const int end_secs_from_midnight_);

  // Computes volumes of all contracts of all base symbols
  void computeVolumesForAllSymbolsCME(const int input_date_, const int begin_secs_from_midnight_,
                                      const int end_secs_from_midnight_);
  // Normal dump to file
  void dumpToFileCMEContracts(int num_imp_contracts);

 private:
  int current_date_YYYYMMDD_;
  int begin_secs_from_midnight_;
  int end_secs_from_midnight_;
  std::string volume_based_exchnage_symbol_filename_;

  int current_trading_month_;
  int current_trading_date_;
  int current_trading_year_;

  std::vector<std::string> cme_base_symbols_;
  std::vector<std::string> bmf_base_symbols_;
  std::vector<std::string> tmx_base_symbols_;
  std::vector<std::string> eurex_base_symbols_;

  // For each base symbol we have a vector of symbols which we want to evaluate
  // based on the traded volume
  std::map<std::string, std::vector<SymbolInfo> > cme_base_symbol_to_all_symbols_map;
  std::map<std::string, std::vector<SymbolInfo> >::iterator cme_base_symbol_to_all_symbols_map_iter;

  void pushInMapForSorting(std::vector<SymbolInfo>& sym_vec, std::map<int, std::string>& sym_map);
};

#endif  // SYMBOL_ALLOCATOR_H
