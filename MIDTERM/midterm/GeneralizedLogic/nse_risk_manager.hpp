#pragma once
#include <math.h>
#include <string>
#include <map>
#include <vector>
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "paths.hpp"
#include "constants.hpp"

namespace NSE_SIMPLEEXEC {
class NseRiskManager : public HFSAT::TimePeriodListener {

protected:
  time_t last_modified_;
  HFSAT::Watch &watch_;
  HFSAT::DebugLogger &dbglogger_;
  std::string strategy_type_;
  std::map<std::string, int> shortcode_to_allowed_orders_map; // Contains number
                                                              // of allowed
                                                              // trades for a
                                                              // shortcode
  std::map<std::string, int>
      shortcode_to_orders_map; // Contains number of trades for a shortcode
  NseRiskManager(HFSAT::Watch &watch_t, HFSAT::DebugLogger &dbglogger_t,
                 std::string strategy_type_)
      : last_modified_(-1), watch_(watch_t), dbglogger_(dbglogger_t),
        strategy_type_(strategy_type_), shortcode_to_allowed_orders_map(),
        shortcode_to_orders_map() {
    ReadConfigToInitializeMap();
    watch_.subscribe_OneMinutePeriod(this);
  }

public:
  static NseRiskManager &GetUniqueInstance(HFSAT::Watch &watch_t,
                                           HFSAT::DebugLogger &dbglogger_t,
                                           std::string strategy_type_) {
    static NseRiskManager uniqueinstance_(watch_t, dbglogger_t, strategy_type_);
    return uniqueinstance_;
  }

  ~NseRiskManager(){};

  // Function that needs to be called initially
  void ReadConfigToInitializeMap() {
    std::ifstream config_file_;
    std::string file_ = HFSAT::IsItSimulationServer()
                            ? TEST_SERVER_MAX_ALLOWED_ORDERS_FILE
                            : MAX_ALLOWED_ORDERS_FILE;
    file_ += "_" + strategy_type_;
    config_file_.open(file_, std::ifstream::in);
    if (!config_file_.is_open()) {
      dbglogger_ << "ALERT/ERROR Could not open the file with max orders per "
                    "shortcode!! -> "
                 << file_ << '\n';
      return;
    }
    std::string line;

    while (getline(config_file_, line)) {
      if (line.substr(0, 1) == "#" || line.empty()) {
        continue;
      }
      std::vector<std::string> tokens_;
      HFSAT::PerishableStringTokenizer::StringSplit(line, ',', tokens_);
      if ( tokens_.size() != 2 ) {
        dbglogger_ << "ALERT/ERROR Invalid entry in the max orders per "
                    "shortcode file!! -> "
                   << file_ << "\n"; 
       continue;
      }
      InitializeMap(tokens_[0], std::abs(atoi(tokens_[1].c_str())));
    }
  }

  void InitializeMap(const std::string &_shortcode_,
                     const int _possible_lots_) {
    shortcode_to_allowed_orders_map[_shortcode_] = _possible_lots_;
  }

  bool AddEntry(const std::string &_shortcode_, const int _num_lots_) {

    if (shortcode_to_orders_map.find(_shortcode_) ==
        shortcode_to_orders_map.end()) {
      shortcode_to_orders_map[_shortcode_] = std::abs(_num_lots_);
    } else {
      shortcode_to_orders_map[_shortcode_] += std::abs(_num_lots_);
    }

    //Send alsert if 80% limit is utilized
    if (shortcode_to_orders_map[_shortcode_] >= RISK_LIMIT_ALERT_THRESHOLD * 
          shortcode_to_allowed_orders_map[_shortcode_]) {
         dbglogger_ << "ALERT -> 80% Max orders limit reached for " 
                    << _shortcode_  << '\n';
    }


    if (shortcode_to_allowed_orders_map.find(_shortcode_) ==
        shortcode_to_allowed_orders_map.end()) {
      shortcode_to_orders_map[_shortcode_] -= std::abs(_num_lots_);
      return false;
    } else {
      if (shortcode_to_orders_map[_shortcode_] <=
          shortcode_to_allowed_orders_map[_shortcode_]) {
        return true;
      }
    }
    shortcode_to_orders_map[_shortcode_] -= std::abs(_num_lots_);
    return false;
  }

  double GetNumberOfRequestedOrders(const std::string &_shortcode_) {
    return ((shortcode_to_orders_map.find(_shortcode_) ==
             shortcode_to_orders_map.end())
                ? 0
                : (shortcode_to_orders_map[_shortcode_]));
  }

  void OnTimePeriodUpdate(const int x) {
    std::string file_ = HFSAT::IsItSimulationServer()
                            ? TEST_SERVER_MAX_ALLOWED_ORDERS_FILE
                            : MAX_ALLOWED_ORDERS_FILE;
    file_ += "_" + strategy_type_;
    time_t t_last_modified_ = HFSAT::FileUtils::lastModified(file_);
    if (t_last_modified_ > last_modified_) {
      if (last_modified_ != -1) {
        dbglogger_ << "ALERT -> Attempting to read the file with max orders "
                      "per shortcode since it seems modified"
                   << '\n';
      }
      ReadConfigToInitializeMap();
      last_modified_ = t_last_modified_;
    }
  }
};
}
