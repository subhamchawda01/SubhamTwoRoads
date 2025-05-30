/**
    \file MinuteBar/base_minute_bar_exec_logic.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/MinuteBar/base_minute_bar_exec_logic.hpp"

namespace HFSAT {

BaseMinuteBarExecLogic::BaseMinuteBarExecLogic(DebugLogger& dbglogger, const Watch& watch,
                                               const std::string& logic_name, const std::string& config_file,
                                               int client_id, StrategySecurityPairToMinuteBarOrderManager& om_map)
    : dbglogger_(dbglogger),
      watch_(watch),
      logic_name_(logic_name),
      config_file_(config_file),
      client_id_(client_id),
      strat_secid_to_om_map_(om_map),
      logic_name_to_func_map_(),
      bar_update_function_(),
      indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()) {
  InitFunctionPtrMap();
  CheckValidLogicType();
  bar_update_function_ = logic_name_to_func_map_[logic_name_].first;
  signal_update_function_ = logic_name_to_func_map_[logic_name_].second;
}

void BaseMinuteBarExecLogic::OnBarUpdate(const unsigned int security_id, const DataBar& minute_bar) {
  (this->*(HFSAT::BaseMinuteBarExecLogic::bar_update_function_))(security_id, minute_bar);
}

void BaseMinuteBarExecLogic::OnSignalUpdate(int signal_id, double signal_value) {
  (this->*(HFSAT::BaseMinuteBarExecLogic::signal_update_function_))(signal_id, signal_value);
}

void BaseMinuteBarExecLogic::SendTrade(HFSAT::TradeType_t buy_sell, int security_id, int size) {
  std::pair<int, int> strat_secid_pair = std::make_pair(client_id_, security_id);
  if (strat_secid_to_om_map_.find(strat_secid_pair) != strat_secid_to_om_map_.end()) {
    if (buy_sell == HFSAT::kTradeTypeBuy) {
      strat_secid_to_om_map_[strat_secid_pair]->Buy(size);
    } else if (buy_sell == HFSAT::kTradeTypeSell) {
      strat_secid_to_om_map_[strat_secid_pair]->Sell(size);
    } else {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "BuySell Type No Info : " << buy_sell << " ClientID: " << client_id_
                                   << " SecurityID: " << indexer_.GetSecurityNameFromId(security_id)
                                   << DBGLOG_ENDL_FLUSH;
    }
  } else {
    DBGLOG_CLASS_FUNC_LINE_ERROR << "Can't find order manager for ClientID: " << client_id_
                                 << " Security: " << indexer_.GetSecurityNameFromId(security_id) << DBGLOG_ENDL_FLUSH;
  }
}

void BaseMinuteBarExecLogic::InitFunctionPtrMap() {
  logic_name_to_func_map_["SimpleLogic"] =
      std::make_pair(&BaseMinuteBarExecLogic::SimpleLogicBarUpdate, &BaseMinuteBarExecLogic::SimpleLogicSignalUpdate);
}

void BaseMinuteBarExecLogic::CheckValidLogicType() {
  // If the given Logic type is not supported, gracefully exit
  if (logic_name_to_func_map_.find(logic_name_) == logic_name_to_func_map_.end()) {
    std::cerr << __FUNCTION__ << " Minute Bar Logic type : " << logic_name_
              << " is not supported. Please add your logic in the InitFunctionPtrMap() call first.  \n";
    std::cerr << " Currently Supported : \n";

    for (auto logic_name_func_pair : logic_name_to_func_map_) {
      std::cerr << logic_name_func_pair.first << ", ";
    }
    ExitVerbose(kExitErrorCodeGeneral);
  }
}
}
