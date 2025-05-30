#include "tradeengine/Indicator/TradePrice.hpp"

TradePrice::TradePrice(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_, HFSAT::DebugLogger& _dbglogger_,
                       double weight_, int64_t last_trade_mid_price_cutoff_usecs, int vwap_levels)
    : BasePrice(smv, _watch_, _dbglogger_, weight_),
      last_traded_usecs_(0),
      current_time_usecs_(0),
      last_trade_mid_price_cutoff_usecs_(last_trade_mid_price_cutoff_usecs),
      vwap_levels_(vwap_levels) {}

TradePrice::~TradePrice() {}

void TradePrice::OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
  current_time_usecs_ = watch_.usecs_from_midnight();
  if (current_time_usecs_ - last_traded_usecs_ > last_trade_mid_price_cutoff_usecs_) {
    CalculateVWAPPrice();
  }
}

void TradePrice::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                              const HFSAT::MarketUpdateInfo& _market_update_info_) {
  current_time_usecs_ = watch_.usecs_from_midnight();
  base_bid_price_ = _trade_print_info_.trade_price_;
  base_ask_price_ = _trade_print_info_.trade_price_;
  last_traded_usecs_ = current_time_usecs_;
}

bool TradePrice::CalculateVWAPPrice() {
  double total_bid_size = 0;
  double total_bid_value = 0;
  double total_ask_size = 0;
  double total_ask_value = 0;

  for (int level_count_ = 0; level_count_ < vwap_levels_; level_count_++) {
    HFSAT::MarketUpdateInfoLevelStruct* bid_info_ = smv_->bid_info(level_count_);
    HFSAT::MarketUpdateInfoLevelStruct* ask_info_ = smv_->ask_info(level_count_);

    if ((ask_info_ == NULL) || (bid_info_ == NULL)) {
      // Hit bottom of book
      base_bid_price_ = smv_->market_update_info().bestbid_price_;
      base_ask_price_ = smv_->market_update_info().bestask_price_;
      return false;
    }

    double _bid_price_ = bid_info_->limit_price_;
    double _ask_price_ = ask_info_->limit_price_;

    int bid_size_ = bid_info_->limit_size_;
    int ask_size_ = ask_info_->limit_size_;

    total_bid_size += bid_size_;
    total_bid_value += (bid_size_ * _bid_price_);
    total_ask_size += ask_size_;
    total_ask_value += (ask_size_ * _ask_price_);
  }

  double avg_bid_px = total_bid_value / total_bid_size;
  double avg_ask_px = total_ask_value / total_ask_size;
  base_bid_price_ = (avg_bid_px * total_ask_size + avg_ask_px * total_bid_size) / (total_bid_size + total_ask_size);
  base_ask_price_ = base_bid_price_;
  return true;
}
