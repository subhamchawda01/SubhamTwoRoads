/**
    \file SmartOrderRoutingCode/base_pnl.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/load_sim_slippage_config.hpp"
#include "baseinfra/VolatileTradingInfo/base_commish.hpp"
#include "baseinfra/SmartOrderRouting/base_pnl.hpp"

namespace HFSAT {

BasePNL::BasePNL(DebugLogger& _dbglogger_, Watch& _watch_, SecurityMarketView& _dep_market_view_, int t_runtime_id_, bool t_are_we_running_bardata_sim)
    : dbglogger_(_dbglogger_),
      watch_(_watch_),
      dep_market_view_(_dep_market_view_),
      runtime_id_(t_runtime_id_),
      pnl_(0),
      realized_pnl_(0),
      opentrade_unrealized_pnl_(0),
      min_pnl_till_now_(0),
      total_pnl_(0),
      last_closing_trade_pnl_(0),
      position_(0),
      max_pnl_(0.0),
      drawdown_(0.0),
      commish_dollars_per_unit_(BaseCommish::GetCommishPerContract(_dep_market_view_.shortcode(), watch_.YYYYMMDD())),
      retail_commish_dollars_per_unit_(
          BaseCommish::GetRetailCommishPerContract(_dep_market_view_.shortcode(), watch_.YYYYMMDD())),
      di_reserves_(SecurityDefinitions::GetDIReserves(watch_.YYYYMMDD(), dep_market_view_.shortcode())),
      asx_reserves_(SecurityDefinitions::GetASXReserves(watch_.YYYYMMDD(), dep_market_view_.secname())),
      numbers_to_dollars_(
          SecurityDefinitions::GetContractNumbersToDollars(_dep_market_view_.shortcode(), watch_.YYYYMMDD())),
      current_price_(0),
      average_open_price_(0.0),
      last_bid_price_(0.0),
      last_ask_price_(0.0),
      last_7_trade_lines_(),
      mult_base_pnl_(0),
      mult_risk_(0.0),
      port_base_pnl_(0),
      port_risk_(0.0),
      base_pnl_listener_(NULL),
      base_pnl_listener_index_(-1),
      is_shc_di1_(false),
      is_asx_shc_(false),
      asx_bond_face_value_(-1),
      asx_term_(10),
      is_asx_bond_(false),
      is_asx_ir_(false) {
  bzero(numbered_secname_, 40);
  snprintf(numbered_secname_, 40, "%s.%d", dep_market_view_.shortcode().c_str(), runtime_id_);

  for (size_t i = 0; i < strlen(numbered_secname_); ++i) {
    if (numbered_secname_[i] == ' ') {  // Liffe symbol naming crap
      numbered_secname_[i] = '~';
    }
  }

  if (dep_market_view_.shortcode().find("DI1") != std::string::npos &&
      dep_market_view_.exch_source() == HFSAT::kExchSourceBMF) {
    is_shc_di1_ = true;
  }

  if (dep_market_view_.shortcode() == "XT_0" || dep_market_view_.shortcode() == "YT_0" ||
      dep_market_view_.shortcode() == "XTE_0" || dep_market_view_.shortcode() == "YTE_0") {
    is_asx_shc_ = true;
    is_asx_bond_ = true;
    asx_bond_face_value_ = 100000;

    if (dep_market_view_.shortcode() == "XT_0" || dep_market_view_.shortcode() == "XTE_0") {
      asx_term_ = 10;
    } else if (dep_market_view_.shortcode() == "YT_0" || dep_market_view_.shortcode() == "YTE_0") {
      asx_term_ = 3;
    }
  }

  if (dep_market_view_.shortcode() == "IR_0" || dep_market_view_.shortcode() == "IR_1" ||
      dep_market_view_.shortcode() == "IR_2" || dep_market_view_.shortcode() == "IR_3" ||
      dep_market_view_.shortcode() == "IR_4") {
    is_asx_shc_ = true;
    is_asx_ir_ = true;
    asx_bond_face_value_ = 1000000;
  }

  if ((dep_market_view_.this_smv_exch_source_ == kExchSourceNSE) ||
      (dep_market_view_.this_smv_exch_source_ == kExchSourceBSE)) {
    if (t_are_we_running_bardata_sim) {
      //commish_dollars_per_unit_ *= CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD);
      commish_dollars_per_unit_ += HFSAT::SimSlippageConfig::GetUniqueInstance().GetSlippageValueFromShortcode(dep_market_view_.shortcode()) / 10000; 
    } else {
      commish_dollars_per_unit_ *= CurrencyConvertor::Convert(kCurrencyINR, kCurrencyUSD);
    }
  }

  dep_market_view_.subscribe_tradeprints(this);  // attaching itself as a general market data listener to l1 events or
                                                 // l1 price changes ( not size changes )
  dep_market_view_.ComputeMktPrice();
}

std::string BasePNL::ToString() {
  std::ostringstream t_oss_;
  for (auto i = 0u; i < last_7_trade_lines_.size(); ++i) {
    t_oss_ << last_7_trade_lines_[i] << "\n";
  }

  return t_oss_.str();
}
}
