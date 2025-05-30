#include "baseinfra/MarketAdapter/generic_l1_data_market_view_manager.hpp"
#define CCPROFILING_TRADEINIT 0

namespace HFSAT {

GenericL1DataMarketViewManager::GenericL1DataMarketViewManager(
    DebugLogger& dbglogger, const Watch& watch, const SecurityNameIndexer& sec_name_indexer,
    const std::vector<SecurityMarketView*>& security_market_view_map)
    : BaseMarketViewManager(dbglogger, watch, sec_name_indexer, security_market_view_map) {

  for (unsigned int t_ctr_ = 0; t_ctr_ < sec_name_indexer.NumSecurityId(); t_ctr_++) {
    security_market_view_map[t_ctr_]->SetL1OnlyFlag(true);
    SecurityMarketView &smv_ = *(security_market_view_map[t_ctr_]);
    const std::string& shortcode = security_market_view_map[t_ctr_]->shortcode();
    
    if (strncmp(shortcode.c_str(), "NSE_", 4) == 0) {
      HFSAT::NSETradingLimit_t *t_trading_limit_ =
          HFSAT::NSESecurityDefinitions::GetTradingLimits(security_market_view_map[t_ctr_]->shortcode());
      if ((t_trading_limit_ != NULL) && (t_trading_limit_->upper_limit_ != 0) &&
          (t_trading_limit_->lower_limit_ != 0)) {
        smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
        smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
      } else {
        dbglogger << "Error: Price Limit not present for " << smv_.shortcode() << "\n";
      }
    } else if (strncmp(shortcode.c_str(), "BSE_", 4) == 0) {
      HFSAT::BSETradingLimit_t *t_trading_limit_ =
          HFSAT::BSESecurityDefinitions::GetTradingLimits(security_market_view_map[t_ctr_]->shortcode());
      if ((t_trading_limit_ != NULL) && (t_trading_limit_->upper_limit_ != 0) &&
          (t_trading_limit_->lower_limit_ != 0)) {
        smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
        smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
      } else {
        dbglogger << "Error: Price Limit not present for " << smv_.shortcode() << "\n";
      }
    }
    dbglogger << "Price Limit for " << smv_.shortcode() << " : "
               << smv_.lower_int_price_limit_ * smv_.min_price_increment_ << "-"
               << smv_.upper_int_price_limit_ * smv_.min_price_increment_ << "\n";
  }
}

void GenericL1DataMarketViewManager::OnL1New(const unsigned int security_id, const GenericL1DataStruct& l1_data) {
  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);
 
  if(!smv_.is_ready_) {
    smv_.is_ready_ = true;
  }

  int int_price_bid = (l1_data.bestbid_price > 0 ? smv_.GetIntPx(l1_data.bestbid_price) : kInvalidIntPrice);
  int int_price_ask = (l1_data.bestask_price > 0 ? smv_.GetIntPx(l1_data.bestask_price) : kInvalidIntPrice);
  std::string shortcode_prefix = smv_.shortcode().substr(0, 4);

  if (smv_.lower_int_price_limit_ != -1) {
    if ((int_price_bid > smv_.upper_int_price_limit_) && (l1_data.bestbid_price > 0)){
      if (shortcode_prefix == "NSE_") {
        HFSAT::NSETradingLimit_t *t_trading_limit_ =
            HFSAT::NSESecurityDefinitions::ChangeTradingLimits(smv_.shortcode(), int_price_bid);
        if (t_trading_limit_ != NULL) {
          dbglogger_ << "Price Limit crossed for shortcode " << smv_.shortcode()
                     << " PrevUL: " << smv_.upper_int_price_limit_ * 0.05
                     << " PrevLL: " << smv_.lower_int_price_limit_ * 0.05 << " OrderPrice: " << l1_data.bestbid_price
                     << " NewUL: " << t_trading_limit_->upper_limit_ * 0.05
                     << " NewLL: " << t_trading_limit_->lower_limit_ * 0.05 << '\n';
          smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
          smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
        } else {
          dbglogger_ << "ERROR: Trading limit existed before but not now for " << smv_.shortcode() << '\n';
        }
      } else if (shortcode_prefix == "BSE_") {
        HFSAT::BSETradingLimit_t *t_trading_limit_ =
            HFSAT::BSESecurityDefinitions::ChangeTradingLimits(smv_.shortcode(), int_price_bid);
        if (t_trading_limit_ != NULL) {
          dbglogger_ << "Price Limit crossed for shortcode " << smv_.shortcode()
                     << " PrevUL: " << smv_.upper_int_price_limit_ * 0.05
                     << " PrevLL: " << smv_.lower_int_price_limit_ * 0.05 << " OrderPrice: " << l1_data.bestbid_price
                     << " NewUL: " << t_trading_limit_->upper_limit_ * 0.05
                     << " NewLL: " << t_trading_limit_->lower_limit_ * 0.05 << '\n';
          smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
          smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
        } else {
          dbglogger_ << "ERROR: Trading limit existed before but not now for " << smv_.shortcode() << '\n';
        }
      }
    }
    else if ((int_price_ask < smv_.lower_int_price_limit_) && (l1_data.bestask_price > 0)) {
      if (shortcode_prefix == "NSE_") {
        HFSAT::NSETradingLimit_t *t_trading_limit_ =
            HFSAT::NSESecurityDefinitions::ChangeTradingLimits(smv_.shortcode(), int_price_ask);
        if (t_trading_limit_ != NULL) {
          dbglogger_ << "Price Limit crossed for shortcode " << smv_.shortcode()
                     << " PrevUL: " << smv_.upper_int_price_limit_ * 0.05
                     << " PrevLL: " << smv_.lower_int_price_limit_ * 0.05 << " OrderPrice: " << l1_data.bestask_price
                     << " NewUL: " << t_trading_limit_->upper_limit_ * 0.05
                     << " NewLL: " << t_trading_limit_->lower_limit_ * 0.05 << '\n';
          smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
          smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
        } else {
          dbglogger_ << "ERROR: Trading limit existed before but not now for " << smv_.shortcode() << '\n';
        }
      } else if (shortcode_prefix == "BSE_") {
        HFSAT::BSETradingLimit_t *t_trading_limit_ =
            HFSAT::BSESecurityDefinitions::ChangeTradingLimits(smv_.shortcode(), int_price_ask);
        if (t_trading_limit_ != NULL) {
          dbglogger_ << "Price Limit crossed for shortcode " << smv_.shortcode()
                     << " PrevUL: " << smv_.upper_int_price_limit_ * 0.05
                     << " PrevLL: " << smv_.lower_int_price_limit_ * 0.05 << " OrderPrice: " << l1_data.bestask_price
                     << " NewUL: " << t_trading_limit_->upper_limit_ * 0.05
                     << " NewLL: " << t_trading_limit_->lower_limit_ * 0.05 << '\n';
          smv_.upper_int_price_limit_ = t_trading_limit_->upper_limit_;
          smv_.lower_int_price_limit_ = t_trading_limit_->lower_limit_;
        } else {
          dbglogger_ << "ERROR: Trading limit existed before but not now for " << smv_.shortcode() << '\n';
        }
      }
    } 
  }

