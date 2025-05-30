// =====================================================================================
//
//       Filename:  cboe_daily_token_symbol_handler.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  20241013 
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

#include <iostream>
#include "dvccode/Utils/cboe_refdata_loader.hpp"
#include "dvccode/Utils/cboe_daily_token_symbol_handler.hpp"
#include "dvccode/IBUtils/ControlCommandClient.hpp"
#include "dvccode/CDef/debug_logger.hpp"

namespace HFSAT {
namespace IBUtils {

class AddUpdateComboProductMap {
 private:
  DebugLogger& dbglogger_;                       ///< error logger
  HFSAT::Utils::CBOEDailyTokenSymbolHandler& cboe_daily_token_symbol_handler_;
  HFSAT::Utils::CBOERefDataLoader& cboe_ref_data_loader_;
  std::string control_ip;
  int32_t control_port;
  std::string req_combo_data_ip;
  int32_t req_combo_data_port;
  ControlCommandClient cmdRequestShortCode;
  ControlCommandClient cmdRequestComboData;

  AddUpdateComboProductMap(AddUpdateComboProductMap& disabled_copy_constructor) = delete;

  AddUpdateComboProductMap(DebugLogger& _dbglogger_,std::string _control_ip_, int32_t _control_port_, std::string _req_combo_data_ip_, int32_t _req_combo_data_port_);
 public:
  static AddUpdateComboProductMap& GetUniqueInstance(DebugLogger& _dbglogger_,std::string _control_ip_,int32_t _control_port_, std::string _req_combo_data_ip_, int32_t _req_combo_data_port_);
  std::string updateRequiredMapsForCombo(std::string combo_product_string);
  void updateRequiredMapsForCombo(std::string combo_product_string,std::string shortcode_);
  static void updateTheInternalMaps(std::string combo_product_string, std::string shortcode_, int32_t trading_date);

};

}
}

