/**
   \file Tools/get_sim_config_description_from_file.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
*/

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/SimMarketMaker/sim_config.hpp"

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "USAGE : shortcode sim_config_file_name" << std::endl;
    exit(0);
  }

  std::string shortcode_ = argv[1];
  std::string sim_config_file_name_ = argv[2];

  std::string filename_ = "/spare/local/logs/alllogs/gscdff.log";

  HFSAT::DebugLogger dbglogger_(10240, 1);
  dbglogger_.OpenLogFile(filename_.c_str(), std::ofstream::out);

  HFSAT::Watch watch_(dbglogger_, 20120426);  // Randomly using a valid date.

  HFSAT::SimConfigStruct sim_config_struct_ =
      HFSAT::SimConfig::GetSimConfigsForShortcode(dbglogger_, watch_, shortcode_, sim_config_file_name_);
  std::cout << "USE_ACCURATE_SEQD_TO_CONF " << sim_config_struct_.use_accurate_seqd_to_conf_ << std::endl
            << "USE_ACCURATE_CXL_SEQD_TO_CONF " << sim_config_struct_.use_accurate_cxl_seqd_to_conf_ << std::endl
            << "USE_ONLY_FULL_MKT_FOR_SIM " << sim_config_struct_.using_only_full_mkt_for_sim_ << std::endl
            << "MAX_CONF_ORDERS_ABOVE_BEST_LEVEL " << sim_config_struct_.max_conf_orders_above_best_level_ << std::endl
            << "USE_NO_CXL_FROM_FRONT " << sim_config_struct_.use_no_cxl_from_front_ << std::endl
            << "USE_FGBM_SIM " << sim_config_struct_.use_fgbm_sim_market_maker_ << std::endl
            << "USE_TARGET_PRICE_SIM " << sim_config_struct_.use_tgt_sim_market_maker_ << std::endl
            << "WEIGHT_1 " << sim_config_struct_.tgt_sim_wt1_ << std::endl
            << "WEIGHT_2 " << sim_config_struct_.tgt_sim_wt2_ << std::endl
            << "USE_ORS_EXEC " << sim_config_struct_.use_ors_exec_ << std::endl;
  return 0;
}