  MarketUpdateInfoLevelStruct new_level_bid_(0, int_price_bid, l1_data.bestbid_price, l1_data.bestbid_size, l1_data.bestbid_ordercount, watch_.tv());
  smv_.ReplaceTopBid(new_level_bid_);
  MarketUpdateInfoLevelStruct new_level_ask_(0, int_price_ask, l1_data.bestask_price, l1_data.bestask_size, l1_data.bestask_ordercount, watch_.tv());
  smv_.ReplaceTopAsk(new_level_ask_);

  smv_.market_update_info_.bestbid_price_ = l1_data.bestbid_price;
  smv_.market_update_info_.bestbid_size_ = l1_data.bestbid_size;
  smv_.market_update_info_.bestbid_ordercount_ = l1_data.bestbid_ordercount;
  smv_.market_update_info_.bestbid_int_price_ = int_price_bid;

  smv_.market_update_info_.bestask_price_ = l1_data.bestask_price;
  smv_.market_update_info_.bestask_size_ = l1_data.bestask_size;
  smv_.market_update_info_.bestask_ordercount_ = l1_data.bestask_ordercount;
  smv_.market_update_info_.bestask_int_price_ = int_price_ask;
  // updatel1pricesnow
  smv_.UpdateL1Prices();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(15);
#endif

  smv_.NotifyL1PriceListeners();
}

void GenericL1DataMarketViewManager::OnTrade(const unsigned int security_id, const GenericL1DataStruct& l1_data) {
  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

  MarketUpdateInfoLevelStruct new_level_bid_(0, smv_.GetIntPx(l1_data.bestbid_price), l1_data.bestbid_price, l1_data.bestbid_size, l1_data.bestbid_ordercount, watch_.tv());
  smv_.ReplaceTopBid(new_level_bid_);
  MarketUpdateInfoLevelStruct new_level_ask_(0, smv_.GetIntPx(l1_data.bestask_price), l1_data.bestask_price, l1_data.bestask_size, l1_data.bestask_ordercount, watch_.tv());
  smv_.ReplaceTopAsk(new_level_ask_);
  smv_.market_update_info_.bestbid_price_ = l1_data.bestbid_price;
  smv_.market_update_info_.bestbid_size_ = l1_data.bestbid_size;
  smv_.market_update_info_.bestbid_ordercount_ = l1_data.bestbid_ordercount;
  smv_.market_update_info_.bestbid_int_price_ = (l1_data.bestbid_price > 0 ? smv_.GetIntPx(l1_data.bestbid_price) : kInvalidIntPrice);

  smv_.market_update_info_.bestask_price_ = l1_data.bestask_price;
  smv_.market_update_info_.bestask_size_ = l1_data.bestask_size;
  smv_.market_update_info_.bestask_ordercount_ = l1_data.bestask_ordercount;
  smv_.market_update_info_.bestask_int_price_ = (l1_data.bestask_price > 0 ? smv_.GetIntPx(l1_data.bestask_price) : kInvalidIntPrice);
  smv_.trade_print_info_.trade_price_ = l1_data.price;
  smv_.trade_print_info_.buysell_ = l1_data.side;
  smv_.trade_print_info_.size_traded_ = l1_data.size;
  smv_.trade_print_info_.int_trade_price_ = smv_.GetIntPx(l1_data.price);
  smv_.UpdateL1Prices();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(15);
#endif

  smv_.NotifyTradeListeners();
  if(!l1_data.is_intermediate) {
    smv_.NotifyL1PriceListeners();
  }
}

void GenericL1DataMarketViewManager::OnIndexPrice(const unsigned int t_security_id_, double const t_spot_price_){
  SecurityMarketView &smv_ = *(security_market_view_map_[t_security_id_]);
  smv_.market_update_info_.mid_price_ = t_spot_price_;
  smv_.market_update_info_.bestask_price_ = t_spot_price_;
  smv_.market_update_info_.bestbid_price_ = t_spot_price_;
  smv_.trade_print_info_.trade_price_ = t_spot_price_;
  smv_.trade_print_info_.size_traded_ = 1;
  smv_.is_ready_ = true;
  smv_.NotifySpotIndexListeners();
}       

void GenericL1DataMarketViewManager::SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView* smv_ = security_market_view_map_[i];
    smv_->set_skip_listener_notification_end_time(start_time);
  }
}
}
