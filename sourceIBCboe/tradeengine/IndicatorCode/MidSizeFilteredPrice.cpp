#include "tradeengine/Indicator/MidSizeFilteredPrice.hpp"

MidSizeFiltered::MidSizeFiltered(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                                 double weight_, int size_filter)
    : BasePrice(smv, _watch_, _dbglogger_, weight_), size_filter_(size_filter), max_levels_(10) {
  // dbglogger_ << " SIZE FILTER " << size_filter << DBGLOG_ENDL_FLUSH;
}

MidSizeFiltered::~MidSizeFiltered() {}

void MidSizeFiltered::OnMarketUpdate(const unsigned int _security_id_,
                                     const HFSAT::MarketUpdateInfo& _market_update_info_) {
  CalculateMidSizeFiltered();
}

void MidSizeFiltered::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                                   const HFSAT::MarketUpdateInfo& _market_update_info_) {
  CalculateMidSizeFiltered();
}

bool MidSizeFiltered::CalculateMidSizeFiltered() {
  double total_bid_size = 0;
  double total_bid_value = 0;
  double total_ask_size = 0;
  double total_ask_value = 0;

  for (int level_count_ = 0; level_count_ < max_levels_; level_count_++) {
    HFSAT::MarketUpdateInfoLevelStruct* bid_info_ = smv_->bid_info(level_count_);

    if (bid_info_ == NULL) {
      break;
    }
    double _bid_price_ = bid_info_->limit_price_;
    int bid_size_ = bid_info_->limit_size_;
    total_bid_size += bid_size_;
    total_bid_value += (bid_size_ * _bid_price_);

    if (total_bid_size >= size_filter_) {
      break;
    }
  }

  for (int level_count_ = 0; level_count_ < max_levels_; level_count_++) {
    HFSAT::MarketUpdateInfoLevelStruct* ask_info_ = smv_->ask_info(level_count_);

    if (ask_info_ == NULL) {
      break;
    }
    double _ask_price_ = ask_info_->limit_price_;
    int ask_size_ = ask_info_->limit_size_;
    total_ask_size += ask_size_;
    total_ask_value += (ask_size_ * _ask_price_);

    if (total_ask_size >= size_filter_) {
      break;
    }
  }

  if (total_bid_size == 0 || total_ask_size == 0) {
    base_bid_price_ = (smv_->market_update_info().bestbid_price_ + smv_->market_update_info().bestask_price_) * 0.5;
    base_ask_price_ = base_bid_price_;
    return false;
  }
  double avg_bid_px = total_bid_value / total_bid_size;
  double avg_ask_px = total_ask_value / total_ask_size;
  base_bid_price_ = (avg_bid_px + avg_ask_px) / 2;
  base_ask_price_ = base_bid_price_;
  return true;
}
