#include "tradeengine/Indicator/TradeSpreadPrice.hpp"

TradeTimeInfo::TradeTimeInfo(double px, int trade_size, int64_t time)
    : price_(px), trade_size_(trade_size), time_(time) {}

TradeSpreadPrice::TradeSpreadPrice(HFSAT::SecurityMarketView* smv, HFSAT::Watch& _watch_,
                                   HFSAT::DebugLogger& _dbglogger_, double weight_, int64_t window, int vwap_levels,
                                   double skew_factor)
    : BasePrice(smv, _watch_, _dbglogger_, weight_),
      last_traded_usecs_(0),
      current_time_usecs_(0),
      window_(window),
      last_bid_px_(0),
      last_ask_px_(0),
      sum_bid_size_(0),
      sum_bid_spread_(0),
      sum_ask_size_(0),
      sum_ask_spread_(0),
      vwap_levels_(vwap_levels),
      skew_(0),
      skew_factor_(skew_factor) {}

TradeSpreadPrice::~TradeSpreadPrice() {}

void TradeSpreadPrice::OnMarketUpdate(const unsigned int _security_id_,
                                      const HFSAT::MarketUpdateInfo& _market_update_info_) {
  current_time_usecs_ = watch_.usecs_from_midnight();
  last_bid_px_ = _market_update_info_.bestbid_price_;
  last_ask_px_ = _market_update_info_.bestask_price_;
  bool is_dirty = false;
  while (bid_queue_.size() > 0 && bid_queue_.front().time_ < current_time_usecs_ - window_) {
    sum_bid_spread_ -= bid_queue_.front().price_ * bid_queue_.front().trade_size_;
    sum_bid_size_ -= bid_queue_.front().trade_size_;
    /*dbglogger_ << " OMU BID POP " << bid_queue_.front().price_ << " "
            << bid_queue_.front().trade_size_ << " val "
            << bid_queue_.front().price_*bid_queue_.front().trade_size_ << " sum_spread "
            << sum_bid_spread_ << " sum_size "
            << sum_bid_size_ << DBGLOG_ENDL_FLUSH;*/
    bid_queue_.pop_front();
    is_dirty = true;
  }
  while (ask_queue_.size() > 0 && ask_queue_.front().time_ < current_time_usecs_ - window_) {
    sum_ask_spread_ -= ask_queue_.front().price_ * ask_queue_.front().trade_size_;
    sum_ask_size_ -= ask_queue_.front().trade_size_;
    /*dbglogger_ << " OMU ASK POP " << ask_queue_.front().price_ << " "
            << ask_queue_.front().trade_size_ << " val "
            << ask_queue_.front().price_*ask_queue_.front().trade_size_ << " sum_spread "
            << sum_ask_spread_ << " sum_size "
            << sum_ask_size_ << DBGLOG_ENDL_FLUSH;*/
    ask_queue_.pop_front();
    is_dirty = true;
  }
  if (is_dirty) {
    double sum_trade_size_ = sum_bid_size_ + sum_ask_size_;
    skew_ = 0;
    if (sum_trade_size_ != 0) {
      double total_sum_spread_ = sum_bid_spread_ - sum_ask_spread_;
      skew_ = total_sum_spread_ / sum_trade_size_;
      /*dbglogger_ << " OTP total_sum_spread " << total_sum_spread_
              << " sum_size " << sum_trade_size_
              << " skew " << skew_ << DBGLOG_ENDL_FLUSH;*/
    }
  }
  CalculateVWAPPrice();
  /*double mid_px = (_market_update_info_.bestbid_price_ + _market_update_info_.bestask_price_)/2;
  base_bid_price_ = mid_px + skew_;
  base_ask_price_ = base_bid_price_;*/
  base_bid_price_ += skew_factor_ * skew_;
  base_ask_price_ = base_bid_price_;
  // dbglogger_ << " OM midpx " << mid_px << " base_bid_price " << base_bid_price_ << DBGLOG_ENDL_FLUSH;
}

void TradeSpreadPrice::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                                    const HFSAT::MarketUpdateInfo& _market_update_info_) {
  current_time_usecs_ = watch_.usecs_from_midnight();
  if (last_bid_px_ == kInvalidPrice || last_ask_px_ == kInvalidPrice) return;
  if (_trade_print_info_.buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) {
    // Buy Trade Handling
    double spread = _trade_print_info_.trade_price_ - last_bid_px_;
    bid_queue_.push_back(TradeTimeInfo(spread, _trade_print_info_.size_traded_, current_time_usecs_));
    sum_bid_spread_ += spread * (_trade_print_info_.size_traded_);
    sum_bid_size_ += _trade_print_info_.size_traded_;
    /*dbglogger_ << " OTP BID ADD spread " << spread << " size "
            << _trade_print_info_.size_traded_ << " val "
            << spread*(_trade_print_info_.size_traded_)
            << " sum_spread " << sum_bid_spread_ << " sum_size "
            << sum_bid_size_ << DBGLOG_ENDL_FLUSH;*/
  } else {
    // Sell Trade Handling
    double spread = last_ask_px_ - _trade_print_info_.trade_price_;
    ask_queue_.push_back(TradeTimeInfo(spread, _trade_print_info_.size_traded_, current_time_usecs_));
    sum_ask_spread_ += spread * (_trade_print_info_.size_traded_);
    sum_ask_size_ += _trade_print_info_.size_traded_;
    /*dbglogger_ << " OTP ASK ADD spread " << spread << " size "
            << _trade_print_info_.size_traded_ << " val "
            << spread*(_trade_print_info_.size_traded_)
            << " sum_spread " << sum_ask_spread_ << " sum_size "
            << sum_ask_size_ << DBGLOG_ENDL_FLUSH;*/
  }
  double sum_trade_size_ = sum_bid_size_ + sum_ask_size_;
  skew_ = 0;
  if (sum_trade_size_ != 0) {
    double total_sum_spread_ = sum_bid_spread_ - sum_ask_spread_;
    skew_ = total_sum_spread_ / sum_trade_size_;
    /*dbglogger_ << " OTP total_sum_spread " << total_sum_spread_
            << " sum_size " << sum_trade_size_
            << " skew " << skew_ << DBGLOG_ENDL_FLUSH;*/
  }
  last_traded_usecs_ = current_time_usecs_;
  CalculateVWAPPrice();
  /*double mid_px = (_market_update_info_.bestbid_price_ + _market_update_info_.bestask_price_)/2;
  base_bid_price_ = mid_px + skew_;
  base_ask_price_ = base_bid_price_;*/
  base_bid_price_ += skew_factor_ * skew_;
  base_ask_price_ = base_bid_price_;
  // dbglogger_ << " OTP midpx " << mid_px << " base_bid_price " << base_bid_price_ << DBGLOG_ENDL_FLUSH;
}

bool TradeSpreadPrice::CalculateVWAPPrice() {
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
