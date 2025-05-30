#include "tradeengine/Indicator/VWAPSizeFilteredPrice.hpp"

VWAPSizeFiltered::VWAPSizeFiltered(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_,
                                   HFSAT::DebugLogger& _dbglogger_, double weight_, int size_filter)
    : BasePrice(smv, _watch_, _dbglogger_, weight_), size_filter_(size_filter), vwap_levels_(10) {
  // dbglogger_ << " SIZE FILTER " << size_filter << DBGLOG_ENDL_FLUSH;
}

VWAPSizeFiltered::~VWAPSizeFiltered() {}

void VWAPSizeFiltered::OnMarketUpdate(const unsigned int _security_id_,
                                      const HFSAT::MarketUpdateInfo& _market_update_info_) {
  CalculateVWAPSizeFiltered();
}

void VWAPSizeFiltered::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                                    const HFSAT::MarketUpdateInfo& _market_update_info_) {
  CalculateVWAPSizeFiltered();
}

bool VWAPSizeFiltered::CalculateVWAPSizeFiltered() {
  double total_bid_size = 0;
  double total_bid_value = 0;
  double total_ask_size = 0;
  double total_ask_value = 0;

  for (int level_count_ = 0; level_count_ < vwap_levels_; level_count_++) {
    HFSAT::MarketUpdateInfoLevelStruct* bid_info_ = smv_->bid_info(level_count_);

    if (bid_info_ == NULL) {
      break;
    }
    double _bid_price_ = bid_info_->limit_price_;
    int bid_size_ = bid_info_->limit_size_;
    total_bid_size += bid_size_;
    total_bid_value += (bid_size_ * _bid_price_);

    if (total_bid_size >= size_filter_) {
      /*dbglogger_ << " VSF BID " << _bid_price_ << " x " << bid_size_
              << " " << total_bid_size << " val "  << total_bid_value
              << DBGLOG_ENDL_FLUSH;*/
      break;
    }
  }

  for (int level_count_ = 0; level_count_ < vwap_levels_; level_count_++) {
    HFSAT::MarketUpdateInfoLevelStruct* ask_info_ = smv_->ask_info(level_count_);

    if (ask_info_ == NULL) {
      break;
    }
    double _ask_price_ = ask_info_->limit_price_;
    int ask_size_ = ask_info_->limit_size_;
    total_ask_size += ask_size_;
    total_ask_value += (ask_size_ * _ask_price_);

    if (total_ask_size >= size_filter_) {
      /*dbglogger_ << " VSF ASK " << _ask_price_ << " x " << ask_size_
              << " " << total_ask_size << " val "  << total_ask_value
              << DBGLOG_ENDL_FLUSH;*/
      break;
    }
  }

  if (total_bid_size == 0 || total_ask_size == 0) {
    base_bid_price_ = smv_->market_update_info().bestbid_price_;
    base_ask_price_ = smv_->market_update_info().bestask_price_;
    return false;
  }
  double avg_bid_px = total_bid_value / total_bid_size;
  double avg_ask_px = total_ask_value / total_ask_size;
  base_bid_price_ = (avg_bid_px * total_ask_size + avg_ask_px * total_bid_size) / (total_bid_size + total_ask_size);
  base_ask_price_ = base_bid_price_;
  // dbglogger_ << " VSF Price " << base_bid_price_ << DBGLOG_ENDL_FLUSH;
  return true;
}
