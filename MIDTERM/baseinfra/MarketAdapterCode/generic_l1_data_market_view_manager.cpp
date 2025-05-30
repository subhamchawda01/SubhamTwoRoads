#include "baseinfra/MarketAdapter/generic_l1_data_market_view_manager.hpp"
#define CCPROFILING_TRADEINIT 1

namespace HFSAT {

GenericL1DataMarketViewManager::GenericL1DataMarketViewManager(
    DebugLogger& dbglogger, const Watch& watch, const SecurityNameIndexer& sec_name_indexer,
    const std::vector<SecurityMarketView*>& security_market_view_map)
    : BaseMarketViewManager(dbglogger, watch, sec_name_indexer, security_market_view_map) {}

void GenericL1DataMarketViewManager::OnL1New(const unsigned int security_id, const GenericL1DataStruct& l1_data) {
  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

  if (!smv_.initial_book_constructed_) {
    int int_price_ = smv_.GetIntPx(l1_data.delta.bestbid_price);
    BuildIndex(security_id, HFSAT::kTradeTypeBuy, int_price_);

    smv_.initial_book_constructed_ = true;
  }

  if (!smv_.is_ready_) {
    if (smv_.market_update_info_.bestask_size_ && smv_.market_update_info_.bestbid_size_) {
      smv_.is_ready_ = true;
    }
  }

  smv_.market_update_info_.bestbid_price_ = l1_data.delta.bestbid_price;
  smv_.market_update_info_.bestbid_size_ = l1_data.delta.bestbid_size;
  smv_.market_update_info_.bestbid_ordercount_ = l1_data.delta.bestbid_ordercount;
  smv_.market_update_info_.bestbid_int_price_ = smv_.GetIntPx(l1_data.delta.bestbid_price);

  smv_.market_update_info_.bestask_price_ = l1_data.delta.bestask_price;
  smv_.market_update_info_.bestask_size_ = l1_data.delta.bestask_size;
  smv_.market_update_info_.bestask_ordercount_ = l1_data.delta.bestask_ordercount;
  smv_.market_update_info_.bestask_int_price_ = smv_.GetIntPx(l1_data.delta.bestask_price);
  // updatel1pricesnow
  smv_.UpdateL1Prices();

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(15);
#endif

  smv_.NotifyL1PriceListeners();
  smv_.NotifyOnReadyListeners();
}

void GenericL1DataMarketViewManager::OnTrade(const unsigned int security_id, const GenericL1DataStruct& l1_data) {
  SecurityMarketView& smv_ = *(security_market_view_map_[security_id]);

  if (!smv_.initial_book_constructed_) {
    int int_price_ = smv_.GetIntPx(l1_data.trade.price);
    BuildIndex(security_id, HFSAT::kTradeTypeBuy, int_price_);

    smv_.initial_book_constructed_ = true;
  }

  smv_.trade_print_info_.trade_price_ = l1_data.trade.price;
  smv_.trade_print_info_.buysell_ = l1_data.trade.side;
  smv_.trade_print_info_.size_traded_ = l1_data.trade.size;
  smv_.trade_print_info_.int_trade_price_ = smv_.GetIntPx(l1_data.trade.price);

#if CCPROFILING_TRADEINIT
  HFSAT::CpucycleProfiler::GetUniqueInstance(30).End(15);
#endif

  if (smv_.is_ready_) {
    smv_.NotifyTradeListeners();
    smv_.NotifyOnReadyListeners();
  }
}

void GenericL1DataMarketViewManager::SetTimeToSkipUntilFirstEvent(const ttime_t start_time) {
  for (auto i = 0u; i < security_market_view_map_.size(); i++) {
    SecurityMarketView* smv_ = security_market_view_map_[i];
    smv_->set_skip_listener_notification_end_time(start_time);
  }
}
}
